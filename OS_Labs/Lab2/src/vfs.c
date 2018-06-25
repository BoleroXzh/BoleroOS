#include <os.h>
#include "mylibc.h"

static void vfs_init();
static int vfs_access(const char* path, int flags);
static int vfs_mount(const char* path, filesystem_t* fs);
static int vfs_unmount(const char* path);
static int vfs_open(const char* path, int flags);
static ssize_t vfs_read(int fd, void* buf, size_t nbyte);
static ssize_t vfs_write(int fd, void* buf, size_t nbyte);
static off_t vfs_lseek(int fd, off_t offset, int whence);
static int vfs_close(int fd);

static char* cpu_info = "Model name	: Intel(R) Core(TM) i7-7700HQ CPU @ 2.80GHz\n";
static char* mem_info = "MemTotal:        8290964 kB\n";
static char* dev_null = "NULL";
static char* dev_zero = "0000000000000000000000000";
static char* dev_random;
static uint8_t mounted[3] = {0, 0, 0};
static char mounted_name[3][NAME_LEN];

MOD_DEF(vfs){
    .init = vfs_init,
    .access = vfs_access,
    .mount = vfs_mount,
    .unmount = vfs_unmount,
    .open = vfs_open,
    .read = vfs_read,
    .write = vfs_write,
    .lseek = vfs_lseek,
    .close = vfs_close,
};

// fsops

static void fsops_init(filesystem_t* fs, const char* name, inode_t* dev) {
    if (strcmp(name, "procfs") == 0) {
        fs->type = PROCFS;
    } else if (strcmp(name, "devfs") == 0) {
        fs->type = DEVFS;
    } else if (strcmp(name, "kvfs") == 0) {
        fs->type = KVFS;
    } else {
        printf("Error in fsops_init\n");
        return;
    }
}

static inode_t* fsops_lookup(filesystem_t* fs, const char* path, int flags) {
    for (int i = 0; i < INODE_NUM; i++) {
        if (fs->inode[i] != NULL && strcmp(fs->inode[i]->name, path) == 0) {
            // printf("Error in fsops_lookup\n");
            return fs->inode[i];
        }
    }
    for (int i = 0; i < INODE_NUM; i++) {
        if (fs->inode[i] == NULL) {
            fs->inode[i] = (inode_t*)pmm->alloc(sizeof(inode_t));
            if (NULL == fs->inode[i]) {
                printf("Error in fsops_lookup\n");
            }
            strcpy(fs->inode[i]->name, path);
            fs->inode[i]->type = flags;
            return fs->inode[i];
        }
    }
    printf("Error in fsops_lookup\n");
    return NULL;
}

static int fsops_close(inode_t* inode) {
    return 0;
}

filesystem_t* create_procfs() {
    filesystem_t* fs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if (!fs) {
        printf("Error in create_procfs\n");
    }
    fs->ops = &procfs_ops;
    fs->ops->init(fs, "procfs", NULL);
    return fs;
}

filesystem_t* create_devfs() {
    filesystem_t* fs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if (!fs) {
        printf("Error in create_devfs\n");
    }
    fs->ops = &devfs_ops;
    fs->ops->init(fs, "devfs", NULL);
    return fs;
}

filesystem_t* create_kvfs() {
    filesystem_t* fs = (filesystem_t*)pmm->alloc(sizeof(filesystem_t));
    if (!fs) {
        printf("Error in create_kvfs\n");
    }
    fs->ops = &kvfs_ops;
    fs->ops->init(fs, "kvfs", NULL);
    return fs;
}

// fileops

