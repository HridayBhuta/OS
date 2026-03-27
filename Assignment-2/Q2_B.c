#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/time.h>

#include "io_bound.h" 

#define NUM_CHILDREN 8

int main() {
    struct timeval start, end;
    struct rusage usage;

    gettimeofday(&start, NULL);

    for (int i = 0; i < NUM_CHILDREN; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            perror("Fork failed");
            exit(1);
        } else if (pid == 0) {
            io_bound_task(); 
            exit(0);
        }
    }

    for (int i = 0; i < NUM_CHILDREN; i++) {
        wait(NULL);
    }

    gettimeofday(&end, NULL);

    if (getrusage(RUSAGE_CHILDREN, &usage) != 0) {
        perror("getrusage failed");
        exit(1);
    }

    double elapsed_time = (end.tv_sec - start.tv_sec) + 
                          (end.tv_usec - start.tv_usec) / 1000000.0;

    printf("Program-B (I/O-Bound)\n");
    printf("Involuntary Context Switches: %ld\n", usage.ru_nivcsw);
    printf("Voluntary Context Switches: %ld\n", usage.ru_nvcsw);
    printf("Total Elapsed Time: %.2fs\n\n", elapsed_time);

    return 0;
}