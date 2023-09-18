#ifndef _OX64_H_
#define _OX64_H_

#undef TARGET_M_MODE
#define NO_TRAP_DELEGATION  1

// wild guess:
// xtal-clk {
//      compatible = "fixed-clock";
//      clock-frequency = <0x2625a00>;
#define ONE_SECOND 1000000

// reg = <0x30002000 0x1000>;
// interrupts = <0x14 0x04>;
#define UART0_IRQ_NUM   0x14

// reg = <0x200008c4 0x1000>;
#define GPIO_BASE 0x200008c4

// ox64.dts:
//      timer@e4000000 {
//          compatible = "thead,c900-clint";
//           reg = <0xe4000000 0xc000>;
#define CLINT0_BASE_ADDRESS   0xe4000000

#define NUM_HARTS   1

#endif // ifndef _OX64_H_
