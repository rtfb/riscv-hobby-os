#ifndef _RUNFLAGS_H_
#define _RUNFLAGS_H_

#include "sys.h"

// Runflags is a thin abstraction on top of FDT for easier passing the runtime
// flags around the system.

#define RUNFLAGS_TESTS 1

#define RUNFLAGS_DRY_RUN    ((1 << 1) | RUNFLAGS_TESTS)
#define RUNFLAGS_SMOKE_TEST ((1 << 2) | RUNFLAGS_TESTS)
#define RUNFLAGS_TINY_STACK ((1 << 3) | RUNFLAGS_TESTS)

uint32_t parse_runflags();

#endif // ifndef _RUNFLAGS_H_
