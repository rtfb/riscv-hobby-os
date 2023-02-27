#include "sys.h"
#include "userland.h"
#include "string.h"
#include "syscalls.h"
#include "fs.h"

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

int _userland ustrlen(char const *s) {
    unsigned int i = 0;
    while (*s++) i++;
    return i;
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

int _userland u_main_hello1() {
    prints("Hello from hellosayer 1\n");
    exit();
    return 0;
}

char dash_h[] _user_rodata = "-h";

int _userland u_main_hello2(int argc, char const* argv[]) {
    if (argc > 1 && !ustrncmp(argv[1], dash_h, 2)) {
        prints("A hidden greeting!\n");
    } else {
        prints("Very welcome from hellosayer 2\n");
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


int _userland u_main_hanger() {
    prints("I will hang now, bye\n");
    wait();
    return 0;
}

int _userland u_main_smoke_test(int argc, char const *argv[]) {
    prints("\nInit userland smoke test!\n");
    char *sh_args[] = {"sh", "/home/smoke-test.sh"};
    run_program("sh", sh_args);

    // any trailing arg will cause it to exit and not hang forever
    if (argc > 1) {
        exit(0);
    }
    for (;;)
        ;
}

char ps_header_fmt[] _user_rodata = "PID  STATE  NAME\n";
char ps_process_info_fmt[] _user_rodata = "%d    %c      %s\n";
char ps_dash_s_flag[] _user_rodata = "-s";

char _userland state_to_char(uint32_t state) {
    switch (state) {
        case 0: return 'A'; // available
        case 1: return 'G'; // ready (read: good)
        case 2: return 'R'; // running
        case 3: return 'S'; // sleeping
        default: return 'U'; // unknown
    }
    return 'U';
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
    uint32_t my_pid = 0;
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
        printf(ps_process_info_fmt, info.pid, state_to_char(info.state), info.name);
    }
    exit(0);
    return 0;
}

int _userland u_main_cat(int argc, char const *argv[]) {
    if (argc < 2) {
        exit(0);
    }
    uint32_t fd = open(argv[1], 0);
    if (fd == -1) {
        prints("ERROR: open=-1\n");
        exit(0);
    }
    char fbuf[64];
    int32_t status = read(fd, fbuf, 64);
    if (status == -1) {
        prints("ERROR: read=-1\n");
        exit(0);
    }
    fbuf[status] = 0;
    prints(fbuf);
    close(fd);
    exit(0);
    return 0;
}

char pipe_fmt[] _user_rodata = "fd0=0x%x, fd1=0x%x\n";

int _userland u_main_pipe(int argc, char const *argv[]) {
    uint32_t fds[2];
    int32_t status = pipe(fds);
    if (status == -1) {
        prints("ERROR: pipe=-1\n");
        exit(-1);
    }
    printf(pipe_fmt, fds[0], fds[1]);
    close(fds[0]);
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
        if (nread == EOF) {
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
