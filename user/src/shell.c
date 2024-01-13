#include "shell.h"
#include "ustr.h"

char prog_name_hanger[] _user_rodata = "hang";

cmd_t* _userland parse(cmdbuf_t *pool, char const *input);
uint32_t _userland traverse(cmd_t *node, int depth);

// trimright finds the end of the provided string and trims any trailing
// whitespace characters (\n,\r,\t,' ') by overwriting them with 0. Returns the
// new length of the string.
int _userland trimright(char *str) {
    int i = 0;
    while (str[i] != 0) i++;
    i--;
    while (i >= 0) {
        char ch = str[i];
        if (ch != '\r' && ch != '\n' && ch != '\t' && ch != ' ')
            break;
        str[i] = 0;
        i--;
    }
    return i + 1;
}

void _userland sh_init_cmd(cmd_t *cmd) {
    cmd->buf[0] = 0;
    for (int i = 0; i < ARGV_BUF_SIZE; i++) {
        cmd->args[i] = 0;
    }
    cmd->next = 0;
}

cmdbuf_t _userland sh_init_cmd_slots(cmd_t *slots, int count) {
    cmdbuf_t buf;
    buf.pool = slots;
    buf.count = count;
    for (int i = 0; i < count; i++) {
        sh_init_cmd(slots);
        slots++;
    }
    return buf;
}

cmd_t* _userland sh_alloc_cmd(cmdbuf_t *pool) {
    if (pool->count <= 0) {
        return 0;
    }
    pool->count--;
    return &pool->pool[pool->count];
}

void _userland sh_free_cmd_chain(cmdbuf_t *pool, cmd_t *chain) {
    while (chain) {
        pool->count++;
        cmd_t *next = chain->next;
        sh_init_cmd(chain);
        chain = next;
    }
}

// wait for as many children as traverse() has spawned, except for the hangers.
void _userland sh_wait_for_all_children(cmd_t *cmd_chain) {
    for (cmd_t *node = cmd_chain; node != 0; node = node->next) {
        if (ustrncmp(node->buf, prog_name_hanger, ARRAY_LENGTH(prog_name_hanger)) == 0) {
            sleep(1);
            continue; // don't wait for a hanger
        }
        wait();
    }
}

int _userland run_shell_script(char const *filepath, cmdbuf_t cmdpool) {
    uint32_t fd = open(filepath, 0);
    if (fd == -1) {
        prints("ERROR: open=-1\n");
        return -1;
    }
    char *fbuf = (char*)pgalloc();
    int32_t status = read(fd, fbuf, PAGE_SIZE);
    if (status == -1) {
        pgfree(fbuf);
        prints("ERROR: read=-1\n");
        return -1;
    }
    close(fd);
    fbuf[status] = 0;
    int start = 0;
    int end = 0;
    char pbuf[32];
    char *parsed_args[8];
    while (fbuf[end] != 0) {
        pbuf[0] = 0;
        int i = 0;
        while (fbuf[end] != 0 && fbuf[end] != '\n') {
            pbuf[i++] = fbuf[end++];
        }
        while (fbuf[end] != 0 && fbuf[end] == '\n') {
            end++;
        }
        start = end + 1;
        pbuf[i] = 0;
        if (!*pbuf) {
            break;
        }
        cmd_t *cmd_chain = parse(&cmdpool, pbuf);
        traverse(cmd_chain, 0);
        sh_wait_for_all_children(cmd_chain);
        sh_free_cmd_chain(&cmdpool, cmd_chain);
    }
    pgfree(fbuf);
    return 0;
}