static int fileops_open(inode_t* inode, file_t* file, int flags) {
    if (inode == NULL || (inode->type == O_RDONLY && flags == O_WRONLY) ||
        (inode->type == O_WRONLY && flags == O_RDONLY)) {
        printf("Error in fileops_open\n");
        return -1;
    }
    file->flags = flags;
    file->offset = 0;

    for (int i = 0; i < DIRECT_NUM; i++) {
        if (inode->file[i] != NULL && inode->file[i]->fd == file->fd) {
            return file->fd;
        }
    }
    file->fd = -1;
    for (int i = 0; i < FILE_NUM; i++) {
        if (flist[i] == NULL) {
            file->fd = i;
            break;
        }
    }
    if (file->fd == -1) {
        printf("Error in fileops_open\n");
    }
    flist[file->fd] = file;
    for (int i = 0; i < DIRECT_NUM; i++) {
        if (inode->file[i] == NULL) {
            inode->file[i] = file;
            file->inode = inode;
            return file->fd;
        }
    }
    printf("Error in fileops_open\n");
    return -1;
}

static ssize_t fileops_read(inode_t* inode,
                            file_t* file,
                            char* buf,
                            size_t size) {
    if (inode == NULL || file->flags == O_WRONLY) {
        printf("Error in fileops_read\n");
    }
    int len = size;
    if (len + file->offset > strlen(inode->data))
        len = strlen(inode->data) - file->offset;
    memcpy(buf, inode->data + file->offset, len);
    buf[len] = '\0';
    file->offset += len;
    return len;
}

static ssize_t fileops_write(inode_t* inode,
                             file_t* file,
                             const char* buf,
                             size_t size) {
    if (inode == NULL || file->flags == O_WRONLY) {
        printf("Error in fileops_write\n");
    }
    int len =
        (file->offset + size > DATA_SIZE ? DATA_SIZE - file->offset : size);
    if (len < 0) {
        len = 0;
    }
    memcpy(inode->data + file->offset, buf, len);
    inode->data[file->offset + len] = '\0';
    file->offset += len;
    return len;
}

static off_t fileops_lseek(inode_t* inode,
                           file_t* file,
                           off_t offset,
                           int whence) {
    file->offset = whence;
    if (strlen(file->inode->data) < file->offset) {
        file->offset = strlen(file->inode->data);
    }
    return 0;
}

static int fileops_close(inode_t* inode, file_t* file, int p) {
    for (int i = 0; i < DIRECT_NUM; i++) {
        if (inode->file[i] != NULL && inode->file[i]->fd == file->fd) {
            inode->file[i] = NULL;
            break;
        }
    }
    pmm->free(file);
    flist[p] = NULL;
    return 0;
}

// vfs

static void vfs_init() {
    for (int i = 0; i < FILE_NUM; i++) {
        flist[i] = NULL;
    }
    procfs_ops.init = &fsops_init;
    procfs_ops.lookup = &fsops_lookup;
    procfs_ops.close = &fsops_close;
    devfs_ops.init = &fsops_init;
    devfs_ops.lookup = &fsops_lookup;
    devfs_ops.close = &fsops_close;
    kvfs_ops.init = &fsops_init;
    kvfs_ops.lookup = &fsops_lookup;
    kvfs_ops.close = &fsops_close;
    procfs = create_procfs();
    devfs = create_devfs();
    kvfs = create_kvfs();
    int cpuinfo = vfs->open("/proc/cpuinfo", 2);
    vfs->write(cpuinfo, cpu_info, strlen(cpu_info));
    int meminfo = vfs->open("/proc/meminfo", 2);
    vfs->write(meminfo, mem_info, strlen(mem_info));
    int devnull = vfs->open("/dev/null", 2);
    vfs->write(devnull, dev_null, strlen(dev_null));
    int devzero = vfs->open("/dev/zero", 2);
    vfs->write(devzero, dev_zero, strlen(dev_zero));
    int devrandom = vfs->open("/dev/random", 2);
    itoa(rand(), dev_random, 10);
    vfs->write(devrandom, dev_random, strlen(dev_random));
}

