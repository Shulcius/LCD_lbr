/**
 * @file spi_nor.h
 * @brief Переносимый драйвер SPI NOR Flash (ядро на чистом C).
 *
 * Профиль по умолчанию для «LE250U40»: LE25U40CMC — **4 Mbit = 524288 байт**, не 4 МБ.
 * Также узнаёт W25Q32/GD25Q32 (4 MiB) по JEDEC, но это другой target.
 * Объём: таблица известных ID / 2^capacity_id / spi_nor_config_t — без хардкода 0x400000.
 *
 * Пресет чипа LE25U40CMC — см. chips/le25u40cmc.h (не пины МК; ядро от него не зависит).
 *
 * EN: Portable SPI NOR driver. No Arduino dependency in this header/source.
 */
#ifndef SPI_NOR_H
#define SPI_NOR_H

#include "spi_nor_hal.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Коды возврата (0 = OK, <0 = ошибка). */
enum {
  SPI_NOR_OK              =  0,
  SPI_NOR_ERR_PARAM       = -1,
  SPI_NOR_ERR_HAL         = -2,
  SPI_NOR_ERR_TIMEOUT     = -3,
  SPI_NOR_ERR_RANGE       = -4,
  SPI_NOR_ERR_BUSY        = -5,
  SPI_NOR_ERR_UNKNOWN_ID  = -6,
  SPI_NOR_ERR_UNSUPPORTED = -7
};

/** Геометрия / идентификация после init / probe. */
typedef struct spi_nor_info {
  uint8_t  manufacturer_id;
  uint8_t  memory_type;
  uint8_t  capacity_id;
  uint8_t  device_id;       /**< из ABH, 0 если не читали */
  uint32_t capacity_bytes;  /**< полный объём */
  uint32_t page_size;       /**< обычно 256 */
  uint32_t sector_size;     /**< erase unit, обычно 4096 */
  uint32_t block_size;      /**< обычно 65536 */
  const char *name;         /**< строка из таблицы или "generic" */
} spi_nor_info_t;

/**
 * Опциональный override геометрии (если JEDEC неизвестен или нужно форсировать).
 * Поле = 0 → «не задано, брать из JEDEC/таблицы/дефолта».
 */
typedef struct spi_nor_config {
  uint32_t capacity_bytes;
  uint32_t page_size;
  uint32_t sector_size;
  uint32_t block_size;
  uint32_t busy_timeout_ms; /**< 0 → 30000 мс по умолчанию */
} spi_nor_config_t;

typedef struct spi_nor {
  const spi_nor_hal_t *hal;
  spi_nor_info_t       info;
  spi_nor_config_t     cfg;
  int                  probed;
} spi_nor_t;

/**
 * Привязать HAL и (опционально) конфиг. Не трогает шину.
 * @param cfg NULL — дефолты (page 256, sector 4K, block 64K, timeout 30 s).
 */
int spi_nor_init(spi_nor_t *dev, const spi_nor_hal_t *hal, const spi_nor_config_t *cfg);

/**
 * Прочитать JEDEC ID (9Fh), заполнить info, сопоставить известный чип.
 * Если ID неизвестен, ёмкость берётся как 2^capacity_id (стандарт JEDEC),
 * либо из cfg->capacity_bytes.
 */
int spi_nor_probe(spi_nor_t *dev);

/** JEDEC ID без изменения геометрии (3 байта mfr/type/cap). */
int spi_nor_read_jedec_id(spi_nor_t *dev, uint8_t id_out[3]);

/** Device ID через ABh (после 3 dummy-байт) — полезно для LE25U40 (ожидание 0x6E). */
int spi_nor_read_device_id(spi_nor_t *dev, uint8_t *device_id);

int spi_nor_read_status(spi_nor_t *dev, uint8_t *status);
int spi_nor_is_busy(spi_nor_t *dev, int *busy);
int spi_nor_wait_ready(spi_nor_t *dev, uint32_t timeout_ms);

int spi_nor_write_enable(spi_nor_t *dev);
int spi_nor_write_disable(spi_nor_t *dev);

/**
 * Чтение данных (команда 03h, 24-битный адрес).
 * Адрес + len должны укладываться в capacity_bytes.
 */
int spi_nor_read(spi_nor_t *dev, uint32_t addr, uint8_t *buf, size_t len);

/**
 * Программирование страницы (02h). Данные не должны пересекать границу page_size
 * относительно addr; len ∈ (0 .. page_size], addr+len не за page boundary.
 * Перед вызовом сектор должен быть стёрт (ячейки 0xFF).
 */
int spi_nor_page_program(spi_nor_t *dev, uint32_t addr, const uint8_t *data, size_t len);

/**
 * Запись произвольной длины: разбивает на page program.
 * Не делает erase — вызывающий код стирает сам.
 */
int spi_nor_write(spi_nor_t *dev, uint32_t addr, const uint8_t *data, size_t len);

/** Стереть сектор (обычно 4 KiB), addr выравнивается вниз к sector_size. */
int spi_nor_sector_erase(spi_nor_t *dev, uint32_t addr);

/** Стереть блок (обычно 64 KiB). */
int spi_nor_block_erase(spi_nor_t *dev, uint32_t addr);

/** Полная очистка чипа. */
int spi_nor_chip_erase(spi_nor_t *dev);

/**
 * Опционально: READ SFDP (5Ah). Многие старые LE25 SFDP не имеют —
 * тогда вернётся SPI_NOR_ERR_UNSUPPORTED или мусор (проверяйте сигнатуру 'SFDP').
 */
int spi_nor_read_sfdp(spi_nor_t *dev, uint32_t addr, uint8_t *buf, size_t len);

/** Справка: строка ошибки (EN/коротко). */
const char *spi_nor_strerror(int err);

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_H */
