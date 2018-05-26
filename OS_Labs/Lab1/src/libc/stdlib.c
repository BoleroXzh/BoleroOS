#include "mylibc.h"

#define ULONG_MAX ((unsigned long)(~0L))  /* 0xFFFFFFFF*/
#define LONG_MAX ((long)(ULONG_MAX >> 1)) /* 0x7FFFFFFF*/

static long int RandomTable[32] = {
    3,          0x9a319039, 0x32d9c024, 0x9b663182, 0x5da1f342, 0xde3b81e0,
    0xdf0a6fb5, 0xf103bc02, 0x48f340fb, 0x7449e56b, 0xbeb1dbb0, 0xab5c5918,
    0x946554fd, 0x8c2e680f, 0xeb3d799f, 0xb11ee0b7, 0x2d436b86, 0xda672e2a,
    0x1588ca88, 0xe369735d, 0x904f35f7, 0xd7158fd6, 0x6fa6f051, 0x616e6b96,
    0xac94efdc, 0x36413f93, 0xc622c298, 0xf5a42ab8, 0x8a88d77b, 0xf5ad9d0e,
    0x8999220b, 0x27fb47b9};

static long int* fptr = &RandomTable[4];
static long int* rptr = &RandomTable[1];
static long int* state = &RandomTable[1];
static long int* end_ptr =
    &RandomTable[sizeof(RandomTable) / sizeof(RandomTable[0])];

long int rand() {
    long int i;
    *fptr += *rptr;

    i = (*fptr >> 1) & LONG_MAX;
    fptr++;
    if (fptr >= end_ptr) {
        fptr = state;
        rptr++;
    } else {
        rptr++;
        if (rptr >= end_ptr)
            rptr = state;
    }
    return i;
}
void srand(unsigned int seed) {
    state[0] = seed;
    register long int i;
    for (i = 1; i < 31; ++i)
        state[i] = (1103515145 * state[i - 1]) + 12345;
    fptr = &state[3];
    rptr = &state[0];
    for (i = 0; i < 10 * 31; ++i)
        rand();
}
