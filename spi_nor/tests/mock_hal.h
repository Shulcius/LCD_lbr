#ifndef SPI_NOR_MOCK_HAL_H
#define SPI_NOR_MOCK_HAL_H

#include "spi_nor_hal.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mock_flash {
  uint8_t *mem;
  size_t size;
  uint8_t jedec[3];
  uint8_t device_id;
  uint8_t status; /* WIP|WEL… */
  int cs_low;
  /* состояние парсера транзакции */
  int phase; /* 0 idle after CS high */
  uint8_t cmd;
  uint32_t addr;
  int addr_bytes_got;
  int dummy_left;
  int programming;
  int erasing;
} mock_flash_t;

int mock_flash_init(mock_flash_t *f, size_t size, uint8_t mfr, uint8_t type, uint8_t cap, uint8_t did);
void mock_flash_free(mock_flash_t *f);
void mock_hal_bind(mock_flash_t *f, spi_nor_hal_t *hal);

#ifdef __cplusplus
}
#endif

#endif
