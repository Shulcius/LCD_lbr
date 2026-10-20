/**
 * Минимальный пример: прочитать JEDEC ID и объём LE25U40CMC.
 *
 * Ожидание для LE25U40CMC: mfr=0x62 type=0x06 cap=0x13 → 524288 байт.
 * Device ID (ABh): 0x6E.
 *
 * ПИНЫ МК — замените под свою плату. Не путать с пинами корпуса SOIC-8:
 *   корпус pin6 = SCK, pin7 = HOLD (у W25Q часто наоборот).
 * VDD чипа: 3.3 V.
 */
#include <SPI.h>

#include "spi_nor.h"
#include "spi_nor_port_arduino.h"
#include "le25u40cmc.h"

/* === ПИНЫ МК (заполните сами) === */
#ifndef FLASH_PIN_CS
#define FLASH_PIN_CS   10
#endif
#ifndef FLASH_PIN_WP
#define FLASH_PIN_WP   -1   /* -1 = не использовать GPIO; на чипе подтянуть к 3.3V */
#endif
#ifndef FLASH_PIN_HOLD
#define FLASH_PIN_HOLD -1
#endif

static spi_nor_arduino_ctx_t g_actx;
static spi_nor_hal_t g_hal;
static spi_nor_t g_nor;

void setup()
{
  Serial.begin(115200);
  while (!Serial) { /* USB CDC */ }

  spi_nor_arduino_pins_t pins;
  pins.cs = FLASH_PIN_CS;
  pins.sck = -1;
  pins.mosi = -1;
  pins.miso = -1;
  pins.wp = FLASH_PIN_WP;
  pins.hold = FLASH_PIN_HOLD;
  pins.spi_hz = 1000000UL;

  if (spi_nor_arduino_hal_init(&g_actx, &pins, &g_hal) != 0) {
    Serial.println(F("HAL init failed"));
    return;
  }

  /* Профиль LE25U40CMC: 512 KiB, не 4 МБ */
  if (spi_nor_init(&g_nor, &g_hal, &le25u40cmc_config) != SPI_NOR_OK) {
    Serial.println(F("spi_nor_init failed"));
    return;
  }

  int rc = spi_nor_probe(&g_nor);
  Serial.print(F("probe rc="));
  Serial.println(rc);
  Serial.print(F("JEDEC: "));
  Serial.print(g_nor.info.manufacturer_id, HEX);
  Serial.print(' ');
  Serial.print(g_nor.info.memory_type, HEX);
  Serial.print(' ');
  Serial.println(g_nor.info.capacity_id, HEX);
  Serial.print(F("device_id="));
  Serial.println(g_nor.info.device_id, HEX);
  Serial.print(F("name="));
  Serial.println(g_nor.info.name);
  Serial.print(F("capacity_bytes="));
  Serial.println(g_nor.info.capacity_bytes);

  if (le25u40cmc_id_match(&g_nor.info)) {
    Serial.println(F("ID match: LE25U40CMC"));
  } else {
    Serial.println(F("ID mismatch — проверьте проводку / чип"));
  }
}

void loop() {}
