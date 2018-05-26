#ifndef _MYLIBC_H_
#define _MYLIBC_H_

#include <am.h>
#include <amdev.h>
#include <arch.h>
#include <stdarg.h>

// stdlib.h

void srand(unsigned int seed);
long int rand();

// stdio.h

int printf(const char* fmt, ...);
int vsprintf(char* buf, const char* fmt, va_list args);

// string.h

void* memset(void* b, int c, size_t n);
void* memcpy(void* dst, const void* src, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dst, const char* src);
char* strncpy(char* dst, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);

// ioe.h

uint32_t uptime();
_KbdReg read_key();
void draw_rect(uint32_t* pixels, int x, int y, int w, int h);
void draw_sync();
int screen_width();
int screen_height();

#endif