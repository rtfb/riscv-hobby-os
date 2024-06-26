#ifndef _PROC_H_
#define _PROC_H_

#include "bakedinfs.h"
#include "cpu.h"
#include "fs.h"
#include "riscv.h"
#include "spinlock.h"
#include "syscalls.h"
#include "sys.h"

#ifndef MAX_PROCS
#define MAX_PROCS 8
#endif

// PROC_STATE_AVAILABLE signifies an unoccupied slot in the process table, it's
// available for use by a new process.
#define PROC_STATE_AVAILABLE 0

// PROC_STATE_READY means the process is ready to be scheduled.
#define PROC_STATE_READY 1

// PROC_STATE_RUNNING means this process was given the time to run by the
// scheduler.
#define PROC_STATE_RUNNING 2

// PROC_STATE_SLEEPING means this process has yielded execution via sleep() or
// wait() system calls. It will not be scheduled until either enough time has
// elapsed (in case of sleep()) or until the child process exits (in case of
// wait()).
#define PROC_STATE_SLEEPING 3

#define PROC_STATE_ZOMBIE 4

// PROC_MAGIC_STACK_SENTINEL sits between stack_page and kstack_page and must
// never be modified. If something modified it, it must've been a stack
// overflow on the kernel side and we check for that upon exit from a syscall.
#define PROC_MAGIC_STACK_SENTINEL 0xdeadf00d

// PWAKE_COND_* constants are equivalent to the user-facing WAIT_COND_*
// constants. They're made separate to provide different names specific to the
// domains they're used in, and to allow kernel-private wait conditions if
// needed.
//
// PWAKE_COND_CHAN means a straightforward wait on whatever object the
// proc.chan field points to.
//
// PWAKE_COND_NSCHEDS means a wait until a target process's (specified by
// .target_pid and pointed to by proc.chan) nscheds counter reaches
// want_nscheds count.
#define PWAKE_COND_CHAN     0
#define PWAKE_COND_NSCHEDS  1

// pwake_cond_t describes the conditions for the process to wake up. type
// should be one of PWAKE_COND_* constants, other fields are type-specific.
typedef struct pwake_cond_s {
    uint32_t type;
    uint32_t target_pid;
    uint64_t want_nscheds;
} pwake_cond_t;

typedef struct trap_frame_s {
    regsize_t regs[31]; // all registers except x0
    regsize_t pc;
} trap_frame_t;

typedef struct process_s {
    spinlock lock;
    context_t ctx;
    regsize_t usatp;        // upagetable converted to satp format
    regsize_t *upagetable;  // user virtual page table
    uint32_t pid;
    char const *name;
    struct process_s* parent;
    trap_frame_t trap;

    // stack_page is a physical address of the base of the page allocated for
    // stack (i.e. it's the value returned by allocate_page()). We need to save
    // it so that we can later pass it to release_page(), as well as when
    // copying the entire stack around, e.g. during fork().
    void *stack_page;

    uintptr_t *perrno;  // points to the last word within stack_page, that's where we store errno

    uint32_t *magic;    // a magic number for detection of kstack_page overflows
    void *kstack_page;

    // state contains the state of the process, as well as the process table
    // slot itself (e.g. signifying the availability of the slot).
    uint32_t state;

    // wakeup_time contains a timer value when this process should continue
    // executing after a sleep() system call. If state != PROC_STATE_SLEEPING,
    // the value of wakeup_time has no meaning and should be ignored. If the
    // process was put into sleep by a wait() syscall instead of sleep(),
    // wakeup_time should be set to zero.
    uint64_t wakeup_time;

    void *chan; // pointer to an object this process is waiting on (e.g. a pipe)
    pwake_cond_t cond;

    file_t* files[MAX_PROC_FDS];

    // procfs-related stuff
    bifs_directory_t *procfs_dir;
    char piddir[MAX_FILENAME_LEN];
    bifs_file_t *procfs_name_file;

    uint64_t nscheds; // number of times the process was scheduled
} process_t;

typedef struct proc_table_s {
    spinlock lock;
    process_t procs[MAX_PROCS];
    int num_procs;  // XXX: do we really need it?
    uint32_t pid_counter;
} proc_table_t;

// defined in proc.c
extern proc_table_t proc_table;

// trap_frame is the piece of memory to hold all user registers when we enter
// the trap. When the scheduler picks the new process to run, it will save
// trap_frame in the process_t of the old process and will populate trap_frame
// with the new process's context.
//
// When we're ready to return to userland, trap_frame is expected to already
// contain everything the userland needs to have. In case of a fresh process, it
// needs to have at least regs[REG_SP] populated.
//
// In between the traps mscratch/sscratch will point here so that we can save
// user registers without trashing any.
extern trap_frame_t trap_frame;

// defined in context.s
void swtch(context_t *old, context_t *new);

