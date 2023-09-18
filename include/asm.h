#ifndef _ASM_H_
#define _ASM_H_

#ifdef __ASSEMBLER__
    #define __STR(x) x
#else
    #define __STR(x) #x
#endif

#define STR(x) __STR(x)

#if (__riscv_xlen == 64)
    #define OP_LOAD   STR(ld)
    #define OP_STOR   STR(sd)
    #define REGSZ     8
#elif (__riscv_xlen == 32)
    #define OP_LOAD   STR(lw)
    #define OP_STOR   STR(sw)
    #define REGSZ     4
#else
#error "unsupported XLEN"
#endif

#endif // ifndef _ASM_H_
