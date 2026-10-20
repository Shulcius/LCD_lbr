/**
 * @file le25u40cmc.h
 * @brief Пресет чипа LE25U40CMC (Onsemi/ex-Sanyo) — не конфиг пинов МК.
 *
 * Зачем файл: готовые константы и spi_nor_config_t, чтобы не помнить JEDEC/объём.
 * Пины GPIO хоста задаются в порту / sketch, не здесь.
 *
 * Факты (даташит):
 *   - 4 Mbit = 524288 байт (НЕ 4 МБ / 0x400000)
 *   - JEDEC 9Fh: 62 06 13; Device ID ABh: 0x6E
 *   - VDD 2.3–3.6 V → работать на 3.3 V
 *   - SOIC-8: 1=CS 2=SO 3=WP 4=GND 5=SI 6=SCK 7=HOLD 8=VDD
 *     (у многих W25Q пины 6/7 обратные)
 *
 * Маркер «LE250U40 CMB» ≈ этот чип (CMB → корпус CMC).
 */
#ifndef SPI_NOR_CHIP_LE25U40CMC_H
#define SPI_NOR_CHIP_LE25U40CMC_H

#include "spi_nor.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LE25U40CMC_CAPACITY_BYTES   524288u   /* 0x80000 — 4 Mbit */
#define LE25U40CMC_PAGE_SIZE        256u
#define LE25U40CMC_SECTOR_SIZE      4096u
#define LE25U40CMC_BLOCK_SIZE       65536u
#define LE25U40CMC_ADDR_MAX         0x07FFFFu

#define LE25U40CMC_JEDEC_MFR        0x62u
#define LE25U40CMC_JEDEC_TYPE       0x06u
#define LE25U40CMC_JEDEC_CAP        0x13u
#define LE25U40CMC_DEVICE_ID        0x6Eu

#define LE25U40CMC_VDD_MIN_MV       2300
#define LE25U40CMC_VDD_MAX_MV       3600
#define LE25U40CMC_VDD_NOM_MV       3300

/* Номера ног корпуса SOIC-8 (справка; не GPIO МК) */
enum {
  LE25U40_PKG_PIN_CS   = 1,
  LE25U40_PKG_PIN_SO   = 2,
  LE25U40_PKG_PIN_WP   = 3,
  LE25U40_PKG_PIN_GND  = 4,
  LE25U40_PKG_PIN_SI   = 5,
  LE25U40_PKG_PIN_SCK  = 6,
  LE25U40_PKG_PIN_HOLD = 7,
  LE25U40_PKG_PIN_VDD  = 8
};

/** Готовый конфиг: spi_nor_init(&dev, &hal, &le25u40cmc_config); */
static const spi_nor_config_t le25u40cmc_config = {
  .capacity_bytes  = LE25U40CMC_CAPACITY_BYTES,
  .page_size       = LE25U40CMC_PAGE_SIZE,
  .sector_size     = LE25U40CMC_SECTOR_SIZE,
  .block_size      = LE25U40CMC_BLOCK_SIZE,
  .busy_timeout_ms = 30000u
};

static inline int le25u40cmc_id_match(const spi_nor_info_t *info)
{
  if (!info) {
    return 0;
  }
  return (info->manufacturer_id == LE25U40CMC_JEDEC_MFR &&
          info->memory_type == LE25U40CMC_JEDEC_TYPE &&
          info->capacity_id == LE25U40CMC_JEDEC_CAP) ? 1 : 0;
}

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_CHIP_LE25U40CMC_H */
