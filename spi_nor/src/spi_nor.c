/**
 * @file spi_nor.c
 * @brief Ядро драйвера SPI NOR (чистый C, без Arduino).
 */
#include "spi_nor.h"
#include "spi_nor_cmd.h"

#include <string.h>

#ifndef SPI_NOR_DEFAULT_PAGE
#define SPI_NOR_DEFAULT_PAGE 256u
#endif
#ifndef SPI_NOR_DEFAULT_SECTOR
#define SPI_NOR_DEFAULT_SECTOR 4096u
#endif
#ifndef SPI_NOR_DEFAULT_BLOCK
#define SPI_NOR_DEFAULT_BLOCK 65536u
#endif
#ifndef SPI_NOR_DEFAULT_TIMEOUT_MS
#define SPI_NOR_DEFAULT_TIMEOUT_MS 30000u
#endif

typedef struct {
  uint8_t mfr;
  uint8_t type;
  uint8_t cap_id;
  uint32_t capacity;
  uint32_t page;
  uint32_t sector;
  uint32_t block;
  const char *name;
} spi_nor_known_t;

/* Известные чипы. LE25U40 — профиль по умолчанию для платы LE250U40. */
static const spi_nor_known_t k_known[] = {
  /* Onsemi/Sanyo LE25U40CMC: 4 Mbit = 512 KiB, JEDEC 62 06 13 */
  { 0x62u, 0x06u, 0x13u, 524288u,    256u, 4096u, 65536u, "LE25U40CMC" },
  /* Winbond W25Q32: 32 Mbit = 4 MiB — только явный другой target */
  { 0xEFu, 0x40u, 0x16u, 4194304u,   256u, 4096u, 65536u, "W25Q32" },
  { 0xEFu, 0x70u, 0x16u, 4194304u,   256u, 4096u, 65536u, "W25Q32JV" },
  /* GigaDevice GD25Q32 */
  { 0xC8u, 0x40u, 0x16u, 4194304u,   256u, 4096u, 65536u, "GD25Q32" },
  /* Macronix MX25L3206 / близкие */
  { 0xC2u, 0x20u, 0x16u, 4194304u,   256u, 4096u, 65536u, "MX25L32" },
};

static void delay_ms_hal(const spi_nor_hal_t *hal, uint32_t ms)
{
  if (hal->delay_ms) {
    hal->delay_ms(hal->user, ms);
  } else if (hal->delay_us) {
    while (ms--) {
      hal->delay_us(hal->user, 1000u);
    }
  }
}

static int xfer(const spi_nor_hal_t *hal, const uint8_t *tx, uint8_t *rx, size_t len)
{
  if (!hal || !hal->spi_xfer) {
    return SPI_NOR_ERR_PARAM;
  }
  if (hal->spi_xfer(hal->user, tx, rx, len) != 0) {
    return SPI_NOR_ERR_HAL;
  }
  return SPI_NOR_OK;
}

static int xfer_cs(const spi_nor_hal_t *hal, const uint8_t *tx, uint8_t *rx, size_t len)
{
  int rc;
  if (!hal->cs_assert || !hal->cs_deassert) {
    return SPI_NOR_ERR_PARAM;
  }
  hal->cs_assert(hal->user);
  rc = xfer(hal, tx, rx, len);
  hal->cs_deassert(hal->user);
  return rc;
}

static int cmd1(spi_nor_t *dev, uint8_t opcode)
{
  return xfer_cs(dev->hal, &opcode, NULL, 1u);
}

static int ensure_ready(spi_nor_t *dev)
{
  uint32_t t = dev->cfg.busy_timeout_ms;
  if (t == 0u) {
    t = SPI_NOR_DEFAULT_TIMEOUT_MS;
  }
  return spi_nor_wait_ready(dev, t);
}

static int check_range(const spi_nor_t *dev, uint32_t addr, size_t len)
{
  uint64_t end;
  if (!dev->probed || dev->info.capacity_bytes == 0u) {
    return SPI_NOR_ERR_PARAM;
  }
  if (len == 0u) {
    return SPI_NOR_ERR_PARAM;
  }
  end = (uint64_t)addr + (uint64_t)len;
  if (end > (uint64_t)dev->info.capacity_bytes) {
    return SPI_NOR_ERR_RANGE;
  }
  return SPI_NOR_OK;
}

static const spi_nor_known_t *lookup_jedec(uint8_t mfr, uint8_t type, uint8_t cap)
{
  size_t i;
  for (i = 0; i < sizeof(k_known) / sizeof(k_known[0]); i++) {
    if (k_known[i].mfr == mfr && k_known[i].type == type && k_known[i].cap_id == cap) {
      return &k_known[i];
    }
  }
  return NULL;
}

