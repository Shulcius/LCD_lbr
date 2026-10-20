/*
  LCD 1602 (HD44780-совместимый)

  pinout:
    VSS       - 1     - GND
    VCC       - 2     - 5V (некоторые модули 3.3 V — смотреть плату)
    VEE       - 3     - contrast (подстройка контраста)
    RS        - 4     - 0 = команда, 1 = данные
    R/W       - 5     - 0 = запись, 1 = чтение (часто на GND)
    E         - 6     - Enable
    DB0-DB7   - 7-14  - данные (8-bit: все; 4-bit: DB4-DB7)
    LED +     - 15    - подсветка +
    LED -     - 16    - подсветка -

  Символ: 5x8 (или 5x10) точек.

  4-bit режим: в data_pins[0..3] передайте GPIO для DB4..DB7
               (data_pins[4..7] не используются).
  8-bit режим: data_pins[0..7] = DB0..DB7.
*/
#ifndef LCD1602_H
#define LCD1602_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==================== ОСНОВНЫЕ КОМАНДЫ ==================== */
#define LCD_CLEAR           0x01u
#define LCD_HOME            0x02u

#define LCD_ENTRY_MODE      0x04u
#define LCD_ENTRY_RIGHT     0x02u
#define LCD_ENTRY_LEFT      0x00u
#define LCD_ENTRY_SHIFT_ON  0x01u
#define LCD_ENTRY_SHIFT_OFF 0x00u

#define LCD_DISPLAY_CTRL    0x08u
#define LCD_DISPLAY_ON      0x04u
#define LCD_DISPLAY_OFF     0x00u
#define LCD_CURSOR_ON       0x02u
#define LCD_CURSOR_OFF      0x00u
#define LCD_BLINK_ON        0x01u
#define LCD_BLINK_OFF       0x00u

#define LCD_SHIFT           0x10u
#define LCD_SHIFT_DISPLAY   0x08u
#define LCD_SHIFT_CURSOR    0x00u
#define LCD_SHIFT_RIGHT     0x04u
#define LCD_SHIFT_LEFT      0x00u

#define LCD_FUNCTION_SET    0x20u
#define LCD_8BIT_MODE       0x10u
#define LCD_4BIT_MODE       0x00u
#define LCD_2_LINES         0x08u
#define LCD_1_LINE          0x00u
#define LCD_FONT_5x10       0x04u
#define LCD_FONT_5x8        0x00u

#define LCD_SET_CGRAM_ADDR  0x40u
#define LCD_SET_DDRAM_ADDR  0x80u

/* ======== СОСТОЯНИЕ / КОНФИГ ========= */
typedef struct {
  bool display : 1;
  bool cursor  : 1;
  bool blink   : 1;
  uint8_t cur_pos : 7; /* линейная позиция 0..(cols*rows-1) */
} lcd1602_state_t;

typedef struct {
  uint8_t rs_pin;
  uint8_t rw_pin;          /* если R/W на GND — всё равно укажите пин или тот же вывод, см. init */
  uint8_t e_pin;
  uint8_t data_pins[8];    /* 8-bit: DB0..DB7; 4-bit: [0..3]=DB4..DB7 */
  uint8_t cols;
  uint8_t rows;
  bool    eight_bit;       /* true = 8-bit шина, false = 4-bit */
  uint8_t display_mode;    /* кеш LCD_DISPLAY_CTRL | флаги */
  uint8_t entry_mode;      /* кеш LCD_ENTRY_MODE | флаги */
  lcd1602_state_t state;
} lcd1602_t;

/**
 * Инициализация пинов и контроллера HD44780.
 * @param bit_mode  true  → 8-bit (DB0..DB7),
 *                  false → 4-bit (data_pins[0..3] = DB4..DB7).
 * Перед вызовом заполните rs_pin, rw_pin, e_pin, data_pins[].
 * Если R/W аппаратно на GND — всё равно задайте rw_pin на свободный
 * выход и библиотека будет держать его LOW, либо используйте тот же
 * номер, что и неиспользуемый пин, подтянутый как OUTPUT LOW.
 */
void lcd_init(lcd1602_t *lcd, uint8_t cols, uint8_t rows, bool bit_mode);

void lcd_clear(lcd1602_t *lcd);
void lcd_home(lcd1602_t *lcd);
void lcd_set_cursor(lcd1602_t *lcd, uint8_t col, uint8_t row);

void lcd_write_char(lcd1602_t *lcd, char c);
void lcd_write_string(lcd1602_t *lcd, const char *str);

void lcd_display_on(lcd1602_t *lcd);
void lcd_display_off(lcd1602_t *lcd);
void lcd_cursor_on(lcd1602_t *lcd);
void lcd_cursor_off(lcd1602_t *lcd);
void lcd_blink_on(lcd1602_t *lcd);
void lcd_blink_off(lcd1602_t *lcd);

/** Сдвиг содержимого / курсора (комбинация LCD_SHIFT_*). */
void lcd_scroll_left(lcd1602_t *lcd);
void lcd_scroll_right(lcd1602_t *lcd);

/**
 * Запись своего символа в CGRAM (index 0..7),
 * pattern — 8 строк по 5 бит (младшие биты значащие).
 */
void lcd_create_char(lcd1602_t *lcd, uint8_t index, const uint8_t pattern[8]);

/** Низкоуровневая отправка (если нужно расширять). */
void lcd_command(lcd1602_t *lcd, uint8_t cmd);
void lcd_write_raw(lcd1602_t *lcd, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif /* LCD1602_H */
