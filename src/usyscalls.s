.include "src/machine-word.inc"

### User payload = code + readonly data for U-mode ############################
#
.macro  macro_syscall nr
        li      a7, \nr                 # for fun let's pretend syscall is kinda like Linux: syscall nr in a7, other arguments in a0..a6
        ecall
.endm
.macro  macro_sys_poweroff exit_code
        li      a0, \exit_code
        macro_syscall 0
.endm
.macro  macro_sys_print addr
        la      a0, \addr
        macro_syscall 4
.endm

.balign 4
.section .user_text
.globl sys_puts
sys_puts:
        stackalloc_x 1
        sx      ra, 1, (sp)
        macro_syscall 4
        lx      ra, 1, (sp)
        stackfree_x 1
        ret

.globl getpid
getpid:
        stackalloc_x 1
        sx      ra, 1, (sp)
        macro_syscall 20
        lx      ra, 1, (sp)
        stackfree_x 1
        ret
