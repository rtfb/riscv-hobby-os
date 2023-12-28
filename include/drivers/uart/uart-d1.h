#ifndef _UART_D1_H_
#define _UART_D1_H_

// Register names and values based on D1_User_Manual_V0.1(Draft Version).pdf,
// found here[1], because the official documentation page serves me a Bad
// Gateway.
//
// Some of the registers have the same value because they have different meaning
// for reading and writing.
//
// [1]: https://github.com/mangopi-sbc/MQ-Pro/blob/main/3.Docs/D1_User_Manual_V0.1(Draft%20Version).pdf

#define UART_RBR        0x0000  // Receive Buffer Register
#define UART_THR        0x0000  // Transmit Holding Register
#define UART_DLL        0x0000  // Divisor Latch Low Register
#define UART_DLH        0x0004  // Divisor Latch High Register
#define UART_IER        0x0004  // Interrupt Enable Register
#define UART_IIR        0x0008  // Interrupt Identity Register
#define UART_FCR        0x0008  // FIFO Control Register
#define UART_LCR        0x000C  // Line Control Register
#define UART_MCR        0x0010  // Modem Control Register
#define UART_LSR        0x0014  // Line Status Register
#define UART_MSR        0x0018  // Modem Status Register
#define UART_SCH        0x001c  // Scratch Register
#define UART_USR        0x007c  // UART Status Register
#define UART_TFL        0x0080  // Transmit FIFO Level Register
#define UART_RFL        0x0084  // Receive FIFO Level Register
#define UART_HSK        0x0088  // DMA Handshake Configuration Register
#define UART_DMA_REQ_EN 0x008c  // DMA Request Enable Register
#define UART_HALT       0x00a4  // Halt TX Register
#define UART_DBG_DLL    0x00b0  // Debug DLL Register
#define UART_DBG_DLH    0x00b4  // Debug DLH Register
#define UART_A_FCC      0x00f0  // FIFO Clock Control Register

#endif // ifndef _UART_D1_H_
