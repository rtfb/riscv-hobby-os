#ifndef _UART_H_
#define _UART_H_

// #define UART_BASE 0x10010000 // for run-baremetal
#define UART_BASE 0x10013000 // for run-baremetal32 and for HiFive
#define UART_TXDATA 0
#define UART_RXDATA 4
#define UART_RXCTRL 0xc
#define UART_BAUD_RATE_DIVISOR 0x18

void uart_init();
char uart_readchar();
void uart_writechar(char ch);

#endif // ifndef _UART_H_
