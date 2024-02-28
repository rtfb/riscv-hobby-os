#include "errno.h"
#include "kernel.h"
#include "pagealloc.h"
#include "pipe.h"
#include "vm.h"

#define PIPE_BUF_SIZE    (PAGE_SIZE / 4)

pipes_t pipes;

void init_pipes() {
    pipes.lock = 0;
    pipes.buf_page = kalloc("init_pipes", -1);
    if (!pipes.buf_page) {
        panic("init pipes alloc");
        return;
    }
    for (int i = 0; i < MAX_PIPES; i++) {
        pipe_t *p = &pipes.all[i];
        p->flags = 0;
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
    process_t* proc = myproc();
    pipe_t *pipe = alloc_pipe(); // acquires pipe->lock
    if (!pipe) {
        *proc->perrno = ENOMEM;
        return -1;
    }

    pipe->wpos = 0;
    pipe->rpos = 0;

    file_t *f0 = fs_alloc_file();
    file_t *f1 = fs_alloc_file();
    if (f0 == 0 || f1 == 0) {
        *proc->perrno = ENFILE;
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

    int32_t fd0 = fd_alloc(proc, f0);
    int32_t fd1 = fd_alloc(proc, f1);
    if (fd0 == -1 || fd1 == -1) {
        *proc->perrno = EMFILE;
        release(&pipe->lock);
        return -1;
    }
    release(&pipe->lock);
    uint32_t *pipefd_phys = va2pa(proc->upagetable, pipefd);
    pipefd_phys[0] = fd0;
    pipefd_phys[1] = fd1;
    return 0;
}

int32_t pipe_close_file(file_t *file) {
    pipe_t *pipe = (pipe_t*)file->fs_file;
    if (!pipe) {
        return 0;
    }
    acquire(&pipe->lock);
    if (file->flags & FFLAGS_READABLE) {
        // the reading end is being closed
        proc_mark_for_wakeup(pipe);
        if (file->refcount == 0) {
            // that was the last ref, so we can clean up the entire pipe:
            // since no one will be reading, there's no point in writing
            pipe->wf->fs_file = 0;
            free_pipe(pipe);
        }
    } else if (file->flags & FFLAGS_WRITABLE) {
        // the writing end is being closed: mark it as such, but leave the pipe
        // alive for the reader to finish reading
        if (file->refcount == 0) {
            pipe->flags |= PIPE_FLAG_WRITE_CLOSED;
        }
        proc_mark_for_wakeup(pipe);
    }
    release(&pipe->lock);
    return 0;
}

int32_t pipe_read(file_t *f, uint32_t pos, void *buf, uint32_t size) {
    // 'pos' parameter is ignored by pipe_read
    process_t* proc = myproc();
    pipe_t *pipe = (pipe_t*)f->fs_file;
    acquire(&pipe->lock);
    // nothing to read, so we have to either sleep, waiting for the writing end
    // to write something, or return EOF if the writing end is already closed.
    // This can happen after a wakeup, so keep doing this in a loop.
    while (pipe->rpos == pipe->wpos && ((pipe->flags & PIPE_BUF_FULL) == 0)) {
        if (pipe->flags & PIPE_FLAG_WRITE_CLOSED) {
            release(&pipe->lock);
            return 0;
        }
        // it's possible the writing process has filled the buffer and fell
        // asleep. So let it know it now has some room for writing.
        proc_mark_for_wakeup(pipe);
        release(&pipe->lock); // release the lock before sleep, otherwise the writing end will deadlock
        proc_yield(pipe);
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
        return -EPIPE;
    }
    acquire(&pipe->lock);
    process_t* proc = myproc();
    int32_t nwritten = 0;
    while (1) {
        // we have (at most) two chunks available for writing: wpos til the end of
        // buf, and beginning of buf til rpos:
        int32_t available = PIPE_BUF_SIZE - pipe->wpos + pipe->rpos;
        if (pipe->flags & PIPE_BUF_FULL) {
            available = 0;
        }
        if (available < 0) {
            release(&pipe->lock);
            return -ENOBUFS;
        }
        int32_t wr = pipe_do_write(pipe, buf+nwritten, nbytes - nwritten);
        if (wr > 0) {
            // if at least one byte was written, let the reading end know that
            // it can wake up and try reading
            proc_mark_for_wakeup(pipe);
        }
        nwritten += wr;
        if (nwritten == nbytes) {
            // If we were able to write everything, report a complete
            // successful write to the caller:
            release(&pipe->lock);
            return nwritten;
        }
        // Otherwise, block on a (maybe partial) write.
        release(&pipe->lock); // release the lock before sleep, otherwise the reading end will deadlock
        proc_yield(pipe);
        if (!f->fs_file) { // the pipe was closed while we slept
            return -EPIPE;
        }
        acquire(&pipe->lock); // reacquire the lock after wakeup
    }
    // TODO: should never reach this. Panic, for clarity?
    release(&pipe->lock);
    return 0;
}
