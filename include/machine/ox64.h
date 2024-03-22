#ifndef _OX64_H_
#define _OX64_H_

#define BOOT_HART_ID    0

#define BOOT_MODE_M     0
#define HAS_S_MODE      1

#define PAGE_SIZE       512 // bytes

#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0x200000

// wild guess:
// xtal-clk {
//      compatible = "fixed-clock";
//      clock-frequency = <0x2625a00>;
#define ONE_SECOND 1000000

#define IRQ_NUM_BASE    16

// reg = <0x30002000 0x1000>;
// interrupts = <0x14 0x04>;
#define UART0_IRQ_NUM   (IRQ_NUM_BASE + 4)

#define UART_BASE       0x30002000

// reg = <0x200008c4 0x1000>;
#define GPIO_BASE 0x200008c4

// ox64.dts:
//      timer@e4000000 {
//          compatible = "thead,c900-clint";
//           reg = <0xe4000000 0xc000>;
#define CLINT0_BASE_ADDRESS   0xe4000000

#define NUM_HARTS   1

#define PLIC_BASE                   0xe0000000
#define PLIC_NUM_INTR_SOURCES       21
#define PLIC_MAX_PRIORITY           7

#endif // ifndef _OX64_H_
