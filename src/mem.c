#include "sys.h"

// memset fills the memory starting at ptr and ending at ptr+size with the
// specified value. The size is specified in bytes. If value does not fit in
// the range exactly, the remainder is filled with the least significant bytes
// of value byte-by-byte (little-endian).
void memset(void *ptr, regsize_t size, regsize_t value) {
    while (size >= sizeof(value)) {
        *(regsize_t*)ptr = value;
        ptr += sizeof(regsize_t);
        size -= sizeof(regsize_t);
    }
    while (size > 0) {
        *(uint8_t*)ptr = value & 0xff;
        ptr++;
        value >>= 8;
        size--;
    }
}
