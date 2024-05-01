#ifndef _QEMU_E_H_
#define _QEMU_E_H_

#define BOOT_HART_ID    0

#define BOOT_MODE_M     1
#define HAS_S_MODE      0
#define HAS_BOOTARGS    0
#define CODE_IN_ROM     1

#define PAGE_SIZE       512 // bytes

// Irrelevant on qemu.
#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0

// Based on RISCV_ACLINT_DEFAULT_TIMEBASE_FREQ = 10000000 value from QEMU ACLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/riscv_aclint.h
// It's used in riscv/sifive_e and in riscv/virt machines.
#define ONE_SECOND        (10*1000*1000)

// https://github.com/qemu/qemu/blob/master/include/hw/riscv/sifive_e.h#L82
//      SIFIVE_E_UART0_IRQ    = 3,
#define UART0_IRQ_NUM   3

#define UART_BASE       0x10013000

// https://github.com/qemu/qemu/blob/master/hw/riscv/sifive_e.c#L61
//      SIFIVE_E_DEV_GPIO0 = 0x10012000
#define GPIO_BASE 0x10012000

#define CLINT0_BASE_ADDRESS   0x2000000

#define NUM_HARTS   1

#define PLIC_BASE                   0x0c000000
#define PLIC_NUM_INTR_SOURCES       53
#define PLIC_MAX_PRIORITY           7

#define MAX_PROCS   4

#endif // ifndef _QEMU_E_H_
