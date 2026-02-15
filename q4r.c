#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
    char msg[] = "\n[WARNING] Carrier Interrupt Signal Received\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    sigint_received = 1;
}

int main() {
    int frequency = 800;     
    char input[1024]; 

    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        return 1;
    }

    while (1) {
        if (sigint_received) {
            frequency = 800;
            sigint_received = 0;
        }

        printf("station-controller$ ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            if (errno == EINTR || sigint_received) {
                clearerr(stdin);
                continue;
            }
            
            printf("\n");
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) {
            continue;
        }

        char *args[64];
        int i = 0;
        char *token = strtok(input, " \t");
        
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " \t");
        }
        args[i] = NULL;

        if (strcmp(args[0], "quit") == 0) {
            break;
        } 
        else if (strcmp(args[0], "set_freq") == 0) {
            if (args[1] != NULL) {
                int new_freq = atoi(args[1]);
                if (new_freq > 0) {
                    frequency = new_freq;
                } else {
                    printf("Error: Frequency must be a positive integer.\n");
                }
            } else {
                printf("Error: Missing <MHz> argument.\n");
            }
        } 
        else if (strcmp(args[0], "get_freq") == 0) {
            printf("Current base station frequency: %d MHz\n", frequency);
        } 
        
        else if (strcmp(args[0], "top") == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                signal(SIGINT, SIG_DFL);
                execlp("top", "top", NULL);
                perror("Command execution failed");
                exit(1);
            } else if (pid > 0) {
                while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
            }
        } 
        else if (strcmp(args[0], "ping") == 0) {
            if (args[1] != NULL) {
                pid_t pid = fork();
                if (pid == 0) {
                    signal(SIGINT, SIG_DFL);
                    execlp("ping", "ping", "-c", "4", args[1], NULL);
                    perror("Command execution failed");
                    exit(1);
                } else if (pid > 0) {
                    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
                }
            } else {
                printf("Error: Missing <address> argument for ping.\n");
            }
        } 
        
        else {
            printf("Error: Command '%s' not recognized.\n", args[0]);
        }
    }

    return 0;
}