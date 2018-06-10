#include "kvdb.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/file.h>

static char _key[MAX_N], _value[MAX_N];
static char _input[MAX_N], _buffer[MAX_N];
static FILE* backup;

char* read_line(char* buf, FILE* fp) {
    char* ret;
    if ((ret = fgets(buf, MAX_N, fp)) == NULL && ferror(fp)) {
        perror("Fail to readline");
        exit(1);
    }
    int buf_size = strlen(buf);
    buf[buf_size - 1] = '\0';
    return ret;
}

void setlock(int fd, int option) {
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_pid = getpid();
    if (option == 0) {
        lock.l_type = F_RDLCK;
        fcntl(fd, F_SETLK, &lock);
    } else if (option == 1) {
        lock.l_type = F_WRLCK;
        fcntl(fd, F_SETLKW, &lock);
    }
}

void unlock(int fd) {
    struct flock lock;
    lock.l_whence = SEEK_SET;
    lock.l_start = 0;
    lock.l_len = 0;
    lock.l_type = F_UNLCK;
    lock.l_pid = getpid();
    fcntl(fd, F_SETLKW, &lock);
}

void recov(kvdb_t* db) {
    unlock(db->fp->_fileno);
    if (fclose(db->fp) != 0) {
        perror("Fail to fclose db before recovery_begin");
        exit(1);
    }
    if ((db->fp = fopen(db->name, "w")) == NULL) {
        perror("Fail to fopen db when recover");
        exit(1);
    }
    setlock(db->fp->_fileno, 1);
    if ((backup = fopen("backup", "r")) == NULL) {
        unlock(db->fp->_fileno);
        if (fclose(db->fp) != 0) {
            perror("Fail to fclose db after recovery_begin");
            exit(1);
        }
        return;
    }
    setlock(backup->_fileno, 0);
    if (fseek(backup, 0, SEEK_SET) != 0) {
        perror("Fail to fseek backup when recover");
        exit(1);
    }

    int begin_flag = 0;
    int sprintf_offset = 0;
    _buffer[0] = '\0';
    while (read_line(_input, backup) != NULL) {
        if (strcmp(_input, "Backup_begin") == 0) {
            begin_flag = 1;
            sprintf_offset = 0;
            _buffer[0] = '\0';
            continue;
        } else {
            if (begin_flag == 0)
                continue;
            else if (strcmp(_input, "Backup_end") == 0) {
                fwrite(_buffer, 1, strlen(_buffer), db->fp);
                if (ferror(db->fp)) {
                    perror("Fail to fwrite from backup to db when recover");
                    exit(1);
                }
                begin_flag = 0;
                _buffer[0] = '\0';
                sprintf_offset = 0;
                continue;
            } else {
                int sub_offset =
                    sprintf(_buffer + sprintf_offset, "%s\n", _input);
                if (sub_offset == -1) {
                    perror(
                        "Fail to sprintf from _input to _buffer when recover");
                    exit(1);
                }
                sprintf_offset += sub_offset;
                continue;
            }
        }
    }
    unlock(backup->_fileno);
    if (fclose(backup) != 0) {
        perror("Fail to fclose backup after recovery");
        exit(1);
    }

    unlock(db->fp->_fileno);
    if (fclose(db->fp) != 0) {
        perror("Fail to fclose db after recovery");
        exit(1);
    }
}

int kvdb_open(kvdb_t* db, const char* filename) {
    pthread_mutex_init(&db->mutex, NULL);
    if (pthread_mutex_lock(&db->mutex) != 0) {
        perror("Fail to lock when kvdb_open");
        return -1;
    }
    int flag = 0;
    strcpy(db->name, filename);
    if ((db->fp = fopen(filename, "a+")) == NULL) {
        perror("Fail to fopen before recovery");
        flag = -1;
    }
    setlock(db->fp->_fileno, 0);
    recov(db);
    if ((db->fp = fopen(filename, "a+")) == NULL) {
        perror("Fail to fopen after recovery");
        flag = -1;
    }
    setlock(db->fp->_fileno, 0);
    setlock(db->fp->_fileno, 1);
    if (pthread_mutex_unlock(&db->mutex) != 0) {
        perror("Fail to unlock when kvdb_open");
        return -1;
    }
    return flag;
}

