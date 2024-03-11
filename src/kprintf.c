#include "drivers/uart/uart.h"
#include "kernel.h"
#include "printf-macro.h"
#include "sys.h"

// kprint_radix prints a given number in a given radix. It stores the string in
// a provided buffer, fills it starting from the end, and returns a pointer
// into it, pointing to the beginning of the number.
char const* kprint_radix(long int num, int radix, char *buf, unsigned int size) {
    PRINT_RADIX_IMPL;
}

// the unsigned version of kprint_radix
char const* kprint_radixu(unsigned long int num, int radix, char *buf, unsigned int size) {
    PRINT_RADIX_IMPL;
}

// kprintfvec prints directly to uart.
int32_t kprintfvec(char const* fmt, regsize_t* args) {
    int bufsize = PAGE_SIZE;
    int nargs = 7;
    char digit_buf[DIGIT_BUF_SZ];
    #define _PRINTF_WRITE_CHAR(c) uart_writechar(c)
    #define _PRINT_RADIX kprint_radix
    #define _PRINT_RADIXU kprint_radixu
    PRINTF_IMPL;
    #undef _PRINTF_WRITE_CHAR
    #undef _PRINT_RADIX
    #undef _PRINT_RADIXU
}

// ksprintfvec is an equivalent of kprintfvec but for writing into a provided
// target buffer.
int32_t ksprintfvec(sprintfer_t *sprintfer, regsize_t* args) {
    char *buf = sprintfer->buf;
    regsize_t bufsize = sprintfer->bufsz;
    char const *fmt = sprintfer->fmt;
    // place the digit_buf at the top of the given buf:
    bufsize -= DIGIT_BUF_SZ;
    char *digit_buf = buf + bufsize;
    int nargs = 7;

    #define _PRINTF_WRITE_CHAR(c) buf[dsti] = c
    #define _PRINT_RADIX kprint_radix
    #define _PRINT_RADIXU kprint_radixu
    PRINTF_IMPL;
    #undef _PRINTF_WRITE_CHAR
    #undef _PRINT_RADIX
    #undef _PRINT_RADIXU
}