// init_test_processes initializes the process table with a set of userland
// processes that will get executed by default. Kind of like what an initrd
// would do, but a poor man's version until we can do better.
void init_test_processes(uint32_t runflags);

void init_process_table();
void scheduler();
void sched();
void forkret();
void ret_to_user(regsize_t satp);  // defined in context.s

// find_ready_proc iterates over the proc table looking for the first available
// proc that's in a PROC_STATE_READY state. Wraps around and starts from zero
// until reaches the same index it started with, in which case it returns the
// same process.
//
// MUST be called with proc_table.lock held.
process_t* find_ready_proc();

// proc_fork implements the fork syscall. It will create a new entry in the
// process table, prepare it for being scheduled and return to the parent
// process the pid of the child.
uint32_t proc_fork();

// proc_exit implements the exit syscall. It will remove the process from the
// table, release all the resources taken by the process and call the
// scheduler.
regsize_t proc_exit();

// proc_execv implements the exec system call. The 'v' suffix means the
// arguments are passed vectorized, in an array of pointers to strings. The
// last element in argv must be a NULL pointer.
//
// For now, the filename argument is expected to contain a program name as
// specified in userland_programs.
uint32_t proc_execv(char const* filename, char const* argv[]);

// proc_wait implements the wait system call.
int32_t proc_wait();

// proc_yield is like proc_wait, but it doesn't return, it jumps to the newly
// scheduled userland process immediately. Use it in cases when further
// execution is impossible (e.g. i/o is blocked) and another process should be
// scheduled.
void proc_yield(void *chan);

// proc_sleep implements the sleep system call.
int32_t proc_sleep(uint64_t milliseconds);

// proc_mark_for_wakeup sets proc's state to PROC_STATE_READY.
void proc_mark_for_wakeup(void *chan);

// should_wake_up checks the proc's wakeup_time against current time and
// returns true if wakeup_time >= now.
int should_wake_up(process_t* proc);

int32_t psleep(process_t *proc);
int32_t proc_wait_by_cond(process_t *proc, pwake_cond_t *cond);
void update_proc_by_chan(process_t *proc, void *chan);

process_t* alloc_process();
uintptr_t init_proc(process_t* proc, regsize_t pc, char const *name);
uintptr_t init_procfs_files(process_t *proc, char const *name);

// alloc_pid returns a unique process identifier suitable to assign to a newly
// created process.
uint32_t alloc_pid();

// current_proc returns the userland process that's currently scheduled for
// running.
process_t* current_proc();

// myproc is an opinionated version of current_proc which panics if no process
// is found.
process_t* myproc();

// copy_trap_frame copies the contents of src into dst.
void copy_trap_frame(trap_frame_t* dst, trap_frame_t* src);
void copy_context(context_t *dst, context_t *src);
void patch_proc_sp(process_t *proc, regsize_t sp);

// copy_files copies non-NULL files from src to dst and increases reference
// count of each.
void copy_files(process_t *dst, process_t *src);

uint32_t proc_plist(uint32_t *pids, uint32_t size);

uint32_t proc_pinfo(uint32_t pid, pinfo_t *pinfo);

// proc_open and proc_close are the entry points of open()/close() syscalls,
// they start by dealing with the process-level file descriptors, then call the
// lower level FS stuff.
int32_t proc_open(char const *filepath, uint32_t flags);
int32_t proc_close(uint32_t fd);
int32_t proc_read(uint32_t fd, void *buf, uint32_t size);
int32_t proc_write(uint32_t fd, void *buf, uint32_t nbytes);

// proc_dup takes a given file descriptor and duplicates it. It allocates a new
// file descriptor with the lowest available number, and assigns it the same
// file that the provided fd points to. The refcount of the underlying file is
// incremented.
int32_t proc_dup(uint32_t fd);

uint32_t proc_lsdir(char const *dir, dirent_t *dirents, regsize_t size);

regsize_t proc_pgalloc();
regsize_t proc_pgfree(void *page);

regsize_t proc_getpid();
regsize_t proc_pipe(uint32_t *fds);
regsize_t proc_sysinfo();
regsize_t proc_gpio(uint32_t pin_num, uint32_t enable, uint32_t value);
regsize_t proc_restart();

uint32_t proc_detach();
int32_t proc_isopen(int32_t fd);
uint32_t proc_pipeattch(uint32_t pid, int32_t src_fd);

// fd_alloc allocates a file descriptor in process's open files list and
// assigns a file pointer to it. proc.lock must be held.
int32_t fd_alloc(process_t *proc, file_t *f);
void fd_free(process_t *proc, int32_t fd);

// find_proc_by_pid finds a process by a given pid. Returns NULL if nothing is
// found. Must be called with proc_table.lock held.
process_t* find_proc_by_pid(uint32_t pid);

regsize_t reoffset_user_stack(process_t *dest, process_t *src, int reg);
void inject_argv(process_t *proc, int argc, char const *argv[]);

#endif // ifndef _PROC_H_
