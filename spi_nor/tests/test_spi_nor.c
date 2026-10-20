/**
 * Host-side smoke tests for spi_nor (mock HAL ≈ LE25U40CMC).
 */
#include "spi_nor.h"
#include "mock_hal.h"

#include <stdio.h>
#include <string.h>

#define LE_SIZE 524288u

static int g_fail;

static void expect_eq_u32(const char *name, uint32_t got, uint32_t want)
{
  if (got != want) {
    printf("FAIL %s: got %lu want %lu\n", name, (unsigned long)got, (unsigned long)want);
    g_fail++;
  } else {
    printf("OK   %s\n", name);
  }
}

static void expect_eq_i(const char *name, int got, int want)
{
  if (got != want) {
    printf("FAIL %s: got %d want %d\n", name, got, want);
    g_fail++;
  } else {
    printf("OK   %s\n", name);
  }
}

static void expect_eq_u8(const char *name, uint8_t got, uint8_t want)
{
  if (got != want) {
    printf("FAIL %s: got 0x%02X want 0x%02X\n", name, got, want);
    g_fail++;
  } else {
    printf("OK   %s\n", name);
  }
}

int main(void)
{
  mock_flash_t flash;
  spi_nor_hal_t hal;
  spi_nor_t nor;
  spi_nor_config_t cfg;
  uint8_t buf[64];
  uint8_t pat[64];
  int rc;
  size_t i;

  g_fail = 0;
  memset(&cfg, 0, sizeof(cfg));
  /* Явно LE25U40: 512 KiB — не 4 МБ */
  cfg.capacity_bytes = LE_SIZE;
  cfg.page_size = 256;
  cfg.sector_size = 4096;
  cfg.block_size = 65536;
  cfg.busy_timeout_ms = 1000;

  if (mock_flash_init(&flash, LE_SIZE, 0x62u, 0x06u, 0x13u, 0x6Eu) != 0) {
    printf("mock alloc failed\n");
    return 1;
  }
  mock_hal_bind(&flash, &hal);

  rc = spi_nor_init(&nor, &hal, &cfg);
  expect_eq_i("init", rc, SPI_NOR_OK);

  rc = spi_nor_probe(&nor);
  expect_eq_i("probe", rc, SPI_NOR_OK);
  expect_eq_u8("mfr", nor.info.manufacturer_id, 0x62u);
  expect_eq_u8("type", nor.info.memory_type, 0x06u);
  expect_eq_u8("cap_id", nor.info.capacity_id, 0x13u);
  expect_eq_u8("device_id", nor.info.device_id, 0x6Eu);
  expect_eq_u32("capacity", nor.info.capacity_bytes, LE_SIZE);
  expect_eq_i("not_4MB", nor.info.capacity_bytes == 4194304u ? 1 : 0, 0);
  expect_eq_i("name_le25", strcmp(nor.info.name, "LE25U40CMC") == 0 ? 1 : 0, 1);

  /* erase sector 0 + program + readback */
  rc = spi_nor_sector_erase(&nor, 0);
  expect_eq_i("sector_erase", rc, SPI_NOR_OK);

  for (i = 0; i < sizeof(pat); i++) {
    pat[i] = (uint8_t)(0xA0u + i);
  }
  rc = spi_nor_write(&nor, 0x100u, pat, sizeof(pat));
  expect_eq_i("write", rc, SPI_NOR_OK);

  memset(buf, 0, sizeof(buf));
  rc = spi_nor_read(&nor, 0x100u, buf, sizeof(buf));
  expect_eq_i("read", rc, SPI_NOR_OK);
  expect_eq_i("readback", memcmp(buf, pat, sizeof(pat)) == 0 ? 1 : 0, 1);

  /* range check: за границей 512K */
  rc = spi_nor_read(&nor, LE_SIZE - 10u, buf, 64u);
  expect_eq_i("range_overflow", rc, SPI_NOR_ERR_RANGE);

  mock_flash_free(&flash);

  /* Без таблицы: неизвестный ID + override capacity */
  if (mock_flash_init(&flash, 4096u, 0xAAu, 0xBBu, 0x0Cu, 0x00u) != 0) {
    return 1;
  }
  mock_hal_bind(&flash, &hal);
  memset(&cfg, 0, sizeof(cfg));
  cfg.capacity_bytes = 4096u;
  spi_nor_init(&nor, &hal, &cfg);
  rc = spi_nor_probe(&nor);
  expect_eq_i("generic_probe", rc, SPI_NOR_OK);
  expect_eq_u32("generic_cap", nor.info.capacity_bytes, 4096u);
  mock_flash_free(&flash);

  if (g_fail) {
    printf("\n%d failure(s)\n", g_fail);
    return 1;
  }
  printf("\nAll smoke tests passed.\n");
  return 0;
}
