#ifndef _D1_H_
#define _D1_H_

#include "machine/plic-d1.h"

#undef TARGET_M_MODE
#define NO_TRAP_DELEGATION  1

// For some reason on D1 this has to be an absolute address, not an offset.
#define LINUX_IMAGE_HEADER_TEXT_OFFSET   0x40200000

#define ONE_SECOND 24*1000*1000

#define UART0_IRQ_NUM   18

#define UART_BASE       0x02500000

#define GPIO_BASE       0x02000000

#define CLINT0_BASE_ADDRESS   0x14000000

#define NUM_HARTS   1

#endif // ifndef _D1_H_
