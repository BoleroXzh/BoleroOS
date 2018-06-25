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
    _Area stack;
    volatile int sleep;
    sem_t* waiting;
};
spinlock_t thread_lock;
thread_t* thread_list[MAX_THREAD_NUM];
uint32_t current_thread_id;

// vfs.h

#define INODE_NUM 256
#define NAME_LEN 256
#define FILE_NUM 256
#define DATA_SIZE 4096
#define DIRECT_NUM 16
#define PROCFS 0
#define DEVFS 1
#define KVFS 2
#define O_RDONLY 0
#define O_WRONLY 1
#define O_RDWR 2

struct fsops {
    void (*init)(filesystem_t* fs, const char* name, inode_t* dev);
    inode_t* (*lookup)(filesystem_t* fs, const char* path, int flags);
    int (*close)(inode_t* inode);
};

fsops_t procfs_ops, devfs_ops, kvfs_ops;

struct fileops {
    int (*open)(inode_t* inode, file_t* file, int flags);
    ssize_t (*read)(inode_t* inode, file_t* file, char* buf, size_t size);
    ssize_t (*write)(inode_t* inode,
                     file_t* file,
                     const char* buf,
                     size_t size);
    off_t (*lseek)(inode_t* inode, file_t* file, off_t offset, int whence);
};

struct filesystem {
    int type;
    int mounted;
    char mount_name[NAME_LEN];
    fsops_t* ops;
    inode_t* inode[INODE_NUM];
};

filesystem_t *procfs, *devfs, *kvfs;

struct inode {
    int type;
    char name[NAME_LEN];
    char data[DATA_SIZE];
    file_t* file[DIRECT_NUM];
};

struct file {
    int fd;
    int flags;
    off_t offset;
    inode_t* inode;
};

file_t* flist[FILE_NUM];

#endif