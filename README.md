# LCD_lbr

Библиотеки для дисплеев и периферии на МК.

## LCD1602 (HD44780)

Файлы в корне: `LCD1602.h`, `LCD1602.c` (Arduino).

**Что было сломано и починено:**
- `lcd_state_t` → `lcd1602_state_t`
- в `init` был `digitalWrite(..., OUTPUT)` вместо `pinMode`
- пустые `clear` / `set_cursor` / `write_*`
- регистр include `lcd1602.h` при файле `LCD1602.h`

**API:** `lcd_init(lcd, cols, rows, bit_mode)` — `bit_mode=true` → 8-bit, `false` → 4-bit (`data_pins[0..3]` = DB4..DB7).

Пример: `examples/lcd1602_hello/`.

Пины МК в примерe — плейсхолдеры, задайте свои.
