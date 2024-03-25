#ifndef _STAR64_H_
#define _STAR64_H_

#define BOOT_HART_ID    1

#define BOOT_MODE_M     0
#define HAS_S_MODE      1
#define CONFIG_MMU      1

#define PAGE_SIZE       4096 // bytes

#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0x40200000

// determined experimentally:
#define ONE_SECOND 4180000

// star64.dts:
// serial@10000000 {
//     interrupts = <0x00000020>;
#define UART0_IRQ_NUM   0x20
#define UART_BASE       0x10000000

// star64.dts:
// reg = <0x00000000 0x13040000 0x00000000 0x00010000>;
#define GPIO_BASE 0x13040000
// #define GPIO_BASE 0x10060000

// star64.dts:
// clint@2000000 {
//     reg = <0x00000000 0x02000000 0x00000000 0x00010000>;
#define CLINT0_BASE_ADDRESS   0x2000000

#define NUM_HARTS   1

#define PLIC_BASE               0x0c000000
#define PLIC_NUM_INTR_SOURCES   136
#define PLIC_MAX_PRIORITY       7

#endif // ifndef _STAR64_H_
