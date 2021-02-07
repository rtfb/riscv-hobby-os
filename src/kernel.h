#ifndef _KERNEL_H_
#define _KERNEL_H_

// 1.3 Privilege Levels, Table 1.1: RISC-V privilege levels.
#define MODE_U      0 << 11
#define MODE_S      1 << 11
#define MODE_M      3 << 11
#define MODE_MASK ~(3 << 11)

void schedule_user_process();
void set_user_mode();
void jump_to_func(void *func);
unsigned int get_mstatus();
void set_mstatus(unsigned int mstatus);

extern void user_entry_point();

#endif // ifndef _KERNEL_H_
