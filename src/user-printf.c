#include "userland.h"

// print_radix prints a given number in a given radix. It stores the string in
// a provided buffer, fills it starting from the end, and returns a pointer
// into it, pointing to the beginning of the number.
char const* _userland print_radix(regsize_t num, int radix, char *buf, unsigned int size) {
    int dsti = size - 1;
    buf[dsti] = 0; // terminate the string
    dsti--;
    int limit = 0;
    int negative = 0;
    if (num == 0) {
        buf[dsti] = '0';
        return &buf[dsti];
    }
    if (num < 0) {
        limit++; // leave space for the minus sign
        negative = 1;
        num = -num;
    }
    while (num > 0 && dsti >= limit) {
        int digit = num % radix;
        if (digit > 9) {
            buf[dsti] = 'a' + (digit - 10);
        } else {
            buf[dsti] = '0' + digit;
        }
        num /= radix;
        dsti--;
    }
    dsti++;
    if (negative) {
        buf[dsti] = '-';
    }
    return &buf[dsti];
}

// printfvec does formatted output. It formats the output into an internal
// buffer and then calls sys_puts with it. The buffer is limited to 96
// characters, which, together with other stack variables is more than 1/4th of
// our entire stack frame, we can't afford more.
//
// Returns the size of the resulting formatted string.
//
// It supports the following verbs: %c, %d, %s, %x.
int _userland printfvec(char const* fmt, regsize_t* args) {
    int argnum = 0;
    char buf[96];
    int srci = 0;
    int dsti = 0;
    const int rbufsz = 16;
    char rbuf[rbufsz];
    while (fmt[srci] != 0) {
        char c = fmt[srci];
        if (c == '%' && argnum < 7) {
            srci++;
            c = fmt[srci];
            if (c == '%') {
                buf[dsti] = '%';
            } else {
                char const* tmp;
                switch (c) {
                    case 'c':
                        ;
                        char cb[2];
                        cb[0] = args[argnum++];
                        cb[1] = 0;
                        tmp = cb;
                        break;
                    case 'd':
                        ;
                        int i = args[argnum++];
                        tmp = print_radix(i, 10, rbuf, rbufsz);
                        break;
                    case 'x':
                        ;
                        int h = args[argnum++];
                        tmp = print_radix(h, 16, rbuf, rbufsz);
                        break;
                    case 's':
                        tmp = (char const*)args[argnum++];
                        break;
                }
                for (; *tmp != 0; tmp++) {
                    buf[dsti] = *tmp;
                    dsti++;
                }
                dsti--;
            }
        } else {
            buf[dsti] = c;
        }
        srci++;
        dsti++;
    }
    buf[dsti] = 0;
    sys_puts(buf);
    return dsti;
}
