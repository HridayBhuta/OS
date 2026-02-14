#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    // Ensure the user provided at least a search term and one file
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <pattern> <file1> [file2...]\n", argv[0]);
        return 1;
    }

    // Create two pipes for our three processes: grep | cut | sort
    int pipe1[2]; 
    int pipe2[2];

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("Pipe creation failed");
        return 1;
    }

    // ==========================================
    // CHILD 1: grep -h -n <pattern> <files...>
    // ==========================================
    pid_t pid1 = fork();
    if (pid1 < 0) return 1;

    if (pid1 == 0) {
        // Redirect stdout to pipe1's write end
        dup2(pipe1[1], STDOUT_FILENO);
        
        // Close all pipe ends in this child (it only needs the one we duplicated)
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        // Dynamically build the arguments array for execvp
        // Size: 1(grep) + 1(-h) + 1(-n) + (argc - 1 arguments) + 1(NULL) = argc + 3
        char *grep_args[argc + 3];
        grep_args[0] = "grep";
        grep_args[1] = "-h"; // Hide filenames so line number is ALWAYS field 1
        grep_args[2] = "-n"; // Include line numbers

        // Copy the pattern and filenames from the user's input
        for (int i = 1; i < argc; i++) {
            grep_args[i + 2] = argv[i];
        }
        grep_args[argc + 2] = NULL; // Arrays passed to exec MUST end with NULL

        execvp("grep", grep_args);
        perror("exec grep failed"); 
        exit(1);
    }

    // ==========================================
    // CHILD 2: cut -d: -f1
    // ==========================================
    pid_t pid2 = fork();
    if (pid2 < 0) return 1;

    if (pid2 == 0) {
        // Redirect stdin to read from pipe1
        dup2(pipe1[0], STDIN_FILENO);
        // Redirect stdout to write to pipe2
        dup2(pipe2[1], STDOUT_FILENO);
        
        // Close all pipe ends 
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        // -d: sets the delimiter to a colon, -f1 extracts the first field
        execlp("cut", "cut", "-d:", "-f1", NULL);
        perror("exec cut failed");
        exit(1);
    }

    // ==========================================
    // CHILD 3: sort -n -u
    // ==========================================
    pid_t pid3 = fork();
    if (pid3 < 0) return 1;

    if (pid3 == 0) {
        // Redirect stdin to read from pipe2
        dup2(pipe2[0], STDIN_FILENO);
        // (We don't redirect stdout here because we want it to print to the terminal)
        
        // Close all pipe ends 
        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);

        // -n sorts numerically, -u removes duplicates (unique)
        execlp("sort", "sort", "-n", "-u", NULL);
        perror("exec sort failed");
        exit(1);
    }

    // ==========================================
    // PARENT PROCESS
    // ==========================================
    // The parent MUST close all its copies of the pipe ends. 
    // If it doesn't, the children will wait forever for an EOF signal.
    close(pipe1[0]); close(pipe1[1]);
    close(pipe2[0]); close(pipe2[1]);

    // Wait for all three children to finish executing before ending the program
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}