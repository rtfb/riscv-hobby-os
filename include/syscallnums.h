// This file is to be included both in usyscalls.S and in C code, so let's keep
// it neat and minimal.

#define SYS_NR_restart         0
#define SYS_NR_exit            1
#define SYS_NR_fork            2
#define SYS_NR_read            3
#define SYS_NR_write           4
#define SYS_NR_wait            7
#define SYS_NR_execv          11
#define SYS_NR_getpid         20
