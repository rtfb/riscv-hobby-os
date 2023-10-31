#ifndef _FS_H_
#define _FS_H_

#include "sys.h"
#include "spinlock.h"

#define MAX_PROC_FDS 8 // number of open files per process
#define MAX_FILES 32 // system-global number of file objects
#define FD_STDIN 0
#define FD_STDOUT 1
#define FD_STDERR 2

#define FDMODE_READ  (1 << 0)
#define FDMODE_WRITE (1 << 1)

#define FFLAGS_READABLE    (1 << 1)
#define FFLAGS_WRITABLE    (1 << 2)
#define FFLAGS_DIR         (1 << 3)
#define FFLAGS_UART_STREAM (1 << 8)
#define FFLAGS_BIFS_FILE   (1 << 9)
#define FFLAGS_PIPE        (1 << 10)

typedef struct file_s {
    // TODO: add lock here and fix all code to lock properly

    // The files are ref-counted, otherwise it's not clear how to do pipes.
    // fork() does refcount++ so that parent/child can do what they want
    // with their copies of file descriptors without affecting each other
    uint32_t refcount;

    uint32_t flags;    // FFLAGS_* bit flags
    uint32_t position;
    uint32_t mode;     // FDMODE_*
    void     *fs_file; // a pointer to the underlying FS-level file struct
    int32_t (*read)(struct file_s *f, uint32_t pos, void *buf, uint32_t size);
    int32_t (*write)(struct file_s *f, uint32_t pos, void *buf, uint32_t nbytes);
} file_t;

typedef struct file_table_s {
    spinlock lock;
    file_t files[MAX_FILES];
} file_table_t;

// defined in fs.c
extern file_table_t ftable;
extern file_t stdin;
extern file_t stdout;
extern file_t stderr;

void fs_init();
file_t* fs_alloc_file();
void fs_free_file(file_t *f);
int32_t fs_open(file_t *f, char const *filepath, uint32_t flags);
int32_t fs_read(file_t *f, uint32_t pos, void *buf, uint32_t size);
int32_t fs_write(file_t *f, uint32_t pos, void *buf, uint32_t nbytes);

#endif // ifndef _FS_H_
