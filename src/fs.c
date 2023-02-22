#include "fs.h"
#include "pipe.h"
#include "bakedinfs.h"

file_table_t ftable;

void fs_init() {
    ftable.lock = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        ftable.files[i].refcount = 0;
    }
    bifs_init();
}

file_t* fs_alloc_file() {
    acquire(&ftable.lock);
    for (int i = 0; i < MAX_FILES; i++) {
        if (ftable.files[i].refcount == 0) {
            ftable.files[i].refcount = 1;
            release(&ftable.lock);
            return &ftable.files[i];
        }
    }
    release(&ftable.lock);
    // TODO: set errno
    return 0;
}

void fs_free_file(file_t *f) {
    f->refcount--;
    if (f->refcount == 0) {
        if (f->flags & FFLAGS_PIPE) {
            pipe_close_file(f);
        }
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
    return f->read(f, pos, buf, size);
}

int32_t fs_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes) {
    return f->write(f, pos, buf, nbytes);
}
