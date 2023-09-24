#include "asm.h"
.balign 4

.section .text
.global poweroff                        # Poweroff hart.
                                        # @param[in] a0 error code 0=success, -1=fail

.ifdef QEMU_EXIT
poweroff:
        .equ QEMU_TEST_BASE,    QEMU_EXIT
        .equ TEST_REG_TX,       0
        .equ TEST_CMD_FAIL,     0x3333
        .equ TEST_CMD_PASS,     0x5555

        li      t0, TEST_CMD_PASS
        bgez    a0, 1f
        li      t0, TEST_CMD_FAIL
1:      li      t1, QEMU_TEST_BASE
        sw      t0, TEST_REG_TX(t1)     # write to QEMU virtual test control device

2:      wfi
        j       2b

.else

poweroff:
1:      wfi
        j       1b

.endif
