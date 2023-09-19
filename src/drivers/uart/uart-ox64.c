#include "sys.h"
#include "proc.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart-ox64.h"

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    *(uint32_t*)(UART_BASE + BL808_UART_REG_UTX_CONFIG) = (
          BL808_UART_TX_BREAK_BIT_CNT
        | BL808_UART_TX_DATA_BIT_CNT
        | BL808_UART_ENABLE_FREERUN
        | BL808_UART_TX_ENABLE
    );
    *(uint32_t*)(UART_BASE + BL808_UART_REG_INT_MASK) = ~0;
}

void uart_writechar(char ch) {
    volatile int32_t* fifo_config_1 = (int32_t*)(UART_BASE + BL808_UART_FIFO_CONFIG_1);
    int32_t tx_avail_count = 0;
    // spin while config register keeps reporting a full TX FIFO
    while (tx_avail_count == 0) {
        tx_avail_count = (*fifo_config_1 & BL808_UART_TX_FIFO_CNT_MSK);
    }
    volatile int32_t* tx_fifo = (int32_t*)(UART_BASE + BL808_UART_REG_TXFIFO);
    *tx_fifo = ch;
}

void uart_machine_wait_status() {
    // spin while TX status is busy
    volatile int32_t* status_reg = (int32_t*)(UART_BASE + BL808_UART_REG_STATUS);
    uint32_t status = 0;
    do {
        status = *status_reg;
    } while((status & 1) != 0);
}
