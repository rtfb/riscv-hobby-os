#include "pipe.h"
#include "pagealloc.h"

#define PIPE_BUF_SIZE    (PAGE_SIZE / 4)

pipes_t pipes;

void init_pipes() {
    pipes.lock = 0;
    pipes.buf_page = allocate_page();
    if (!pipes.buf_page) {
        // TODO: panic
        return;
    }
    for (int i = 0; i < MAX_PIPES; i++) {
        pipe_t *p = &pipes.all[i];
        p->flags = 0;
        // p->read = pipe_read;
        // p->write = pipe_write;
        p->buf = pipes.buf_page + PIPE_BUF_SIZE * i;
    }
}

pipe_t* alloc_pipe() {
    acquire(&pipes.lock);
    for (int i = 0; i < MAX_PIPES; i++) {
        pipe_t *pp = &pipes.all[i];
        if (!pp->flags) {
            pp->flags |= PIPE_FLAG_ALLOCATED;
            acquire(&pp->lock);
            release(&pipes.lock);
            return pp;
        }
    }
    release(&pipes.lock);
    return 0;
}

// free_pipe marks a pipe object as unoccupied. pipe->lock must be held.
void free_pipe(pipe_t *pipe) {
    pipe->flags = 0;
}

int32_t pipe_open(uint32_t pipefd[2]) {
    pipe_t *pipe = alloc_pipe();
    if (!pipe) {
        // TODO: set errno
        return -1;
    }

    pipe->wpid = -1;
    pipe->rpid = -1;

    pipe->wpos = 0;
    pipe->rpos = 0;

    file_t *f0 = fs_alloc_file();
    file_t *f1 = fs_alloc_file();
    if (f0 == 0 || f1 == 0) {
        // TODO: set errno to indicate out of file objects
        release(&pipe->lock);
        return -1;
    }
    f0->flags = FFLAGS_PIPE | FFLAGS_READABLE;
    f0->fs_file = pipe;
    f0->read = pipe_read;
    f1->flags = FFLAGS_PIPE | FFLAGS_WRITABLE;
    f1->fs_file = pipe;
    f1->write = pipe_write;
    pipe->rf = f0;
    pipe->wf = f1;

    process_t* proc = myproc();
    acquire(&proc->lock);
    int32_t fd0 = fd_alloc(proc, f0);
    int32_t fd1 = fd_alloc(proc, f1);
    if (fd0 == -1 || fd1 == -1) {
        // TODO: set errno to indicate out of proc FDs
        release(&pipe->lock);
        release(&proc->lock);
        return -1;
    }
    release(&proc->lock);
    release(&pipe->lock);
    pipefd[0] = fd0;
    pipefd[1] = fd1;
    return 0;
}

int32_t pipe_close_file(file_t *file) {
    pipe_t *pipe = (pipe_t*)file->fs_file;
    acquire(&pipe->lock);
    if (file->flags & FFLAGS_READABLE) {
        // the reading end is being closed: we can clean up the entire pipe,
        // since no one will be reading, there's no point in writing
        pipe->rpid = -1;
        if (pipe->wpid != -1) {
            proc_mark_for_wakeup(pipe->wpid);
        }
        pipe->wf->fs_file = 0;
        free_pipe(pipe);
    } else if (file->flags & FFLAGS_WRITABLE) {
        // the writing end is being closed: mark it as such, but leave the pipe
        // alive for the reader to finish reading
        pipe->wpid = -1;
        pipe->flags |= PIPE_FLAG_WRITE_CLOSED;
        if (pipe->rpid != -1) {
            release(&pipe->lock);
            proc_mark_for_wakeup(pipe->rpid);
            return 0;
        }
    }
    release(&pipe->lock);
    return 0;
}

