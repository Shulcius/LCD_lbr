# spi_nor — переносимый драйвер SPI NOR Flash

Чистый **C** (ядро без Arduino). Подходит для Arduino, «китайских» МК и российского RISC-V: ядро одно, меняется только тонкий HAL.

## Важно про LE250U40

Маркер **«LE250U40 CMB FLASH 4 MB»** с высокой вероятностью = чип **LE25U40CMC** (Onsemi / ex-Sanyo):

| Параметр | Значение |
|----------|----------|
| Объём | **4 Mbit = 524 288 байт** (`0x80000`), **не** 4 МБ |
| JEDEC `9Fh` | `62 06 13` |
| Device ID `ABh` | `6Eh` |
| VDD | 2.3–3.6 V → на плате **3.3 V** |
| Корпус SOIC-8 | 1 CS, 2 SO, 3 WP, 4 GND, 5 SI, **6 SCK**, **7 HOLD**, 8 VDD |

«CMB» здесь скорее код корпуса **CMC**, не combo-плата.

Пресет чипа: `chips/le25u40cmc.h`. Альтернатива **25Q32 (4 МБ)** — только явный `chips/w25q32.h`.

### Зачем `chips/`, если это не пины МК?

Это **не** board/MCU config. Это справочный пресет под конкретный flash-чип: объём, page/sector, ожидаемый JEDEC, готовый `spi_nor_config_t`.  
Пины хоста живут в `ports/` / sketch. Ядро от `chips/` не зависит — после `probe()` по JEDEC часто хватает и без пресета.

## Структура

```
spi_nor/
  include/     spi_nor.h, spi_nor_hal.h, spi_nor_cmd.h
  src/         spi_nor.c          ← ядро
  chips/       пресеты чипов (LE25U40CMC, опц. W25Q32) — не GPIO МК
  ports/
    arduino/   HAL на SPI.h
    generic/   заглушка bare-metal / RISC-V (TODO в .c)
  examples/
    arduino_jedec_id/
    generic_hal_hookup/
  tests/       host mock + smoke
```

## HAL (что реализовать на новом МК)

```c
typedef struct spi_nor_hal {
  int  (*spi_xfer)(void *ctx, const uint8_t *tx, uint8_t *rx, size_t len);
  void (*cs_assert)(void *ctx);    /* CS# = 0 */
  void (*cs_deassert)(void *ctx);  /* CS# = 1 */
  void (*delay_us)(void *ctx, uint32_t us);
  void (*delay_ms)(void *ctx, uint32_t ms); /* можно NULL */
  void (*gpio_write)(void *ctx, int pin_id, int value); /* WP/HOLD, можно NULL */
  void *user;
} spi_nor_hal_t;
```

## Публичный API (кратко)

```c
spi_nor_init(dev, &hal, &le25u40cmc_config); /* или NULL + probe */
spi_nor_probe(dev);          /* JEDEC → имя/объём */
spi_nor_read_jedec_id(...);
spi_nor_read_device_id(...); /* LE25: 0x6E */
spi_nor_read / page_program / write
spi_nor_sector_erase / block_erase / chip_erase
spi_nor_wait_ready / read_status
spi_nor_read_sfdp          /* опционально; у LE25 обычно нет */
```

Объём **не зашит как 4 МБ**: берётся из таблицы известных JEDEC (`LE25U40CMC` → 512 KiB) или `2^capacity_id`, либо из `spi_nor_config_t.capacity_bytes`.

## Arduino (PlatformIO / IDE)

1. Include path: `spi_nor/include`, `spi_nor/ports/arduino`, `spi_nor/chips`.
2. Скомпилируйте `src/spi_nor.c` и `ports/arduino/spi_nor_port_arduino.cpp`.
3. Задайте **свои** пины МК в sketch (см. `examples/arduino_jedec_id`).
4. WP# и HOLD# на чипе подтяните к 3.3 V, если GPIO не используете.

## RISC-V / generic

Скопируйте `ports/generic/` и реализуйте `platform_*` в `spi_nor_port_generic.c` (SPI mode 0, CS, delay). Пример: `examples/generic_hal_hookup/main.c`.

## Тесты (host)

```bash
cd spi_nor/tests && make test
```

Mock ≈ LE25U40CMC (JEDEC `62 06 13`, 512 KiB): probe, erase, write, read.

## Что остаётся пользователю

- Распиновка **GPIO МК** (CS/SCK/MOSI/MISO, опционально WP/HOLD).
- Подтвердить чип по JEDEC (если 25Q32 — `chips/w25q32.h`).
- Питание 3.3 V и клипса с учётом SCK на pin 6 корпуса LE25.
