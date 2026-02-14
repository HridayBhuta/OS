#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

// A single global flag is the standard, safe way to communicate with a signal handler.
// (We keep the actual 'frequency' variable local inside main, as requested).
volatile sig_atomic_t sigint_received = 0;

void handle_sigint(int sig) {
    // It is unsafe to use printf() inside a signal handler. 
    // Using write() is the POSIX-compliant, safe way to print here.
    char msg[] = "\n[WARNING] Carrier Interrupt Signal Received\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    sigint_received = 1;
}

int main() {
    // 1. Initialize local frequency variable to 800 MHz
    int frequency = 800; 
    
    // 1023 characters + 1 for the null terminator '\0'
    char input[1024]; 

    // 2. Setup the SIGINT (Ctrl+C) signal handler
    struct sigaction sa;
    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // 0 means it will cleanly interrupt blocking functions like fgets
    
    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("Error setting up signal handler");
        return 1;
    }

    // 3. The Main Shell Loop
    while (1) {
        // Check if Ctrl+C was pressed recently and reset frequency to safe mode
        if (sigint_received) {
            frequency = 800;
            sigint_received = 0;
        }

        // Print prompt
        printf("station-controller$ ");
        fflush(stdout); // Force the prompt to print immediately before waiting for input

        // Read user input
        if (fgets(input, sizeof(input), stdin) == NULL) {
            // If fgets was interrupted by our Ctrl+C signal
            if (errno == EINTR || sigint_received) {
                clearerr(stdin); // Clear the error state
                continue;        // Loop back to top to handle the signal and reprint prompt
            }
            // If user pressed Ctrl+D (EOF), exit gracefully
            printf("\n");
            break;
        }

        // Strip the trailing newline character added by fgets
        input[strcspn(input, "\n")] = 0;

        // Skip if the user just pressed Enter without typing anything
        if (strlen(input) == 0) {
            continue;
        }

        // 4. Parse the input using strtok (split by spaces or tabs)
        char *args[64];
        int i = 0;
        char *token = strtok(input, " \t");
        
        while (token != NULL) {
            args[i++] = token;
            token = strtok(NULL, " \t");
        }
        args[i] = NULL; // The array must be NULL-terminated for exec() commands

        // 5. Evaluate Internal Commands
        if (strcmp(args[0], "quit") == 0) {
            break; // Exit the shell loop
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
        
        // 6. Evaluate External Commands
        else if (strcmp(args[0], "top") == 0) {
            pid_t pid = fork();
            if (pid == 0) {
                // Child: restore default Ctrl+C behavior so top can be quit cleanly
                signal(SIGINT, SIG_DFL);
                execlp("top", "top", NULL);
                perror("Command execution failed");
                exit(1);
            } else if (pid > 0) {
                // Parent: wait for top to finish (looping protects against EINTR interruptions)
                while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
            }
        } 
        else if (strcmp(args[0], "ping") == 0) {
            if (args[1] != NULL) {
                pid_t pid = fork();
                if (pid == 0) {
                    // Child: restore default Ctrl+C behavior
                    signal(SIGINT, SIG_DFL);
                    // Pass -c 4 to limit ping to exactly 4 times
                    execlp("ping", "ping", "-c", "4", args[1], NULL);
                    perror("Command execution failed");
                    exit(1);
                } else if (pid > 0) {
                    // Parent: wait for ping to finish
                    while (waitpid(pid, NULL, 0) == -1 && errno == EINTR);
                }
            } else {
                printf("Error: Missing <address> argument for ping.\n");
            }
        } 
        
        // 7. Undefined Command
        else {
            printf("Error: Command '%s' not recognized.\n", args[0]);
        }
    }

    return 0;
}