#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>

int expr_id = 0;
char command[1000], expr_name[1000], expr_func[1000];
FILE* fp;

void addfunc(char* func) {
    if ((fp = fopen("/tmp/test.c", "w")) == NULL) {
        printf("Cannot create temporary file.\n");
        return;
    } else {
        fprintf(fp, "%s", func);
        fclose(fp);
        if (system("gcc -shared -fPIC /tmp/test.c -o /tmp/test.so -ldl") != 0)
            return;
    }
    if ((fp = fopen("/tmp/lib.c", "a+")) == NULL) {
        printf("Cannot create temporary file.\n");
        return;
    } else {
        fprintf(fp, "%s", func);
        fclose(fp);
    }
}

void addexpr(char* expr) {
    if (expr[strlen(expr) - 1] == '\n')
        expr[strlen(expr) - 1] = '\0';
    sprintf(expr_name, "_expr_wrap_%d", expr_id++);
    sprintf(expr_func, "int %s() { return %s; }", expr_name, expr);
    if (system("cp /tmp/lib.c /tmp/exec.c") != 0) {
        printf("Cannot open the temporary file.\n");
        return;
    }
    if ((fp = fopen("/tmp/exec.c", "a+")) == NULL) {
        printf("Cannot open the temporary file.\n");
        return;
    }
    fprintf(fp, "%s", expr_func);
    fclose(fp);
    if (system("gcc -shared -fPIC /tmp/exec.c -o /tmp/exec.so -ldl") != 0)
        return;
    void* handle = dlopen("/tmp/exec.so", RTLD_LAZY);
    if (handle == NULL) {
        printf("Cannot load module.\n");
        return;
    }
    int (*exprfunc)();
    exprfunc = dlsym(handle, expr_name);
    if (exprfunc == NULL) {
        printf("%s\n", dlerror());
        return;
    } else
        printf("%s = %d\n", expr, exprfunc());
    dlclose(handle);
}

int main() {
    fp = fopen("/tmp/lib.c", "w");
    fclose(fp);
    while (1) {
        printf(">>> ");
        if (fgets(command, sizeof(command), stdin) != NULL) {
            if (strncmp("\n", command, 1) == 0)
                continue;
            if (strncmp("exit", command, 4) == 0)
                return 0;
            else if (strncmp("int", command, 3) == 0)
                addfunc(command);
            else
                addexpr(command);
        }
    }
    return 0;
}
