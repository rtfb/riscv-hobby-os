#include "sys.h"
#include "plic.h"
#include "drivers/uart/uart.h"

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    // enable reading:
    *(uint32_t*)(UART_BASE + UART_RXCTRL) = 1;

    // enable read interrupts:
    *(uint32_t*)(UART_BASE + UART_IE) = UART_IE_RXWM;

    // set baud divisor (the SiFive FE310-G002 manual lists a table of possible
    // values in Section 18.9, determined this particular choice
    // experimentally. Furthermore, it's the default on HiFive1-revB board):
    *(uint32_t*)(UART_BASE + UART_BAUD_RATE_DIVISOR) = 138;

    plic_enable_intr(UART0_IRQ_NUM);
    plic_set_intr_priority(UART0_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
}

void uart_writechar(char ch) {
    volatile int32_t* tx = (int32_t*)(UART_BASE + UART_TXDATA);
    while ((int32_t)(*tx) < 0);
    *tx = ch;
}

int uart_rx_num_avail() {
    volatile int32_t* rx = (int32_t*)(UART_BASE + UART_RXDATA);
    int32_t word = *rx;
    return word > 0;
}

char uart_readchar() {
    volatile int32_t* rx = (int32_t*)(UART_BASE + UART_RXDATA);
    int32_t word = *rx;
    return word & 0xff;
}

void uart_machine_wait_status() {
    // no-op
}
