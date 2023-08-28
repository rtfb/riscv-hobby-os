#include "fs.h"
#include "pipe.h"
#include "bakedinfs.h"
#include "drivers/uart/uart.h"

file_table_t ftable;
file_t stdin;
file_t stdout;
file_t stderr;

void fs_init() {
    ftable.lock = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        ftable.files[i].refcount = 0;
    }
    stdin.flags = FFLAGS_READABLE | FFLAGS_UART_STREAM;
    stdin.mode = FDMODE_READ;
    stdin.fs_file = 0;  // XXX: can we leave it NULL?
    stdin.read = uart_read;
    stdin.write = 0;
    stdout.flags = FFLAGS_WRITABLE | FFLAGS_UART_STREAM;
    stdout.mode = FDMODE_WRITE;
    stdout.fs_file = 0;  // XXX: can we leave it NULL?
    stdout.read = 0;
    stdout.write = uart_write;
    stderr.flags = FFLAGS_WRITABLE | FFLAGS_UART_STREAM;
    stderr.mode = FDMODE_WRITE;
    stderr.fs_file = 0;  // XXX: can we leave it NULL?
    stderr.read = 0;
    stderr.write = uart_write;
    bifs_init();
}

file_t* fs_alloc_file() {
    acquire(&ftable.lock);
    for (int i = 0; i < MAX_FILES; i++) {
        file_t *pf = &ftable.files[i];
        if (pf->refcount == 0) {
            pf->refcount = 1;
            pf->position = 0;
            pf->flags = 0;
            release(&ftable.lock);
            return pf;
        }
    }
    release(&ftable.lock);
    // TODO: set errno
    return 0;
}

void fs_free_file(file_t *f) {
    f->refcount--;
    // if (f->flags & FFLAGS_PIPE && f->refcount < 2) {
    if (f->flags & FFLAGS_PIPE) {
        pipe_close_file(f);
    }
}

int32_t fs_open(file_t *f, char const *filepath, uint32_t flags) {
    f->fs_file = (void*)bifs_open(filepath, flags);
    if (!f->fs_file) {
        // TODO: errno = ENOENT
        return -1;
    }
    f->read = bifs_read;
    f->write = bifs_write;
    return 0;
}

int32_t fs_read(file_t *f, uint32_t pos, void *buf, uint32_t size) {
    if (!f->read) {
        // TODO: set errno
        return -1;
    }
    return f->read(f, pos, buf, size);
}

int32_t fs_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes) {
    if (!f->write) {
        // TODO: set errno
        return -1;
    }
    return f->write(f, pos, buf, nbytes);
}
