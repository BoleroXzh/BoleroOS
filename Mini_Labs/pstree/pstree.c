#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <dirent.h>

#define MAX_PROC_NUM 2048
#define FILE_LEN 1024
#define PROC_NAME_LEN 200
#define ID_LEN 20
#define DEFAULT_DIR "/proc/"
#define DEFAULT_DIR2 "/task/"

typedef struct Proc {
    int pid, ppid, rec, vis, hidden;
    char name[PROC_NAME_LEN];
} Proc;

typedef struct Global_Var {
    int total_proc_num, _p_flag, start_ppid, first_build_flag;
    char proc_name_list[MAX_PROC_NUM][PROC_NAME_LEN],
        hidden_proc_name_list[MAX_PROC_NUM][PROC_NAME_LEN];
    Proc proc_list[MAX_PROC_NUM];
} Global_Var;
Global_Var g;

void Build_Print_Pstree(int ppid, int rec) {
    int i;
    if (g.first_build_flag == 1 && g.start_ppid != 0) {
        int j;
        for (j = 0; j < g.total_proc_num; j++) {
            if (g.proc_list[j].pid == ppid) {
                if (g._p_flag == 1)
                    printf("|--%s(%d)\n", g.proc_list[j].name,
                           g.proc_list[j].pid);
                else
                    printf("|--%s\n", g.proc_list[j].name);
                rec++;
                g.first_build_flag = 0;
                break;
            }
        }
    }
    for (i = 0; i < g.total_proc_num; i++) {
        int j;
        if ((g.proc_list[i].vis == 0) && g.proc_list[i].ppid == ppid) {
            g.proc_list[i].vis = 1;
            g.proc_list[i].rec = rec + 1;
            for (j = 0; j < rec; j++)
                printf("    ");
            if (g.proc_list[i].pid == 1)
                strcpy(g.proc_list[i].name, "systemd");
            if (g.proc_list[i].pid == 0)
                continue;
            if (g.proc_list[i].hidden == 1) {
                if (g._p_flag == 1)
                    printf("|--{%s}(%d)\n", g.proc_list[i].name,
                           g.proc_list[i].pid);
                else
                    printf("|--{%s}\n", g.proc_list[i].name);
            } else {
                if (g._p_flag == 1)
                    printf("|--%s(%d)\n", g.proc_list[i].name,
                           g.proc_list[i].pid);
                else
                    printf("|--%s\n", g.proc_list[i].name);
            }
            Build_Print_Pstree(g.proc_list[i].pid, g.proc_list[i].rec);
        }
    }
}

void Output_Usage() {
    printf(
        "Usage: pstree [ -p ] [ -n ] [USER]\n       pstree -V\nDisplay a tree "
        "of processes.\n\n -n, --numeric-sort\tsort output by PID\n -p, "
        "--show-pids\tshow PIDs; implies -c\n -V, --version\tdisplay version "
        "information\n PID\tstart at this PID; default is 1 (init)\n");
}

void Output_Version() {
    printf("pstree 1.0\nCopyright (C) 2018 Yi-cheng Huang\n");
}

void Process_Command_n(int argc, char* argv[]) {
    if (argc >= 2 &&
        (strcmp(argv[1], "-n") == 0 || strcmp(argv[1], "-np") == 0 ||
         strcmp(argv[1], "-pn") == 0)) {
        for (int i = 0; i < g.total_proc_num; i++) {
            for (int j = 0; j < g.total_proc_num; j++) {
                if (g.proc_list[i].pid < g.proc_list[j].pid) {
                    Proc temp_proc;
                    temp_proc.pid = g.proc_list[j].pid;
                    temp_proc.ppid = g.proc_list[j].ppid;
                    temp_proc.rec = g.proc_list[j].rec;
                    temp_proc.vis = g.proc_list[j].vis;
                    temp_proc.hidden = g.proc_list[j].hidden;
                    g.proc_list[j].pid = g.proc_list[i].pid;
                    g.proc_list[j].ppid = g.proc_list[i].ppid;
                    g.proc_list[j].rec = g.proc_list[i].rec;
                    g.proc_list[j].vis = g.proc_list[i].vis;
                    g.proc_list[j].hidden = g.proc_list[i].hidden;
                    g.proc_list[i].pid = temp_proc.pid;
                    g.proc_list[i].ppid = temp_proc.ppid;
                    g.proc_list[i].rec = temp_proc.rec;
                    g.proc_list[i].vis = temp_proc.vis;
                    g.proc_list[i].hidden = temp_proc.hidden;
                    strcpy(temp_proc.name, g.proc_list[j].name);
                    strcpy(g.proc_list[j].name, g.proc_list[i].name);
                    strcpy(g.proc_list[i].name, temp_proc.name);
                }
            }
        }
    }
}