static uint32_t capacity_from_jedec_id(uint8_t capacity_id)
{
  /* Стандартная кодировка: объём = 2^capacity_id байт (для многих NOR). */
  if (capacity_id >= 32u) {
    return 0u;
  }
  return 1u << capacity_id;
}

static void apply_geometry(spi_nor_t *dev, const spi_nor_known_t *k)
{
  uint32_t cap = 0u;
  uint32_t page = SPI_NOR_DEFAULT_PAGE;
  uint32_t sector = SPI_NOR_DEFAULT_SECTOR;
  uint32_t block = SPI_NOR_DEFAULT_BLOCK;
  const char *name = "generic";

  if (k) {
    cap = k->capacity;
    page = k->page;
    sector = k->sector;
    block = k->block;
    name = k->name;
  } else {
    cap = capacity_from_jedec_id(dev->info.capacity_id);
  }

  /* Ручной override имеет приоритет. */
  if (dev->cfg.capacity_bytes) {
    cap = dev->cfg.capacity_bytes;
  }
  if (dev->cfg.page_size) {
    page = dev->cfg.page_size;
  }
  if (dev->cfg.sector_size) {
    sector = dev->cfg.sector_size;
  }
  if (dev->cfg.block_size) {
    block = dev->cfg.block_size;
  }

  dev->info.capacity_bytes = cap;
  dev->info.page_size = page;
  dev->info.sector_size = sector;
  dev->info.block_size = block;
  dev->info.name = name;
}

int spi_nor_init(spi_nor_t *dev, const spi_nor_hal_t *hal, const spi_nor_config_t *cfg)
{
  if (!dev || !hal || !hal->spi_xfer || !hal->cs_assert || !hal->cs_deassert || !hal->delay_us) {
    return SPI_NOR_ERR_PARAM;
  }
  memset(dev, 0, sizeof(*dev));
  dev->hal = hal;
  if (cfg) {
    dev->cfg = *cfg;
  }
  if (dev->cfg.busy_timeout_ms == 0u) {
    dev->cfg.busy_timeout_ms = SPI_NOR_DEFAULT_TIMEOUT_MS;
  }
  /* Геометрия по умолчанию до probe — безопасные типичные значения, без 4 МБ. */
  dev->info.page_size = dev->cfg.page_size ? dev->cfg.page_size : SPI_NOR_DEFAULT_PAGE;
  dev->info.sector_size = dev->cfg.sector_size ? dev->cfg.sector_size : SPI_NOR_DEFAULT_SECTOR;
  dev->info.block_size = dev->cfg.block_size ? dev->cfg.block_size : SPI_NOR_DEFAULT_BLOCK;
  dev->info.capacity_bytes = dev->cfg.capacity_bytes; /* 0 до probe, если не задано */
  dev->info.name = "unprobed";
  return SPI_NOR_OK;
}

int spi_nor_read_jedec_id(spi_nor_t *dev, uint8_t id_out[3])
{
  uint8_t tx[4];
  uint8_t rx[4];
  int rc;

  if (!dev || !dev->hal || !id_out) {
    return SPI_NOR_ERR_PARAM;
  }
  tx[0] = SPI_NOR_CMD_JEDEC_ID;
  tx[1] = 0xFFu;
  tx[2] = 0xFFu;
  tx[3] = 0xFFu;
  rc = xfer_cs(dev->hal, tx, rx, 4u);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  id_out[0] = rx[1];
  id_out[1] = rx[2];
  id_out[2] = rx[3];
  return SPI_NOR_OK;
}

int spi_nor_read_device_id(spi_nor_t *dev, uint8_t *device_id)
{
  /* ABh + 3 dummy → 1 байт device ID (LE25U40CMC = 0x6E). */
  uint8_t tx[5];
  uint8_t rx[5];
  int rc;

  if (!dev || !dev->hal || !device_id) {
    return SPI_NOR_ERR_PARAM;
  }
  memset(tx, 0xFF, sizeof(tx));
  tx[0] = SPI_NOR_CMD_READ_ID;
  rc = xfer_cs(dev->hal, tx, rx, 5u);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  *device_id = rx[4];
  return SPI_NOR_OK;
}

