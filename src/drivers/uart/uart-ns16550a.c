#include "mmreg.h"
#include "plic.h"
#include "drivers/uart/uart-ns16550a.h"

#define read(reg) read8(UART_BASE + reg)
#define write(reg, val) write8(UART_BASE + reg, val)

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    write(UART_LCR, 0b11); // 8 data bits in LCR[1:0]
    write(UART_FCR, 0b1);  // enable FIFO in FCR[0]
    write(UART_IER, 0b1);  // enable received data available interrupts in IER[0]

    plic_set_intr_priority(UART0_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
    plic_enable_intr(UART0_IRQ_NUM);
}

void uart_writechar(char ch) {
    write(UART_THR, ch);
}

int uart_rx_num_avail() {
    uint8_t lsr = read(UART_LSR);
    return lsr & 0b1;
}

char uart_readchar() {
    return read(UART_RBR);
}

void uart_machine_wait_status() {
    // no-op
}
