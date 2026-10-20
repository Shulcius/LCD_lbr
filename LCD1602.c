/**
 * @file LCD1602.c
 * @brief Драйвер HD44780 / LCD 1602 для Arduino (4- и 8-bit).
 */
#include "LCD1602.h"

#include <Arduino.h>

/* ---------- низкий уровень ---------- */

static void lcd_pulse_enable(lcd1602_t *lcd)
{
  digitalWrite(lcd->e_pin, LOW);
  delayMicroseconds(1);
  digitalWrite(lcd->e_pin, HIGH);
  delayMicroseconds(1);   /* >450 ns */
  digitalWrite(lcd->e_pin, LOW);
  delayMicroseconds(50);  /* командам обычно хватает ~37 us */
}

static void lcd_write_8bits(lcd1602_t *lcd, uint8_t value)
{
  uint8_t i;
  for (i = 0; i < 8u; i++) {
    digitalWrite(lcd->data_pins[i], (value >> i) & 0x01u);
  }
  lcd_pulse_enable(lcd);
}

/** Один полубайт на DB4..DB7 (value в младших 4 битах). */
static void lcd_write_4bits(lcd1602_t *lcd, uint8_t nibble)
{
  uint8_t i;
  for (i = 0; i < 4u; i++) {
    digitalWrite(lcd->data_pins[i], (nibble >> i) & 0x01u);
  }
  lcd_pulse_enable(lcd);
}

static void lcd_send(lcd1602_t *lcd, uint8_t value, uint8_t rs_level)
{
  digitalWrite(lcd->rs_pin, rs_level ? HIGH : LOW);
  digitalWrite(lcd->rw_pin, LOW); /* всегда запись */

  if (lcd->eight_bit) {
    lcd_write_8bits(lcd, value);
  } else {
    lcd_write_4bits(lcd, (uint8_t)(value >> 4));
    lcd_write_4bits(lcd, (uint8_t)(value & 0x0Fu));
  }
}

void lcd_command(lcd1602_t *lcd, uint8_t cmd)
{
  lcd_send(lcd, cmd, 0);
  /* Clear/Home дольше */
  if (cmd == LCD_CLEAR || cmd == LCD_HOME) {
    delay(2);
  } else {
    delayMicroseconds(50);
  }
}

void lcd_write_raw(lcd1602_t *lcd, uint8_t value)
{
  lcd_send(lcd, value, 1);
}

static void lcd_apply_display_mode(lcd1602_t *lcd)
{
  lcd_command(lcd, (uint8_t)(LCD_DISPLAY_CTRL | lcd->display_mode));
  lcd->state.display = (lcd->display_mode & LCD_DISPLAY_ON) != 0;
  lcd->state.cursor  = (lcd->display_mode & LCD_CURSOR_ON) != 0;
  lcd->state.blink   = (lcd->display_mode & LCD_BLINK_ON) != 0;
}

/* ---------- публичный API ---------- */

void lcd_init(lcd1602_t *lcd, uint8_t cols, uint8_t rows, bool bit_mode)
{
  uint8_t i;
  uint8_t n_data;

  if (!lcd) {
    return;
  }

  lcd->cols = cols ? cols : 16u;
  lcd->rows = rows ? rows : 2u;
  lcd->eight_bit = bit_mode;
  lcd->display_mode = (uint8_t)(LCD_DISPLAY_ON | LCD_CURSOR_OFF | LCD_BLINK_OFF);
  lcd->entry_mode = (uint8_t)(LCD_ENTRY_LEFT | LCD_ENTRY_SHIFT_OFF);
  lcd->state.display = true;
  lcd->state.cursor = false;
  lcd->state.blink = false;
  lcd->state.cur_pos = 0;

  pinMode(lcd->rs_pin, OUTPUT);
  pinMode(lcd->rw_pin, OUTPUT);
  pinMode(lcd->e_pin, OUTPUT);
  digitalWrite(lcd->rs_pin, LOW);
  digitalWrite(lcd->rw_pin, LOW);
  digitalWrite(lcd->e_pin, LOW);

  n_data = lcd->eight_bit ? 8u : 4u;
  for (i = 0; i < n_data; i++) {
    pinMode(lcd->data_pins[i], OUTPUT);
    digitalWrite(lcd->data_pins[i], LOW);
  }

  delay(50); /* >40 ms после питания */

  if (lcd->eight_bit) {
    lcd_command(lcd, (uint8_t)(LCD_FUNCTION_SET | LCD_8BIT_MODE));
    delayMicroseconds(4500);
    lcd_command(lcd, (uint8_t)(LCD_FUNCTION_SET | LCD_8BIT_MODE));
    delayMicroseconds(150);
    lcd_command(lcd, (uint8_t)(LCD_FUNCTION_SET | LCD_8BIT_MODE));
  } else {
    /* Спецпоследовательность: пока в 8-bit, шлём только старший ниббл. */
    digitalWrite(lcd->rs_pin, LOW);
    digitalWrite(lcd->rw_pin, LOW);
    lcd_write_4bits(lcd, 0x03u);
    delayMicroseconds(4500);
    lcd_write_4bits(lcd, 0x03u);
    delayMicroseconds(150);
    lcd_write_4bits(lcd, 0x03u);
    delayMicroseconds(150);
    lcd_write_4bits(lcd, 0x02u); /* перевод в 4-bit */
  }

  {
    uint8_t func = LCD_FUNCTION_SET;
    func |= lcd->eight_bit ? LCD_8BIT_MODE : LCD_4BIT_MODE;
    func |= (lcd->rows > 1u) ? LCD_2_LINES : LCD_1_LINE;
    func |= LCD_FONT_5x8;
    lcd_command(lcd, func);
  }

  lcd_apply_display_mode(lcd);
  lcd_clear(lcd);
  lcd_command(lcd, (uint8_t)(LCD_ENTRY_MODE | lcd->entry_mode));
}

