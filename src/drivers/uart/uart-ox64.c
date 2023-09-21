#include "sys.h"
#include "proc.h"
#include "plic.h"
#include "drivers/uart/uart.h"
#include "drivers/uart/uart-ox64.h"

#define read(reg) *(uint32_t*)(UART_BASE + reg)
#define write(reg, val) *(uint32_t*)(UART_BASE + reg) = val
#define wr_bitfield(reg_val, new_val, mask, shift) ((reg_val & ~mask) | (new_val << shift))

// uart_machine_init does a machine-specific initialization of UART.
void uart_machine_init() {
    // mask all interrupts before any adjustments:
    write(BL808_UART_REG_INT_MASK, ~0);

    uint32_t val = read(BL808_UART_REG_INT_EN);
    val |= BL808_UART_RX_FIFO_READY
           // | BL808_UART_RX_TIMEOUT_INT // TODO: deal with timeouts later
           | BL808_UART_RX_FIFO_ERROR;
    write(BL808_UART_REG_INT_EN, val);

    uint32_t fifo_config_1 = read(BL808_UART_FIFO_CONFIG_1);
    int rx_fifo_threshold = 0;
    uint32_t new_val = wr_bitfield(fifo_config_1, rx_fifo_threshold,
                                   BL808_UART_RX_FIFO_THRES_MSK,
                                   BL808_UART_RX_FIFO_THRES_OFS);
    write(BL808_UART_FIFO_CONFIG_1, new_val);

    // now we're okay to unmask RX interrupts:
    val = read(BL808_UART_REG_INT_MASK);
    val &= ~(BL808_UART_RX_FIFO_READY
           // | BL808_UART_RX_TIMEOUT_INT
           | BL808_UART_RX_FIFO_ERROR);
    write(BL808_UART_REG_INT_MASK, val);

    val = read(BL808_UART_REG_UTX_CONFIG);
    val |= BL808_UART_ENABLE_FREERUN
         | BL808_UART_TX_ENABLE;
    write(BL808_UART_REG_UTX_CONFIG, val);
    val = read(BL808_UART_REG_URX_CONFIG);
    val |= BL808_UART_RX_ENABLE;
    write(BL808_UART_REG_URX_CONFIG, val);

    plic_enable_intr(UART0_IRQ_NUM);
    plic_set_intr_priority(UART0_IRQ_NUM, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
}

void uart_writechar(char ch) {
    int32_t tx_avail_count = 0;
    // spin while config register keeps reporting a full TX FIFO
    while (tx_avail_count == 0) {
        tx_avail_count = read(BL808_UART_FIFO_CONFIG_1) & BL808_UART_TX_FIFO_CNT_MSK;
    }
    write(BL808_UART_REG_TXFIFO, ch);
}

int uart_rx_num_avail() {
    uint32_t fifo_config_1 = read(BL808_UART_FIFO_CONFIG_1);
    return (fifo_config_1 & BL808_UART_RX_FIFO_CNT_MSK) >> 8;
}

char uart_readchar() {
    // spin while config register keeps reporting an empty RX FIFO
    while (uart_rx_num_avail() == 0) {
        // TODO: make sure this doesn't get compiled out
    }
    return read(BL808_UART_REG_RXFIFO) & 0xff;
}

void uart_machine_wait_status() {
    // spin while TX status is busy
    uint32_t status = 0;
    do {
        status = read(BL808_UART_REG_STATUS);
    } while((status & BL808_UART_STATUS_TX_BUSY_MASK) != 0);
}
