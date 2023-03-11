#ifndef _SHELL_H_
#define _SHELL_H_

#include "userland.h"

#define NUM_CMD_SLOTS         4

#define ARGV_BUF_SIZE         4
#define CMD_BUF_SIZE         32

typedef struct cmd {
    // contains the bytes for cmd and args, in the form: "wc\0-c\0",
    // "cat\0/path/to/file\0"
    char buf[CMD_BUF_SIZE];

    // args contains the pointers into buf. args[0]=cmd, args[1]=arg1, etc
    char const *args[ARGV_BUF_SIZE];

    struct cmd *next;
} cmd_t;

typedef struct cmdbuf {
    cmd_t   *pool;
    int     count;
} cmdbuf_t;

void _userland run_program(char *name, char *argv[]);

cmdbuf_t _userland sh_init_cmd_slots(cmd_t *slots, int count);
cmd_t* _userland sh_alloc_cmd(cmdbuf_t *pool);

#endif // ifndef _SHELL_H_
