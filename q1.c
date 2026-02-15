#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <stdbool.h>

void print_factors(int x) {
    printf("Child: Factors of %d are: ", x);
    for (int i = 1; i <= x; i++) {
        if (x % i == 0) {
            printf("%d ", i);
        }
    }
    printf("\n");
}

int main() {
    int n;
    int arr[] = {3, 15, 4, 6, 7, 17, 9, 2};
    int arr_size = sizeof(arr) / sizeof(arr[0]);
    bool visited[arr_size];
    int visited_count = 0;

    for (int i = 0; i < arr_size; i++) visited[i] = false;

    printf("Enter n (n > 0): ");
    if (scanf("%d", &n) != 1 || n <= 0) {
        printf("Invalid input.\n");
        return 1;
    }

    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return 1;
    }

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork");
        return 1;
    }

    if (pid > 0) {
        close(pipefd[0]);
        srand(time(NULL));

        while (visited_count < arr_size) {
            int idx = rand() % arr_size;
            
            if (!visited[idx]) {
                visited[idx] = true;
                visited_count++;
            }

            int x = arr[idx];
            printf("\nParent: Selected %d. Sending to child...\n", x);
            
            write(pipefd[1], &x, sizeof(x));
            
            printf("Parent: Sleeping for %d seconds...\n", x % n);
            sleep(x % n);
        }

        int stop_signal = -1;
        write(pipefd[1], &stop_signal, sizeof(stop_signal));
        
        close(pipefd[1]);
        wait(NULL);
        printf("Parent: All numbers visited or interrupted. Exiting.\n");

    } 
    else {
        close(pipefd[1]);
        int received_x;

        while (1) {
            if (read(pipefd[0], &received_x, sizeof(received_x)) > 0) {
                if (received_x == -1) break;

                print_factors(received_x);
                
                unsigned int sleep_time = (unsigned int)time(NULL) % n;
                printf("Child: Sleeping for %u seconds...\n", sleep_time);
                sleep(sleep_time);
            } else {
                break; 
            }
        }
        close(pipefd[0]);
        printf("Child: Terminating.\n");
        exit(0);
    }
    return 0;
}