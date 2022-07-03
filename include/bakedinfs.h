#ifndef _BAKEDINFS_H_
#define _BAKEDINFS_H_

#include "sys.h"
#include "fs.h"

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
int32_t bifs_read(file_t *f, uint32_t pos, void *buf, uint32_t size);
int32_t bifs_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes);
bifs_file_t* bifs_open(char const *filepath, uint32_t flags);

bifs_directory_t* bifs_opendir(bifs_directory_t *parent, char const *name, int start, int end);
bifs_file_t* bifs_openfile(bifs_directory_t *parent, char const *name, int start, int end);

// next_slash starts looking at path[pos] and keeps going until it finds a slash
// symbol ('/'), it then returns its index. If no slash is present, pos will
// point to the terminating zero.
int next_slash(char const *path, int pos);

#endif // ifndef _BAKEDINFS_H_
