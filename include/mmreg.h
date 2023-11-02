#ifndef _MMREG_H_
#define _MMREG_H_

#include "sys.h"

// Macros for accessing and manipulating memory-mapped registers.

#define read32(reg) *(uint32_t*)(reg)
#define write32(reg, val) *(uint32_t*)(reg) = val

#define set_bitfield(reg_val, new_val, mask, shift) ((reg_val & ~mask) | (new_val << shift))

#endif // ifndef _MMREG_H_
