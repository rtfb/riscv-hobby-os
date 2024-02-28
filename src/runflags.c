#include "fdt.h"
#include "runflags.h"
#include "string.h"

char const *test_script = 0;

uint32_t parse_runflags() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return RUNFLAGS_DRY_RUN;
    }
    char const *bootargs = fdt_get_bootargs();
    int end_of_prefix = 0;
    if (has_prefix(bootargs, "test-script", &end_of_prefix)) {
        test_script = bootargs + end_of_prefix + 1;
        return RUNFLAGS_SMOKE_TEST;
    }
    if (has_prefix(bootargs, "tiny-stack", &end_of_prefix)) {
        test_script = bootargs + end_of_prefix + 1;
        return RUNFLAGS_TINY_STACK;
    }
    return 0;
}
