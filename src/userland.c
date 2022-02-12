#include "sys.h"
#include "userland.h"
#include "string.h"
#include "syscalls.h"

int _userland ustrncmp(char const *a, char const *b, unsigned int num) {
    unsigned int i = 0;
    do {
        if (!*a && !*b) {
            return 0;
        }
        if (!*a) {
            return -1;
        }
        if (!*b) {
            return 1;
        }
        if (*a != *b) {
            break;
        }
        i++;
        a++;
        b++;
    } while(i < num);
    return *a - *b;
}

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
        sys_puts("ERROR: fork!\n");
    } else if (pid == 0) { // child
        uint32_t code = execv(name, (char const**)argv);
        // normally exec doesn't return, but if it did, it's an error:
        sys_puts("ERROR: execv\n");
        exit();
    } else { // parent
        wait();
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

char prog_name_fmt[] _user_rodata = "fmt";

int _userland u_main_shell(int argc, char* argv[]) {
    sys_puts("\nInit userland!\n");
    char buf[16];
    char *parsed_args[8];
    for (;;) {
        buf[0] = 0;
        sys_puts("> ");
        int32_t nread = read(1, buf, 15);
        if (nread < 0) {
            sys_puts("ERROR: read\n");
        } else {
            buf[nread] = 0;
            trimright(buf);
            if (*buf == 0) {
                continue;
            }
            parse_command(buf, parsed_args, 8);
            run_program(parsed_args[0], parsed_args);
            if (ustrncmp(parsed_args[0], prog_name_fmt, ARRAY_LENGTH(prog_name_fmt)) == 0) {
                sleep(2000);
            }
        }
    }
}

int _userland u_main_hello1() {
    sys_puts("Hello from hellosayer 1\n");
    exit();
    return 0;
}

char dash_h[] _user_rodata = "-h";

int _userland u_main_hello2(int argc, char const* argv[]) {
    if (argc > 1 && !ustrncmp(argv[1], dash_h, 2)) {
        sys_puts("A hidden greeting!\n");
    } else {
        sys_puts("Very welcome from hellosayer 2\n");
    }
    exit();
    return 0;
}

char fmt[] _user_rodata = "formatted string: num=%d, zero=%d, char=%c, hex=0x%x, str=%s\n";
char fmt2[] _user_rodata = "only groks 7 args: %d %d %d %d %d %d %d %d %d\n";

int _userland u_main_fmt() {
    char foo[] = "foo";
    printf(fmt, 387, 0, 'X', 0xaddbeef, foo);
    printf(fmt2, 11, 12, 13, 14, 15, 16, 17, 18, 19);
    exit();
    return 0;
}

char sysinfo_fmt[] _user_rodata = "Total RAM: %d\nFree RAM: %d\nNum procs: %d\n";
char unclaimed_mem_fmt[] _user_rodata = "Unclaimed mem: 0x%x-0x%x (%d bytes)\n";
char dash_f[] _user_rodata = "-f";

int _userland u_main_sysinfo(int argc, char const* argv[]) {
    sysinfo_t info;
    sysinfo(&info);
    printf(sysinfo_fmt, info.totalram, info.freeram, info.procs);
    if (argc > 1 && !ustrncmp(argv[1], dash_f, 2)) {
        printf(unclaimed_mem_fmt, info.unclaimed_start, info.unclaimed_end,
               info.unclaimed_end - info.unclaimed_start);
    }
    exit();
    return 0;
}

int _userland u_main_smoke_test() {
    sys_puts("\nInit userland smoke test!\n");
    run_program("sysinfo", 0);
    run_program("fmt", 0);
    for (;;)
        ;
}
