#ifndef _QEMU_VIRT_H_
#define _QEMU_VIRT_H_

#define BOOT_HART_ID    0

#define BOOT_MODE_M     1
#define HAS_S_MODE      1
#define CONFIG_MMU      1

#define PAGE_SIZE       4096 // bytes

// Irrelevant on qemu.
#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0

// Based on RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ = 10000000 value from QEMU ACLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/riscv_aclint.h
// It's used in riscv/sifive_e and in riscv/virt machines.
#define ONE_SECOND        (10*1000*1000)

// https://github.com/qemu/qemu/blob/master/include/hw/riscv/virt.h#L89
//      UART0_IRQ = 10,
#define UART0_IRQ_NUM   10

// https://github.com/qemu/qemu/blob/master/hw/riscv/virt.c#L97
//      [VIRT_UART0] =        { 0x10000000,         0x100 },
#define UART_BASE       0x10000000

// not available on -machine=virt?
#define GPIO_BASE       0

#define CLINT0_BASE_ADDRESS   0x2000000

#define NUM_HARTS   1

#define PLIC_BASE                   0x0c000000
#define PLIC_NUM_INTR_SOURCES       53
#define PLIC_MAX_PRIORITY           7

#include "machine/plic-qemu-virt.h"

#endif // ifndef _QEMU_VIRT_H_
