#include "fdt.h"
#include "runflags.h"
#include "string.h"

uint32_t parse_runflags() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return RUNFLAGS_DRY_RUN;
    }
    if (!strncmp(fdt_get_bootargs(), "smoke-test", ARRAY_LENGTH("smoke-test"))) {
        return RUNFLAGS_SMOKE_TEST;
    }
    if (!strncmp(fdt_get_bootargs(), "tiny-stack", ARRAY_LENGTH("tiny-stack"))) {
        return RUNFLAGS_TINY_STACK;
    }
    return 0;
}
