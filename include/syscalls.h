#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include "sys.h"
#include "syscallnums.h"

// Notes:
// * xxxxram numbers are in pages, multiply them by PAGE_SIZE to get bytes
// * the commented fields are not (yet?) implemented, uncomment them as we go
typedef struct sysinfo_s {
    // uint32_t uptime;             // Seconds since boot
    // uint32_t loads[3];  // 1, 5, and 15 minute load averages
    uint32_t totalram;  // Total usable main memory size
    uint32_t freeram;   // Available memory size
    // uint32_t sharedram; // Amount of shared memory
    // uint32_t bufferram; // Memory used by buffers
    // uint32_t totalswap; // Total swap space size
    // uint32_t freeswap;  // Swap space still available
    uint32_t procs;    // Number of current processes

    // the region of unclaimed memory between stack_top_addr and the first page
    regsize_t unclaimed_start;
    regsize_t unclaimed_end;
} sysinfo_t;

typedef struct pinfo_s {
    uint32_t pid;
    char name[16];
    uint32_t state;
    uint64_t nscheds; // number of times the process was scheduled
} pinfo_t;

#define DIRENT_READABLE   (1 << 0)
#define DIRENT_WRITABLE   (1 << 1)
#define DIRENT_EXECUTABLE (1 << 2)
#define DIRENT_DIRECTORY  (1 << 16)

#define MAX_FILENAME_LEN 16

// dirent_t represents a directory entry. When a directory is open()ed, read()
// should be given dirents to fill in.
typedef struct dirent_s {
    uint32_t flags; // DIRENT_*
    char name[MAX_FILENAME_LEN];
} dirent_t;

#define WAIT_COND_CHILD     0
#define WAIT_COND_NSCHEDS   1

typedef struct wait_cond_s {
    uint32_t type;
    uint32_t target_pid;
    uint64_t want_nscheds;
} wait_cond_t;

// TODO: unify the return values of syscalls

void sys_restart();
void sys_exit();
uint32_t sys_fork();
int32_t sys_read();
int32_t sys_write();
int32_t sys_open();
int32_t sys_close();
int32_t sys_wait();
uint32_t sys_execv();
uint32_t sys_getpid();
uint32_t sys_dup();
uint32_t sys_pipe();
uint32_t sys_sysinfo();
uint32_t sys_sleep();
uint32_t sys_plist();
uint32_t sys_pinfo();
regsize_t sys_pgalloc();
regsize_t sys_pgfree();
uint32_t sys_gpio();
uint32_t sys_detach();
int32_t sys_isopen();
uint32_t sys_pipeattch();
uint32_t sys_lsdir();

// These are implemented in assembler as of now:
extern void poweroff();

#endif // ifndef _SYSCALLS_H_
