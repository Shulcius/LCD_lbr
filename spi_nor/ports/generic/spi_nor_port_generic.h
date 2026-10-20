/**
 * @file spi_nor_port_generic.h
 * @brief Заглушка HAL под bare-metal / RISC-V / «китайский» МК.
 *
 * Скопируйте .c/.h в свой проект и реализуйте функции с пометкой TODO.
 * Ядро spi_nor от этого порта не зависит — только от spi_nor_hal_t.
 */
#ifndef SPI_NOR_PORT_GENERIC_H
#define SPI_NOR_PORT_GENERIC_H

#include "spi_nor_hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct spi_nor_generic_pins {
  /* Логические идентификаторы ног МК — заполните под свою плату. */
  int cs;
  int sck;
  int mosi;
  int miso;
  int wp;   /* -1 если не подключен (на чипе подтянуть к VDD) */
  int hold; /* -1 если не подключен */
} spi_nor_generic_pins_t;

typedef struct spi_nor_generic_ctx {
  spi_nor_generic_pins_t pins;
  void *spi_hw; /* указатель на регистры/дескриптор SPI контроллера */
} spi_nor_generic_ctx_t;

/**
 * Собрать hal из колбэков ниже.
 * До вызова spi_nor_probe() реализуйте и проверьте spi_xfer / CS / delay.
 */
int spi_nor_generic_hal_init(spi_nor_generic_ctx_t *ctx,
                             const spi_nor_generic_pins_t *pins,
                             spi_nor_hal_t *hal_out);

#ifdef __cplusplus
}
#endif

#endif /* SPI_NOR_PORT_GENERIC_H */
