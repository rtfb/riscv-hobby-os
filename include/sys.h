#ifndef _SYS_H_
#define _SYS_H_

// The system-specific macros can be listed with this:
// $ riscv64-linux-gnu-gcc -march=rv64g -mabi=lp64 -dM -E - < /dev/null | grep riscv
#if __riscv_xlen == 32
    #define XLEN 32
    #define int64_t long long
    #define uint64_t unsigned long long
    #define uint32_t unsigned long
    #define uintptr_t unsigned long
#elif __riscv_xlen == 64
    #define XLEN 64
    #define int64_t long
    #define uint64_t unsigned long
    #define uint32_t unsigned int
    #define uintptr_t unsigned long
#else
    #error Unknown xlen
#endif

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

extern void sys_puts(char const* msg);

#endif // ifndef _SYS_H_
