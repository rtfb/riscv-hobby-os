#include "drivers/uart/uart.h"
#include "mmreg.h"
#include "plic.h"

void plic_init() {
    for (int i = 0; i < PLIC_NUM_INTR_SOURCES; i++) {
        plic_set_intr_priority(i, 0);
    }
}

void plic_enable_intr(int intr_no) {
    write32(PLIC_ENABLE, 1 << intr_no);
}

void plic_set_intr_priority(int intr_no, int priority) {
    write32(PLIC_PRIORITY + (uintptr_t)4*intr_no, priority);
}

void plic_set_threshold(int threshold) {
    write32(PLIC_THRESHOLD, threshold);
}

void plic_dispatch_interrupts() {
    uint32_t intr_no = read32(PLIC_CLAIM_RW);  // claim this interrupt
    switch (intr_no) {
        case UART0_IRQ_NUM:
            uart_handle_interrupt();
            break;
    }
    write32(PLIC_CLAIM_RW, intr_no);
}
