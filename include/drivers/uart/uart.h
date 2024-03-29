#ifndef _UART_H_
#define _UART_H_

#include "fs.h"

#ifndef UART_BASE
#error "UART_BASE undefined"
#endif

#define UART_TXDATA 0x00
#define UART_RXDATA 0x04
#define UART_TXCTRL 0x08
#define UART_RXCTRL 0x0c
#define UART_IE     0x10
#define UART_IP     0x14
#define UART_BAUD_RATE_DIVISOR 0x18

#define UART_IE_TXWM 1
#define UART_IE_RXWM 2

#define ASCII_DEL 0x7f

#define UART_BUF_SZ 64

typedef struct uart_state_s {
    // TODO: spinlock lock;
    char rxbuf[UART_BUF_SZ];
    char txbuf[UART_BUF_SZ];
    int rx_rpos;
    int rx_wpos;
    int tx_rpos;
    int tx_wpos;
    int rx_num_newlines;  // number of '\n' chars in the buffer
    int rx_buff_full;     // indicates that rxbuf got full
} uart_state_t;

extern uart_state_t uart0; // defined in uart.c

void uart_init();

// uart_handle_interrupt is an entry point for UART interrupt handling.
void uart_handle_interrupt();

// uart_enqueue_chars reads characters from rx FIFO and enqueues them into the
// uart0's rxbuf. If rxbuf is full, rx_buff_full is set to 1 and all
// incoming data is dropped on the floor (unless it's a backspace, which frees
// up some space instead of using up more buffer space).
//
// When a newline character gets enqueued, all processes waiting on uart0 are
// woken up to get them a chance to continue reading.
int uart_enqueue_chars();

// uart_readline attempts to read a full line of characters from rxbuf. If
// rxbuf is empty or does not contain a newline character, the calling process
// yields execution and goes to sleep.
int32_t uart_readline(char* buf, uint32_t bufsize);

int32_t uart_print(char const* data, uint32_t size);

int32_t uart_read(file_t* f, uint32_t pos, void* buf, uint32_t bufsize);
int32_t uart_write(file_t* f, uint32_t pos, void* buf, uint32_t bufsize);
int32_t uart_prints(char const* data);

//
// Functions below are implemented in machine-specific uart-*.c files
//

// uart_machine_init performs machine-specific initialization.
extern void uart_machine_init();

// uart_writechar writes a single char to UART synchronously.
extern void uart_writechar(char ch);

// uart_readchar reads a single char from UART synchronously.
extern char uart_readchar();

// uart_rx_num_avail returns a number of characters available in RX FIFO. On
// some implementations it may return a 0/1, indicating a presence/absence of
// any characters.
int uart_rx_num_avail();

// uart_machine_wait_status waits for UART to report it's no longer busy.
extern void uart_machine_wait_status();

#endif // ifndef _UART_H_
