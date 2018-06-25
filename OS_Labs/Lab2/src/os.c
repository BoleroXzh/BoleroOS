#include <os.h>
#include "mylibc.h"

#define FILE_TEST

static void os_init();
static void os_run();
static _RegSet* os_interrupt(_Event ev, _RegSet* regs);
static thread_t* current_thread;

MOD_DEF(os){
    .init = os_init,
    .run = os_run,
    .interrupt = os_interrupt,
};

static void os_init() {
    for (const char* p = "Hello, OS World!\n"; *p; p++) {
        _putc(*p);
    }
    current_thread = NULL;
}

static void file_test() {
    printf("\n");
    printf("#########################\n");
    printf("file_test start\n");
    printf("#########################\n");
    printf("\n");
    vfs->init();
    char wbuf[] = "It's a file_test", rbuf[256], dir[] = "/Document/'\0'";
    int fd;
    fd = vfs->open(dir, O_RDWR);
    vfs->write(fd, wbuf, strlen(wbuf));
    vfs->lseek(fd, 0, 0);
    vfs->read(fd, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(fd);
    printf("\n");
    printf("#########################\n");
    printf("test_1 end\n");
    printf("#########################\n");
    printf("\n");
    int devnull = vfs->open("/dev/null", 2);
    vfs->read(devnull, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(devnull);
    int devzero = vfs->open("/dev/zero", 2);
    vfs->read(devzero, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(devzero);
    int devrandom = vfs->open("/dev/random", 2);
    vfs->read(devrandom, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(devrandom);
    printf("\n");
    printf("#########################\n");
    printf("test_2 end\n");
    printf("#########################\n");
    printf("\n");
    int cpuinfo=vfs->open("/proc/cpuinfo", 2);
    vfs->read(cpuinfo, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(cpuinfo);
    cpuinfo=vfs->open("/proc/cpuinfo", 2);
    vfs->read(cpuinfo, rbuf, 255);
    printf("%s\n", rbuf);
    vfs->close(cpuinfo);
    int meminfo=vfs->open("/proc/meminfo", 2);
    vfs->read(meminfo, rbuf, 255);
    printf("%s\n", rbuf);
    printf("\n");
    printf("#########################\n");
    printf("file_test end\n");
    printf("#########################\n");
    printf("\n");
}

static void os_run() {
    _intr_write(1);  // enable interrupt
#ifdef FILE_TEST
    file_test();
#endif
    while (1)
        ;  // should never return
}

static _RegSet* os_interrupt(_Event ev, _RegSet* regs) {
    if (ev.event == _EVENT_IRQ_TIMER || ev.event == _EVENT_YIELD) {
        if (current_thread != NULL) {
            current_thread->tregs = regs;
        }
        current_thread = kmt->schedule();
        return current_thread->tregs;
    }
    if (ev.event == _EVENT_IRQ_IODEV)
        _putc('I');
    if (ev.event == _EVENT_ERROR) {
        _putc('x');
        _halt(1);
    }
    return NULL;  // this is allowed by AM
}
