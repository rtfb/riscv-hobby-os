#include "sys.h"
#include "kernel.h"
#include "drivers/hd44780/hd44780.h"
#include "riscv.h"
#include "gpio.h"

#define LOW  0
#define HIGH 1

lcd_t lcd;
void lcd_init() {
    kprintf("lcd_init\n");
    lcd.rs_pin = GPIO_PIN_10;
    lcd.enable_pin = GPIO_PIN_11;
    lcd.data_pins[0] = GPIO_PIN_4;
    lcd.data_pins[1] = GPIO_PIN_5;
    lcd.data_pins[2] = GPIO_PIN_6;
    lcd.data_pins[3] = GPIO_PIN_7;
    lcd.display_function = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    lcd_begin(20, 4);
}

void _init_4_bit_mode() {
    // we start in 8bit mode, try to set 4 bit mode
    lcd_write_4bits(0x03);
    busy_loop_delay_millis(5); // wait min 4.1ms
    // second try
    lcd_write_4bits(0x03);
    busy_loop_delay_millis(5); // wait min 4.1ms
    // third go!
    lcd_write_4bits(0x03);
    busy_loop_delay_millis(1);
    // finally, set to 4-bit interface
    lcd_write_4bits(0x02);
}

void lcd_begin(uint8_t cols, uint8_t rows) {
    lcd.display_function |= LCD_2LINE;
    lcd.num_lines = 4;
    lcd_set_row_offsets(0x00, 0x40, 0x00 + cols, 0x40 + cols);
    gpio_enable_pin_out(lcd.rs_pin);
    gpio_enable_pin_out(lcd.enable_pin);
    gpio_enable_pin_out(lcd.data_pins[0]);
    gpio_enable_pin_out(lcd.data_pins[1]);
    gpio_enable_pin_out(lcd.data_pins[2]);
    gpio_enable_pin_out(lcd.data_pins[3]);
    // SEE PAGE 45/46 FOR INITIALIZATION SPECIFICATION!
    // according to datasheet, we need at least 40ms after power rises above 2.7V
    // before sending commands. Arduino can turn on way before 4.5V so we'll wait 50
    busy_loop_delay_millis(500);
    // Now we pull both RS and R/W low to begin commands
    gpio_set_pin(lcd.rs_pin, LOW);
    gpio_set_pin(lcd.enable_pin, LOW);

    _init_4_bit_mode();

    // finally, set # lines, font size, etc.
    lcd_command(LCD_FUNCTIONSET | lcd.display_function);
    // turn the display on with no cursor or blinking default
    lcd.display_control = LCD_DISPLAYON | LCD_CURSOROFF | LCD_BLINKOFF;
    lcd_display();
    // clear it off
    lcd_clear();
    lcd_cursor();
    // Initialize to default text direction (for romance languages)
    lcd.display_mode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    // set the entry mode
    lcd_command(LCD_ENTRYMODESET | lcd.display_mode);
    busy_loop_delay_millis(200);
}

void lcd_set_row_offsets(int row0, int row1, int row2, int row3) {
    lcd.row_offsets[0] = row0;
    lcd.row_offsets[1] = row1;
    lcd.row_offsets[2] = row2;
    lcd.row_offsets[3] = row3;
}

void lcd_display() {
  lcd.display_control |= LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

void lcd_no_display() {
  lcd.display_control &= ~LCD_DISPLAYON;
  lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

void lcd_set_cursor(uint8_t col, uint8_t row) {
    const int max_lines = 4;
    if (row >= max_lines) {
        row = max_lines - 1;    // we count rows starting w/0
    }
    if (row >= lcd.num_lines) {
        row = lcd.num_lines - 1; // we count rows starting w/0
    }
    lcd_command(LCD_SETDDRAMADDR | (col + lcd.row_offsets[row]));
}

void lcd_clear() {
    lcd_command(LCD_CLEARDISPLAY);  // clear display, set cursor position to zero
    busy_loop_delay_millis(2);      // clear command takes a long time!
}

void lcd_home() {
    lcd_command(LCD_RETURNHOME);    // set cursor position to zero
    busy_loop_delay_millis(2);      // clear command takes a long time!
}

// Turns the underline cursor on/off
void lcd_no_cursor() {
    lcd.display_control &= ~LCD_CURSORON;
    lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

void lcd_cursor() {
    lcd.display_control |= LCD_CURSORON;
    lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

// Turn on and off the blinking cursor
void lcd_no_blink() {
    lcd.display_control &= ~LCD_BLINKON;
    lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

void lcd_blink() {
    lcd.display_control |= LCD_BLINKON;
    lcd_command(LCD_DISPLAYCONTROL | lcd.display_control);
}

void lcd_print(char const *str) {
    while (*str) {
        lcd_write(*str);
        str++;
    }
}

void lcd_command(uint8_t value) {
    lcd_send(value, LOW);
}

void lcd_write(uint8_t value) {
    lcd_send(value, HIGH);
}

// write either command or data, with automatic 4/8-bit selection
void lcd_send(uint8_t value, uint8_t mode) {
    gpio_set_pin(lcd.rs_pin, mode);
    // if there is a RW pin indicated, set it low to Write
    lcd_write_4bits(value >> 4);
    lcd_write_4bits(value);
}

void lcd_write_4bits(uint8_t value) {
    for (int i = 0; i < 4; i++) {
        gpio_set_pin(lcd.data_pins[i], (value >> i) & 0x01);
    }
    lcd_pulse_enable();
}

void lcd_pulse_enable() {
    gpio_set_pin(lcd.enable_pin, LOW);
    busy_loop_delay_tiny();
    gpio_set_pin(lcd.enable_pin, HIGH);
    busy_loop_delay_tiny();
    gpio_set_pin(lcd.enable_pin, LOW);
    busy_loop_delay_millis(1);
}

void busy_loop_delay_tiny() {
    uint64_t now = time_get_now();
    uint64_t target_time = now + 1;
    do {
        now = time_get_now();
    } while (now < target_time);
}

void busy_loop_delay_millis(uint32_t milliseconds) {
    uint64_t now = time_get_now();
    uint64_t delta = (ONE_SECOND * milliseconds) / 1000;
    uint64_t target_time = now + delta;
    do {
        now = time_get_now();
    } while (now < target_time);
}