static int vfs_access(const char* path, int flags) {
    char* lpath = NULL;
    inode_t* handle = NULL;
    if ((lpath = strstr(path, "/procfs")) != NULL) {
        handle = procfs->ops->lookup(procfs, lpath + strlen("/procfs"), flags);
    } else if ((lpath = strstr(path, "/devfs")) != NULL) {
        handle = devfs->ops->lookup(devfs, lpath + strlen("/devfs"), flags);
    } else if ((lpath = strstr(path, "/")) != NULL) {
        handle = kvfs->ops->lookup(kvfs, lpath + strlen("/"), flags);
    } else {
        printf("Error in vfs_access\n");
        return -1;
    }
    if (handle == NULL || (handle->type == O_RDONLY && flags == O_WRONLY) ||
        (handle->type == O_WRONLY && flags == O_RDONLY)) {
        printf("Error in vfs_access\n");
        return -1;
    }
    return 0;
}

static int vfs_mount(const char* path, filesystem_t* fs) {
    mounted[fs->type] = 1;
    strcpy(mounted_name[fs->type], path);
    return 0;
}

static int vfs_unmount(const char* path) {
    if (strcmp(path, mounted_name[PROCFS]) == 0) {
        mounted[PROCFS] = 0;
    } else if (strcmp(path, mounted_name[DEVFS]) == 0) {
        mounted[DEVFS] = 0;
    } else if (strcmp(path, mounted_name[KVFS]) == 0) {
        mounted[KVFS] = 0;
    }
    return 0;
}

static int vfs_open(const char* path, int flags) {
    char* lpath = NULL;
    inode_t* handle = NULL;
    if ((lpath = strstr(path, "/procfs")) != NULL) {
        handle = procfs->ops->lookup(procfs, lpath + strlen("/procfs"), flags);
    } else if ((lpath = strstr(path, "/devfs")) != NULL) {
        handle = devfs->ops->lookup(devfs, lpath + strlen("/devfs"), flags);
    } else if ((lpath = strstr(path, "/")) != NULL) {
        handle = kvfs->ops->lookup(kvfs, lpath + strlen("/"), flags);
    } else {
        printf("Error in vfs_open\n");
        return -1;
    }
    if (handle == NULL || (handle->type == O_RDONLY && flags == O_WRONLY) ||
        (handle->type == O_WRONLY && flags == O_RDONLY)) {
        printf("Error in vfs_open\n");
        return -1;
    }
    file_t* f = (file_t*)pmm->alloc(sizeof(file_t));
    if (f == NULL) {
        printf("Error in vfs_open\n");
        return -1;
    }
    int fd = fileops_open(handle, f, flags);
    return fd;
}

static ssize_t vfs_read(int fd, void* buf, size_t nbyte) {
    int pos = -1;
    for (int i = 0; i < FILE_NUM; i++) {
        if (flist[i] != NULL && flist[i]->fd == fd) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return -1;
        printf("Error in vfs_read\n");
    }
    return fileops_read(flist[pos]->inode, flist[pos], buf, nbyte);
}

static ssize_t vfs_write(int fd, void* buf, size_t nbyte) {
    int pos = -1;
    for (int i = 0; i < FILE_NUM; i++) {
        if (flist[i] != NULL && flist[i]->fd == fd) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return -1;
        printf("Error in vfs_write\n");
    }
    return fileops_write(flist[pos]->inode, flist[pos], buf, nbyte);
}

static off_t vfs_lseek(int fd, off_t offset, int whence) {
    int pos = -1;
    for (int i = 0; i < FILE_NUM; i++) {
        if (flist[i] != NULL && flist[i]->fd == fd) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return -1;
        printf("Error in vfs_lseek\n");
    }
    return fileops_lseek(flist[pos]->inode, flist[pos], offset, whence);
}

static int vfs_close(int fd) {
    int pos = -1;
    for (int i = 0; i < FILE_NUM; i++) {
        if (flist[i] != NULL && flist[i]->fd == fd) {
            pos = i;
            break;
        }
    }
    if (pos == -1) {
        return -1;
        printf("Error in vfs_close\n");
    }
    return fileops_close(flist[pos]->inode, flist[pos], pos);
}
