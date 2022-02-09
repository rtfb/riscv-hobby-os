#ifndef _UART_H_
#define _UART_H_

#ifndef UART_BASE
#define UART_BASE 0x10010000
#endif

#define UART_TXDATA 0
#define UART_RXDATA 4
#define UART_RXCTRL 0xc
#define UART_BAUD_RATE_DIVISOR 0x18

void uart_init();
char uart_readchar();
void uart_writechar(char ch);
int32_t uart_readline(char* buf, uint32_t bufsize);

#endif // ifndef _UART_H_
