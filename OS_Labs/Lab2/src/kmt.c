#include <os.h>
#include "mylibc.h"

static void kmt_init();
static int kmt_create(thread_t* thread, void (*entry)(void* arg), void* arg);
static void kmt_teardown(thread_t* thread);
static thread_t* kmt_schedule();
static void kmt_spin_init(spinlock_t* lk, const char* name);
static void kmt_spin_lock(spinlock_t* lk);
static void kmt_spin_unlock(spinlock_t* lk);
static void kmt_sem_init(sem_t* sem, const char* name, int value);
static void kmt_sem_wait(sem_t* sem);
static void kmt_sem_signal(sem_t* sem);

MOD_DEF(kmt){.init = kmt_init,
             .create = kmt_create,
             .schedule = kmt_schedule,
             .teardown = kmt_teardown,
             .spin_init = kmt_spin_init,
             .spin_lock = kmt_spin_lock,
             .spin_unlock = kmt_spin_unlock,
             .sem_init = kmt_sem_init,
             .sem_wait = kmt_sem_wait,
             .sem_signal = kmt_sem_signal};

void kmt_init() {
    //for (int32_t i = 1; i <= MAX_THREAD_NUM; i++) {
        //;
    //}
    static int i = 1;
    while (i <= MAX_THREAD_NUM) {
        thread_list[i] = NULL;
        i++;
    }
    current_thread_id = -1;
    kmt_spin_init(&thread_lock, "thread_lock");
    kmt_spin_init(&sem_lock, "sem_lock");
}

int kmt_create(thread_t* thread, void (*entry)(void* arg), void* arg) {
    kmt_spin_lock(&thread_lock);
    int i;
    for (i = 1; i <= MAX_THREAD_NUM; i++) {
        if (thread_list[i] == NULL) {
            break;
        }
    }
    if (i > MAX_THREAD_NUM) {
        printf("There are too many threads already!");
        kmt_spin_unlock(&thread_lock);
        return -1;
    }
    thread_list[i] = thread;
    thread->id = i;
    thread->tregs = _make(thread->stack, entry, arg);
    thread->stack.start = pmm->alloc(STACK_SIZE);
    thread->stack.end = thread->stack.start + STACK_SIZE;
    thread->sleep = 0;
    thread->waiting = NULL;
    kmt_spin_unlock(&thread_lock);
    return 0;
}

void kmt_teardown(thread_t* thread) {
    kmt_spin_lock(&thread_lock);
    if (thread_list[thread->id] != thread) {
        printf("Can not find the thread");
        return;
    }
    pmm->free(thread_list[thread->id]->stack.start);
    thread_list[thread->id] = NULL;
    thread->id = 0;
    kmt_spin_unlock(&thread_lock);
}

thread_t* kmt_schedule() {
    uint32_t i = current_thread_id;
    while (thread_list[i] == NULL || thread_list[i]->sleep != 0) {
        i %= MAX_THREAD_NUM;
        i++;
    }
    current_thread_id = i;
    return thread_list[i];
}

void kmt_spin_init(spinlock_t* lk, const char* name) {
    lk->locked = 0;
    lk->name = name;
}

void kmt_spin_lock(spinlock_t* lk) {
    if (lk->locked == 1) {
        printf("The lock has been locked!");
    } else {
        lk->locked = 1;
        _intr_write(0);
    }
}

void kmt_spin_unlock(spinlock_t* lk) {
    if (lk->locked == 0) {
        printf("The lock has been unlocked!");
    } else {
        lk->locked = 0;
        _intr_write(1);
    }
}

void kmt_sem_init(sem_t* sem, const char* name, int value) {
    sem->count = value;
    sem->name = name;
}

void kmt_sem_wait(sem_t* sem) {
    kmt_spin_lock(&sem_lock);
    while (sem->count == 0) {
        thread_list[current_thread_id]->waiting = sem;
        thread_list[current_thread_id]->sleep = 1;
        kmt_spin_unlock(&sem_lock);
        _yield();
        kmt_spin_lock(&sem_lock);
    }
    sem->count--;
    kmt_spin_unlock(&sem_lock);
}

void kmt_sem_signal(sem_t* sem) {
    kmt_spin_lock(&sem_lock);
    sem->count++;
    uint32_t threadnum;
    for (threadnum = 1; threadnum <= MAX_THREAD_NUM; threadnum++) {
        if (thread_list[threadnum]->sleep != 0 &&
            thread_list[threadnum]->waiting == sem) {
            break;
        }
    }
    if (threadnum <= MAX_THREAD_NUM) {
        thread_list[threadnum]->sleep = 0;
        thread_list[threadnum]->waiting = NULL;
    }
    kmt_spin_unlock(&sem_lock);
}