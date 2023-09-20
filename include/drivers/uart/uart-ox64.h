#ifndef _UART_OX64_H_
#define _UART_OX64_H_

#define BL808_UART_REG_UTX_CONFIG    0x00
#define BL808_UART_REG_URX_CONFIG    0x04
#define BL808_UART_REG_STATUS        0x30
#define BL808_UART_REG_BCR_INT_CFG   0x50
#define BL808_UART_FIFO_CONFIG_1     0x84
#define BL808_UART_REG_TXFIFO        0x88
#define BL808_UART_REG_RXFIFO        0x8c

#define BL808_UART_TX_FIFO_CNT_MSK   0x3f
#define BL808_UART_RX_FIFO_CNT_MSK   (0x3f << 8)
#define BL808_UART_RX_FIFO_THRES_MSK (0x1f << 24)
#define BL808_UART_RX_FIFO_THRES_OFS 24

#define BL808_UART_REG_INT_MASK      0x24
#define BL808_UART_REG_INT_EN        0x2c

#define BL808_UART_TX_ENABLE         (1 << 0)
#define BL808_UART_RX_ENABLE         (1 << 0)
#define BL808_UART_ENABLE_FREERUN    (1 << 2)

#define BL808_UART_TX_BREAK_BIT_CNT  (4 << 10)
#define BL808_UART_TX_DATA_BIT_CNT   (7 << 8)

#define BL808_UART_RX_DATA_BUF_LEN   (32 << 16)   // 32 characters
#define BL808_UART_RX_DATA_BIT_CNT   (7 << 8)

#define BL808_UART_RX_BUF_FULL_INT   (1 << 1)
#define BL808_UART_RX_FIFO_READY     (1 << 3)
#define BL808_UART_RX_TIMEOUT_INT    (1 << 4)
#define BL808_UART_RX_FIFO_ERROR     (1 << 7)
#define BL808_UART_RX_BCR_INT        (1 << 9)

#endif // ifndef _UART_OX64_H_
