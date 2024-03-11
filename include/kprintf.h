#ifndef _KPRINTF_H_
#define _KPRINTF_H_

#include "sys.h"

// sprintfer_t encapsulates the target buffer, buffer size, and the format
// string into a single struct for a single-parameter passing into
// ksprintfvec().
typedef struct sprintfer_s {
    void        *buf;
    regsize_t   bufsz;
    char const  *fmt;
} sprintfer_t;

// implemented in kprintf.S
// NOTE: gcc has a neat attribute, like this:
//     void kprintf(char const *msg, ...) __attribute__ ((format (printf, 1, 2)));
// with it, we'd get compile-time checks for format string compatibility with
// the args. However, that check is obviously very strict and demands me to use,
// e.g. %lx instead of %x on rv32. I don't want to bother with that at this
// point, so will just leave this note here as a reminder and we can implement
// it later.
// (docs: https://gcc.gnu.org/onlinedocs/gcc-3.2/gcc/Function-Attributes.html)
extern void kprintf(char const *msg, ...);
extern int32_t ksprintf(sprintfer_t *sprintfer, ...);

// implement in kprintf.c
extern int32_t kprintfvec(char const* fmt, regsize_t* args);
extern int32_t ksprintfvec(sprintfer_t *sprintfer, regsize_t* args);

#endif // ifndef _KPRINTF_H_
