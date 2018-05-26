#include "mylibc.h"

static char sprint_buf[1024];

int printf(const char* fmt, ...) {
    va_list args;
    int n;
    va_start(args, fmt);
    n = vsprintf(sprint_buf, fmt, args);
    for (int j = 0; j < n; j++)
        _putc(sprint_buf[j]);
    va_end(args);
    return n;
}
