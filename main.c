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
#include <stdbool.h>

const int MSG_MAX = PATH_MAX + 100;

#define READ_END 0
#define WRITE_END 1
#define MAX_ARGUMENTS 100
#define MAX_CMD 100

// Good reference http://www.cs.loyola.edu/~jglenn/702/S2005/Examples/dup2.html



// Get number of seconds from timeval struct
double parse_timeval(struct timeval tv) {
    return (double) (tv.tv_usec) / 1000000 +
           (double) (tv.tv_sec);
}

// Get difference in seconds from timespec struct
double diff_timespec(struct timespec tv1, struct timespec tv2) {
    return (double) (tv2.tv_nsec - tv1.tv_nsec) / 1000000000 +
           (double) (tv2.tv_sec - tv1.tv_sec);
}

// Calculate running statics after process has been created
void handle_process_created(pid_t pid, char *cmd, struct timespec start_time) {

    // Notify process creation
    printf("Process %d created with command %s\n", (int) pid, cmd);

    // Wait for the process to finish and get the resource usage statistics,
    // use of wait4 here may be incompatible in some platforms
    int status;
    struct rusage resource_usage;
    wait4(pid, &status, WUNTRACED, &resource_usage);

    // Record ending time
    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);

    // Calculate running real time of process
    const double running_time = diff_timespec(start_time, end_time);

    // Notify process finished and return code
    // No signal name (like SIGTERM) available because `sys_signame` is not implemented in linux!
    if (WIFEXITED(status)) {
        printf("The command \"%s\" terminated with returned status code = %d\n", cmd, WEXITSTATUS(status));
    } else if (WIFSIGNALED(status)) {
        printf("The command \"%s\" is interrupted by the signal number = %d (%s: %s)\n", cmd, WTERMSIG(status),
               sys_siglist[WTERMSIG(status)], strsignal(WTERMSIG(status)));
    } else if (WIFSTOPPED(status)) {
        printf("The command \"%s\" is stopped (can be continued with fg/bg) by the signal number = %d (%s: %s)\n", cmd,
               WSTOPSIG(status),
               sys_siglist[WTERMSIG(status)], strsignal(WSTOPSIG(status)));
    }

    // Print statistics
    printf("real: %.2f, user: %.2f, system: %.2f, context switch: %ld\n", running_time,
           parse_timeval(resource_usage.ru_utime), parse_timeval(resource_usage.ru_stime),
           resource_usage.ru_nivcsw + resource_usage.ru_nvcsw);

}

// Handle exec errors
void handle_exec_err(char *cmd) {
    char msg[MSG_MAX];
    strcat(msg, "timekeeper experienced an error in starting the command (exec) ");
    strcat(msg, cmd);
    perror(msg);
}

// Handle fork errors
void handle_fork_err(char *cmd) {
    char msg[MSG_MAX];
    strcat(msg, "timekeeper experienced an error in starting the command (fork) ");
    strcat(msg, cmd);
    perror(msg);
}


// Fork and run a command
void run_command(char *argv[], int i_cmd, int pipes[][2], int n_pipes) {

    pid_t pid;
    bool from_path = false;
    char executable_path[PATH_MAX];

    // Find the executable out if necessary
    if (*argv[0] == '/' || *argv[0] == '.') {
        realpath(argv[0], executable_path);
        from_path = false;
    } else {
        strcpy(executable_path, argv[0]);
        from_path = true;
    }

    // Start recording time
    struct timespec start_time;
    clock_gettime(CLOCK_REALTIME, &start_time);

    pid = fork();

    if (pid == 0) { // Child
        // Set response to SIGINT
        signal(SIGINT, SIG_DFL);

        // Handle close and dup2 pipes
        if (n_pipes == 0) { // No pipes for 1 command
            // Do nothing
        } else if (i_cmd == 0) { // First command
            dup2(pipes[i_cmd][WRITE_END], STDOUT_FILENO);
            // Close all pipes except dup-ed one
            for (int i = 0; i < n_pipes; i++) {
                for (int j = 0; j < 2; j++) {
                    if (i == i_cmd && j == WRITE_END) {
                        // Do nothing
                    } else {
                        close(pipes[i][j]);
                    }
                }
            }
        } else if (i_cmd == n_pipes) {// Last command
            dup2(pipes[i_cmd - 1][READ_END], STDIN_FILENO);
            // Close all pipes except dup-ed one
            for (int i = 0; i < n_pipes; i++) {
                for (int j = 0; j < 2; j++) {
                    if (i == i_cmd - 1 && j == READ_END) {
                        // Do nothing
                    } else {
                        close(pipes[i][j]);
                    }
                }
            }
        } else { // Normal commands
            dup2(pipes[i_cmd][WRITE_END], STDOUT_FILENO);
            dup2(pipes[i_cmd - 1][READ_END], STDIN_FILENO);
            // Close all pipes except dup-ed one
            for (int i = 0; i < n_pipes; i++) {
                for (int j = 0; j < 2; j++) {
                    if (i == i_cmd - 1 && j == READ_END || i == i_cmd && j == WRITE_END) {
                        // Do nothing
                    } else {
                        close(pipes[i][j]);
                    }
                }
            }
        }

        // Do the exec work now
        if (from_path) {
            execvp(executable_path, argv);
        } else {
            execv(executable_path, argv);
        }
        handle_exec_err(executable_path); // Handle exec error if program flows to here
    } else if (pid < 0) { // Fork error
        handle_fork_err(executable_path);
    } else { // Parent process
        handle_process_created(pid, executable_path, start_time); // Do the timekeeping things
    }
}

int main(int argc, char *argv[]) {

    // Set ignore to SIGINT
    signal(SIGINT, SIG_IGN);

    // exit if no command to exec
    if (argc <= 1) {
        return 0;
    }

    // Define spaces to place the commands
    char *current_cmd[MAX_ARGUMENTS];
    char current_cmd_tail = 0;
    char **all_cmds[MAX_CMD];
    int n_cmds = 0;


    // Parse all non-tail commands
    for (int i = 1; i < argc; i++) {
        if (*argv[i] == '!' && *(argv[i] + 1) == '\0') {
            current_cmd[current_cmd_tail] = NULL;

            // Copy all string pointers from current command to new command so it can be reused
            char *new_cmd[MAX_ARGUMENTS];
            for (int i = 0; i < current_cmd_tail + 1; i++) {
                new_cmd[i] = current_cmd[i];
            }

            all_cmds[n_cmds] = new_cmd;
            n_cmds++;
            current_cmd_tail = 0;
        } else {
            current_cmd[current_cmd_tail] = argv[i];
            current_cmd_tail++;
        }
    }

    // Clean-up: Parse last command
    current_cmd[current_cmd_tail] = NULL;
    all_cmds[n_cmds] = current_cmd;
    n_cmds++;

    // Generate (n_cmds - 1) pipes
    int pipes[n_cmds - 1][2];
    for (int i = 0; i < n_cmds - 1; i++) {
        pipe(pipes[i]);
    }

    // Run all commands with supplying their order and all pipes for them to close
    for (int i = 0; i < n_cmds; i++) {
        run_command(all_cmds[i], i, pipes, n_cmds - 1);
    }

    // Close all pipes in parent process
    for (int i = 0; i < n_cmds - 1; i++) {
        close(pipes[i][READ_END]);
        close(pipes[i][WRITE_END]);
    }

}