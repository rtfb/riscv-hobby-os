#include "sys.h"
#include "proc.h"
#include "drivers/uart/uart.h"

#ifdef CONFIG_LCD_ENABLED
#include "drivers/hd44780/hd44780.h"
#endif

uart_state_t uart0;

void uart_init() {
    uart_machine_init();
    uart0.rx_rpos = 0;
    uart0.rx_wpos = 0;
    uart0.tx_rpos = 0;
    uart0.tx_wpos = 0;
    uart0.rx_num_newlines = 0;
    uart0.rx_buff_full = 0;
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
    int wpos = uart0.rx_wpos;
    int num_enqueued = 0;
    while (1) {
        if (uart_rx_num_avail() == 0) {
            break;
        }
        char ch = uart_readchar();
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
#ifdef CONFIG_LCD_ENABLED
                lcd_backspace();
#endif
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
#ifdef CONFIG_LCD_ENABLED
        lcd_printn(&ch, 1);
#endif
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

int32_t uart_prints(char const* data) {
    while (*data) {
        uart_writechar(*data++);
    }
    uart_machine_wait_status();
}

int32_t uart_print(char const* data, uint32_t size) {
    if (size == -1) {
#ifdef CONFIG_LCD_ENABLED
        lcd_print(data);
#endif
        return uart_prints(data);
    }
#ifdef CONFIG_LCD_ENABLED
    lcd_printn(data, size);
#endif
    int i = 0;
    while (i < size) {
        uart_writechar(data[i]);
        i++;
    }
    uart_machine_wait_status();
    return i;
}

int32_t uart_read(file_t* f, uint32_t pos, void* buf, uint32_t bufsize) {
    return uart_readline(buf, bufsize);
}

int32_t uart_write(file_t* f, uint32_t pos, void* buf, uint32_t bufsize) {
    return uart_print(buf, bufsize);
}
