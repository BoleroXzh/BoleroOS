#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/ptrace.h>
#include <regex.h>

#define MAX_LEN 100
#define MAX_NUM 100

char command[MAX_LEN];
int paused = 0, first_time = 1;
int addr_head = 0, addr_tail = 0;
int addr_list[MAX_NUM], addr_num = 0;

int find_data_addr(char* argv[]) {
    char path[MAX_LEN], buf[MAX_LEN];
    regex_t reg1, reg2;
    regmatch_t match[1];
    const char* pattern1 = "rw-p";
    const char* pattern2 = "heap";
    strcpy(path, "/proc/");
    strcat(path, argv[1]);
    strcat(path, "/maps");
    FILE* fp;
    if ((fp = fopen(path, "r")) == NULL) {
        return 0;
    }
    regcomp(&reg1, pattern1, REG_EXTENDED);
    regcomp(&reg2, pattern2, REG_EXTENDED);
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        if (regexec(&reg2, buf, 1, match, 0) == 0) {
            break;
        }
        if (regexec(&reg1, buf, 1, match, 0) == 0 &&
            regexec(&reg2, buf, 1, match, 0) != 0) {
            char begin[20], end[20];
            int offset = 0;
            while (buf[offset] != '-') {
                begin[offset] = buf[offset];
                offset++;
            }
            begin[offset++] = '\0';
            int t = offset;
            while (buf[offset] != ' ') {
                end[offset - t] = buf[offset];
                offset++;
            }
            end[offset] = '\0';
            int current_addr_head, current_addr_tail;
            sscanf(begin, "%x", &current_addr_head);
            sscanf(end, "%x", &current_addr_tail);
            if (addr_head == 0 || addr_head > current_addr_head) {
                addr_head = current_addr_head;
            }
            if (addr_tail == 0 || addr_tail < current_addr_tail) {
                addr_tail = current_addr_tail;
            }
        }
    }
    regfree(&reg1);
    regfree(&reg2);
    (void)fclose(fp);
    return 1;
}

int inputcheck(int argc, char* argv[]) {
    if (argc != 2 || argv[1][0] < '0' || argv[1][0] > '9') {
        return 0;
    }
    char path[MAX_LEN];
    strcpy(path, "/proc/");
    strcat(path, argv[1]);
    if (fopen(path, "r") == NULL) {
        return 0;
    }
    return 1;
}

void ReadMe() {
    printf(
        "This program supports the following features (In fact, it can only "
        "change the score.)\n");
    printf("help               Help Manual\n");
    printf("pause              Pause the process.\n");
    printf("resume             Resume the process.\n");
    printf(
        "lookup <number>    Search for the address of all memory with the "
        "value of <number>.\n");
    printf(
        "setup <number>     Set the value of the target variable to "
        "<number>.\n");
    printf("exit               Exit this program.\n");
}

void pause(int pid) {
    if (paused == 1) {
        printf("The game has been paused!\n");
        return;
    }
    int temp = ptrace(PTRACE_ATTACH, pid, NULL, NULL);
    if (temp == -1) {
        printf("Fail to pause the game!\n");
        return;
    }
    paused = 1;
    printf("The game is paused.\n");
}

void resume(int pid) {
    if (paused == 0) {
        printf("The game is running!\n");
    }
    int temp = ptrace(PTRACE_DETACH, pid, NULL, NULL);
    if (temp == -1) {
        printf("Fail to resume the game!\n");
        return;
    }
    paused = 0;
    printf("The game is resumed.\n");
}

void first_lookup(int pid, int number) {
    addr_num = 0;
    for (int i = addr_head; i < addr_tail; i++) {
        int data = ptrace(PTRACE_PEEKDATA, pid, i, NULL);
        if (data == number) {
            printf("address : %d      value : %d\n", i, data);
            addr_list[++addr_num] = i;
        }
    }
    if (addr_num == 0) {
        printf("Can not find such value!\n");
        return;
    } else {
        first_time = 0;
    }
}

void continue_lookup(int pid, int number) {
    int current_num = 0;
    int current_list[MAX_NUM];
    for (int i = 1; i <= addr_num; i++) {
        int data = ptrace(PTRACE_PEEKDATA, pid, addr_list[i], NULL);
        if (data == number) {
            printf("address : %d      value : %d\n", addr_list[i], data);
            current_list[++current_num] = addr_list[i];
        }
    }
    addr_num = current_num;
    if (addr_num == 0) {
        printf("Can not find such value!\n");
        first_time = 1;
    } else {
        for (int i = 1; i <= addr_num; i++) {
            addr_list[i] = current_list[i];
        }
    }
}

void lookup(int pid, int number) {
    if (first_time) {
        first_lookup(pid, number);
    } else {
        continue_lookup(pid, number);
    }
}

void setup(int pid, int number) {
    if (addr_num < 1) {
        printf("There is no target address!\n");
        return;
    }
    if (addr_num > 1) {
        printf("There are too many target addresses!\n");
        return;
    }
    if (addr_num == 1) {
        int temp = ptrace(PTRACE_POKEDATA, pid, addr_list[1], number);
        if (temp == -1) {
            printf("Fail to set the value!\n");
            return;
        }
        int data = ptrace(PTRACE_PEEKDATA, pid, addr_list[1], NULL);
        printf("The value has been changed to %d.\n", data);
        first_time = 1;
    }
}

int main(int argc, char* argv[]) {
    if (inputcheck(argc, argv) == 0) {
        printf(
            "Please check the pid and use <sudo ./memhack [PID]> to run this "
            "memhack!\n");
        return 0;
    }
    int pid = atoi(argv[1]);
    if (!find_data_addr(argv)) {
        printf("Can not open the maps!\n");
        return 0;
    }
    ReadMe();
    while (1) {
        if (fgets(command, sizeof(command), stdin) != NULL) {
            if (strncmp("help", command, 4) == 0) {
                ReadMe();
            }
            if (strncmp("exit", command, 4) == 0) {
                return 0;
            }
            if (strncmp("pause", command, 5) == 0) {
                pause(pid);
            }
            if (strncmp("resume", command, 6) == 0) {
                resume(pid);
            }
            if (strncmp("lookup", command, 6) == 0) {
                char* raw_number = strtok(command, " ");
                raw_number = strtok(NULL, " ");
                int number = atoi(raw_number);
                lookup(pid, number);
            }
            if (strncmp("setup", command, 5) == 0) {
                char* raw_number = strtok(command, " ");
                raw_number = strtok(NULL, " ");
                int number = atoi(raw_number);
                setup(pid, number);
            }
        }
    }
    return 0;
}
