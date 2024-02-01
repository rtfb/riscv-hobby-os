#include "gpio.h"
#include "mmreg.h"

#define read(reg) read32(GPIO_BASE + reg)
#define write(reg, val) write32(GPIO_BASE + reg, val)

void gpio_enable_pin_out(int pin) {
    uint32_t out_en = read(GPIO_PIN_OUT_EN);
    out_en |= (1 << pin);
    write(GPIO_PIN_OUT_EN, out_en);

    uint32_t iof_en = read(GPIO_PIN_IOF_EN);
    iof_en &= ~(1 << pin);
    write(GPIO_PIN_IOF_EN, iof_en);
}

void gpio_disable_pin_out(int pin) {
    uint32_t out_en = read(GPIO_PIN_OUT_EN);
    out_en &= ~(1 << pin);
    write(GPIO_PIN_OUT_EN, out_en);

    uint32_t iof_en = read(GPIO_PIN_IOF_EN);
    iof_en |= (1 << pin);
    write(GPIO_PIN_IOF_EN, iof_en);
}

void gpio_set_pin(int pin, int value) {
    uint32_t val_bits = read(GPIO_PIN_OUT_VAL);
    if (value) {
        val_bits |= (1 << pin);
    } else {
        val_bits &= ~(1 << pin);
    }
    write(GPIO_PIN_OUT_VAL, val_bits);
}

void gpio_toggle_pin(int pin) {
    uint32_t val_bits = read(GPIO_PIN_OUT_VAL);
    val_bits ^= (1 << pin);
    write(GPIO_PIN_OUT_VAL, val_bits);
}

uint32_t gpio_do_syscall(uint32_t pin_num, uint32_t enable_disable, uint32_t value) {
    if (pin_num == GPIO_PIN_0 || pin_num == GPIO_PIN_1) {
        return GPIO_ERR_0_1;
    }
    if (pin_num == GPIO_PIN_12) {
        return GPIO_ERR_12;
    }
    if (pin_num == GPIO_PIN_16) {
        return GPIO_ERR_16;
    }
    switch (enable_disable) {
        case 0:
            gpio_disable_pin_out(pin_num);
            break;
        case 1:
            gpio_enable_pin_out(pin_num);
            break;
        case -1:
            switch (value) {
                case 0:
                case 1:
                    gpio_set_pin(pin_num, value);
                    break;
                case 2:
                    gpio_toggle_pin(pin_num);
                    break;
                default:
                    return GPIO_ERR_BAD_VALUE;
            }
            break;
        default:
            return GPIO_ERR_BAD_ENABLE;
    }
    return 0;
}
