#include "sys.h"
#include "userland.h"
#include "string.h"
#include "syscalls.h"

#define PRINT_FREQ ONE_SECOND

uint64_t get_clock_cycles();
int m_read_hart_id();

int _userland u_main_init(int argc, char* argv[]) {
    sys_puts("\nInit userland!\n");
    uint32_t pid = fork();
    if (pid == -1) {
        sys_puts("ERROR!\n");
    } else if (pid == 0) { // child
        uint32_t code = execv("1", 0); // "1" == u_main
        // normally exec doesn't return, but if it did, it's an error:
        sys_puts("ERROR: execv\n");
    } else { // parent
        uint32_t pid = fork();
        if (pid == -1) {
            sys_puts("ERROR!\n");
        } else if (pid == 0) { // child
            uint32_t code = execv("2", 0); // "2" == u_main2
            // normally exec doesn't return, but if it did, it's an error:
            sys_puts("ERROR: execv\n");
        } else { // parent
            exit();
        }
    }
}

int _userland u_main() {
    int counter = 0;
    int flipper = 0;
    int num_flips = 0;
    char pidstr[2];
    uint32_t pid = getpid();
    pidstr[0] = '0' + (char)pid;
    pidstr[1] = 0;
    while (1) {
        counter++;
        if (counter % PRINT_FREQ == 0) {
            flipper++;
            if ((flipper & 1) == 0) {
                sys_puts(pidstr);
            } else {
                sys_puts("|");
                num_flips++;
            }
        }
        if (num_flips == 3) {
            uint32_t pid = fork();
            if (pid == -1) {
                sys_puts("ERROR!\n");
                // don't fork again:
                num_flips++;
            } else if (pid == 0) { // child
                // don't fork again:
                num_flips++;
                // replace the pid to be printed:
                pid = getpid();
                pidstr[0] = '0' + (char)pid;
                uint32_t code = execv("3", 0); // "3" == u_main3
                // normally exec doesn't return, but if it did, it's an error:
                sys_puts("ERROR: execv\n");
            } else { // parent
                // don't fork again:
                num_flips++;
            }
        }
    }

    sys_puts("C Done.\n");
    return 0;
}

int _userland u_main2() {
    int counter = 0;
    int flipper = 0;
    int num_flips = 0;
    char pidstr[2];
    uint32_t pid = getpid();
    pidstr[0] = '0' + (char)pid;
    pidstr[1] = 0;
    while (1) {
        counter++;
        if (counter % PRINT_FREQ == 0) {
            flipper++;
            if ((flipper & 1) == 0) {
                sys_puts(pidstr);
            } else {
                sys_puts("o");
                num_flips++;
            }
        }
        if (num_flips == 31) {
            sys_puts("31");
            num_flips++;
            exit();
        }
    }

    sys_puts("C Done.\n");
    return 0;
}

int _userland u_main3() {
    int counter = 0;
    int flipper = 0;
    char pidstr[2];
    uint32_t pid = getpid();
    pidstr[0] = '0' + (char)pid;
    pidstr[1] = 0;
    while (1) {
        counter++;
        if (counter % PRINT_FREQ == 0) {
            flipper++;
            if ((flipper & 1) == 0) {
                sys_puts(pidstr);
            } else {
                sys_puts(".");
            }
        }
    }

    sys_puts("C Done.\n");
    return 0;
}

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
                uint32_t pid = fork();
                if (pid == -1) {
                    sys_puts("ERROR: fork!\n");
                } else if (pid == 0) { // child
                    uint32_t code = execv("5", 0); // "5" == u_main_hello1
                    // normally exec doesn't return, but if it did, it's an error:
                    sys_puts("ERROR: execv\n");
                } else { // parent
                    wait();
                }
            } else if (ustrncmp(buf, h2, ARRAY_LENGTH(h2)) == 0) {
                uint32_t pid = fork();
                if (pid == -1) {
                    sys_puts("ERROR: fork!\n");
                } else if (pid == 0) { // child
                    uint32_t code = execv("6", 0); // "6" == u_main_hello2
                    // normally exec doesn't return, but if it did, it's an error:
                    sys_puts("ERROR: execv\n");
                } else { // parent
                    wait();
                }
            } else if (ustrncmp(buf, si, ARRAY_LENGTH(h2)) == 0) {
                uint32_t pid = fork();
                if (pid == -1) {
                    sys_puts("ERROR: fork!\n");
                } else if (pid == 0) { // child
                    uint32_t code = execv("7", 0); // "7" == u_main_sysinfo
                    // normally exec doesn't return, but if it did, it's an error:
                    sys_puts("ERROR: execv\n");
                } else { // parent
                    wait();
                }
            } else if (ustrncmp(buf, fmt, ARRAY_LENGTH(h2)) == 0) {
                uint32_t pid = fork();
                if (pid == -1) {
                    sys_puts("ERROR: fork!\n");
                } else if (pid == 0) { // child
                    uint32_t code = execv("8", 0); // "8" == u_main_fmt
                    // normally exec doesn't return, but if it did, it's an error:
                    sys_puts("ERROR: execv\n");
                } else { // parent
                    wait();
                }
            } else {
                sys_puts("unknown command: ");
                sys_puts(buf);
                sys_puts("\n");
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

int _userland u_main_sysinfo() {
    sysinfo_t info;
    sysinfo(&info);
    char buf[4];
    buf[0] = '0' + info.freeram/10;
    buf[1] = '0' + info.freeram%10;
    buf[2] = '\n';
    buf[3] = 0;
    sys_puts("Free RAM pages: ");
    sys_puts(buf);
    exit();
    return 0;
}

uint64_t _userland get_clock_cycles() {
    register uint64_t a0 asm ("a0");
#if XLEN == 64
    asm volatile (
        "csrr a0, cycle"
        : "=r"(a0)  // output in a0
    );
#elif XLEN == 32
    // TODO
#else
    #error Unknown xlen
#endif
    return a0;
}

int _userland m_read_hart_id() {
    register int a0 asm ("a0");
    asm volatile (
        "csrr a0, mhartid"
        : "=r"(a0)  // output in a0
    );
    return a0;
}

void _userland push_pop_stack() {
    asm(
#if XLEN == 64
        "addi sp, sp, -16;"
        "ld   t0, 0(sp);"
        "ld   t1, 8(sp);"
        "sd   t0, 0(sp);"
        "sd   t1, 8(sp);"
        "addi sp, sp, 16;"
#elif XLEN == 32
        "addi sp, sp, -8;"
        "lw   t0, 0(sp);"
        "lw   t1, 4(sp);"
        "sw   t0, 0(sp);"
        "sw   t1, 4(sp);"
        "addi sp, sp, 8;"
#else
    #error Unknown xlen
#endif
    : // no output
    : // no input
    : "t0", "t1"  // t0 and t1 get clobbered
    );
}
