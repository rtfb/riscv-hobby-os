#include "mmreg.h"
#include "proc.h"
#include "plic.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart-d1.h"

#define UART0 (UART_BASE + 0 * 0x4000)

#define LSR_TX_IDLE    (1 << 5)    // THR can accept another character to send
#define LSR_DATA_READY (1 << 0)    // FIFO is non-empty

#define LCR_DIVISOR_LATCH_ACCESS  (1 << 7)

#define read(reg) read32(UART0 + reg)
#define write(reg, val) write32(UART0 + reg, val)

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    write(UART_IER, 0x0);      // disable all interrupts
    write(UART_FCR, 0xf7);     // reset FIFO

    // The manual says to "Set the flow control parameter by writing the
    // UART_MCR register", which probably implies bit 5, the "auto flow
    // control". I don't know what that does, however, and it doesn't seem to be
    // necessary:
    // write(UART_MCR, 0x0);

    // This bit is very weird: if I execute the block below, I get an infinite
    // barrage of UART interrupts. PLIC just keeps firing with UART0_IRQ_NUM,
    // but nothing in the UART itself indicates anything actionable is going on.
    // And when I mark the interrupt as complete, PLIC immediately fires again,
    // bogging down the CPU.
    /*
    uint32_t val = read(UART_LCR);
    val |= LCR_DIVISOR_LATCH_ACCESS;    // select Divisor Latch Register
    write(UART_LCR, val);
    write(UART_DLL, 0xd & 0xff);        // 0x0d=13 240000000/(13*16) = 115200 Divisor Latch Low
    // write(UART_DLH, (0xd >> 8) & 0xff); // Divisor Latch High
    write(UART_DLH, 0); // Divisor Latch High
    val = read(UART_LCR);
    val &= ~LCR_DIVISOR_LATCH_ACCESS;
    write(UART_LCR, val);

    uint32_t val = read(UART_LCR);
    val &= ~0x1f;
    val |= (0x3 << 0) | (0 << 2) | (0 << 3); // 8 bit, 1 stop bit, parity disabled
    write(UART_LCR, val);
    */

    write(UART_FCR, 1); // enable FIFOs
    write(UART_IER, 1); // only enable RX IRQ

    plic_set_intr_priority(UART0_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
    plic_enable_intr(UART0_IRQ_NUM);
}

void uart_writechar(char ch) {
    while ((read(UART_LSR) & LSR_TX_IDLE) == 0)
        ;
    write(UART_THR, ch);
}

char uart_readchar() {
    return read(UART_RBR);
}

int uart_rx_num_avail() {
    uint32_t rfl = read(UART_RFL);
    return rfl;
}

void uart_machine_wait_status() {
    // no-op
}
