#ifndef _UART_NS16550A_H_
#define _UART_NS16550A_H_

// Qemu virt machine uses NS16550A UART[1]. Register constants based on this[2]
// tutorial.
//
// [1]: https://www.qemu.org/docs/master/system/riscv/virt.html
// [2]: https://www.lammertbies.nl/comm/info/serial-uart

#define UART_RBR        0x0000  // Receive Buffer Register
#define UART_THR        0x0000  // Transmit Holding Register
#define UART_DLL        0x0000  // Divisor Latch Low Register
#define UART_DLH        0x0001  // Divisor Latch High Register
#define UART_IER        0x0001  // Interrupt Enable Register
#define UART_IIR        0x0002  // Interrupt Identity Register
#define UART_FCR        0x0002  // FIFO Control Register
#define UART_LCR        0x0003  // Line Control Register
#define UART_MCR        0x0004  // Modem Control Register
#define UART_LSR        0x0005  // Line Status Register
#define UART_MSR        0x0006  // Modem Status Register
#define UART_SCH        0x0007  // Scratch Register

#endif // ifndef _UART_NS16550A_H_
