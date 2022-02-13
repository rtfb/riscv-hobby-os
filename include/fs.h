#ifndef _FS_H_
#define _FS_H_

#include "sys.h"

#define MAX_PROC_FDS 8
#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

#define FDMODE_READ (1 << 0)
#define FDMODE_WRITE (1 << 1)

#define FILE_FLAGS_READABLE  (1 << 0)
#define FILE_FLAGS_WRITABLE  (1 << 1)
#define FILE_FLAGS_DIR       (1 << 2)
#define FILE_FLAGS_UART_STREAM (1 << 8)
#define FILE_FLAGS_BIFS_FILE   (1 << 9)

typedef struct file_s {
    uint32_t flags; // FILE_FLAGS_* bit flags
    void     *fs_file; // a pointer to the underlying FS-level file struct
    int32_t (*read)(void *fs_file, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);
    int32_t (*write)(void *fs_file, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);
} file_t;

typedef struct fd_s {
    file_t file;
    uint32_t position;
    uint32_t mode;
} fd_t;

void fs_init();
int32_t fs_open(file_t *f, char const *filepath, uint32_t flags);
int32_t fs_read(file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);
int32_t fs_write(file_t *f, uint32_t pos, void *buf, uint32_t count, uint32_t elem_size);

#endif // ifndef _FS_H_
