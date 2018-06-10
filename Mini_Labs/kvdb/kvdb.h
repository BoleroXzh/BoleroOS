#include <stdio.h>
#include <pthread.h>
#define MAX_NAME_LEN 1024
#define MAX_N 1 << 20

struct kvdb {
    FILE* fp;
    char name[MAX_NAME_LEN];
    pthread_mutex_t mutex;
};
typedef struct kvdb kvdb_t;

int kvdb_open(kvdb_t* db, const char* filename);
int kvdb_close(kvdb_t* db);
int kvdb_put(kvdb_t* db, const char* key, const char* value);
char* kvdb_get(kvdb_t* db, const char* key);