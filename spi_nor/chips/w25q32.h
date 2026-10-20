/**
 * @file w25q32.h
 * @brief Пресет чипа W25Q32/GD25Q32-класса (4 МБ) — не конфиг пинов МК.
 *
 * НЕ дефолт для LE25U40CMC. Подключайте только при JEDEC capacity 0x16
 * (4 194 304 байт).
 */
#ifndef SPI_NOR_CHIP_W25Q32_H
#define SPI_NOR_CHIP_W25Q32_H

#include "spi_nor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q32_CAPACITY_BYTES  4194304u  /* 0x400000 — 32 Mbit = 4 MiB */
#define W25Q32_PAGE_SIZE       256u
#define W25Q32_SECTOR_SIZE     4096u
#define W25Q32_BLOCK_SIZE      65536u

static const spi_nor_config_t w25q32_config = {
  .capacity_bytes  = W25Q32_CAPACITY_BYTES,
  .page_size       = W25Q32_PAGE_SIZE,
  .sector_size     = W25Q32_SECTOR_SIZE,
  .block_size      = W25Q32_BLOCK_SIZE,
  .busy_timeout_ms = 60000u
};

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_CHIP_W25Q32_H */
