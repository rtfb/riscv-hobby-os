#ifndef _PRINTF_MACRO_H_
#define _PRINTF_MACRO_H_

#define PRINTF_IMPL                                                    \
    int argnum = 0;                                                    \
    int srci = 0;                                                      \
    int dsti = 0;                                                      \
    const int rbufsz = 16;                                             \
    char rbuf[rbufsz];                                                 \
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
                        int i = args[argnum++];                        \
                        tmp = _PRINT_RADIX(i, 10, rbuf, rbufsz);       \
                        break;                                         \
                    case 'p':                                          \
                        /* do a 0x, then fallthrough to do %x: */      \
                        _PRINTF_WRITE_CHAR('0');                       \
                        _PRINTF_WRITE_CHAR('x');                       \
                    case 'x':                                          \
                        ;                                              \
                        int h = args[argnum++];                        \
                        tmp = _PRINT_RADIX(h, 16, rbuf, rbufsz);       \
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

#define PRINT_RADIX_IMPL                                               \
    int dsti = size - 1;                                               \
    buf[dsti] = 0; /* terminate the string */                          \
    dsti--;                                                            \
    int limit = 0;                                                     \
    int negative = 0;                                                  \
    if (num == 0) {                                                    \
        buf[dsti] = '0';                                               \
        return &buf[dsti];                                             \
    }                                                                  \
    if (num < 0) {                                                     \
        limit++; /* leave space for the minus sign */                  \
        negative = 1;                                                  \
        num = -num;                                                    \
    }                                                                  \
    while (num > 0 && dsti >= limit) {                                 \
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
        buf[dsti] = '-';                                               \
    }                                                                  \
    return &buf[dsti]

#endif // ifndef _PRINTF_MACRO_H_
