// This file is an input to scripts/gen-syscalls.py, not meant to be directly
// #included in a .c file.

1: exit(int status);
2: fork();
3: read(uint32_t fd, char* buf, uint32_t size);

// write writes the given data to file descriptor fd. The three special file
// descriptors are stdin=0, stdout=1 and stderr=2. Other descriptors should be obtained
// via open(). The size parameter is optional: it specifies how many bytes to
// write from data, but it can be -1 if data contains a zero-terminated string,
// then data will be written until the first zero byte is encountered.
4: write(uint32_t fd, void* data, uint32_t size);
5: open(char const *filepath, uint32_t flags);
6: close(uint32_t fd);
7: wait(wait_cond_t *cond);
11: execv(char const* filename, char const* argv[]);
20: getpid();

// The numbers below differ from Linux, may need to renumber one day if we ever
// want to achieve ABI compatibility. But for now, I don't want to make
// syscall_vector too big while very sparsely populated.
// __NR_dup is 41 on Linux
28: dup(uint32_t fd);
// __NR_pipe is 42 on Linux
29: pipe(uint32_t fd[2]);
// __NR_sysinfo is 116 on Linux
30: sysinfo(sysinfo_t* info);
// __NR_nanosleep is 162 on Linux
31: sleep(uint64_t milliseconds);

// These are non-standard syscalls
32: plist(uint32_t *pids, uint32_t size);
33: pinfo(uint32_t pid, pinfo_t *pinfo);
34: pgalloc();
35: pgfree(void *page);
36: gpio(uint32_t pin_num, uint32_t enable, uint32_t value);
37: detach();
38: isopen(int32_t fd);
39: pipeattch(uint32_t pid, int32_t src_fd);
40: lsdir(char const *dir, dirent_t *dirents, int size);
