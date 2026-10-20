/**
 * Пример «подключи свой HAL» для RISC-V / bare-metal.
 *
 * Соберите с: -I../../include -I../../boards/le250u40_cmb -I../../ports/generic
 *             ../../src/spi_nor.c ../../ports/generic/spi_nor_port_generic.c
 *
 * Сейчас platform_* в generic-порте — заглушки (spi_xfer вернёт ошибку).
 * Реализуйте их под свой МК, затем этот main станет рабочим smoke-тестом.
 */
#include <stdio.h>

#include "spi_nor.h"
#include "spi_nor_port_generic.h"
#include "board_config.h"

/* Пины МК — плейсхолдеры, замените на реальные номера/маски */
#ifndef BOARD_FLASH_CS
#define BOARD_FLASH_CS   0
#endif
#ifndef BOARD_FLASH_SCK
#define BOARD_FLASH_SCK  1
#endif
#ifndef BOARD_FLASH_MOSI
#define BOARD_FLASH_MOSI 2
#endif
#ifndef BOARD_FLASH_MISO
#define BOARD_FLASH_MISO 3
#endif

int main(void)
{
  spi_nor_generic_ctx_t ctx;
  spi_nor_hal_t hal;
  spi_nor_t nor;
  spi_nor_generic_pins_t pins;
  int rc;

  pins.cs = BOARD_FLASH_CS;
  pins.sck = BOARD_FLASH_SCK;
  pins.mosi = BOARD_FLASH_MOSI;
  pins.miso = BOARD_FLASH_MISO;
  pins.wp = -1;
  pins.hold = -1;
  ctx.spi_hw = NULL; /* TODO: указатель на SPI-контроллер */

  if (spi_nor_generic_hal_init(&ctx, &pins, &hal) != 0) {
    printf("generic HAL init failed\n");
    return 1;
  }

  /* Дефолт профиля LE250U40: 524288 байт, не 0x400000 */
  rc = spi_nor_init(&nor, &hal, &le25u40cmc_config);
  if (rc != SPI_NOR_OK) {
    printf("init: %s\n", spi_nor_strerror(rc));
    return 1;
  }

  rc = spi_nor_probe(&nor);
  printf("probe: %s\n", spi_nor_strerror(rc));
  if (rc == SPI_NOR_OK) {
    printf("JEDEC %02X %02X %02X  device=%02X  name=%s  bytes=%lu\n",
           nor.info.manufacturer_id, nor.info.memory_type, nor.info.capacity_id,
           nor.info.device_id, nor.info.name,
           (unsigned long)nor.info.capacity_bytes);
  }
  return (rc == SPI_NOR_OK) ? 0 : 2;
}
