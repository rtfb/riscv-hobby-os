#include "kernel.h"

void* shift_right_addr(void* addr, int bits) {
    unsigned long iaddr = (unsigned long)addr;
    return (void*)(iaddr >> bits);
}

void set_pmpaddr0(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr0, %0;"  // set pmpaddr0 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr1(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr1, %0;"  // set pmpaddr1 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr2(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr2, %0;"  // set pmpaddr2 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpaddr3(void* addr) {
    addr = shift_right_addr(addr, 2);
    asm volatile (
        "csrw   pmpaddr3, %0;"  // set pmpaddr3 to the requested addr
        :                       // no output
        : "r"(addr)             // input in addr
    );
}

void set_pmpcfg0(unsigned long value) {
    asm volatile (
        "csrw   pmpcfg0, %0;"   // set pmpcfg0 to the requested value
        :                       // no output
        : "r"(value)            // input in value
    );
}
