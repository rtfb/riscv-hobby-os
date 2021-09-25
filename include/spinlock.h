#ifndef _SPINLOCK_H_
#define _SPINLOCK_H_

typedef uint32_t spinlock;

// implemented in spinlock.s
void acquire(spinlock *lock);
void release(spinlock *lock);

#endif // ifndef _SPINLOCK_H_
