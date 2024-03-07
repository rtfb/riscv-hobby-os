#include "sys.h"

// memset fills the memory starting at ptr and ending at ptr+size with the
// specified value. The size is specified in bytes. If value does not fit in
// the range exactly, the remainder is filled with the least significant bytes
// of value byte-by-byte (little-endian).
void memset(void *ptr, regsize_t size, regsize_t value) {
    regsize_t *rptr = (regsize_t*)ptr;
    regsize_t niters = size / sizeof(value);
    int remainder = size % sizeof(value);
    for (regsize_t i = 0; i < niters; i++) {
        *rptr++ = value;
    }
    if (remainder > 0) {
        uint8_t *bptr = (uint8_t*)rptr;
        while (remainder > 0) {
            *bptr = value & 0xff;
            bptr++;
            value >>= 8;
            remainder--;
        }
    }
}