void lcd_clear(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd_command(lcd, LCD_CLEAR);
  lcd->state.cur_pos = 0;
}

void lcd_home(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd_command(lcd, LCD_HOME);
  lcd->state.cur_pos = 0;
}

void lcd_set_cursor(lcd1602_t *lcd, uint8_t col, uint8_t row)
{
  /* Типичная карта DDRAM для 16x2 / 20x2 / 20x4 */
  static const uint8_t row_offsets[] = { 0x00u, 0x40u, 0x14u, 0x54u };
  uint8_t addr;

  if (!lcd) {
    return;
  }
  if (row >= lcd->rows) {
    row = (uint8_t)(lcd->rows - 1u);
  }
  if (col >= lcd->cols) {
    col = (uint8_t)(lcd->cols - 1u);
  }
  if (row > 3u) {
    row = 3u;
  }

  addr = (uint8_t)(row_offsets[row] + col);
  lcd_command(lcd, (uint8_t)(LCD_SET_DDRAM_ADDR | addr));
  lcd->state.cur_pos = (uint8_t)(row * lcd->cols + col);
}

void lcd_write_char(lcd1602_t *lcd, char c)
{
  if (!lcd) {
    return;
  }
  lcd_write_raw(lcd, (uint8_t)c);
  if (lcd->state.cur_pos < 127u) {
    lcd->state.cur_pos++;
  }
}

void lcd_write_string(lcd1602_t *lcd, const char *str)
{
  if (!lcd || !str) {
    return;
  }
  while (*str) {
    lcd_write_char(lcd, *str++);
  }
}

void lcd_display_on(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode |= LCD_DISPLAY_ON;
  lcd_apply_display_mode(lcd);
}

void lcd_display_off(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode = (uint8_t)(lcd->display_mode & (uint8_t)~LCD_DISPLAY_ON);
  lcd_apply_display_mode(lcd);
}

void lcd_cursor_on(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode |= LCD_CURSOR_ON;
  lcd_apply_display_mode(lcd);
}

void lcd_cursor_off(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode = (uint8_t)(lcd->display_mode & (uint8_t)~LCD_CURSOR_ON);
  lcd_apply_display_mode(lcd);
}

void lcd_blink_on(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode |= LCD_BLINK_ON;
  lcd_apply_display_mode(lcd);
}

void lcd_blink_off(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd->display_mode = (uint8_t)(lcd->display_mode & (uint8_t)~LCD_BLINK_ON);
  lcd_apply_display_mode(lcd);
}

void lcd_scroll_left(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd_command(lcd, (uint8_t)(LCD_SHIFT | LCD_SHIFT_DISPLAY | LCD_SHIFT_LEFT));
}

void lcd_scroll_right(lcd1602_t *lcd)
{
  if (!lcd) {
    return;
  }
  lcd_command(lcd, (uint8_t)(LCD_SHIFT | LCD_SHIFT_DISPLAY | LCD_SHIFT_RIGHT));
}

void lcd_create_char(lcd1602_t *lcd, uint8_t index, const uint8_t pattern[8])
{
  uint8_t i;
  if (!lcd || !pattern || index > 7u) {
    return;
  }
  lcd_command(lcd, (uint8_t)(LCD_SET_CGRAM_ADDR | (uint8_t)((index & 0x07u) << 3)));
  for (i = 0; i < 8u; i++) {
    lcd_write_raw(lcd, pattern[i]);
  }
  /* Вернуть курсор в DDRAM (иначе следующий символ уйдёт в CGRAM). */
  lcd_set_cursor(lcd, (uint8_t)(lcd->state.cur_pos % lcd->cols),
                 (uint8_t)(lcd->state.cur_pos / lcd->cols));
}
