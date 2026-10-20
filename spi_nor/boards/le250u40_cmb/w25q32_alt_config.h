/**
 * @file w25q32_alt_config.h
 * @brief Альтернативный target: Winbond/GD 25Q32 (4 МБ).
 *
 * НЕ является дефолтом для профиля LE250U40 / LE25U40CMC.
 * Подключайте только если на плате реально стоит 25Q32-класс
 * (JEDEC capacity 0x16 → 4 194 304 байт).
 */
#ifndef W25Q32_ALT_CONFIG_H
#define W25Q32_ALT_CONFIG_H

#include "spi_nor.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W25Q32_CAPACITY_BYTES  4194304u  /* 0x400000 — 32 Mbit = 4 MiB */
#define W25Q32_PAGE_SIZE       256u
#define W25Q32_SECTOR_SIZE     4096u
#define W25Q32_BLOCK_SIZE      65536u

static const spi_nor_config_t w25q32_alt_config = {
  .capacity_bytes  = W25Q32_CAPACITY_BYTES,
  .page_size       = W25Q32_PAGE_SIZE,
  .sector_size     = W25Q32_SECTOR_SIZE,
  .block_size      = W25Q32_BLOCK_SIZE,
  .busy_timeout_ms = 60000u
};

#ifdef __cplusplus
}
#endif

#endif /* W25Q32_ALT_CONFIG_H */
