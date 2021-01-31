.include "src/machine-word.inc"

.section .user_text
.globl get_clock_cycles
get_clock_cycles:
                                        # 2.2 CSR Listing, Table 2.2: Currently allocated RISC-V user-level CSR addresses.
.if XLEN == 64
        csrr    a0, cycle               # read user level cycle counter
.else
        # TODO
.endif
        ret


.globl push_pop_stack
push_pop_stack:
        stackalloc_x 2
        lx      t0, 0,(sp)
        lx      t1, 1,(sp)
        sx      t0, 0,(sp)
        sx      t1, 1,(sp)
        stackfree_x 2
        ret


.globl m_read_hard_id
m_read_hard_id:
        csrr    a0, mhartid             # causes Illegal instruction (mcause=2) in User mode
        ret