int Process_Command_PID(int argc, char* argv[]) {
    if (argc == 3) {
        int arg_len = strlen(argv[2]);
        int temp_num, t, k;
        for (t = 0; t < arg_len; t++) {
            if (argv[2][t] < '0' || argv[2][t] > '9') {
                Output_Usage();
                return -1;
            }
        }
        temp_num = atoi(argv[2]);
        if (temp_num != 0) {
            for (k = 0; k < g.total_proc_num; k++) {
                if (g.proc_list[k].pid == temp_num) {
                    g.start_ppid = temp_num;
                    break;
                }
            }
            if (k == g.total_proc_num) {
                printf("Cannot find the pid you input!\n");
                return -1;
            }
        }
    }
    return 0;
}

int Process_Command(int argc, char* argv[]) {
    int i;
    for (i = 0; i < argc; i++) {
        assert(argv[i]);
    }
    assert(!argv[argc]);
    if (argc >= 4) {
        Output_Usage();
        return -1;
    }
    if (argc == 2 &&
        (strcmp(argv[1], "-V") == 0 || strcmp(argv[1], "--version") == 0)) {
        Output_Version();
        return -1;
    }
    if (argc >= 2) {
        if ((strcmp(argv[1], "-p") == 0 ||
             strcmp(argv[1], "--show-pids") == 0 ||
             strcmp(argv[1], "-np") == 0 || strcmp(argv[1], "-pn") == 0))
            g._p_flag = 1;
        else if (strcmp(argv[1], "-n") != 0 &&
                 strcmp(argv[1], "--numeric-sort") != 0) {
            Output_Usage();
            return -1;
        }
    }
    return 0;
}

int Read_Proc_List() {
    int proc_num = 0, i;
    struct dirent* dp;
    DIR* dirp = opendir(DEFAULT_DIR);
    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] >= '0' && dp->d_name[0] <= '9') {
            strcpy(g.proc_name_list[proc_num], dp->d_name);
            proc_num++;
        }
    }
    (void)closedir(dirp);
    for (i = 0; i < proc_num; i++) {
        char path[MAX_PROC_NUM], file_content[FILE_LEN],
            file_name[PROC_NAME_LEN], ppid[ID_LEN];
        FILE* fp;
        if (atoi(g.proc_name_list[i]) != 0)
            g.proc_list[i].pid = atoi(g.proc_name_list[i]);
        g.proc_list[i].hidden = 0;
        strcpy(path, DEFAULT_DIR);
        strcat(path, g.proc_name_list[i]);
        strcat(path, "/status");
        fp = fopen(path, "r");
        while (!feof(fp)) {
            fgets(file_content, FILE_LEN, fp);
            int size = strlen(file_content);
            int j, k;
            if (strncmp(file_content, "PPid", 4) == 0) {
                for (j = 0; j < size; j++)
                    if (file_content[j] >= '0' && file_content[j] <= '9')
                        break;
                for (k = 0; k < size - j; k++) {
                    ppid[k] = file_content[j + k];
                }
            }
            if (atoi(ppid) != 0)
                g.proc_list[i].ppid = atoi(ppid);
            if (strncmp(file_content, "Name", 4) == 0) {
                for (j = 4; j < size; j++)
                    if (file_content[j] >= 'a' && file_content[j] <= 'z')
                        break;
                for (k = 0; k < size; k++)
                    file_name[k] = file_content[j + k];
                file_name[k - 1] = '\0';
            }
            char* temp_p = file_name + strlen(file_name) - 1;
            while (isspace(*temp_p)) {
                *temp_p = '\0';
                temp_p--;
            }
            strcpy(g.proc_list[i].name, file_name);
        }
        (void)fclose(fp);
    }
    return proc_num;
}

