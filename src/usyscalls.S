.include "src/machine-word.inc"
#include "syscallnums.h"

.macro  macro_syscall nr
        li      a7, \nr                 # for fun let's pretend syscall is kinda like Linux: syscall nr in a7, other arguments in a0..a6
        ecall
.endm

.balign 4
.section .user_text
.globl exit
exit:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_exit
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl fork
fork:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_fork
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl read
read:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_read
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl sys_puts
sys_puts:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_write
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl wait
wait:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_wait
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl execv
execv:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_execv
        lx      ra, 0, (sp)
        stackfree_x 1
        ret

.globl getpid
getpid:
        stackalloc_x 1
        sx      ra, 0, (sp)
        macro_syscall SYS_NR_getpid
        lx      ra, 0, (sp)
        stackfree_x 1
        ret
