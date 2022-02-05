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
    while (i > 0) {
        char ch = str[i];
        if (ch != '\r' && ch != '\n' && ch != '\t' && ch != ' ')
            break;
        str[i] = 0;
        i--;
    }
    return i;
}

void _userland run_program(char *name) {
    uint32_t pid = fork();
    if (pid == -1) {
        sys_puts("ERROR: fork!\n");
    } else if (pid == 0) { // child
        uint32_t code = execv(name, 0);
        // normally exec doesn't return, but if it did, it's an error:
        sys_puts("ERROR: execv\n");
        exit();
    } else { // parent
        wait();
    }
}

char unknown_cmd_fmt[] _user_rodata = "unknown command: %s\n";

int _userland u_main_shell(int argc, char* argv[]) {
    sys_puts("\nInit userland!\n");
    char h1[] = "hello1";
    char h2[] = "hello2";
    char si[] = "sysinfo";
    char fmt[] = "fmt";
    char buf[16];
    for (;;) {
        buf[0] = 0;
        sys_puts("> ");
        int32_t nread = read(1, buf, 15);
        if (nread < 0) {
            sys_puts("ERROR: read\n");
        } else {
            buf[nread] = 0;
            trimright(buf);
            if (ustrncmp(buf, h1, ARRAY_LENGTH(h1)) == 0) {
                run_program("hello1");
            } else if (ustrncmp(buf, h2, ARRAY_LENGTH(h2)) == 0) {
                run_program("hello2");
            } else if (ustrncmp(buf, si, ARRAY_LENGTH(h2)) == 0) {
                run_program("sysinfo");
            } else if (ustrncmp(buf, fmt, ARRAY_LENGTH(h2)) == 0) {
                run_program("fmt");
            } else {
                printf(unknown_cmd_fmt, buf);
            }
        }
    }
}

int _userland u_main_hello1() {
    sys_puts("Hello from hellosayer 1\n");
    exit();
    return 0;
}

int _userland u_main_hello2() {
    sys_puts("Very welcome from hellosayer 2\n");
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

int _userland u_main_sysinfo() {
    sysinfo_t info;
    sysinfo(&info);
    printf(sysinfo_fmt, info.totalram, info.freeram, info.procs);
    exit();
    return 0;
}
