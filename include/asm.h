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

#if TARGET_M_MODE
    #define REG_IE       STR(mie)
    #define REG_TVEC     STR(mtvec)
    #define REG_CAUSE    STR(mcause)
    #define REG_EPC      STR(mepc)
    #define REG_TVAL     STR(mtval)
    #define REG_SCRATCH  STR(mscratch)
    #define REG_STATUS   STR(mstatus)

    #define OP_xRET      STR(mret)
#else
    #define REG_IE       STR(sie)
    #define REG_TVEC     STR(stvec)
    #define REG_CAUSE    STR(scause)
    #define REG_EPC      STR(sepc)
    #define REG_TVAL     STR(stval)
    #define REG_SCRATCH  STR(sscratch)
    #define REG_STATUS   STR(sstatus)

    #define OP_xRET      STR(sret)
#endif

#endif // ifndef _ASM_H_
