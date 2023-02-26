#ifndef _USYSCALLS_H_
#define _USYSCALLS_H_

#include "syscalls.h"

// Declarations for userland end of the system calls. Implemented in
// usyscalls.s.

extern void exit();
extern uint32_t fork();
extern int32_t read(uint32_t fd, char* buf, uint32_t size);

// write writes the given data to file descriptor fd. The three special file
// descriptors are stdin=0, stdout=1 and stderr=2. Other descriptors should be obtained
// via open(). The size parameter is optional: it specifies how many bytes to
// write from data, but it can be -1 if data contains a zero-terminated string,
// then data will be written until the first zero byte is encountered.
extern int32_t write(uint32_t fd, char const* data, uint32_t size);
extern int32_t open(char const *filepath, uint32_t flags);
extern int32_t close(uint32_t fd);
extern int32_t wait();
extern uint32_t execv(char const* filename, char const* argv[]);
extern uint32_t getpid();
extern uint32_t dup(uint32_t fd);
extern uint32_t pipe(uint32_t fd[2]);
extern uint32_t sysinfo(sysinfo_t* info);
extern uint32_t sleep(uint64_t milliseconds);
extern uint32_t plist(uint32_t *pids, uint32_t size);
extern uint32_t pinfo(uint32_t pid, pinfo_t *pinfo);

#endif // ifndef _USYSCALLS_H_
