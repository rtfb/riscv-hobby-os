#include "errno.h"
#include "fs.h"
#include "gpio.h"
#include "shell.h"
#include "string.h"
#include "syscalls.h"
#include "sys.h"
#include "userland.h"
#include "ustr.h"

int _userland u_main_hello1() {
    prints("Hello from hellosayer 1\n");
    void *mem = (void*)pgalloc();
    char *s = (char*)mem;
    s[0] = 'y';
    s[1] = 'o';
    s[2] = '!';
    s[3] = '\n';
    s[4] = 0;
    prints(s);
    pgfree(mem);
    exit(0);
    return 0;
}

char dash_h[] _user_rodata = "-h";

int _userland u_main_hello2(int argc, char const* argv[]) {
    if (argc > 1 && !ustrncmp(argv[1], dash_h, 2)) {
        prints("A hidden greeting!\n");
    } else {
        prints("Very welcome from hellosayer 2\n");
    }
    exit(0);
    return 0;
}

char fmt[] _user_rodata = "formatted string: num=%d, zero=%d, char=%c, hex=0x%x, str=%s\n";
char fmt2[] _user_rodata = "only groks 7 args: %d %d %d %d %d %d %d %d %d\n";

int _userland u_main_fmt() {
    char foo[] = "foo";
    printf(fmt, 387, 0, 'X', 0xaddbeef, foo);
    printf(fmt2, 11, 12, 13, 14, 15, 16, 17, 18, 19);
    exit(0);
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
    exit(0);
    return 0;
}

int _userland u_main_hanger() {
    prints("I will hang now, bye\n");
    wait();
    return 0;
}

char ps_header_fmt[] _user_rodata = "ST  PID   NSCH   NAME\n";
char ps_process_info_fmt[] _user_rodata = "%c   %d     %d      %s\n";
char ps_dash_s_flag[] _user_rodata = "-s";
char ps_state_lookup_table[] _user_rodata = {
    'A', // 0, available
    'G', // 1, ready (read: good)
    'R', // 2, running
    'S', // 3, sleeping
    'Z', // 4, zombie
};

char _userland state_to_char(uint32_t state) {
    if (state < 0 || state > ARRAY_LENGTH(ps_state_lookup_table)) {
        return 'U';
    }
    return ps_state_lookup_table[state];
}

int _userland u_main_ps(int argc, char const *argv[]) {
    uint32_t pids[8];
    uint32_t num_pids = plist(pids, 8);
    if (num_pids < 0) {
        prints("ERROR: plist\n");
        exit(-1);
        return -1;
    }
    int skip_self = 0;
    uint32_t my_pid = -1;
    if (argc > 1 && !ustrncmp(ps_dash_s_flag, argv[1], 2)) {
        skip_self = 1;
        my_pid = getpid();
    }
    printf(ps_header_fmt);
    for (int i = 0; i < num_pids; i++) {
        if (skip_self && pids[i] == my_pid) {
            continue;
        }
        pinfo_t info;
        uint32_t status = pinfo(pids[i], &info);
        if (status < 0 ) {
            prints("ERROR: pinfo\n");
            continue;
        }
        // truncate nscheds to 32 bits on 32-bit targets. That's because printf
        // doesn't support 64-bit params on 32-bit hosts.
        regsize_t nscheds = info.nscheds;
        printf(ps_process_info_fmt, state_to_char(info.state), info.pid,
                                    nscheds, info.name);
    }
    exit(0);
    return 0;
}

char err_fmt[] _user_rodata = "ERROR: open fd=-1, errno=%d\n";

int _userland u_main_cat(int argc, char const *argv[]) {
    if (argc < 2) {
        exit(0);
    }
    uint32_t fd = open(argv[1], 0);
    if (fd == -1) {
        printf(err_fmt, errno);
        exit(-1);
    }
    char fbuf[64];
    int32_t nread = read(fd, fbuf, 64);
    if (nread == -1) {
        prints("ERROR: read=-1\n");
        exit(-1);
    }
    fbuf[nread] = 0;
    int32_t nwrit = write(1, fbuf, nread);
    if (nwrit == -1) {
        prints("ERROR: write=-1\n");
        exit(-1);
    }
    close(fd);
    exit(0);
    return 0;
}

