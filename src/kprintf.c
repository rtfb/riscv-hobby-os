#include "sys.h"
#include "pagealloc.h"
#include "printf-macro.h"
#include "drivers/uart/uart.h"

// kprint_radix prints a given number in a given radix. It stores the string in
// a provided buffer, fills it starting from the end, and returns a pointer
// into it, pointing to the beginning of the number.
char const* kprint_radix(regsize_t num, int radix, char *buf, unsigned int size) {
    PRINT_RADIX_IMPL;
}

int32_t kprintfvec(char const* fmt, regsize_t* args) {
    int bufsize = PAGE_SIZE;
    int nargs = 7;
    #define _PRINTF_WRITE_CHAR(c) uart_printc(c)
    #define _PRINT_RADIX kprint_radix
    PRINTF_IMPL;
    #undef _PRINTF_WRITE_CHAR
    #undef _PRINT_RADIX
}
