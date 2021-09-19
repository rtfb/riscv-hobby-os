#include "proc.h"
#include "kernel.h"
#include "fdt.h"
#include "string.h"

void init_test_processes() {
    if (!strncmp(fdt_get_bootargs(), "dry-run", ARRAY_LENGTH("dry-run"))) {
        return;
    }
    num_procs = 2;
    proc_table[0].pid = 0;
    proc_table[0].pc = user_entry_point;
    proc_table[1].pid = 1;
    proc_table[1].pc = user_entry_point2;
}
