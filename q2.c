#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int main() {
    int n, k, r;
    printf("Enter n, k, r: ");
    scanf("%d %d %d", &n, &k, &r);

    int pipe_ptc[2]; // parent to child
    int pipe_ctp[2]; // child to parent (for signaling readiness)

    if (pipe(pipe_ptc) == -1 || pipe(pipe_ctp) == -1) {
        perror("pipe");
        exit(1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(1);
    }

    if (pid == 0) {
        // Child process - monitors memory usage
        close(pipe_ptc[1]);
        close(pipe_ctp[0]);

        while (1) {
            for (int i = 0; i < r; i++) {
                if (i > 0) sleep(n);

                // Build pipeline: ps | head using pipe, fork, dup2, exec
                int ps_pipe[2];
                if (pipe(ps_pipe) == -1) {
                    perror("pipe");
                    exit(1);
                }

                pid_t ps_pid = fork();
                if (ps_pid < 0) {
                    perror("fork");
                    exit(1);
                }

                if (ps_pid == 0) {
                    // Exec "ps" with stdout redirected to pipe
                    close(ps_pipe[0]);
                    dup2(ps_pipe[1], STDOUT_FILENO);
                    close(ps_pipe[1]);
                    execlp("ps", "ps", "-eo", "user,pid,%mem,time", "--sort=-%mem", NULL);
                    perror("execlp ps");
                    exit(1);
                }

                pid_t head_pid = fork();
                if (head_pid < 0) {
                    perror("fork");
                    exit(1);
                }

                if (head_pid == 0) {
                    // Exec "head" with stdin redirected from pipe
                    close(ps_pipe[1]);
                    dup2(ps_pipe[0], STDIN_FILENO);
                    close(ps_pipe[0]);
                    char k_str[16];
                    snprintf(k_str, sizeof(k_str), "%d", k + 1);
                    execlp("head", "head", "-n", k_str, NULL);
                    perror("execlp head");
                    exit(1);
                }

                close(ps_pipe[0]);
                close(ps_pipe[1]);

                waitpid(ps_pid, NULL, 0);
                waitpid(head_pid, NULL, 0);
                printf("\n");
            }

            // Signal parent that child is ready for input
            char ready = 'R';
            write(pipe_ctp[1], &ready, 1);

            // Wait for PID from parent via pipe
            int target_pid;
            if (read(pipe_ptc[0], &target_pid, sizeof(int)) <= 0)
                break;

            if (target_pid == -2) {
                printf("Exiting...\n");
                break;
            } else if (target_pid == -1) {
                printf("Skipping kill.\n");
            } else {
                if (kill(target_pid, SIGKILL) == 0)
                    printf("Process %d killed.\n", target_pid);
                else
                    perror("kill");
            }

            sleep(n);
        }

        close(pipe_ptc[0]);
        close(pipe_ctp[1]);
        exit(0);

    } else {
        // Parent process - handles user interaction
        close(pipe_ptc[0]);
        close(pipe_ctp[1]);

        while (1) {
            // Wait for child to signal it has completed r iterations
            char ready;
            if (read(pipe_ctp[0], &ready, 1) <= 0)
                break;

            int target_pid;
            printf("Enter PID to kill (-1 to skip, -2 to exit): ");
            scanf("%d", &target_pid);

            write(pipe_ptc[1], &target_pid, sizeof(int));

            if (target_pid == -2)
                break;
        }

        close(pipe_ptc[1]);
        close(pipe_ctp[0]);
        wait(NULL);
    }
    return 0;
}