#ifndef _OX64_H_
#define _OX64_H_

// wild guess:
// xtal-clk {
//      compatible = "fixed-clock";
//      clock-frequency = <0x2625a00>;
#define ONE_SECOND 1000000
// #define ONE_SECOND 32768

// reg = <0x30002000 0x1000>;
// interrupts = <0x14 0x04>;
#define UART0_IRQ_NUM   0x14

// reg = <0x200008c4 0x1000>;
#define GPIO_BASE 0x200008c4

#endif // ifndef _OX64_H_
