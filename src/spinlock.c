#include "spinlock.h"

void acquire(spinlock *lock) {
  while(__sync_lock_test_and_set(lock, 1) != 0) // emits 'amoswap.w.aq a4,a4,(a5)'
    ;
  __sync_synchronize();                         // emits 'fence'
}

void release(spinlock *lock) {
  __sync_synchronize();                         // emits 'fence'
  __sync_lock_release(lock);                    // emits 'amoswap.w	zero,zero,(a5)'
}
