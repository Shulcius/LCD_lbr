/**
 * @file spi_nor_hal.h
 * @brief Тонкий HAL для spi_nor — без Arduino / без привязки к МК.
 *
 * Порт обязан реализовать spi_xfer, cs_assert/deassert, delay_us (и желательно delay_ms).
 * GPIO (WP/HOLD) — опционально.
 *
 * EN: Thin HAL for portable SPI NOR driver. Core never includes Arduino.h.
 */
#ifndef SPI_NOR_HAL_H
#define SPI_NOR_HAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Полнодуплексный SPI-обмен: одновременно tx и rx длиной len.
 * Допускается tx == NULL (на шину идут 0xFF) или rx == NULL (ответ игнорируется).
 * @return 0 при успехе, <0 при ошибке шины.
 */
typedef int (*spi_nor_spi_xfer_fn)(void *ctx, const uint8_t *tx, uint8_t *rx, size_t len);

typedef void (*spi_nor_cs_fn)(void *ctx);
typedef void (*spi_nor_delay_us_fn)(void *ctx, uint32_t us);
typedef void (*spi_nor_delay_ms_fn)(void *ctx, uint32_t ms);

/** Опционально: уровень GPIO (например WP#, HOLD#). value: 0 = low, 1 = high. */
typedef void (*spi_nor_gpio_write_fn)(void *ctx, int pin_id, int value);

enum {
  SPI_NOR_GPIO_WP   = 0,
  SPI_NOR_GPIO_HOLD = 1
};

typedef struct spi_nor_hal {
  spi_nor_spi_xfer_fn  spi_xfer;     /**< обязательно */
  spi_nor_cs_fn        cs_assert;    /**< обязательно, CS# = low */
  spi_nor_cs_fn        cs_deassert;  /**< обязательно, CS# = high */
  spi_nor_delay_us_fn  delay_us;     /**< обязательно */
  spi_nor_delay_ms_fn  delay_ms;     /**< опционально; если NULL — эмулируется через delay_us */
  spi_nor_gpio_write_fn gpio_write;  /**< опционально; NULL = не трогать WP/HOLD */
  void *user;                        /**< произвольный контекст порта */
} spi_nor_hal_t;

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_HAL_H */
