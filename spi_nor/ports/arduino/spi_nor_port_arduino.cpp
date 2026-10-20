/**
 * @file spi_nor_port_arduino.cpp
 * @brief Реализация Arduino HAL (hardware SPI через SPI.h, mode 0).
 *
 * Питание flash: 3.3 V (LE25U40CMC: 2.3–3.6 V). Не подавать 5 V на VDD чипа.
 */
#include "spi_nor_port_arduino.h"

#include <Arduino.h>
#include <SPI.h>

static uint32_t arduino_hz(const spi_nor_arduino_ctx_t *ctx)
{
  return ctx->pins.spi_hz ? ctx->pins.spi_hz : 1000000u;
}

static int arduino_spi_xfer(void *user, const uint8_t *tx, uint8_t *rx, size_t len)
{
  spi_nor_arduino_ctx_t *ctx = (spi_nor_arduino_ctx_t *)user;
  size_t i;
  if (!ctx || !ctx->ready) {
    return -1;
  }
  SPI.beginTransaction(SPISettings(arduino_hz(ctx), MSBFIRST, SPI_MODE0));
  for (i = 0; i < len; i++) {
    uint8_t out = tx ? tx[i] : (uint8_t)0xFFu;
    uint8_t in = SPI.transfer(out);
    if (rx) {
      rx[i] = in;
    }
  }
  SPI.endTransaction();
  return 0;
}

static void arduino_cs_assert(void *user)
{
  spi_nor_arduino_ctx_t *ctx = (spi_nor_arduino_ctx_t *)user;
  digitalWrite((uint8_t)ctx->pins.cs, LOW);
}

static void arduino_cs_deassert(void *user)
{
  spi_nor_arduino_ctx_t *ctx = (spi_nor_arduino_ctx_t *)user;
  digitalWrite((uint8_t)ctx->pins.cs, HIGH);
}

static void arduino_delay_us(void *user, uint32_t us)
{
  (void)user;
  if (us <= 16383u) {
    delayMicroseconds((unsigned int)us);
    return;
  }
  delay(us / 1000u);
  delayMicroseconds((unsigned int)(us % 1000u));
}

static void arduino_delay_ms(void *user, uint32_t ms)
{
  (void)user;
  delay(ms);
}

static void arduino_gpio_write(void *user, int pin_id, int value)
{
  spi_nor_arduino_ctx_t *ctx = (spi_nor_arduino_ctx_t *)user;
  int8_t pin = -1;
  if (pin_id == SPI_NOR_GPIO_WP) {
    pin = ctx->pins.wp;
  } else if (pin_id == SPI_NOR_GPIO_HOLD) {
    pin = ctx->pins.hold;
  }
  if (pin < 0) {
    return;
  }
  digitalWrite((uint8_t)pin, value ? HIGH : LOW);
}

int spi_nor_arduino_hal_init(spi_nor_arduino_ctx_t *ctx,
                             const spi_nor_arduino_pins_t *pins,
                             spi_nor_hal_t *hal_out)
{
  if (!ctx || !pins || !hal_out || pins->cs < 0) {
    return -1;
  }
  ctx->pins = *pins;
  ctx->ready = 0;

  pinMode((uint8_t)ctx->pins.cs, OUTPUT);
  digitalWrite((uint8_t)ctx->pins.cs, HIGH);

  if (ctx->pins.wp >= 0) {
    pinMode((uint8_t)ctx->pins.wp, OUTPUT);
    digitalWrite((uint8_t)ctx->pins.wp, HIGH);
  }
  if (ctx->pins.hold >= 0) {
    pinMode((uint8_t)ctx->pins.hold, OUTPUT);
    digitalWrite((uint8_t)ctx->pins.hold, HIGH);
  }

  SPI.begin();

  hal_out->spi_xfer = arduino_spi_xfer;
  hal_out->cs_assert = arduino_cs_assert;
  hal_out->cs_deassert = arduino_cs_deassert;
  hal_out->delay_us = arduino_delay_us;
  hal_out->delay_ms = arduino_delay_ms;
  hal_out->gpio_write = arduino_gpio_write;
  hal_out->user = ctx;
  ctx->ready = 1;
  return 0;
}
