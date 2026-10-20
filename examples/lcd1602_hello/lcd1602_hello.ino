/**
 * Минимальный пример LCD 1602 (HD44780), 4-bit режим.
 *
 * Замените пины под свою плату. R/W часто сажают на GND —
 * тогда rw_pin всё равно укажите (библиотека держит его LOW)
 * либо соедините отдельный GPIO и укажите его.
 *
 * Подключите LCD1602.c / LCD1602.h в скетч (тот же каталог
 * или укажите путь в IDE / platformio.ini).
 */
#include "LCD1602.h"

/* === ПИНЫ МК (заполните) === */
static const uint8_t PIN_RS = 12;
static const uint8_t PIN_RW = 11; /* или любой OUTPUT, если R/W на GND */
static const uint8_t PIN_E  = 10;
/* 4-bit: DB4..DB7 */
static const uint8_t PIN_D4 = 5;
static const uint8_t PIN_D5 = 4;
static const uint8_t PIN_D6 = 3;
static const uint8_t PIN_D7 = 2;

static lcd1602_t g_lcd;

void setup()
{
  g_lcd.rs_pin = PIN_RS;
  g_lcd.rw_pin = PIN_RW;
  g_lcd.e_pin  = PIN_E;
  g_lcd.data_pins[0] = PIN_D4;
  g_lcd.data_pins[1] = PIN_D5;
  g_lcd.data_pins[2] = PIN_D6;
  g_lcd.data_pins[3] = PIN_D7;

  /* cols=16, rows=2, bit_mode=false → 4-bit */
  lcd_init(&g_lcd, 16, 2, false);

  lcd_set_cursor(&g_lcd, 0, 0);
  lcd_write_string(&g_lcd, "LCD1602 OK");
  lcd_set_cursor(&g_lcd, 0, 1);
  lcd_write_string(&g_lcd, "Hello!");
}

void loop()
{
  delay(1000);
}
