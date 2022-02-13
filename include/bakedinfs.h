#ifndef _BAKEDINFS_H_
#define _BAKEDINFS_H_

#include "sys.h"

#define BIFS_MAX_FILES 32

#define BIFS_READABLE (1 << 0)
#define BIFS_WRITABLE (1 << 1)
#define BIFS_RAW      (1 << 2) // bifs_file_t.data should be read as is
#define BIFS_BASE64   (1 << 3) // bifs_file_t.data is base64-encoded and should be decoded as part of reading

typedef struct bifs_directory_s {
    uint32_t flags; // BIFS_*
    char *name;
    struct bifs_directory_s *parent;
} bifs_directory_t;

typedef struct bifs_file_s {
    uint32_t flags; // BIFS_*
    char *name;
    bifs_directory_t *parent;
    char *data;
} bifs_file_t;

extern bifs_directory_t *bifs_root;
extern bifs_directory_t bifs_all_directories[BIFS_MAX_FILES];
extern bifs_file_t      bifs_all_files[BIFS_MAX_FILES];

void bifs_init();
bifs_directory_t* bifs_opendir(char const *name);
bifs_file_t* bifs_openfile(bifs_directory_t *parent, char const *name);
int32_t bifs_read(bifs_file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);
int32_t bifs_write(bifs_file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);

#endif // ifndef _BAKEDINFS_H_
