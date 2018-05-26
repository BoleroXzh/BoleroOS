#include "mylibc.h"

void* memset(void* s, int c, size_t n) {
    char* xs = s;
    while (n--)
        *xs++ = c;
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    char* tmp = dest;
    const char* s = src;
    while (n--)
        *tmp++ = *s++;
    return dest;
}

size_t strlen(const char* s) {
    const char* sc;
    for (sc = s; *sc != '\0'; ++sc)
        /* nothing */;
    return sc - s;
}

char* strcpy(char* dest, const char* src) {
    char* tmp = dest;
    while ((*dest++ = *src++) != '\0')
        /* nothing */;
    return tmp;
}

char* strncpy(char* dest, const char* src, size_t n) {
    char* tmp = dest;
    while (n) {
        if ((*tmp = *src) != 0)
            src++;
        tmp++;
        n--;
    }
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    unsigned char c1, c2;
    while (1) {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
    }
    return 0;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    unsigned char c1, c2;
    while (n) {
        c1 = *s1++;
        c2 = *s2++;
        if (c1 != c2)
            return c1 < c2 ? -1 : 1;
        if (!c1)
            break;
        n--;
    }
    return 0;
}
