#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: %s pattern file1 [file2 ...]\n", argv[0]);
        exit(1);
    }

    // Create two pipes: pipe1 connects grep to cut, pipe2 connects cut to sort
    int pipe1[2], pipe2[2];
    
    if (pipe(pipe1) == -1) {
        perror("pipe1");
        exit(1);
    }
    
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        exit(1);
    }

    // Fork first child for grep
    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork1");
        exit(1);
    }
    
    if (pid1 == 0) {
        // Child 1: Execute grep
        // Close read end of pipe1
        close(pipe1[0]);
        
        // Redirect stdout to write end of pipe1
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        
        // Close all pipe2 file descriptors (not needed in this child)
        close(pipe2[0]);
        close(pipe2[1]);
        
        // Prepare arguments for grep
        // grep -n pattern file1 file2 ... /dev/null
        // Adding /dev/null forces grep to always show filenames
        char **grep_args = malloc((argc + 3) * sizeof(char*));
        grep_args[0] = "grep";
        grep_args[1] = "-n";
        for (int i = 1; i < argc; i++) {
            grep_args[i + 1] = argv[i];
        }
        grep_args[argc + 1] = "/dev/null";
        grep_args[argc + 2] = NULL;
        
        execvp("grep", grep_args);
        perror("execvp grep");
        exit(1);
    }

    // Fork second child for cut
    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork2");
        exit(1);
    }
    
    if (pid2 == 0) {
        // Child 2: Execute cut
        // Close write end of pipe1
        close(pipe1[1]);
        
        // Redirect stdin to read end of pipe1
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        
        // Close read end of pipe2
        close(pipe2[0]);
        
        // Redirect stdout to write end of pipe2
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        
        // Execute cut -d: -f2 (field 2 contains line numbers when multiple files)
        char *cut_args[] = {"cut", "-d:", "-f2", NULL};
        execvp("cut", cut_args);
        perror("execvp cut");
        exit(1);
    }

    // Fork third child for sort
    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("fork3");
        exit(1);
    }
    
    if (pid3 == 0) {
        // Child 3: Execute sort
        // Close write end of pipe2
        close(pipe2[1]);
        
        // Redirect stdin to read end of pipe2
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        
        // Close all pipe1 file descriptors (not needed in this child)
        close(pipe1[0]);
        close(pipe1[1]);
        
        // Execute sort -n -u (numeric sort, unique)
        char *sort_args[] = {"sort", "-n", "-u", NULL};
        execvp("sort", sort_args);
        perror("execvp sort");
        exit(1);
    }

    // Parent: Close all pipe file descriptors
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    // Wait for all children to complete
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}