int spi_nor_probe(spi_nor_t *dev)
{
  uint8_t id[3];
  uint8_t did = 0;
  const spi_nor_known_t *k;
  int rc;

  if (!dev) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = spi_nor_read_jedec_id(dev, id);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  (void)spi_nor_read_device_id(dev, &did);

  dev->info.manufacturer_id = id[0];
  dev->info.memory_type = id[1];
  dev->info.capacity_id = id[2];
  dev->info.device_id = did;

  k = lookup_jedec(id[0], id[1], id[2]);
  apply_geometry(dev, k);

  if (dev->info.capacity_bytes == 0u) {
    return SPI_NOR_ERR_UNKNOWN_ID;
  }
  dev->probed = 1;
  return SPI_NOR_OK;
}

int spi_nor_read_status(spi_nor_t *dev, uint8_t *status)
{
  uint8_t tx[2] = { SPI_NOR_CMD_READ_STATUS, 0xFFu };
  uint8_t rx[2];
  int rc;

  if (!dev || !status) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = xfer_cs(dev->hal, tx, rx, 2u);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  *status = rx[1];
  return SPI_NOR_OK;
}

int spi_nor_is_busy(spi_nor_t *dev, int *busy)
{
  uint8_t st;
  int rc;

  if (!busy) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = spi_nor_read_status(dev, &st);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  *busy = (st & SPI_NOR_SR_WIP) ? 1 : 0;
  return SPI_NOR_OK;
}

int spi_nor_wait_ready(spi_nor_t *dev, uint32_t timeout_ms)
{
  uint32_t elapsed = 0u;
  int busy = 1;
  int rc;

  if (!dev || !dev->hal) {
    return SPI_NOR_ERR_PARAM;
  }
  if (timeout_ms == 0u) {
    timeout_ms = SPI_NOR_DEFAULT_TIMEOUT_MS;
  }

  while (elapsed <= timeout_ms) {
    rc = spi_nor_is_busy(dev, &busy);
    if (rc != SPI_NOR_OK) {
      return rc;
    }
    if (!busy) {
      return SPI_NOR_OK;
    }
    delay_ms_hal(dev->hal, 1u);
    elapsed++;
  }
  return SPI_NOR_ERR_TIMEOUT;
}

int spi_nor_write_enable(spi_nor_t *dev)
{
  if (!dev) {
    return SPI_NOR_ERR_PARAM;
  }
  return cmd1(dev, SPI_NOR_CMD_WRITE_ENABLE);
}

int spi_nor_write_disable(spi_nor_t *dev)
{
  if (!dev) {
    return SPI_NOR_ERR_PARAM;
  }
  return cmd1(dev, SPI_NOR_CMD_WRITE_DISABLE);
}

int spi_nor_read(spi_nor_t *dev, uint32_t addr, uint8_t *buf, size_t len)
{
  uint8_t hdr[4];
  int rc;

  if (!dev || !buf) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = check_range(dev, addr, len);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  rc = ensure_ready(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }

  hdr[0] = SPI_NOR_CMD_READ;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFu);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFu);
  hdr[3] = (uint8_t)(addr & 0xFFu);

  if (!dev->hal->cs_assert || !dev->hal->cs_deassert) {
    return SPI_NOR_ERR_PARAM;
  }
  dev->hal->cs_assert(dev->hal->user);
  rc = xfer(dev->hal, hdr, NULL, 4u);
  if (rc == SPI_NOR_OK) {
    rc = xfer(dev->hal, NULL, buf, len);
  }
  dev->hal->cs_deassert(dev->hal->user);
  return rc;
}

int spi_nor_page_program(spi_nor_t *dev, uint32_t addr, const uint8_t *data, size_t len)
{
  uint8_t hdr[4];
  uint32_t page_off;
  int rc;

  if (!dev || !data || len == 0u) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = check_range(dev, addr, len);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  if (len > dev->info.page_size) {
    return SPI_NOR_ERR_RANGE;
  }
  page_off = addr % dev->info.page_size;
  if (page_off + (uint32_t)len > dev->info.page_size) {
    return SPI_NOR_ERR_RANGE;
  }

  rc = ensure_ready(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  rc = spi_nor_write_enable(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }

  hdr[0] = SPI_NOR_CMD_PAGE_PROGRAM;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFu);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFu);
  hdr[3] = (uint8_t)(addr & 0xFFu);

  dev->hal->cs_assert(dev->hal->user);
  rc = xfer(dev->hal, hdr, NULL, 4u);
  if (rc == SPI_NOR_OK) {
    rc = xfer(dev->hal, data, NULL, len);
  }
  dev->hal->cs_deassert(dev->hal->user);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  return ensure_ready(dev);
}

