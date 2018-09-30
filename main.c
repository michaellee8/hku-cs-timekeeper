#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>


void handle_process_created(pid_t pid, char *cmd) {
    printf("Process %d created with command %\n", (int) pid, cmd);
    int status;
    waitpid(pid, &status, 0)
}

void run_command(char *argv[]) {
    pid_t pid;
    int status;
    if (*argv[0] == '/' || *argv[0] == '.') {
        char executable_path[PATH_MAX];
        realpath(argv[0], executable_path);
        pid = fork();
        if (pid == 0) {
            int errno = execv(executable_path, argv);
        } else {
            printf("Process %d created with command %\n", (int) pid, argv[0]);
            waitpid(pid, &status, 0);
        }
    } else {
        pid = fork();
        if (pid == 0) {
            int errno = execvp(argv[0], argv);
        } else {
            printf("Process %d created with command %s\n", (int) pid, argv[0]);
            waitpid(pid, &status, 0);
        }
    }
}

int main(int argc, char *argv[]) {
    run_command(argv + 1);
}