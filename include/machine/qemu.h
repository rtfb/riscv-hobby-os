#ifndef _QEMU_H_
#define _QEMU_H_

// Based on SIFIVE_CLINT_TIMEBASE_FREQ = 10000000 value from QEMU SiFive CLINT
// implementation:
// https://github.com/qemu/qemu/blob/master/include/hw/intc/sifive_clint.h
#define ONE_SECOND        (10*1000*1000)

#endif // ifndef _QEMU_H_
