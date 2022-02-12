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
        macro_syscall SYS_NR_exit
        ret

.globl fork
fork:
        macro_syscall SYS_NR_fork
        ret

.globl read
read:
        macro_syscall SYS_NR_read
        ret

.globl sys_puts
sys_puts:
        macro_syscall SYS_NR_write
        ret

.globl wait
wait:
        macro_syscall SYS_NR_wait
        ret

.globl execv
execv:
        macro_syscall SYS_NR_execv
        ret

.globl getpid
getpid:
        macro_syscall SYS_NR_getpid
        ret

.globl sysinfo
sysinfo:
        macro_syscall SYS_NR_sysinfo
        ret

.globl sleep
sleep:
        macro_syscall SYS_NR_sleep
        ret

.globl plist
plist:
        macro_syscall SYS_NR_plist
        ret

.globl pinfo
pinfo:
        macro_syscall SYS_NR_pinfo
        ret
