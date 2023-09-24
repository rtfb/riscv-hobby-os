#ifndef _MACHINE_H_
#define _MACHINE_H_

#if MACHINE == qemu_u
    #include "machine/qemu_u.h"
#else
    #error "Unknown MACHINE"
#endif

#endif // ifndef _MACHINE_H_
