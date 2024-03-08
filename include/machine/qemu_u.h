#ifndef _QEMU_U_H_
#define _QEMU_U_H_

#define BOOT_HART_ID    0

#include "machine/plic-common.h"

#define BOOT_MODE_M     1
#define HAS_S_MODE      0

#define PAGE_SIZE       512 // bytes

// Irrelevant on qemu.
#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0

// Based on CLINT_TIMEBASE_FREQ = 1000000 value from QEMU sifive_u implementation:
// https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c
#define ONE_SECOND        (1*1000*1000)

// https://github.com/qemu/qemu/blob/master/include/hw/riscv/sifive_u.h#L106
//      SIFIVE_U_UART0_IRQ = 4,
#define UART0_IRQ_NUM   4

#define UART_BASE       0x10010000

// https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_u.c#L84
//      SIFIVE_U_DEV_GPIO = 0x10060000
#define GPIO_BASE 0x10060000

#define CLINT0_BASE_ADDRESS   0x2000000

#define NUM_HARTS   2

#endif // ifndef _QEMU_U_H_
