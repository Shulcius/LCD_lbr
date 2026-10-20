/**
 * @file spi_nor_port_arduino.h
 * @brief Arduino / PlatformIO порт HAL для spi_nor.
 *
 * Ядро spi_nor Arduino не использует — только этот порт.
 * Пины задаются в spi_nor_arduino_pins_t (не зашиты под LE25).
 */
#ifndef SPI_NOR_PORT_ARDUINO_H
#define SPI_NOR_PORT_ARDUINO_H

#include "spi_nor_hal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spi_nor_arduino_pins {
  int8_t cs;    /**< обязателен */
  int8_t sck;   /**< -1 = аппаратный SPI (SPI библиотека) */
  int8_t mosi;  /**< -1 = аппаратный SPI */
  int8_t miso;  /**< -1 = аппаратный SPI */
  int8_t wp;    /**< -1 = не использовать; иначе подтянуть HIGH */
  int8_t hold;  /**< -1 = не использовать; иначе подтянуть HIGH */
  uint32_t spi_hz; /**< 0 → 1 MHz по умолчанию; LE25 до 40 MHz */
} spi_nor_arduino_pins_t;

typedef struct spi_nor_arduino_ctx {
  spi_nor_arduino_pins_t pins;
  int ready;
} spi_nor_arduino_ctx_t;

/**
 * Инициализировать пины/SPI и заполнить hal.
 * @return 0 OK, <0 ошибка.
 */
int spi_nor_arduino_hal_init(spi_nor_arduino_ctx_t *ctx,
                             const spi_nor_arduino_pins_t *pins,
                             spi_nor_hal_t *hal_out);

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_PORT_ARDUINO_H */
