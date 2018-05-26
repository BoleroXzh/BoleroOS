#ifndef __OS_H__
#define __OS_H__

#include <kernel.h>

static inline void puts(const char* p) {
    for (; *p; p++) {
        _putc(*p);
    }
}

// kmt.h

#define MAX_THREAD_NUM 32
#define STACK_SIZE 4096

struct spinlock {
    int locked;
    const char* name;
};

struct semaphore {
    int count;
    const char* name;
};
spinlock_t sem_lock;

struct thread {
    int id;
    _RegSet* tregs;
    // uint8_t fence1[32];
    _Area stack;
    // uint8_t fence2[32];
    volatile int sleep;
    sem_t* waiting;
};
spinlock_t thread_lock;
thread_t* thread_list[MAX_THREAD_NUM];
uint32_t current_thread_id;

#endif