#ifndef _PIPE_H_
#define _PIPE_H_

#include "proc.h"
#include "spinlock.h"
#include "fs.h"

#define MAX_PIPES 4

#define PIPE_FLAG_ALLOCATED     (1 << 0)
#define PIPE_FLAG_WRITE_CLOSED  (1 << 1)
#define PIPE_BUF_FULL           (1 << 2)

// pipe_open takes two pids and creates a pipe between wpid.stdout and
// rpid.stdin. It creates a file-like object pipe_t and replaces wpid.stdout
// with it, and rpid.stdin with the same. When wpid does a write() on stdout,
// it starts filling an internal pipe.buf if it has space remaining. When it
// gets filled, a write blocks by calling proc_yield, which puts the writing
// process to sleep. Same thing happens on the reading end.
typedef struct pipe_s {
    spinlock lock;

    int32_t flags;
    void *buf;          // pipe's buffer

    // backpointers to reading- and writing-end file objects:
    file_t *rf;
    file_t *wf;

    // writing end:
    uint32_t wpid;
    uint32_t wpos;      // writer's position within buf: pos of the next byte to be written

    // reading end:
    uint32_t rpid;
    uint32_t rpos;      // reader's position within buf: pos of the next byte to be read
} pipe_t;

typedef struct pipes_s {
    spinlock lock;
    pipe_t all[MAX_PIPES];
    void *buf_page;
} pipes_t;

// pipe_open allocates a pipe_t and two file descriptors. Both fds will point
// to the same pipe, pipefd[0] being the reading end of the pipe and pipefd[1]
// the writing end.
int32_t pipe_open(uint32_t pipefd[2]);
int32_t pipe_close_file(file_t *file);
void init_pipes();
pipe_t* alloc_pipe();
void free_pipe(pipe_t *pipe);
int32_t pipe_read(file_t *f, uint32_t pos, void *buf, uint32_t size);
int32_t pipe_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes);

#endif // ifndef _PIPE_H_
