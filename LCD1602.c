#include "lcd1602.h"
#include <Arduino.h>

// ==================== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ====================
void lcd_init(lcd1602_t *lcd, uint8_t cols, uint8_t rows, bool bit_mode) {
  digitalWrite(lcd->rs_pin, OUTPUT);
  digitalWrite(lcd->rw_pin, OUTPUT);
  digitalWrite(lcd->e_pin, OUTPUT);
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(lcd->data_pins[i], OUTPUT);
  }
  delay(50);

  
}

void lcd_clear(lcd1602_t *lcd) {

}

void lcd_set_cursor(lcd1602_t *lcd, uint8_t col, uint8_t row) {

}

void lcd_write_char(lcd1602_t *lcd, char c) {

}

void lcd_write_string(lcd1602_t *lcd, const char *str) {
  
}