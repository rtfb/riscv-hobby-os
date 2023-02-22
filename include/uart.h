#ifndef _UART_H_
#define _UART_H_

#ifndef UART_BASE
#define UART_BASE 0x10010000
#endif

#define UART_TXDATA 0
#define UART_RXDATA 4
#define UART_RXCTRL 0xc
#define UART_BAUD_RATE_DIVISOR 0x18

#define ASCII_DEL 0x7f

void uart_init();
char uart_readchar();
void uart_writechar(char ch);
int32_t uart_readline(char* buf, uint32_t bufsize);
int32_t uart_print(char const* data, uint32_t size);

// implemented in uart-print.s
extern int32_t uart_prints(char const* data);
extern int32_t uart_printc(char c);

#endif // ifndef _UART_H_