char pipe_fmt[] _user_rodata = "fd0=0x%x, fd1=0x%x\n";

int _userland u_main_pipe(int argc, char const *argv[]) {
    char const *prog1 = "cat";
    char const *argv1[] = {"cat", "/readme.txt", 0};
    char const *prog2 = "wc";
    uint32_t pipefd[2];

    if (pipe(pipefd) == -1) {
        prints("ERROR: pipe=-1\n");
        exit(-1);
    }

    uint32_t cpid2 = -1;
    uint32_t cpid1 = fork();
    if (cpid1 == -1) {
        prints("ERROR: fork 1!\n");
        exit(-1);
    }

    if (cpid1 == 0) { // child
        close(pipefd[0]);   // close unused read end
        close(1);           // close stdout
        dup(pipefd[1]);     // now replace stdout with the pipe's writing end
        uint32_t code = execv(prog1, (char const**)argv1);
        // normally exec doesn't return, but if it did, it's an error:
        prints("ERROR: execv 1\n");
        exit(-1);
    } else { // parent
        cpid2 = fork();
        if (cpid2 == -1) {
            prints("ERROR: fork 2!\n");
            exit(-1);
        }
        if (cpid2 == 0) { // child
            close(pipefd[1]);   // close unused write end
            close(0);           // close stdin
            dup(pipefd[0]);     // now replace stdin with the pipe's reading end
            uint32_t code = execv(prog2, 0);
            // normally exec doesn't return, but if it did, it's an error:
            prints("ERROR: execv 2\n");
            exit(-1);
        }
    }
    close(pipefd[0]);
    close(pipefd[1]);
    wait();                 // Wait for the first child
    // wait();              // Wait for the other child.
                            // TODO: this deadlocks for some reason, investigate
    exit(0);
    return 0;
}

char pipe2_fmt[] _user_rodata = "Usage: %s <string>\n";

// this program is copied (almost) verbatim from 'man pipe2'
int _userland u_main_pipe2(int argc, char const *argv[]) {
    uint32_t pipefd[2];
    uint32_t cpid;
    char buf;

    if (argc != 2) {
        printf(pipe2_fmt, argv[0]);
        exit(-1);
    }

    if (pipe(pipefd) == -1) {
        prints("ERROR: pipe=-1\n");
        exit(-1);
    }

    cpid = fork();
    if (cpid == -1) {
        prints("ERROR: fork=-1\n");
        exit(-1);
    }

    if (cpid == 0) {            // Child reads from pipe
        close(pipefd[1]);       // Close unused write end

        while (1) {
            int nread = read(pipefd[0], &buf, 1);
            if (nread <= 0) {
                // prints("EREAD\n");
                break;
            } else {
                write(1, &buf, 1);
            }
        }

        close(pipefd[0]);
        exit(0);
    } else {                    // Parent writes argv[1] to pipe
        close(pipefd[0]);       // Close unused read end
        for (int i = 0; i < 50; i++) {
            write(pipefd[1], argv[1], ustrlen(argv[1]));
            write(pipefd[1], "\n", 1);
        }
        close(pipefd[1]);       // Reader will see EOF
        wait();                 // Wait for child
        exit(0);
    }
    return 0;
}

