#include <os.h>

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

static void os_run() {
    _intr_write(1);  // enable interrupt
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
