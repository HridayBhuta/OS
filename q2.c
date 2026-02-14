#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

int main() {
    int n, k, r;

    // 1. Take inputs from user
    printf("Enter time between prints in seconds (n): ");
    if (scanf("%d", &n) != 1 || n <= 0) return 1;
    
    printf("Enter number of processes to display (k): ");
    if (scanf("%d", &k) != 1 || k <= 0) return 1;
    
    printf("Enter iterations before prompting (r): ");
    if (scanf("%d", &r) != 1 || r <= 0) return 1;

    // 2. Setup the two-way pipes
    int p2c[2]; // Parent to Child pipe (for sending the PID to kill)
    int c2p[2]; // Child to Parent pipe (for signaling readiness to prompt)

    if (pipe(p2c) == -1 || pipe(c2p) == -1) {
        perror("Pipe failed");
        return 1;
    }

    // 3. Fork the process
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        return 1;
    }

    if (pid == 0) {
        // ==========================================
        // CHILD PROCESS (The Monitor)
        // ==========================================
        close(p2c[1]); // Close write end of Parent->Child
        close(c2p[0]); // Close read end of Child->Parent

        int iterations = 0;

        while (1) {
            printf("\n--- Top %d Memory Consuming Processes ---\n", k);
            
            // Fork again just to run the 'ps' command cleanly
            pid_t ps_pid = fork();
            if (ps_pid == 0) {
                char cmd[256];
                // Using BSD syntax 'ax' for all processes, 'o' for custom formatting
                // We fetch exactly k+1 lines to include the header
                snprintf(cmd, sizeof(cmd), "ps ax o user,pid,%%mem,time --sort=-%%mem | head -n %d", k + 1);
                execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
                exit(1);
            }
            waitpid(ps_pid, NULL, 0); // Wait for the ps command to finish printing

            iterations++;

            // Check if we reached 'r' iterations
            if (iterations == r) {
                // Send a quick signal byte to the parent so it knows to prompt the user
                char sync_signal = 'R';
                write(c2p[1], &sync_signal, 1);

                // Now HALT and wait for the parent to send the PID
                int target_pid;
                read(p2c[0], &target_pid, sizeof(target_pid));

                // Process the parent's command
                if (target_pid == -2) {
                    printf("\t[Child] Received exit command. Terminating monitor.\n");
                    break; 
                } else if (target_pid == -1) {
                    printf("\t[Child] Skipping kill operation...\n");
                } else {
                    // Attempt to kill the requested process
                    if (kill(target_pid, SIGKILL) == 0) {
                        printf("\t[Child] Successfully sent SIGKILL to PID %d.\n", target_pid);
                    } else {
                        perror("\t[Child] Failed to kill process");
                    }
                }
                
                iterations = 0; // Reset counter after interaction
            } else {
                // Sleep for n seconds before the next print
                sleep(n);
            }
        }

        close(p2c[0]);
        close(c2p[1]);
        exit(0);

    } else {
        // ==========================================
        // PARENT PROCESS (The User Interface)
        // ==========================================
        close(p2c[0]); // Close read end of Parent->Child
        close(c2p[1]); // Close write end of Child->Parent

        while (1) {
            char sync_signal;
            // Block and wait until the child has printed 'r' times
            if (read(c2p[0], &sync_signal, 1) <= 0) {
                break; // Child closed the pipe or exited
            }

            int target_pid;
            printf("\n>>> Enter PID to kill (-1 to skip, -2 to exit): ");
            scanf("%d", &target_pid);

            // Send the user's choice to the child
            write(p2c[1], &target_pid, sizeof(target_pid));

            // If user inputted -2, the parent should also break and exit
            if (target_pid == -2) {
                break;
            }
        }

        close(p2c[1]);
        close(c2p[0]);
        wait(NULL); // Prevent zombie processes by waiting for child to officially die
    }

    return 0;
}