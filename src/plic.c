#include "sys.h"
#include "plic.h"
#include "drivers/uart/uart.h"

void plic_enable_intr(int intr_no) {
    *(uint32_t*)(PLIC_ENABLE) = 1 << intr_no;
}

void plic_set_intr_priority(int intr_no, int priority) {
    uint32_t *priority_base = (uint32_t*)(PLIC_PRIORITY);
    *(priority_base + intr_no) = priority;
}

void plic_set_threshold(int threshold) {
    *(uint32_t*)(PLIC_THRESHOLD) = threshold;
}

void plic_dispatch_interrupts() {
    uint32_t intr_no = *(uint32_t*)(PLIC_CLAIM_RW);  // claim this interrupt
    switch (intr_no) {
        case UART0_IRQ_NO:
            uart_handle_interrupt();
            break;
    }
    *(uint32_t*)(PLIC_CLAIM_RW) = intr_no;  // mark the completion
}