int Read_Hidden_Proc_List(int proc_num) {
    int hidden_proc_num = 0;
    struct dirent *dp, *dp2;
    DIR* dirp = opendir(DEFAULT_DIR);
    dirp = opendir(DEFAULT_DIR);
    while ((dp = readdir(dirp)) != NULL) {
        if (dp->d_name[0] >= '0' && dp->d_name[0] <= '9') {
            char path[MAX_PROC_NUM];
            strcpy(path, DEFAULT_DIR);
            strcat(path, dp->d_name);
            strcat(path, DEFAULT_DIR2);
            DIR* dirp2 = opendir(path);
            while ((dp2 = readdir(dirp2)) != NULL) {
                if (dp2->d_name[0] >= '0' && dp2->d_name[0] <= '9') {
                    if (strcmp(dp2->d_name, dp->d_name) != 0) {
                        strcpy(g.hidden_proc_name_list[hidden_proc_num],
                               dp2->d_name);
                        char sub_path[MAX_PROC_NUM], file_content[FILE_LEN],
                            file_name[PROC_NAME_LEN], ppid[ID_LEN];
                        if (atoi(g.hidden_proc_name_list[hidden_proc_num]) != 0)
                            g.proc_list[proc_num + hidden_proc_num].pid =
                                atoi(g.hidden_proc_name_list[hidden_proc_num]);
                        g.proc_list[proc_num + hidden_proc_num].hidden = 1;
                        strcpy(sub_path, path);
                        strcat(sub_path,
                               g.hidden_proc_name_list[hidden_proc_num]);
                        strcat(sub_path, "/status");
                        FILE* fp = fopen(sub_path, "r");
                        while (!feof(fp)) {
                            fgets(file_content, FILE_LEN, fp);
                            int size = strlen(file_content);
                            int j, k;
                            if (strncmp(file_content, "Tgid", 4) == 0) {
                                for (j = 0; j < size; j++)
                                    if (file_content[j] >= '0' &&
                                        file_content[j] <= '9')
                                        break;
                                for (k = 0; k < size - j; k++) {
                                    ppid[k] = file_content[j + k];
                                }
                            }
                            if (atoi(ppid) != 0)
                                g.proc_list[hidden_proc_num + proc_num].ppid =
                                    atoi(ppid);
                            if (strncmp(file_content, "Name", 4) == 0) {
                                for (j = 4; j < size; j++)
                                    if (file_content[j] >= 'a' &&
                                        file_content[j] <= 'z')
                                        break;
                                for (k = 0; k < size; k++)
                                    file_name[k] = file_content[j + k];
                                file_name[k - 1] = '\0';
                            }
                            char* temp_p = file_name + strlen(file_name) - 1;
                            while (isspace(*temp_p)) {
                                *temp_p = '\0';
                                temp_p--;
                            }
                            strcpy(g.proc_list[hidden_proc_num + proc_num].name,
                                   file_name);
                        }
                        (void)fclose(fp);
                        hidden_proc_num++;
                    }
                }
            }
            (void)closedir(dirp2);
        }
    }
    (void)closedir(dirp);
    return hidden_proc_num;
}

int main(int argc, char* argv[]) {
    int proc_num = 0, hidden_proc_num = 0;
    g._p_flag = 0;
    g.start_ppid = 0;
    g.first_build_flag = 1;
    if (Process_Command(argc, argv) == -1)
        exit(0);
    proc_num = Read_Proc_List();
    hidden_proc_num = Read_Hidden_Proc_List(proc_num);
    g.total_proc_num = hidden_proc_num + proc_num;
    memset(&g.proc_list->vis, 0, g.total_proc_num);
    memset(&g.proc_list->rec, 0, g.total_proc_num);
    if (Process_Command_PID(argc, argv) == -1)
        exit(0);
    Process_Command_n(argc, argv);
    Build_Print_Pstree(g.start_ppid, 0);
    return 0;
}
