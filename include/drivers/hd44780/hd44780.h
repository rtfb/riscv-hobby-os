#ifndef _HD44780_H_
#define _HD44780_H_

// commands
#define LCD_CLEARDISPLAY    0x01
#define LCD_RETURNHOME      0x02
#define LCD_ENTRYMODESET    0x04
#define LCD_DISPLAYCONTROL  0x08
#define LCD_CURSORSHIFT     0x10
#define LCD_FUNCTIONSET     0x20
#define LCD_SETCGRAMADDR    0x40
#define LCD_SETDDRAMADDR    0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT          0x00
#define LCD_ENTRYLEFT           0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON       0x04
#define LCD_DISPLAYOFF      0x00
#define LCD_CURSORON        0x02
#define LCD_CURSOROFF       0x00
#define LCD_BLINKON         0x01
#define LCD_BLINKOFF        0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE     0x08
#define LCD_CURSORMOVE      0x00
#define LCD_MOVERIGHT       0x04
#define LCD_MOVELEFT        0x00

// flags for function set
#define LCD_8BITMODE        0x10
#define LCD_4BITMODE        0x00
#define LCD_2LINE           0x08
#define LCD_1LINE           0x00
#define LCD_5x10DOTS        0x04
#define LCD_5x8DOTS         0x00

#define LCD_NUM_ROWS        4
#define LCD_NUM_COLS        20

#define lcd_write_4bits(value)                                  \
    for (int i = 0; i < 4; i++) {                               \
        gpio_set_pin(lcd.data_pins[i], (value >> i) & 0x01);    \
    }                                                           \
    gpio_set_pin(lcd.enable_pin, LOW);                          \
    busy_loop_delay_tiny();                                     \
    gpio_set_pin(lcd.enable_pin, HIGH);                         \
    busy_loop_delay_tiny();                                     \
    gpio_set_pin(lcd.enable_pin, LOW);                          \
    busy_loop_delay_millis(1);

// lcd_buffer_t maintains all the text that was printed to the LCD screen so
// that we can replay most of it when new text forces a scroll up by one line.
typedef struct lcd_buffer_s {
    char data[LCD_NUM_ROWS * LCD_NUM_COLS];

    char* lines[LCD_NUM_ROWS];   // pointers into data[]
    uint8_t curr_line;           // which line we're currently writing to
    uint8_t pos;                 // position in curr_line
    uint8_t screen_is_full;      // false until we fill all 4 lines
} lcd_buffer_t;

typedef struct lcd_s {
    uint8_t rs_pin;       // LOW: command.  HIGH: character.
    uint8_t enable_pin;   // activated by a HIGH pulse.
    uint8_t data_pins[4];

    uint8_t display_function;
    uint8_t display_control;
    uint8_t display_mode;

    uint8_t num_lines;
    uint8_t row_offsets[4];

    lcd_buffer_t buf;

    // maintains the current LCD cursor position so that we know when to
    // lcd_set_cursor to the next line.
    uint8_t last_cursor_row;
    uint8_t last_cursor_col;
} lcd_t;

extern lcd_t lcd;  // defined in lcd.c

void lcd_init();
void lcd_init_buffer(lcd_buffer_t *buf);
void lcd_begin(uint8_t cols, uint8_t rows);
void lcd_set_row_offsets(int row0, int row1, int row2, int row3);
void lcd_display();
void lcd_no_display();
void lcd_set_cursor(uint8_t col, uint8_t row);
void lcd_clear();
void lcd_home();
void lcd_no_cursor();
void lcd_cursor();
void lcd_no_blink();
void lcd_blink();
void lcd_print(char const *str);
void lcd_printn(char const* data, uint32_t size);

// internal:
void lcd_command(uint8_t value);
void lcd_write(uint8_t value);

// busy_loop_delay_millis spins a busy loop for a specified number of milliseconds.
void busy_loop_delay_millis(uint32_t milliseconds);

// busy_loop_delay_tiny spins a busy loop for a tiniest possible delay on the
// hifive board (1/32768 seconds). All uses cases of this should be replaced
// with polling the LCD busy pin instead of this time-based guesswork.
void busy_loop_delay_tiny();

#endif // ifndef _HD44780_H_
