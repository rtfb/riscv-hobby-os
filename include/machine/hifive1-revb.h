#ifndef _HIFIVE1_REVB_
#define _HIFIVE1_REVB_

#define TARGET_M_MODE   1
#define NO_S_MODE       1

// SiFive FE310-G002 Manual v1p1
// Chapter 16 - Real-Time Clock (RTC)
//
// "The real-time clock (RTC) is located in the always-on domain, and is clocked
// by a selectable low-frequency clock source. For best accuracy, the RTC should
// be driven by an external 32.768 kHz watch crystal oscillator, but to reduce
// system cost, can be driven by a factorytrimmed on-chip oscillator."
#define ONE_SECOND 32768

// https://github.com/sifive/freedom-e-sdk/blob/master/bsp/sifive-hifive1-revb/core.dts#L166
//      interrupts = <3>;
#define UART0_IRQ_NUM   3

// https://github.com/sifive/freedom-e-sdk/blob/master/bsp/sifive-hifive1-revb/core.dts#L142
//      reg = <0x10012000 0x1000>;
#define GPIO_BASE 0x10012000

#define CLINT0_BASE_ADDRESS   0x2000000

#define NUM_HARTS   1

#endif // ifndef _HIFIVE1_REVB_