// parse_command iterates over buf, replacing each whitespace with a zero, thus
// making each word a terminated string. It then collects all those strings
// into argv, terminating it with a null pointer as well.
char const* _userland parse_command(char const *input, cmd_t *dst) {
    int argc = 0;
    while (*input == ' ') input++; // skip any leading whitespaces
    char *buf = dst->buf;
    dst->args[0] = buf;
    while (*input != 0 && *input != '|') {
        *buf = *input;
        input++;
        if (*buf == ' ') {
            *buf = 0;
            buf++;
            while (*input == ' ') {
                input++;
            }
            argc++;
            dst->args[argc] = buf;
        } else {
            buf++;
        }
        if (argc > ARGV_BUF_SIZE - 1) {
            break;
        }
    }
    *buf = 0;
    if (dst->args[argc][0] != '\0') {
        argc++;
    }
    dst->args[argc] = 0;
    return input;
}

cmd_t* _userland parse(cmdbuf_t *pool, char const *input) {
    cmd_t *tail = sh_alloc_cmd(pool);
    if (!tail) {
        return 0;
    }
    cmd_t *head = tail;
    while (1) {
        char const *stopped_at = parse_command(input, tail);
        if (*stopped_at == '\0') {
            break;
        }
        input = stopped_at + 1;
        cmd_t *next = sh_alloc_cmd(pool);
        if (!next) {
            return head;
        }
        tail->next = next;
        tail = next;
    }
    return head;
}

char sh_exec_err_fmt[] _user_rodata = "ERROR: execv %s, %d\n";

uint32_t _userland traverse(cmd_t *node, int depth) {
    if (!node) {
        return -1;
    }
    uint32_t right_pipe_w = traverse(node->next, depth + 1);

    uint32_t pipefd[2] = {-1, -1};
    if (depth > 0) {
        if (pipe(pipefd) == -1) {
            prints("ERROR: pipe=-1\n");
            return -1;
        }
    }
    uint32_t pid = fork();
    if (pid == -1) {
        prints("ERROR: fork!\n");
    } else if (pid == 0) { // child
        if (pipefd[0] != -1) {
            close(0);           // close stdin
            dup(pipefd[0]);     // now replace stdin with the pipe's reading end
            close(pipefd[0]);   // close the extra copy to keep refcount right
        }
        if (pipefd[1] != -1) {
            // the shell only closes the reading end of a newly allocated pipe,
            // in order to maintain a copy of the writing end before it can be
            // assigned to the next process. However, fork() makes an extra
            // refcount++ on that writing end, so we need to deref it:
            close(pipefd[1]);
        }
        if (right_pipe_w != -1) {
            close(1);             // close stdout
            dup(right_pipe_w);    // now replace stdout with the pipe's writing end
            close(right_pipe_w);  // close the extra copy to keep refcount right
        }
        uint32_t code = execv(node->args[0], node->args);
        // normally exec doesn't return, but if it did, it's an error:
        printf(sh_exec_err_fmt, node->args[0], code);
        exit(code);
    }

    // deref the copies held by the shell:
    if (pipefd[0] != -1) {
        close(pipefd[0]);
    }
    if (right_pipe_w != -1) {
        close(right_pipe_w);
    }
    return pipefd[1];
}

int _userland u_main_shell(int argc, char* argv[]) {
    cmd_t *cmd_slots = (cmd_t*)pgalloc();
    int num_slots = PAGE_SIZE / sizeof(cmd_t);
    cmdbuf_t cmdpool = sh_init_cmd_slots(cmd_slots, num_slots);

    if (argc > 1) {
        int code = run_shell_script(argv[1], cmdpool);
        exit(code);
        return code;
    }
    prints("Welcome to sHELL!\n");

    char buf[32];
    for (;;) {
        buf[0] = 0;
        prints(">");
        int32_t nread = read(0, buf, 31);
        if (nread < 0) {
            prints("ERROR: read\n");
        } else {
            buf[nread] = 0;
            trimright(buf);
            if (*buf == 0) {
                continue;
            }
            cmd_t *cmd_chain = parse(&cmdpool, buf);
            traverse(cmd_chain, 0);
            sh_wait_for_all_children(cmd_chain);
            sh_free_cmd_chain(&cmdpool, cmd_chain);
        }
    }
    // Note: we should pgfree(cmd_slots) here, but shell never exits, so that
    // would be just unreachable code.
}
