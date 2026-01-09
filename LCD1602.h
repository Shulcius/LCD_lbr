/*
  pinout:
    VSS       - 1     - GND
    VCC       - 2     - 5V
    VEE       - 3     - contrast adjustment
    RS        - 4     - register selection (0 - instruction register, 1 - data register)
    R/W       - 5     - READ or WRITE selection (0 - write, 1 - read)
    E         - 6     - enable signal
    DB0-DB7   - 7-14  - data INPUT ot OUTPUT lines (8 bits on db0-7, 4 bits on db4-7 from old to "young" bits)
    LED "+"   - 15    - positive for LED contrast adjustment
    LED "-"   - 16    - negative for LED contrast adjustment
  1 icon size:
    5x8 pixels
  register select - cmds:
    0x01 - clear
    0x02 - reset cursor position to 0,0
    0x04 - cursor direction
    0x08 - on/of cmd for display/cursor/blinking
    0x10 - shift cursor/display
    0x20 - func for display/line/font
    0x40 - setting the address in CGRAM
    0x80 - setting the address in DDRAM
*/
#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>
#include <stdbool.h>

// ==================== ОСНОВНЫЕ КОМАНДЫ ====================
#define LCD_CLEAR           0x01    // Очистка всего экрана
#define LCD_HOME            0x02    // Курсор в начало (0,0)

// Эти команды нужно комбинировать с флагами через "|"
// Установка направления курсора (CUR_DIR = 0x04)
#define LCD_ENTRY_MODE      0x04  // Базовое значение
#define LCD_ENTRY_RIGHT     0x02  // Курсор движется вправо
#define LCD_ENTRY_LEFT      0x00  // Курсор движется влево
#define LCD_ENTRY_SHIFT_ON  0x01  // Дисплей сдвигается
#define LCD_ENTRY_SHIFT_OFF 0x00  // Дисплей не сдвигается

// Управление дисплеем (CMD = 0x08)
#define LCD_DISPLAY_CTRL    0x08  // Базовое значение
#define LCD_DISPLAY_ON      0x04  // Включить дисплей
#define LCD_DISPLAY_OFF     0x00  // Выключить дисплей
#define LCD_CURSOR_ON       0x02  // Показать курсор
#define LCD_CURSOR_OFF      0x00  // Скрыть курсор
#define LCD_BLINK_ON        0x01  // Включить мигание
#define LCD_BLINK_OFF       0x00  // Выключить мигание

// Сдвиг (SHIFT = 0x10)
#define LCD_SHIFT           0x10  // Базовое значение
#define LCD_SHIFT_DISPLAY   0x08  // Сдвигать дисплей
#define LCD_SHIFT_CURSOR    0x00  // Сдвигать курсор
#define LCD_SHIFT_RIGHT     0x04  // Сдвиг вправо
#define LCD_SHIFT_LEFT      0x00  // Сдвиг влево

// Настройка функции (FUNC = 0x20)
#define LCD_FUNCTION_SET    0x20  // Базовое значение
#define LCD_8BIT_MODE       0x10  // 8-битный режим
#define LCD_4BIT_MODE       0x00  // 4-битный режим
#define LCD_2_LINES         0x08  // 2 строки
#define LCD_1_LINE          0x00  // 1 строка
#define LCD_FONT_5x10       0x04  // Шрифт 5x10 точек
#define LCD_FONT_5x8        0x00  // Шрифт 5x8 точек

// =============== АДРЕСА ПАМЯТИ ================
#define LCD_SET_CGRAM_ADDR  0x40  // Установить адрес CGRAM
#define LCD_SET_DDRAM_ADDR  0x80  // Установить адрес DDRAM

// ======== СТРУКТУРА СОСТОЯНИЯ ДИСПЛЕЯ =========
typedef struct {
  bool display : 1;    // Дисплей включен/выключен
  bool cursor : 1;     // Курсор виден/невиден
  bool blink : 1;      // Мигание включено/выключено
  uint8_t cur_pos : 7; // Позиция курсора (0-79)
} lcd1602_state_t;

// ========== СТРУКТУРА ПИНОВ ДИСПЛЕЯ ==========
typedef struct {
  uint8_t rs_pin;
  uint8_t rw_pin;
  uint8_t e_pin;
  uint8_t data_pins[8];
  lcd_state_t state;
} lcd1602_t;

// ===== ОСНОВНЫЕ МЕТОДЫ РАБОТЫ С ДИСПЛЕЕМ =====
void lcd_init(lcd1602_t *lcd, uint8_t cols, uint8_t rows, bool bit_mode);
void lcd_clear(lcd1602_t *lcd);
void lcd_set_cursor(lcd1602_t *lcd, uint8_t col, uint8_t row);
void lcd_write_char(lcd1602_t *lcd, char c);
void lcd_write_string(lcd1602_t *lcd, const char *str);

#endif