#include "sys.h"

int __attribute__((__section__(".user_text"))) u_main() {
    char *msg = "Hello from U-mode!\n";

    do_sys_print(msg);
    return 0;

    /*
    register char* _msg asm("a0") = msg;
    register long ret asm("a0");
    register long nr asm("a7") = 4;
    asm volatile ("ecall\n"
                  : "=r" (ret)
                  : "r"(_msg), "r"(nr)
                  : "memory");
    return ret;
    */
}
