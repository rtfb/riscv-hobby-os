#ifndef _QEMU_E_H_
#define _QEMU_E_H_

// Based on RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ = 10000000 value from QEMU ACLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/riscv_aclint.h
// It's used in riscv/sifive_e and in riscv/virt machines.
#define ONE_SECOND        (10*1000*1000)

// https://github.com/qemu/qemu/blob/master/include/hw/riscv/sifive_e.h#L82
//      SIFIVE_E_UART0_IRQ    = 3,
#define UART0_IRQ_NO    3

#endif // ifndef _QEMU_E_H_
