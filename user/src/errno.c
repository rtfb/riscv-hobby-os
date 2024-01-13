#include "sys.h"
#include "userland.h"

uintptr_t* _userland __errno_location() {
    register uintptr_t a0 asm ("a0");
    asm volatile (
        "mv a0, sp"
        : "=r"(a0)   // output in a0
    );
    uintptr_t curr_sp = a0;
    curr_sp &= ~(PAGE_SIZE - 1);
    curr_sp += PAGE_SIZE;
    return ((uintptr_t*)curr_sp) - 1;
}
