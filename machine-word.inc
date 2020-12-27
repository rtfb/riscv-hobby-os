.ifdef RISCV32
.set XLEN,      32
.endif
.ifdef RISCV64
.set XLEN,      64
.endif

.ifdef XLEN
.equ REGBITS,   XLEN
.equ REGBYTES,  XLEN/8
.else
.equ XLEN,      64
.equ REGBITS,   64
.equ REGBYTES,  8
.endif
.if XLEN == 32
    .set RISCV32, 1
    .macro lx a, offset, addr
        lw \a, (\offset*REGBYTES)\addr
    .endm
    .macro sx a, offset, addr
        sw \a, (\offset*REGBYTES)\addr
    .endm
    .macro pointer a
        .word \a
    .endm
    .macro machineword a
        .word \a
    .endm
.else
    .set RISCV64, 1
    .macro lx a, offset, addr
        ld \a, (\offset*REGBYTES)\addr
    .endm
    .macro sx a, offset, addr
        sd \a, (\offset*REGBYTES)\addr
    .endm
    .macro pointer a
        .dword \a
    .endm
    .macro machineword a
        .dword \a
    .endm
.endif
