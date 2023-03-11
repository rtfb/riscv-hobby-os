#include "shell.h"
#include "ustr.h"
#include "pagealloc.h"

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

cmdbuf_t _userland sh_init_cmd_slots(cmd_t *slots, int count) {
    cmdbuf_t buf;
    buf.pool = slots;
    buf.count = count;
    for (int i = 0; i < count; i++) {
        slots->buf[0] = 0;
        // slots->cmd = 0;
        slots->args[0] = 0;
        slots->next = 0;
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

void _userland run_program(char *name, char *argv[]) {
    uint32_t pid = fork();
    if (pid == -1) {
        prints("ERROR: fork!\n");
    } else if (pid == 0) { // child
        uint32_t code = execv(name, (char const**)argv);
        // normally exec doesn't return, but if it did, it's an error:
        prints("ERROR: execv\n");
        exit();
    } else { // parent
        wait();
    }
}

// run_hanger is like the run_program, but it's intended to run a malicious
// program that hangs intentionally, so it doesn't wait() on it, only sleeps
// for a bit to allow it to run briefly.
void _userland run_hanger() {
    uint32_t pid = fork();
    if (pid == -1) {
        prints("ERROR: fork!\n");
    } else if (pid == 0) { // child
        uint32_t code = execv("hang", 0);
        // normally exec doesn't return, but if it did, it's an error:
        prints("ERROR: execv\n");
        exit();
    } else { // parent
        sleep(1);
    }
}

// parse_command iterates over buf, replacing each whitespace with a zero, thus
// making each word a terminated string. It then collects all those strings
// into argv, terminating it with a null pointer as well.
void _userland parse_command(char *buf, char *argv[], int argvsize) {
    int i = 0;
    while (*buf == ' ') buf++; // skip any leading whitespaces
    argv[0] = buf;
    while (*buf) {
        buf++;
        if (*buf == ' ') {
            while (*buf == ' ') {
                *buf = 0;
                buf++;
            }
            i++;
            argv[i] = buf;
        }
        if (i > argvsize - 1) {
            break;
        }
    }
    i++;
    argv[i] = 0;
}

char prog_name_hanger[] _user_rodata = "hang";

int _userland run_shell_script(char const *filepath) {
    uint32_t fd = open(filepath, 0);
    if (fd == -1) {
        prints("ERROR: open=-1\n");
        return -1;
    }
    char fbuf[64];
    int32_t status = read(fd, fbuf, 64);
    if (status == -1) {
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
        parse_command(pbuf, parsed_args, 8);
        if (ustrncmp(parsed_args[0], prog_name_hanger, ARRAY_LENGTH(prog_name_hanger)) == 0) {
            run_hanger();
        } else {
            run_program(parsed_args[0], parsed_args);
        }
    }
    return 0;
}

// parse_command2 iterates over buf, replacing each whitespace with a zero, thus
// making each word a terminated string. It then collects all those strings
// into argv, terminating it with a null pointer as well.
char const* _userland parse_command2(char const *input, cmd_t *dst) {
    int argc = 0;
    while (*input == ' ') input++; // skip any leading whitespaces
    char *buf = dst->buf;
    dst->args[0] = buf;
    // dst->cmd = buf;
    while (*input != 0 && *input != '|') {
        // printf("*input=%c\n", *input);
        *buf = *input;
        // printf("*buf=%c\n", *buf);
        input++;
        if (*buf == ' ') {
            *buf = 0;
            buf++;
            while (*input == ' ') {
                input++;
            }
            argc++;
            // printf("args[%d] = %p\n", argc, buf);
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
        char const *stopped_at = parse_command2(input, tail);
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

void _userland traverse(cmd_t *chain) {
    cmd_t *node = chain;
    uint32_t pipefd[2] = {-1, -1};
    uint32_t pipefd_prev[2] = {-1, -1};
    uint32_t left_pipe_r = -1;   // reading end of the pipe on the left
    while (1) {
        if (node->next) {
            if (pipe(pipefd) == -1) {
                prints("ERROR: pipe=-1\n");
                break;
            }
        }
        uint32_t pid = fork();
        if (pid == -1) {
            prints("ERROR: fork!\n");
        } else if (pid == 0) { // child
            if (left_pipe_r != -1) {
                close(0);           // close stdin
                dup(left_pipe_r);   // now replace stdin with the pipe's reading end
            }
            if (node->next) {
                // close(pipefd[0]);   // deref the read end
                close(1);           // close stdout
                dup(pipefd[1]);     // now replace stdout with the pipe's writing end
            }
            if (pipefd[0] != -1) {
                close(pipefd[0]);   // deref the read end
            }
            if (pipefd[1] != -1) {
                close(pipefd[1]);   // deref the write end
            }
            uint32_t code = execv(node->args[0], node->args);
            // normally exec doesn't return, but if it did, it's an error:
            prints("ERROR: execv\n");
            exit();
        }

        // deref the copies held by the shell:
        if (pipefd_prev[0] != -1) {
            close(pipefd_prev[0]);
        }
        if (pipefd_prev[1] != -1) {
            close(pipefd_prev[1]);
        }

        if (!node->next) {
            break;
        }
        left_pipe_r = pipefd[0];
        pipefd_prev[0] = pipefd[0];
        pipefd_prev[1] = pipefd[1];
        node = node->next;
    }
    wait(); // TODO: wait for N children
}

char prog_name_fmt[] _user_rodata = "fmt";

int _userland u_main_shell(int argc, char* argv[]) {
    if (argc > 1) {
        int code = run_shell_script(argv[1]);
        exit(code);
        return code;
    }
    prints("\nInit userland!\n");

    cmd_t *cmd_slots = (cmd_t*)pgalloc();
    int num_slots = PAGE_SIZE / sizeof(cmd_t);
    cmdbuf_t cmdpool = sh_init_cmd_slots(cmd_slots, num_slots);

    char buf[32];
    // char *parsed_args[8];
    for (;;) {
        buf[0] = 0;
        prints("> ");
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
            traverse(cmd_chain);
            /*
            parse_command(buf, parsed_args, 8);
            if (ustrncmp(parsed_args[0], prog_name_hanger, ARRAY_LENGTH(prog_name_hanger)) == 0) {
                run_hanger();
            } else {
                run_program(parsed_args[0], parsed_args);
            }
            if (ustrncmp(parsed_args[0], prog_name_fmt, ARRAY_LENGTH(prog_name_fmt)) == 0) {
                sleep(2000);
            }
            */
        }
    }
}