int32_t pipe_read(file_t *f, uint32_t pos, void *buf, uint32_t size) {
    // 'pos' parameter is ignored by pipe_read
    process_t* proc = myproc();
    pipe_t *pipe = (pipe_t*)f->fs_file;
    acquire(&pipe->lock);
    pipe->rpid = proc->pid;
    // nothing to read, so we have to either sleep, waiting for the writing end
    // to write something, or return EOF if the writing end is already closed
    if (pipe->rpos >= pipe->wpos && ((pipe->flags & PIPE_BUF_FULL) == 0)) {
        if (pipe->flags & PIPE_FLAG_WRITE_CLOSED) {
            release(&pipe->lock);
            return EOF;
        }
        release(&pipe->lock); // release the lock before sleep, otherwise the writing end will deadlock
        if (pipe->wpid != -1) {
            // it's possible the writing has filled the buffer and fell asleep.
            // So let it know it now has some room for writing.
            proc_mark_for_wakeup(pipe->wpid);
        }
        proc_yield();
        acquire(&pipe->lock); // reacquire the lock after wakeup
    }
    uint8_t *rbuf = pipe->buf + pipe->rpos;
    uint8_t *wbuf = (uint8_t*)buf;
    int32_t nread = 0;
    while (size > 0) {
        *wbuf++ = *rbuf++;
        pipe->rpos++;
        size--;
        nread++;
        if (pipe->rpos == PIPE_BUF_SIZE) {
            pipe->rpos = 0;
        }
        if (pipe->rpos == pipe->wpos) {
            break;
        }
    }
    if (nread > 0) {
        pipe->flags &= ~PIPE_BUF_FULL;
    }
    release(&pipe->lock);
    return nread;
}

// pipe_do_write writes as much as possible to pipe's write buffer, returns
// number of bytes written.
int32_t pipe_do_write(pipe_t *pipe, void *buf, uint32_t nbytes) {
    uint8_t *wbuf = pipe->buf + pipe->wpos;
    uint8_t *rbuf = (uint8_t*)buf;
    int32_t nwritten = 0;
    while (nbytes > 0) {
        *wbuf++ = *rbuf++;
        pipe->wpos++;
        nbytes--;
        nwritten++;
        if (pipe->wpos == PIPE_BUF_SIZE) {
            pipe->wpos = 0;
        }
        if (pipe->wpos == pipe->rpos) {
            pipe->flags |= PIPE_BUF_FULL;
            break;
        }
    }
    return nwritten;
}

int32_t pipe_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes) {
    // 'pos' parameter is ignored by pipe_write
    pipe_t *pipe = (pipe_t*)f->fs_file;
    if (!pipe) {
        // TODO: set errno to 'broken pipe'
        return -1;
    }
    acquire(&pipe->lock);
    process_t* proc = myproc();
    pipe->wpid = proc->pid;
    while (1) {
        // we have (at most) two chunks available for writing: wpos til the end of
        // buf, and beginning of buf til rpos:
        int32_t available = PIPE_BUF_SIZE - pipe->wpos + pipe->rpos;
        if (pipe->flags & PIPE_BUF_FULL) {
            available = 0;
        }
        if (available < 0) {
            // TODO: set errno or even panic
            release(&pipe->lock);
            return -1;
        }
        if (available > 0) {
            int32_t nwritten = pipe_do_write(pipe, buf, nbytes);
            release(&pipe->lock);
            return nwritten;
        }
        // otherwise, sleep:
        if (pipe->rpid != -1) {
            // it's possible the reading end has already tried reading and it
            // fell asleep. So let it know it has something to read now.
            proc_mark_for_wakeup(pipe->rpid);
        }
        release(&pipe->lock); // release the lock before sleep, otherwise the reading end will deadlock
        proc_yield();
        if (!f->fs_file) { // the pipe was closed while we slept
            // TODO: set errno to 'broken pipe'
            return -1;
        }
        acquire(&pipe->lock); // reacquire the lock after wakeup
    }
    // TODO: should never reach this. Panic, for clarity?
    release(&pipe->lock);
    return 0;
}
