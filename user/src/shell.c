#include "shell.h"
#include "ustr.h"

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

char prog_name_fmt[] _user_rodata = "fmt";

int _userland u_main_shell(int argc, char* argv[]) {
    if (argc > 1) {
        int code = run_shell_script(argv[1]);
        exit(code);
        return code;
    }
    prints("\nInit userland!\n");
    char buf[32];
    char *parsed_args[8];
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
            parse_command(buf, parsed_args, 8);
            if (ustrncmp(parsed_args[0], prog_name_hanger, ARRAY_LENGTH(prog_name_hanger)) == 0) {
                run_hanger();
            } else {
                run_program(parsed_args[0], parsed_args);
            }
            if (ustrncmp(parsed_args[0], prog_name_fmt, ARRAY_LENGTH(prog_name_fmt)) == 0) {
                sleep(2000);
            }
        }
    }
}
