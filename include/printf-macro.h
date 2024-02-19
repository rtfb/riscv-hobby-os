#ifndef _PRINTF_MACRO_H_
#define _PRINTF_MACRO_H_

// DIGIT_BUF_SZ should be large enough to hold the largest int64 with a minus
// sign in front of it, and a terminating zero after it
#define DIGIT_BUF_SZ   21

#define PRINTF_IMPL                                                    \
    int argnum = 0;                                                    \
    int srci = 0;                                                      \
    int dsti = 0;                                                      \
    while (fmt[srci] != 0) {                                           \
        char c = fmt[srci];                                            \
        if (c == '%' && argnum < nargs) {                              \
            srci++;                                                    \
            c = fmt[srci];                                             \
            if (c == '%') {                                            \
                _PRINTF_WRITE_CHAR('%');                               \
            } else {                                                   \
                char const* tmp;                                       \
                switch (c) {                                           \
                    case 'c':                                          \
                        ;                                              \
                        char cb[2];                                    \
                        cb[0] = args[argnum++];                        \
                        cb[1] = 0;                                     \
                        tmp = cb;                                      \
                        break;                                         \
                    case 'd':                                          \
                        ;                                              \
                        long int i = args[argnum++];                   \
                        tmp = _PRINT_RADIX(i, 10, digit_buf, DIGIT_BUF_SZ); \
                        break;                                         \
                    case 'p':                                          \
                        /* do a 0x, then print an unsigned %x: */      \
                        _PRINTF_WRITE_CHAR('0');                       \
                        _PRINTF_WRITE_CHAR('x');                       \
                        unsigned long int uh = args[argnum++];         \
                        tmp = _PRINT_RADIXU(uh, 16, digit_buf, DIGIT_BUF_SZ); \
                        break;                                         \
                    case 'x':                                          \
                        ;                                              \
                        long int h = args[argnum++];                   \
                        tmp = _PRINT_RADIX(h, 16, digit_buf, DIGIT_BUF_SZ); \
                        break;                                         \
                    case 's':                                          \
                        tmp = (char const*)args[argnum++];             \
                        break;                                         \
                }                                                      \
                for (; *tmp != 0; tmp++) {                             \
                    _PRINTF_WRITE_CHAR(*tmp);                          \
                    dsti++;                                            \
                    if (dsti >= bufsize) {                             \
                        return dsti;                                   \
                    }                                                  \
                }                                                      \
                dsti--;                                                \
            }                                                          \
        } else {                                                       \
            _PRINTF_WRITE_CHAR(c);                                     \
        }                                                              \
        srci++;                                                        \
        dsti++;                                                        \
        if (dsti >= bufsize) {                                         \
            return dsti;                                               \
        }                                                              \
    }                                                                  \
    return dsti

// BUG: this does not support the largest negative number that fits in
// __riscv_xlen. That's because of the "num = -num" in the negative branch. This
// requires some fiddly fix that I'm not interested in addressing now.
#define PRINT_RADIX_IMPL                                               \
    int dsti = size - 1;                                               \
    buf[dsti] = 0; /* terminate the string */                          \
    dsti--;                                                            \
    int negative = 0;                                                  \
    if (num == 0) {                                                    \
        buf[dsti] = '0';                                               \
        return &buf[dsti];                                             \
    }                                                                  \
    if (num < 0) {                                                     \
        negative = 1;                                                  \
        num = -num;                                                    \
    }                                                                  \
    while (num > 0 && dsti >= 0) {                                     \
        int digit = num % radix;                                       \
        if (digit > 9) {                                               \
            buf[dsti] = 'a' + (digit - 10);                            \
        } else {                                                       \
            buf[dsti] = '0' + digit;                                   \
        }                                                              \
        num /= radix;                                                  \
        dsti--;                                                        \
    }                                                                  \
    dsti++;                                                            \
    if (negative) {                                                    \
        dsti--;                                                        \
        buf[dsti] = '-';                                               \
    }                                                                  \
    return &buf[dsti]

#endif // ifndef _PRINTF_MACRO_H_