int kvdb_close(kvdb_t* db) {
    if (pthread_mutex_lock(&db->mutex) != 0) {
        perror("Fail to lock when kvdb_close");
        return -1;
    }
    int flag = 0;
    unlock(db->fp->_fileno);
    if (fclose(db->fp) != 0) {
        perror("Fail to fclose when kvdb_close");
        flag = -1;
    }
    if (pthread_mutex_unlock(&db->mutex) != 0) {
        perror("Fail to unlock when kvdb_close");
        return -1;
    }
    return flag;
}

int kvdb_put(kvdb_t* db, const char* key, const char* value) {
    if (pthread_mutex_lock(&db->mutex) != 0) {
        perror("Fail to lock when kvdb_put");
        return -1;
    }
    int flag = 0;
    if ((backup = fopen("backup", "a+")) == NULL) {
        perror("Fail to fopen backup when kvdb_put");
        flag = -1;
    }
    setlock(backup->_fileno, 0);
    setlock(backup->_fileno, 1);
    if (fseek(backup, 0, SEEK_END) != 0) {
        perror("Fail to fseek when kvdb_put");
        flag = -1;
    }
    fwrite("Backup_begin\n", 1, strlen("Backup_begin\n"), backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwrite backup_begin when kvdb_put");
        flag = -1;
    }
    fwrite(key, 1, strlen(key), backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwrite backup_key when kvdb_put");
        flag = -1;
    }
    fwrite("\n", 1, 1, backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwrite backup_key_return when kvdb_put");
        flag = -1;
    }
    fwrite(value, 1, strlen(value), backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwrite backup_value when kvdb_put");
        flag = -1;
    }
    fwrite("\n", 1, 1, backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwite backup_value_return when kvdb_put");
        flag = -1;
    }
    fwrite("Backup_end\n", 1, strlen("Backup_end\n"), backup);
    fdatasync(backup->_fileno);
    if (ferror(backup)) {
        perror("Fail to fwrite backup_end");
        flag = -1;
    }
    unlock(backup->_fileno);
    if (fclose(backup) != 0) {
        perror("Fail to fclose backup when kvdb_put");
        flag = -1;
    }
    fwrite(key, 1, strlen(key), db->fp);
    fdatasync(db->fp->_fileno);
    if (ferror(db->fp)) {
        perror("Fail to fwrite key when kvdb_put");
        recov(db);
        flag = -1;
    }
    fwrite("\n", 1, 1, db->fp);
    fdatasync(db->fp->_fileno);
    if (ferror(db->fp)) {
        perror("Fail to fwrite key_return when kvdb_put");
        recov(db);
        flag = -1;
    }
    fwrite(value, 1, strlen(value), db->fp);
    fdatasync(db->fp->_fileno);
    if (ferror(db->fp)) {
        perror("Fail to fwrite value when kvdb_put");
        recov(db);
        flag = -1;
    }
    fwrite("\n", 1, 1, db->fp);
    fdatasync(db->fp->_fileno);
    if (ferror(db->fp)) {
        perror("Fail to fwrite value_return when kvdb_put");
        recov(db);
        flag = -1;
    }
    if (pthread_mutex_unlock(&db->mutex) != 0) {
        perror("Fail to unlock when kvdb_put");
        return -1;
    }
    return flag;
}

char* kvdb_get(kvdb_t* db, const char* key) {
    if (pthread_mutex_lock(&db->mutex) != 0) {
        perror("Fail to lock when kvdb_get");
        return NULL;
    }
    char* ret = NULL;
    if (fseek(db->fp, 0, SEEK_SET) != 0) {
        perror("Fail to fseek when kvdb_get");
        ret = NULL;
    } else {
        while (1) {
            if (!fgets(_key, sizeof(_key), db->fp)) {
                break;
            }
            if (!fgets(_value, sizeof(_value), db->fp)) {
                break;
            }
            _key[strlen(_key) - 1] = '\0';
            _value[strlen(_value) - 1] = '\0';
            if (strcmp(key, _key) == 0) {
                if (!ret)
                    free(ret);
                ret = strdup(_value);
            }
        }
        if (ferror(db->fp)) {
            perror("Fail to fgets when kvdb_get");
            ret = NULL;
        }
    }
    if (pthread_mutex_unlock(&db->mutex) != 0) {
        perror("Fail to unlock when kvdb_get");
        return NULL;
    }
    return ret;
}
