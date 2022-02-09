#include "sys.h"
#include "uart.h"

void uart_init() {
    // enable reading:
    *(uint32_t*)(UART_BASE + UART_RXCTRL) = 1;
    // set baud divisor (the SiFive FE310-G002 manual lists a table of possible
    // values in Section 18.9, determined this particular choice
    // experimentally. Furthermore, it's the default on HiFive1-revB board):
    *(uint32_t*)(UART_BASE + UART_BAUD_RATE_DIVISOR) = 138;
}

char uart_readchar() {
    volatile int32_t* rx = (int32_t*)(UART_BASE + UART_RXDATA);
    int32_t word;
    do {
        word = *rx;
    } while (word < 0);
    return word & 0xff;
}

void uart_writechar(char ch) {
    volatile int32_t* tx = (int32_t*)(UART_BASE + UART_TXDATA);
    while ((int32_t)(*tx) < 0);
    *tx = ch;
}

int32_t uart_readline(char* buf, uint32_t bufsize) {
    int32_t nread = 0;
    for (;;) {
        if (nread >= bufsize) {
            break;
        }
        char ch = uart_readchar();
        if (ch == ASCII_DEL) {
            if (nread > 0) {
                nread--;
                uart_writechar('\b');
                uart_writechar(' ');
                uart_writechar('\b');
            }
        } else {
            buf[nread] = ch;
            nread++;
            uart_writechar(ch); // echo back to console
        }
        if (ch == '\r') {
            uart_writechar('\n'); // echo back to console
            break;
        }
    }
    return nread;
}
