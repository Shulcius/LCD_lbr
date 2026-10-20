/**
 * @file spi_nor_port_generic.c
 * @brief Шаблон порта для RISC-V / bare-metal.
 *
 * Что реализовать пользователю:
 *   1) gpio_write_cs / инициализация пинов CS, (опц.) WP, HOLD
 *   2) spi_hw_xfer — полнодуплексный обмен mode 0, MSB first
 *   3) delay_us / delay_ms (цикл по таймеру или SysTick)
 *
 * LE25U40CMC: VDD = 3.3 V; корпус pin6=SCK, pin7=HOLD (не как у типичного W25Q).
 */
#include "spi_nor_port_generic.h"

#include <stddef.h>

/* ========== TODO: платформенные примитивы ========== */

static void platform_gpio_cs(int pin, int level_high)
{
  (void)pin;
  (void)level_high;
  /* TODO: выставить CS# (active low → level_high==0 значит выбран чип) */
}

static void platform_gpio_out(int pin, int level_high)
{
  (void)pin;
  (void)level_high;
  /* TODO: WP# / HOLD# обычно держать HIGH (= неактивны) */
}

static int platform_spi_xfer(void *spi_hw, const uint8_t *tx, uint8_t *rx, size_t len)
{
  (void)spi_hw;
  (void)tx;
  (void)rx;
  (void)len;
  /* TODO: для каждого байта: сдвинуть tx[i] или 0xFF, принять в rx[i] */
  return -1; /* пока не реализовано */
}

static void platform_delay_us(uint32_t us)
{
  (void)us;
  /* TODO */
}

static void platform_delay_ms(uint32_t ms)
{
  (void)ms;
  /* TODO */
}

/* ========== HAL glue ========== */

static int generic_spi_xfer(void *user, const uint8_t *tx, uint8_t *rx, size_t len)
{
  spi_nor_generic_ctx_t *ctx = (spi_nor_generic_ctx_t *)user;
  if (!ctx) {
    return -1;
  }
  return platform_spi_xfer(ctx->spi_hw, tx, rx, len);
}

static void generic_cs_assert(void *user)
{
  spi_nor_generic_ctx_t *ctx = (spi_nor_generic_ctx_t *)user;
  platform_gpio_cs(ctx->pins.cs, 0);
}

static void generic_cs_deassert(void *user)
{
  spi_nor_generic_ctx_t *ctx = (spi_nor_generic_ctx_t *)user;
  platform_gpio_cs(ctx->pins.cs, 1);
}

static void generic_delay_us(void *user, uint32_t us)
{
  (void)user;
  platform_delay_us(us);
}

static void generic_delay_ms(void *user, uint32_t ms)
{
  (void)user;
  platform_delay_ms(ms);
}

static void generic_gpio_write(void *user, int pin_id, int value)
{
  spi_nor_generic_ctx_t *ctx = (spi_nor_generic_ctx_t *)user;
  int pin = -1;
  if (pin_id == SPI_NOR_GPIO_WP) {
    pin = ctx->pins.wp;
  } else if (pin_id == SPI_NOR_GPIO_HOLD) {
    pin = ctx->pins.hold;
  }
  if (pin < 0) {
    return;
  }
  platform_gpio_out(pin, value);
}

int spi_nor_generic_hal_init(spi_nor_generic_ctx_t *ctx,
                             const spi_nor_generic_pins_t *pins,
                             spi_nor_hal_t *hal_out)
{
  if (!ctx || !pins || !hal_out) {
    return -1;
  }
  ctx->pins = *pins;

  /* WP/HOLD на чипе: если разведены — держать HIGH */
  if (ctx->pins.wp >= 0) {
    platform_gpio_out(ctx->pins.wp, 1);
  }
  if (ctx->pins.hold >= 0) {
    platform_gpio_out(ctx->pins.hold, 1);
  }
  platform_gpio_cs(ctx->pins.cs, 1);

  hal_out->spi_xfer = generic_spi_xfer;
  hal_out->cs_assert = generic_cs_assert;
  hal_out->cs_deassert = generic_cs_deassert;
  hal_out->delay_us = generic_delay_us;
  hal_out->delay_ms = generic_delay_ms;
  hal_out->gpio_write = generic_gpio_write;
  hal_out->user = ctx;
  return 0;
}
