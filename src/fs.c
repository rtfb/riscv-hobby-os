#include "fs.h"
#include "bakedinfs.h"

void fs_init() {
    bifs_init();
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

int32_t fs_read(file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size) {
    return f->read((bifs_file_t*)f->fs_file, pos, buf, count, elem_size);
}

int32_t fs_write(file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size) {
    return f->write((bifs_file_t*)f->fs_file, pos, buf, count, elem_size);
}
