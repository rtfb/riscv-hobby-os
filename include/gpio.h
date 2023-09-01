#ifndef _GPIO_H_
#define _GPIO_H_

// This is a mapping of on-board Arduino-compatible PIN numbers to the GPIO
// pins internal to the FE310 chip, based on HiFive1 RevB Schematics[1]. In
// GPIO_PIN_x, the x corresponds to the label next to a pin on the board, and
// the value corresponds to a GPIO number as described in FE310-G002 Manual[2],
// Capter 17. For example GPIO_PIN_0 (the corner pin on the board) is connected
// to GPIO number 16.
//
// Note that GPIO_PIN_14 is not connected at all.
//
// [1] https://sifive.cdn.prismic.io/sifive/c34f4c7f-0d3a-493e-8a19-a0b18f8a4555_hifive1-b01-schematics.pdf
// [2] https://sifive.cdn.prismic.io/sifive/034760b5-ac6a-4b1c-911c-f4148bb2c4a5_fe310-g002-v1p5.pdf
//
// TODO: isolate this mapping into machine-specific code.
#define GPIO_PIN_0      16
#define GPIO_PIN_1      17
#define GPIO_PIN_2      18
#define GPIO_PIN_3      19
#define GPIO_PIN_4      20
#define GPIO_PIN_5      21
#define GPIO_PIN_6      22
#define GPIO_PIN_7      23
#define GPIO_PIN_8      0
#define GPIO_PIN_9      1
#define GPIO_PIN_10     2
#define GPIO_PIN_11     3
#define GPIO_PIN_12     4   // XXX: doesn't work
#define GPIO_PIN_13     5
#undef GPIO_PIN_14   // not connected. Make sure it won't compile if we ever use it by mistake
#define GPIO_PIN_15     9
#define GPIO_PIN_16     10  // XXX: doesn't work
#define GPIO_PIN_17     11
#define GPIO_PIN_18     12
#define GPIO_PIN_19     13

#define GPIO_PIN_INPUT_VAL  0x00
#define GPIO_PIN_INPUT_EN   0x04
#define GPIO_PIN_OUT_EN     0x08
#define GPIO_PIN_OUT_VAL    0x0c
#define GPIO_PIN_PUE        0x10
#define GPIO_PIN_DS         0x14
// TODO: the rest...
#define GPIO_PIN_IOF_EN     0x38
#define GPIO_PIN_IOF_SEL    0x3c
#define GPIO_PIN_OUT_XOR    0x40
// TODO: the rest...

#define GPIO_ERR_0_1        -1   // can't use GPIO_PIN_0 and GPIO_PIN_1 because they interfere with UART
#define GPIO_ERR_12         -2   // GPIO_PIN_12 does not work yet
#define GPIO_ERR_16         -3   // GPIO_PIN_16 does not work yet
#define GPIO_ERR_BAD_VALUE  -4   // unrecognized value passed to gpio_do_syscall
#define GPIO_ERR_BAD_ENABLE -5   // unrecognized enable_disable passed to gpio_do_syscall

// gpio_enable_pin_out enables a specified pin for output.
void gpio_enable_pin_out(int pin);

// gpio_disable_pin_out disables a specified pin for output.
void gpio_disable_pin_out(int pin);

// gpio_set_pin sets a specified pin bit to (bool)value.
void gpio_set_pin(int pin, int value);

// gpio_toggle_pin sets a specified pin value to an opposite of its current value.
void gpio_toggle_pin(int pin);

// gpio_do_syscall implements the system call that interacts with a given GPIO pin.
// enable_disable can be:
//      1 to write-enable pin_num
//      0 to write-disable pin_num
//     -1 to leave the pin state as is
//
// value is ignored unless enable_disable is -1. Valid values:
//      0 to write 0 to pin_num
//      1 to write 1 to pin_num
//      2 to toggle pin_num to an opposite value
uint32_t gpio_do_syscall(uint32_t pin_num, uint32_t enable_disable, uint32_t value);

#endif // ifndef _GPIO_H_
