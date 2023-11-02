#include "mmreg.h"
#include "plic.h"
#include "drivers/uart/uart.h"

int32_t last_rx_peek;

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    last_rx_peek = 0;

    // enable reading:
    write32(UART_BASE + UART_RXCTRL, 1);

    // enable read interrupts:
    write32(UART_BASE + UART_IE, UART_IE_RXWM);

    // set baud divisor (the SiFive FE310-G002 manual lists a table of possible
    // values in Section 18.9, determined this particular choice
    // experimentally. Furthermore, it's the default on HiFive1-revB board):
    write32(UART_BASE + UART_BAUD_RATE_DIVISOR, 138);

    plic_enable_intr(UART0_IRQ_NUM);
    plic_set_intr_priority(UART0_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
}

void uart_writechar(char ch) {
    while (read32(UART_BASE + UART_TXDATA))
        ;
    write32(UART_BASE + UART_TXDATA, ch);
}

int uart_rx_num_avail() {
    last_rx_peek = read32(UART_BASE + UART_RXDATA);
    return last_rx_peek > 0;
}

char uart_readchar() {
    if (last_rx_peek) {
        char c = last_rx_peek & 0xff;
        last_rx_peek = 0;
        return c;
    }
    int32_t word = read32(UART_BASE + UART_RXDATA);
    return word & 0xff;
}

void uart_machine_wait_status() {
    // no-op
}
