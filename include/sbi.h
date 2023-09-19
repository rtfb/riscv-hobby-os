#ifndef _SBI_H_
#define _SBI_H_

#define SBI_EXT_TIME            0x54494D45
#define SBI_EXT_TIME_SET_TIMER  0

struct sbi_ret {
    long error;
    long value;
};

struct sbi_ret sbi_ecall(
    int ext, int fid,
    unsigned long arg0, unsigned long arg1, unsigned long arg2,
    unsigned long arg3, unsigned long arg4, unsigned long arg5
);

#endif // ifndef _SBI_H_
