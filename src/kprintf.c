#include "sys.h"
#include "printf-macro.h"
#include "drivers/uart/uart.h"

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

int32_t kprintfvec(char const* fmt, regsize_t* args) {
    int bufsize = PAGE_SIZE;
    int nargs = 7;
    #define _PRINTF_WRITE_CHAR(c) uart_writechar(c)
    #define _PRINT_RADIX kprint_radix
    #define _PRINT_RADIXU kprint_radixu
    PRINTF_IMPL;
    #undef _PRINTF_WRITE_CHAR
    #undef _PRINT_RADIX
}
