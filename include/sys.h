#ifndef _SYS_H_
#define _SYS_H_

// The system-specific macros can be listed with this:
// $ riscv64-linux-gnu-gcc -march=rv64g -mabi=lp64 -dM -E - < /dev/null | grep riscv
#if __riscv_xlen == 32
    #define XLEN 32
    #define int32_t long
    #define int64_t long long
    #define uint64_t unsigned long long
    #define uint32_t unsigned long
    #define uintptr_t unsigned long
    #define regsize_t uint32_t
#elif __riscv_xlen == 64
    #define XLEN 64
    #define int32_t int
    #define int64_t long
    #define uint64_t unsigned long
    #define uint32_t unsigned int
    #define uintptr_t unsigned long
    #define regsize_t uint64_t
#else
    #error Unknown xlen
#endif

#define uint8_t unsigned char

#define ARRAY_LENGTH(a) (sizeof(a) / sizeof(a[0]))

// Define a couple section attributes. We specify the section with a
// trailing # in order to avoid a warning from GCC, see this answer:
// https://stackoverflow.com/a/58455496/6763
#define _text __attribute__((__section__(".text#")))
#define _rodata __attribute__((__section__(".rodata#")))

#endif // ifndef _SYS_H_
