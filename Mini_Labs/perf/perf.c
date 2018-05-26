#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>

struct process {
    int num;
    char name[2000][100];
    char buf[2000];
    double time[2000];
    double proportion[2000];
} proc;

int fd[2];

void child_proc(int argc, char* argv[]) {
    argv[0] = "strace";
    argv[1] = "-T";
    for (int i = argc; i >= 2; i--)
        argv[i] = argv[i - 1];
    argv[argc + 1] = NULL;
    char* envp[] = {"PATH=/bin", NULL};
    dup2(fd[1], 2);
    int null = open("/dev/null", O_WRONLY | O_APPEND);
    dup2(null, 1);
    execve("/usr/bin/strace", argv, envp);
    printf("Execve fail.\n");
}

void parent_proc() {
    int i, buf_size = strlen(proc.buf), time_location, flag = 0;
    double time = 0.0;
    char name[100], raw_time[100];
    for (i = 0; i < buf_size; i++) {
        if (proc.buf[i] == '(')
            break;
        name[i] = proc.buf[i];
    }
    name[i] = '\0';
    for (time_location = buf_size - 2; time_location >= 0; time_location--)
        if (proc.buf[time_location] == '<') {
            time_location++;
            break;
        }
    for (i = time_location; i <= buf_size - 3; i++)
        raw_time[i - time_location] = proc.buf[i];
    if (strlen(raw_time) < 2)
        return;
    time = atof(raw_time);
    for (i = 0; i < proc.num; i++)
        if (!strcmp(name, proc.name[i])) {
            proc.time[i] += time;
            flag = 1;
            break;
        }
    if (!flag) {
        strcpy(proc.name[proc.num], name);
        proc.time[proc.num] = time;
        proc.num++;
    }
}

void calculation() {
    double sum = 0.0;
    for (int i = 0; i < proc.num; i++)
        sum += proc.time[i];
    for (int i = 0; i < proc.num; i++)
        proc.proportion[i] = proc.time[i] / sum * 100.0;
    for (int i = 0; i < proc.num; i++)
        printf("%-20s:%.3f%%\n", proc.name[i], proc.proportion[i]);
}

int main(int argc, char* argv[]) {
    int fpid;
    if (pipe(fd) != 0) {
        printf("Pipe create error.\n");
        return 0;
    }
    if (argc < 2) {
        printf("Need more command.\n");
        return 0;
    }
    fpid = fork();
    if (fpid < 0)
        printf("Error in fork.\n");
    else if (fpid == 0) {
        close(fd[0]);
        child_proc(argc, argv);
    } else {
        close(fd[1]);
        dup2(fd[0], 0);
        while (fgets(proc.buf, sizeof(proc.buf), stdin) != NULL)
            parent_proc();
        calculation();
    }

    return 0;
}
