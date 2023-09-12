#ifndef _QEMU_U_H_
#define _QEMU_U_H_

// Based on CLINT_TIMEBASE_FREQ = 1000000 value from QEMU sifive_u implementation:
// https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c
#define ONE_SECOND        (1*1000*1000)

// https://github.com/qemu/qemu/blob/master/include/hw/riscv/sifive_u.h#L106
//      SIFIVE_U_UART0_IRQ = 4,
#define UART0_IRQ_NUM   4

// https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c#L84
//      SIFIVE_U_DEV_GPIO = 0x10060000
#define GPIO_BASE 0x10060000

#endif // ifndef _QEMU_U_H_