int spi_nor_write(spi_nor_t *dev, uint32_t addr, const uint8_t *data, size_t len)
{
  size_t done = 0u;
  int rc;

  if (!dev || !data || len == 0u) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = check_range(dev, addr, len);
  if (rc != SPI_NOR_OK) {
    return rc;
  }

  while (done < len) {
    uint32_t cur = addr + (uint32_t)done;
    uint32_t page_off = cur % dev->info.page_size;
    size_t chunk = (size_t)(dev->info.page_size - page_off);
    if (chunk > (len - done)) {
      chunk = len - done;
    }
    rc = spi_nor_page_program(dev, cur, data + done, chunk);
    if (rc != SPI_NOR_OK) {
      return rc;
    }
    done += chunk;
  }
  return SPI_NOR_OK;
}

static int erase_cmd_addr(spi_nor_t *dev, uint8_t opcode, uint32_t addr)
{
  uint8_t tx[4];
  int rc;

  rc = ensure_ready(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  rc = spi_nor_write_enable(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  tx[0] = opcode;
  tx[1] = (uint8_t)((addr >> 16) & 0xFFu);
  tx[2] = (uint8_t)((addr >> 8) & 0xFFu);
  tx[3] = (uint8_t)(addr & 0xFFu);
  rc = xfer_cs(dev->hal, tx, NULL, 4u);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  return ensure_ready(dev);
}

int spi_nor_sector_erase(spi_nor_t *dev, uint32_t addr)
{
  int rc;
  if (!dev || !dev->probed) {
    return SPI_NOR_ERR_PARAM;
  }
  if (addr >= dev->info.capacity_bytes) {
    return SPI_NOR_ERR_RANGE;
  }
  addr -= (addr % dev->info.sector_size);
  rc = erase_cmd_addr(dev, SPI_NOR_CMD_SECTOR_ERASE_4K, addr);
  return rc;
}

int spi_nor_block_erase(spi_nor_t *dev, uint32_t addr)
{
  if (!dev || !dev->probed) {
    return SPI_NOR_ERR_PARAM;
  }
  if (addr >= dev->info.capacity_bytes) {
    return SPI_NOR_ERR_RANGE;
  }
  addr -= (addr % dev->info.block_size);
  return erase_cmd_addr(dev, SPI_NOR_CMD_BLOCK_ERASE_64K, addr);
}

int spi_nor_chip_erase(spi_nor_t *dev)
{
  int rc;
  if (!dev || !dev->probed) {
    return SPI_NOR_ERR_PARAM;
  }
  rc = ensure_ready(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  rc = spi_nor_write_enable(dev);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  /* C7h — общий; LE25 также принимает 60h — используем C7h. */
  rc = cmd1(dev, SPI_NOR_CMD_CHIP_ERASE);
  if (rc != SPI_NOR_OK) {
    return rc;
  }
  return ensure_ready(dev);
}

int spi_nor_read_sfdp(spi_nor_t *dev, uint32_t addr, uint8_t *buf, size_t len)
{
  uint8_t hdr[5];
  int rc;

  if (!dev || !buf || len == 0u) {
    return SPI_NOR_ERR_PARAM;
  }
  /* LE25U40CMC SFDP обычно не поддерживает — вызывающий проверяет сигнатуру. */
  hdr[0] = SPI_NOR_CMD_READ_SFDP;
  hdr[1] = (uint8_t)((addr >> 16) & 0xFFu);
  hdr[2] = (uint8_t)((addr >> 8) & 0xFFu);
  hdr[3] = (uint8_t)(addr & 0xFFu);
  hdr[4] = 0xFFu; /* 1 dummy */

  dev->hal->cs_assert(dev->hal->user);
  rc = xfer(dev->hal, hdr, NULL, 5u);
  if (rc == SPI_NOR_OK) {
    rc = xfer(dev->hal, NULL, buf, len);
  }
  dev->hal->cs_deassert(dev->hal->user);
  return rc;
}

const char *spi_nor_strerror(int err)
{
  switch (err) {
  case SPI_NOR_OK: return "ok";
  case SPI_NOR_ERR_PARAM: return "bad parameter";
  case SPI_NOR_ERR_HAL: return "HAL / bus error";
  case SPI_NOR_ERR_TIMEOUT: return "busy timeout";
  case SPI_NOR_ERR_RANGE: return "address out of range";
  case SPI_NOR_ERR_BUSY: return "device busy";
  case SPI_NOR_ERR_UNKNOWN_ID: return "unknown JEDEC ID / no capacity";
  case SPI_NOR_ERR_UNSUPPORTED: return "unsupported";
  default: return "unknown error";
  }
}
