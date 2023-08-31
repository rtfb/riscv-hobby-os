#include "sys.h"
#include "proc.h"
#include "drivers/uart/uart.h"
#include "plic.h"
#include "gpio.h"

uart_state_t uart0;

void uart_init() {
    uart0.rx_rpos = 0;
    uart0.rx_wpos = 0;
    uart0.tx_rpos = 0;
    uart0.tx_wpos = 0;
    uart0.rx_num_newlines = 0;
    uart0.rx_buff_full = 0;

    // enable reading:
    *(uint32_t*)(UART_BASE + UART_RXCTRL) = 1;

    // enable read interrupts:
    *(uint32_t*)(UART_BASE + UART_IE) = UART_IE_RXWM;

    // set baud divisor (the SiFive FE310-G002 manual lists a table of possible
    // values in Section 18.9, determined this particular choice
    // experimentally. Furthermore, it's the default on HiFive1-revB board):
    *(uint32_t*)(UART_BASE + UART_BAUD_RATE_DIVISOR) = 138;

    plic_enable_intr(UART0_IRQ_NO);
    plic_set_intr_priority(UART0_IRQ_NO, PLIC_MAX_PRIORITY);
    plic_set_threshold(PLIC_MAX_PRIORITY - 1);
}

void uart_handle_interrupt() {
    int nenq = uart_enqueue_chars();
}

// _decrement_wpos attempts to decrement a given wpos. It doesn't decrement
// behind rx_rpos and doesn't decrement onto the last newline. It also wraps
// around zero.
int _decrement_wpos(int wpos) {
    int orig_wpos = wpos;
    if (wpos == uart0.rx_rpos) {
        return orig_wpos;
    }
    wpos--;
    if (wpos < 0) {
        wpos = UART_BUF_SZ - 1;
    }
    if (uart0.rxbuf[wpos] == '\n') {
        return orig_wpos;
    }
    return wpos;
}

int uart_enqueue_chars() {
    volatile int32_t* rx = (int32_t*)(UART_BASE + UART_RXDATA);
    int wpos = uart0.rx_wpos;
    int num_enqueued = 0;
    while (1) {
        int32_t word = *rx;
        if (word < 0) {
            break;
        }
        char ch = word & 0xff;
        if (ch == ASCII_DEL) {
            int new_wpos = _decrement_wpos(wpos);
            if (new_wpos != wpos) {
                wpos = new_wpos;
                uart_writechar('\b');
                uart_writechar(' ');
                uart_writechar('\b');
                // we've decremented wpos, so unmark the buffer as full if it was such
                if (uart0.rx_buff_full) {
                    uart0.rx_buff_full = 0;
                }
            }
            continue;
        }
        if (uart0.rx_buff_full) {
            proc_mark_for_wakeup(&uart0);
            continue; // if rxbuf is full, just keep draining rx FIFO
        }
        if (ch == '\r') {
            ch = '\n';
            uart0.rx_num_newlines++;
            proc_mark_for_wakeup(&uart0);
        }
        uart_writechar(ch); // echo back to console
        uart0.rxbuf[wpos] = ch;
        wpos++;
        num_enqueued++;
        if (wpos >= UART_BUF_SZ) {
            wpos = 0;
        }
        if (wpos == uart0.rx_rpos) {
            uart0.rx_buff_full = 1;
        }
    }
    uart0.rx_wpos = wpos;
    return num_enqueued;
}

void uart_writechar(char ch) {
    volatile int32_t* tx = (int32_t*)(UART_BASE + UART_TXDATA);
    while ((int32_t)(*tx) < 0);
    *tx = ch;
}

// _can_read_from checks whether the internal state of a given uart device
// allows reading from it.
int _can_read_from(uart_state_t *uart) {
    if (uart->rx_num_newlines > 0) {
        return 1;
    }
    if (uart->rx_buff_full == 1) {
        return 1;
    }
    return 0;
}

int32_t uart_readline(char* buf, uint32_t bufsize) {
    while (!_can_read_from(&uart0)) {
        proc_yield(&uart0);
    }
    int32_t nread = 0;
    int32_t rpos = uart0.rx_rpos;
    int32_t wpos = uart0.rx_wpos;
    for (;;) {
        if (nread >= bufsize) {
            break;
        }
        if (rpos == wpos && nread > 0) {
            break;
        }
        char ch = uart0.rxbuf[rpos];
        rpos++;
        if (rpos == UART_BUF_SZ) {
            rpos = 0;
        }
        if (ch == '\n') {
            uart0.rx_num_newlines--;
            break;
        }
        buf[nread] = ch;
        nread++;
    }
    if (uart0.rx_rpos != rpos || nread > 0) {
        uart0.rx_buff_full = 0; // at least one char was read, so it's no longer full
    }
    uart0.rx_rpos = rpos;
    return nread;
}

int32_t uart_print(char const* data, uint32_t size) {
    if (size == -1) {
        return uart_prints(data);
    }
    int i = 0;
    while (i < size) {
        uart_printc(data[i]);
        i++;
    }
    return i;
}

int32_t uart_read(file_t* f, uint32_t pos, void* buf, uint32_t bufsize) {
    return uart_readline(buf, bufsize);
}

int32_t uart_write(file_t* f, uint32_t pos, void* buf, uint32_t bufsize) {
    return uart_print(buf, bufsize);
}
