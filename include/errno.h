#ifndef _ERRNO_H_
#define _ERRNO_H_

#define ENOENT       2  // No such file or directory
#define ESRCH        3  // No such process
#define EBADF        9  // Bad file descriptor
#define ENOMEM      12  // Cannot allocate memory
#define EFAULT      14  // Bad address
#define EBUSY       16  // Device or resource busy
#define EINVAL      22  // Invalid argument
#define ENFILE      23  // Too many open files in system
#define EMFILE      24  // Too many open files
#define EPIPE       32  // Broken pipe
#define ENOSYS      38  // Function not implemented
#define EBADFD      77  // File descriptor in bad state
#define ENOBUFS    105  // No buffer space available

extern int* __errno_location();
#define errno (*__errno_location())

#endif // ifndef _ERRNO_H_
