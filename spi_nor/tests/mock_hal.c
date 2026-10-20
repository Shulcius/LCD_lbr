#include "mock_hal.h"

#include <stdlib.h>
#include <string.h>

static void mock_finish_erase_program(mock_flash_t *f)
{
  f->status &= (uint8_t)~0x01u; /* clear WIP */
  f->status &= (uint8_t)~0x02u; /* clear WEL */
  f->programming = 0;
  f->erasing = 0;
}

static void mock_on_cs_high(mock_flash_t *f)
{
  if (f->programming || f->erasing) {
    /* мгновенно завершаем для unit-тестов */
    mock_finish_erase_program(f);
  }
  f->phase = 0;
  f->cmd = 0;
  f->addr = 0;
  f->addr_bytes_got = 0;
  f->dummy_left = 0;
}

static uint8_t mock_byte(mock_flash_t *f, uint8_t tin)
{
  uint8_t tout = 0xFFu;

  if (!f->cs_low) {
    return 0xFFu;
  }

  if (f->phase == 0) {
    f->cmd = tin;
    f->phase = 1;
    f->addr = 0;
    f->addr_bytes_got = 0;
    f->dummy_left = 0;
    /* Однобайтовые команды применяются сразу (WREN/WRDI/chip erase). */
    if (tin == 0x06u) {
      f->status |= 0x02u;
    } else if (tin == 0x04u) {
      f->status &= (uint8_t)~0x02u;
    } else if (tin == 0xC7u || tin == 0x60u) {
      if (f->status & 0x02u) {
        memset(f->mem, 0xFF, f->size);
        f->erasing = 1;
        f->status |= 0x01u;
      }
    }
    return 0xFFu;
  }

  switch (f->cmd) {
  case 0x9Fu: /* JEDEC */
    if (f->phase == 1) { tout = f->jedec[0]; f->phase = 2; }
    else if (f->phase == 2) { tout = f->jedec[1]; f->phase = 3; }
    else if (f->phase == 3) { tout = f->jedec[2]; f->phase = 4; }
    else { tout = 0x00u; }
    break;

  case 0xABu: /* device id after 3 dummy */
    if (f->addr_bytes_got < 3) {
      f->addr_bytes_got++;
      tout = 0xFFu;
    } else {
      tout = f->device_id;
    }
    break;

  case 0x05u: /* status */
    tout = f->status;
    break;

  case 0x06u:
  case 0x04u:
    break;

  case 0x03u: /* read */
    if (f->addr_bytes_got < 3) {
      f->addr = (f->addr << 8) | tin;
      f->addr_bytes_got++;
    } else {
      if (f->addr < f->size) {
        tout = f->mem[f->addr];
      }
      f->addr++;
    }
    break;

  case 0x02u: /* page program */
    if (f->addr_bytes_got < 3) {
      f->addr = (f->addr << 8) | tin;
      f->addr_bytes_got++;
    } else {
      if ((f->status & 0x02u) && f->addr < f->size) {
        f->mem[f->addr] &= tin; /* NOR: only 1→0 */
        f->programming = 1;
        f->status |= 0x01u;
      }
      f->addr++;
    }
    break;

  case 0x20u: /* 4K erase */
  case 0xD8u: /* 64K erase */
    if (f->addr_bytes_got < 3) {
      f->addr = (f->addr << 8) | tin;
      f->addr_bytes_got++;
      if (f->addr_bytes_got == 3 && (f->status & 0x02u)) {
        size_t off, len, i;
        if (f->cmd == 0x20u) {
          off = (size_t)(f->addr & ~0xFFFu);
          len = 4096u;
        } else {
          off = (size_t)(f->addr & ~0xFFFFu);
          len = 65536u;
        }
        if (off < f->size) {
          if (off + len > f->size) {
            len = f->size - off;
          }
          for (i = 0; i < len; i++) {
            f->mem[off + i] = 0xFFu;
          }
        }
        f->erasing = 1;
        f->status |= 0x01u;
      }
    }
    break;

  case 0xC7u:
  case 0x60u:
    break;

  case 0x5Au: /* SFDP — нет у LE25: отдаём не-SFDP */
    if (f->addr_bytes_got < 3) {
      f->addr = (f->addr << 8) | tin;
      f->addr_bytes_got++;
    } else if (f->dummy_left == 0) {
      f->dummy_left = 1; /* consume dummy by returning FF next path */
      /* first data phase after dummy handled below on subsequent bytes */
      f->phase = 10;
    }
    break;

  default:
    break;
  }

  if (f->cmd == 0x5Au && f->phase == 10) {
    /* после dummy — мусор без сигнатуры SFDP */
    tout = 0xFFu;
  }

  (void)tin;
  return tout;
}

static int mock_spi_xfer(void *user, const uint8_t *tx, uint8_t *rx, size_t len)
{
  mock_flash_t *f = (mock_flash_t *)user;
  size_t i;
  for (i = 0; i < len; i++) {
    uint8_t tin = tx ? tx[i] : 0xFFu;
    uint8_t tout = mock_byte(f, tin);
    if (rx) {
      rx[i] = tout;
    }
  }
  return 0;
}

static void mock_cs_assert(void *user)
{
  mock_flash_t *f = (mock_flash_t *)user;
  f->cs_low = 1;
  f->phase = 0;
  f->cmd = 0;
  f->addr = 0;
  f->addr_bytes_got = 0;
  f->dummy_left = 0;
}

static void mock_cs_deassert(void *user)
{
  mock_flash_t *f = (mock_flash_t *)user;
  mock_on_cs_high(f);
  f->cs_low = 0;
}

static void mock_delay_us(void *user, uint32_t us)
{
  (void)user;
  (void)us;
}

static void mock_delay_ms(void *user, uint32_t ms)
{
  (void)user;
  (void)ms;
}

int mock_flash_init(mock_flash_t *f, size_t size, uint8_t mfr, uint8_t type, uint8_t cap, uint8_t did)
{
  if (!f || size == 0) {
    return -1;
  }
  memset(f, 0, sizeof(*f));
  f->mem = (uint8_t *)malloc(size);
  if (!f->mem) {
    return -1;
  }
  memset(f->mem, 0xFF, size);
  f->size = size;
  f->jedec[0] = mfr;
  f->jedec[1] = type;
  f->jedec[2] = cap;
  f->device_id = did;
  return 0;
}

void mock_flash_free(mock_flash_t *f)
{
  if (f && f->mem) {
    free(f->mem);
    f->mem = NULL;
  }
}

void mock_hal_bind(mock_flash_t *f, spi_nor_hal_t *hal)
{
  hal->spi_xfer = mock_spi_xfer;
  hal->cs_assert = mock_cs_assert;
  hal->cs_deassert = mock_cs_deassert;
  hal->delay_us = mock_delay_us;
  hal->delay_ms = mock_delay_ms;
  hal->gpio_write = NULL;
  hal->user = f;
}