char wc_charcount_fmt[] _user_rodata = "%d\n";
int _userland u_main_wc(int argc, char const *argv[]) {
    uint32_t fd = 0; // stdin
    if (argc > 1) {
        fd = open(argv[1], 0);
        if (fd == -1) {
            prints("ERROR: open=-1\n");
            exit(-1);
        }
    }
    char fbuf[64];
    uint32_t charcount = 0;
    while (1) {
        int32_t nread = read(fd, fbuf, 64);
        if (nread == 0) {
            break;
        }
        if (nread == -1) {
            prints("ERROR: read=-1\n");
            exit(-1);
        }
        charcount += nread;
    }
    if (fd != 0) {
        close(fd);
    }
    printf(wc_charcount_fmt, charcount);
    exit(0);
}

// coma is a special program that hangs forever. Don't run it, and, more
// importantly, don't wait() on it.
int _userland u_main_coma() {
    for (;;)
        ;
}

int _userland map_gpio_pin_num(int num) {
    switch (num) {
        case  0: return GPIO_PIN_0;
        case  1: return GPIO_PIN_1;
        case  2: return GPIO_PIN_2;
        case  3: return GPIO_PIN_3;
        case  4: return GPIO_PIN_4;
        case  5: return GPIO_PIN_5;
        case  6: return GPIO_PIN_6;
        case  7: return GPIO_PIN_7;
        case  8: return GPIO_PIN_8;
        case  9: return GPIO_PIN_9;
        case 10: return GPIO_PIN_10;
        case 11: return GPIO_PIN_11;
        case 12: return GPIO_PIN_12;   // XXX: doesn't work
        case 13: return GPIO_PIN_13;
        case 14: return -1;            // not connected
        case 15: return GPIO_PIN_15;
        case 16: return GPIO_PIN_16;   // XXX: doesn't work
        case 17: return GPIO_PIN_17;
        case 18: return GPIO_PIN_18;
        case 19: return GPIO_PIN_19;
        default:
                return -1;
    };
}

// gpio is for interacting with gpio pins.
//      > gpio N e      enable pin N for writing
//      > gpio N d      disable pin N for writing
//      > gpio N 1      write 1 to pin N
//      > gpio N 0      write 0 to pin N
//      > gpio N t      toggle the current value of N
char gpio_parse_err_fmt[] _user_rodata = "ERROR: parse pin_num '%s': %d\n";
char gpio_syscall_err_fmt[] _user_rodata = "ERROR: gpio syscall: %d\n";
int _userland u_main_gpio(int argc, char const *argv[]) {
    if (argc < 3) {
        prints("usage:\n"
"gpio N e      enable pin N for writing\n"
"gpio N d      disable pin N for writing\n"
"gpio N 1      write 1 to pin N\n"
"gpio N 0      write 0 to pin N\n"
"gpio N t      toggle the current value of N\n");
        exit(-1);
    }
    int parse_err = 0;
    int pin_num = uatoi(argv[1], &parse_err);
    if (parse_err != 0) {
        printf(gpio_parse_err_fmt, argv[1], parse_err);
        exit(-2);
    }
    if (argv[2][1] != 0) {
        prints("ERROR: arg 2 must be a single character!\n");
        exit(-3);
    }
    pin_num = map_gpio_pin_num(pin_num);
    if (pin_num < 0) {
        prints("ERROR: pin mapping error\n");
        exit(-4);
    }
    uint32_t err = 0;
    switch (argv[2][0]) {
        case 'e':
            err = gpio(pin_num, 1, 0);
            break;
        case 'd':
            err = gpio(pin_num, 0, 0);
            break;
        case '0':
            err = gpio(pin_num, -1, 0);
            break;
        case '1':
            err = gpio(pin_num, -1, 1);
            break;
        case 't':
            err = gpio(pin_num, -1, 2);
            break;
        default:
            prints("ERROR: unrecognized arg 2: ");
            prints(argv[2]);
            prints("\n");
            exit(-5);
    }
    if (err != 0) {
        printf(gpio_syscall_err_fmt, err);
        exit(-6);
    }
    exit(0);
    return 0;
}

char iter_parse_err_fmt[] _user_rodata = "ERROR: parse N '%s': %d\n";
char iter_i_fmt[] _user_rodata = "%d\n";

