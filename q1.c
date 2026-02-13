#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <stdbool.h>

int main() {
    int n;
    int arr[] = {3, 15, 4, 6, 7, 17, 9, 2};
    int arr_size = sizeof(arr) / sizeof(arr[0]);
    bool visited[arr_size];
    int visited_count = 0;
    printf("Enter the value of n: ");
    scanf("%d", &n);
    
}