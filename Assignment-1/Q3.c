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
    int pipe1[2], pipe2[2];
    
    if (pipe(pipe1) == -1) {
        perror("pipe1");
        exit(1);
    }
    
    if (pipe(pipe2) == -1) {
        perror("pipe2");
        exit(1);
    }

    pid_t pid1 = fork();
    if (pid1 == -1) {
        perror("fork1");
        exit(1);
    }
    
    if (pid1 == 0) {
        close(pipe1[0]);
        
        dup2(pipe1[1], STDOUT_FILENO);
        close(pipe1[1]);
        
        close(pipe2[0]);
        close(pipe2[1]);
        
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

    pid_t pid2 = fork();
    if (pid2 == -1) {
        perror("fork2");
        exit(1);
    }
    
    if (pid2 == 0) {
        close(pipe1[1]);
        
        dup2(pipe1[0], STDIN_FILENO);
        close(pipe1[0]);
        
        close(pipe2[0]);
        
        dup2(pipe2[1], STDOUT_FILENO);
        close(pipe2[1]);
        
        char *cut_args[] = {"cut", "-d:", "-f2", NULL};
        execvp("cut", cut_args);
        perror("execvp cut");
        exit(1);
    }

    pid_t pid3 = fork();
    if (pid3 == -1) {
        perror("fork3");
        exit(1);
    }
    
    if (pid3 == 0) {
        close(pipe2[1]);
        
        dup2(pipe2[0], STDIN_FILENO);
        close(pipe2[0]);
        
        close(pipe1[0]);
        close(pipe1[1]);
        
        char *sort_args[] = {"sort", "-n", "-u", NULL};
        execvp("sort", sort_args);
        perror("execvp sort");
        exit(1);
    }

    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);

    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
    waitpid(pid3, NULL, 0);

    return 0;
}