int _userland u_main_iter(int argc, char const *argv[]) {
    if (argc < 2) {
        prints("usage:\n"
"iter N        write numbers [0..N) to stdout\n");
        exit(-1);
    }
    int parse_err = 0;
    int n = uatoi(argv[1], &parse_err);
    if (parse_err != 0) {
        printf(iter_parse_err_fmt, argv[1], parse_err);
        exit(-2);
    }
    for (int i = 0; i < n; i++) {
        printf(iter_i_fmt, i);
    }
    exit(0);
    return 0;
}

char testprintf_fmt[] _user_rodata = "formatted string: num=%d, zero=%d, char=%c, hex=0x%x, str=%s\n";
int _userland u_main_test_printf(int argc, char const* argv[]) {
    char buf[32+32+32+9]; // placeholder: it's here just to use up a lot of stack
    char foo[] = "foo";
    printf(testprintf_fmt, 387, 0, 'X', 0xaddbeef, foo);
    exit(0);
    return 0;
}

char fibd_print_resp_fmt[] _user_rodata = "%d, %d\n";

int _userland u_main_fibd(int argc, char const* argv[]) {
    close(1); // close own stdout
    detach(); // detach from the shell
    uint64_t fib1 = 1;
    uint64_t fib2 = 1;
    uint32_t i = 2;
    while (1) {
        uint64_t fib = fib1 + fib2;
        fib1 = fib2;
        fib2 = fib;
        i += 1;
        if (isopen(1)) {                // check if a client has attached a pipe to stdout
            printf(fibd_print_resp_fmt, i, fib); // if yes, write a response to it
            close(1);                   // and close the stdout again, so that the client can attach again
        }
        sleep(500);
    }
    return 0;
}

char fib_parse_err_fmt[] _user_rodata = "ERROR: parse pid '%s': %d\n";

// fib is the client talking to fibd
int _userland u_main_fib(int argc, char const* argv[]) {
    if (argc < 2) {
        prints("USAGE: fib <pid of fibd>\n");
        exit(0);
        return 0;
    }
    int parse_err = 0;
    int fibd_pid = uatoi(argv[1], &parse_err);
    if (parse_err != 0) {
        printf(fib_parse_err_fmt, argv[1], parse_err);
        exit(-1);
        return -1;
    }
    uint32_t pipefd[2] = {-1, -1};
    if (pipe(pipefd) == -1) {
        prints("ERROR: pipe=-1\n");
        exit(-2);
        return -2;
    }
    // attach the pipe's writing end to the daemon's stdout:
    if (pipeattch(fibd_pid, pipefd[1]) == -1) {
        prints("ERROR: pipeattch=-1\n");
        exit(-3);
        return -3;
    }
    char fbuf[64];
    // read from the pipe. This will block until the daemon responds:
    int32_t nread = read(pipefd[0], fbuf, 64);
    if (nread == -1) {
        prints("ERROR: read=-1\n");
        exit(-4);
        return -4;
    }
    fbuf[nread] = 0;
    int32_t nwrit = write(1, fbuf, nread);
    if (nwrit == -1) {
        prints("ERROR: write=-1\n");
        exit(-5);
        return -5;
    }
    close(pipefd[0]);
    close(pipefd[1]);
    exit(0);
    return 0;
}

char sleep_parse_err_fmt[] _user_rodata = "ERROR: parse millis '%s': %d\n";

// sleep calls the sleep() syscall and returns.
int _userland u_main_sleep(int argc, char const* argv[]) {
    if (argc < 2) {
        prints("USAGE: sleep <milliseconds>\n");
        exit(0);
        return 0;
    }
    int parse_err = 0;
    int millis = uatoi(argv[1], &parse_err);
    if (parse_err != 0) {
        printf(sleep_parse_err_fmt, argv[1], parse_err);
        exit(-1);
        return -1;
    }
    sleep(millis);
    exit(0);
    return 0;
}
