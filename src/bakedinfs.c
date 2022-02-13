#include "fs.h"
#include "bakedinfs.h"

bifs_directory_t *bifs_root;
bifs_directory_t bifs_all_directories[BIFS_MAX_FILES];
bifs_file_t      bifs_all_files[BIFS_MAX_FILES];

void bifs_init() {
    bifs_root = &bifs_all_directories[0];
    bifs_root->flags = BIFS_READABLE;
    bifs_root->name = "/";
    bifs_root->parent = 0;
    bifs_file_t *f0 = &bifs_all_files[0];
    f0->flags = BIFS_READABLE | BIFS_RAW;
    f0->parent = bifs_root;
    f0->name = "readme.txt";
    f0->data = "Hello, I am File Zero.\n";
}

int32_t bifs_read(bifs_file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size) {
    char *cbuf = (char*)buf;
    // TODO: take pos into account
    int i = 0;
    while (i < count * elem_size) {
        char ch = f->data[i];
        if (!ch) {
            break;
        }
        cbuf[i] = ch;
        i++;
    }
    return i;
}

int32_t bifs_write(bifs_file_t *fs_file, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size) {
    return -1; // TODO: implement
}
