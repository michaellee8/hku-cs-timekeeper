#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <linux/limits.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <signal.h>

const int MSG_MAX = PATH_MAX + 100;

#define READ_END 0
#define WRITE_END 1
#define MAX_ARGUMENTS 100
#define MAX_CMD 100

double parse_timeval(struct timeval tv) {
    return (double) (tv.tv_usec) / 1000000 +
           (double) (tv.tv_sec);
}

double diff_timespec(struct timespec tv1, struct timespec tv2) {
    return (double) (tv2.tv_nsec - tv1.tv_nsec) / 1000000000 +
           (double) (tv2.tv_sec - tv1.tv_sec);
}

void handle_process_created(pid_t pid, char *cmd, struct timespec start_time) {
    printf("Process %d created with command %s\n", (int) pid, cmd);
    int status;
    struct rusage resource_usage;
    wait4(pid, &status, WUNTRACED, &resource_usage);
    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);
    const double running_time = diff_timespec(start_time, end_time);
    printf("\n");
    // No signal name available because `sys_signame` is not implemented in linux!
    if (WIFEXITED(status)) {
        printf("The command \"%s\" terminated with returned status code = %d", cmd, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("The command \"%s\" is interrupted by the signal number = %d (%s: %s)", cmd, WTERMSIG(status),
               sys_siglist[WTERMSIG(status)], strsignal(WTERMSIG(status)));
    } else if (WIFSTOPPED(status)) {
        printf("The command \"%s\" is stopped (can be continued with fg/bg) by the signal number = %d (%s: %s)", cmd,
               WSTOPSIG(status),
               sys_siglist[WTERMSIG(status)], strsignal(WSTOPSIG(status)));
    }
    printf("\nreal: %.2f, user: %.2f, system: %.2f, context switch: %ld\n", running_time,
           parse_timeval(resource_usage.ru_utime), parse_timeval(resource_usage.ru_stime),
           resource_usage.ru_nivcsw + resource_usage.ru_nvcsw);

}

void handle_exec_err(char *cmd) {
    char msg[MSG_MAX];
    strcat(msg, "timekeeper experienced an error in starting the command (exec) ");
    strcat(msg, cmd);
    perror(msg);
}

void handle_fork_err(char *cmd) {
    char msg[MSG_MAX];
    strcat(msg, "timekeeper experienced an error in starting the command (fork) ");
    strcat(msg, cmd);
    perror(msg);
}

void close_everything(int fd_read[2], int fd_write[2]) {
    close(fd_read[READ_END]);
    close(fd_read[WRITE_END]);
    close(fd_write[READ_END]);
    close(fd_write[WRITE_END]);
}

void run_command(char *argv[], int fd_read[2], int fd_write[2]) {
    pid_t pid;
    if (*argv[0] == '/' || *argv[0] == '.') {
        char executable_path[PATH_MAX];
        realpath(argv[0], executable_path);
        struct timespec start_time;
        clock_gettime(CLOCK_REALTIME, &start_time);
        pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            dup2(fd_read[READ_END], STDIN_FILENO);
            dup2(fd_write[WRITE_END], STDOUT_FILENO);
            close_everything(fd_read, fd_write);
            execv(executable_path, argv);
            handle_exec_err(executable_path);
        } else if (pid < 0) {
            close_everything(fd_read, fd_write);
            handle_fork_err(executable_path);
        } else {
            close_everything(fd_read, fd_write);
            handle_process_created(pid, executable_path, start_time);
        }
    } else {
        struct timespec start_time;
        clock_gettime(CLOCK_REALTIME, &start_time);
        pid = fork();
        if (pid == 0) {
            signal(SIGINT, SIG_DFL);
            dup2(fd_read[READ_END], STDIN_FILENO);
            dup2(fd_write[WRITE_END], STDOUT_FILENO);
            close_everything(fd_read, fd_write);
            execvp(argv[0], argv);
            handle_exec_err(argv[0]);
        } else if (pid < 0) {
            close_everything(fd_read, fd_write);
            handle_fork_err(argv[0]);
        } else {
            close_everything(fd_read, fd_write);
            handle_process_created(pid, argv[0], start_time);
        }
    }
}

int main(int argc, char *argv[]) {
    signal(SIGINT, SIG_IGN);
    if (argc <= 1) {
        return 0;
    }
    char *command_to_execute[MAX_ARGUMENTS];
    int current_command = 0;
    int *all_fds[MAX_CMD];
    for (int i = 1; i < argc; i++) {
        if (argv[i][0]!='!'){

        }else{
            run_command(command_to_execute,)
        }
    }
}