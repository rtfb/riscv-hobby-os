// This file is to be included both in usyscalls.S and in C code, so let's keep
// it neat and minimal.

#define SYS_NR_restart         0
#define SYS_NR_exit            1
#define SYS_NR_fork            2
#define SYS_NR_read            3
#define SYS_NR_write           4
#define SYS_NR_open            5
#define SYS_NR_close           6
#define SYS_NR_wait            7
#define SYS_NR_execv          11
#define SYS_NR_getpid         20

// The numbers below differ from Linux, may need to renumber one day if we ever
// want to achieve ABI compatibility. But for now, I don't want to make
// trap_vector too big while very sparsely populated.
#define SYS_NR_dup            28  // __NR_dup is 41 on Linux
#define SYS_NR_pipe           29  // __NR_pipe is 42 on Linux
#define SYS_NR_sysinfo        30  // __NR_sysinfo is 116 on Linux
#define SYS_NR_sleep          31  // __NR_nanosleep is 162 on Linux

// These are non-standard syscalls
#define SYS_NR_plist          32
#define SYS_NR_pinfo          33
#define SYS_NR_pgalloc        34
#define SYS_NR_pgfree         35
#define SYS_NR_gpio           36
#define SYS_NR_detach         37
#define SYS_NR_isopen         38
#define SYS_NR_pipeattch      39
