#include "printf-macro.h"
#include "userland.h"
#include "ustr.h"

// print_radix prints a given number in a given radix. It stores the string in
// a provided buffer, fills it starting from the end, and returns a pointer
// into it, pointing to the beginning of the number.
char const* _userland print_radix(long int num, int radix, char *buf, unsigned int size) {
    PRINT_RADIX_IMPL;
}

// the unsigned version of print_radix
char const* print_radixu(unsigned long int num, int radix, char *buf, unsigned int size) {
    PRINT_RADIX_IMPL;
}

int32_t _userland printfvecbuf(char const* fmt, regsize_t* args, int nargs, char *buf, int bufsize) {
    #define _PRINTF_WRITE_CHAR(c) buf[dsti] = c
    #define _PRINT_RADIX print_radix
    #define _PRINT_RADIXU print_radixu
    PRINTF_IMPL;
    #undef _PRINTF_WRITE_CHAR
    #undef _PRINT_RADIX
}

// printfvec does formatted output. It formats the output into an internal
// buffer and then calls sys_puts with it. The buffer is an entire page, and
// it's freed at the end of function. If allocation fails, printfvec returns
// -1.
//
// Returns the size of the resulting formatted string.
//
// It supports the following verbs: %c, %d, %s, %x, %p.
int32_t _userland printfvec(char const* fmt, regsize_t* args) {
    char* buf = (char*)pgalloc();
    if (!buf) {
        return -1;
    }
    int32_t buf_written = printfvecbuf(fmt, args, 7, buf, PAGE_SIZE-1);
    buf[buf_written] = 0;
    int32_t nwritten = write(1, buf, buf_written);
    pgfree(buf);
    return nwritten;
}

// prints is a convenience wrapper around the write() system call, it defaults
// to printing a zero-terminated string to stdout. It is cheaper to call
// prints() than printf() with a single argument because printf() has to
// allocate a buffer and prints() can call write() directly.
int32_t _userland prints(char const* str) {
    return write(1, str, -1);
}
