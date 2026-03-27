#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#define N 4 
#define ARRAY_SIZE (1 << N) // 2^n

int global_array[ARRAY_SIZE];

struct thread_result {
    int thread_id;
    double average;
    int minimum;
    int maximum;
};

struct thread_params {
    int thread_id;
    int start_index;
    int segment_size;
};

int compare(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

void* sort_segment(void* arg) {
    struct thread_params* params = (struct thread_params*)arg;
    
    int start = params->start_index;
    int end = start + params->segment_size;
    
    qsort(&global_array[start], params->segment_size, sizeof(int), compare);
    
    int min_val = global_array[start];
    int max_val = global_array[end - 1];
    double sum = 0;
    
    for (int i = start; i < end; i++) {
        sum += global_array[i];
    }
    double avg = sum / params->segment_size;
    
    struct thread_result* result = (struct thread_result*)malloc(sizeof(struct thread_result));
    result->thread_id = params->thread_id;
    result->average = avg;
    result->minimum = min_val;
    result->maximum = max_val;
    
    pthread_exit((void*)result);
}

int main() {
    int m;
    
    srand(time(NULL));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        global_array[i] = rand() % 100;
    }
    
    printf("Enter the number of segments\n");
    if (scanf("%d", &m) != 1 || m < 2 || m > ARRAY_SIZE || (m & (m - 1)) != 0) {
        printf("Invalid input. 'm' must be a power of 2 and between 2 and %d.\n", ARRAY_SIZE);
        return 1;
    }
    
    printf("\nInitial Unsorted Array: ");
    for (int i = 0; i < ARRAY_SIZE; i++) {
        printf("%d ", global_array[i]);
    }
    printf("\n\n");
    
    pthread_t threads[m];
    struct thread_params params[m];
    int segment_size = ARRAY_SIZE / m;
    
    for (int i = 0; i < m; i++) {
        params[i].thread_id = i;
        params[i].start_index = i * segment_size;
        params[i].segment_size = segment_size;
        
        pthread_create(&threads[i], NULL, sort_segment, (void*)&params[i]);
    }
    
    double global_sum = 0;
    int global_min = 100;
    int global_max = -1;
    struct thread_result* results[m];
    
    printf("--- Worker Thread Stats ---\n");
    
    for (int i = 0; i < m; i++) {
        void* ret_val;
        pthread_join(threads[i], &ret_val);
        results[i] = (struct thread_result*)ret_val;
        
        printf("Thread %d: Avg: %.2f | Min: %d | Max: %d\n", 
               results[i]->thread_id, results[i]->average, 
               results[i]->minimum, results[i]->maximum);
               
        global_sum += (results[i]->average * segment_size);
        if (results[i]->minimum < global_min) global_min = results[i]->minimum;
        if (results[i]->maximum > global_max) global_max = results[i]->maximum;
        
        free(results[i]);
    }
    
    double global_avg = global_sum / ARRAY_SIZE;
    
    printf("\n--- Global Statistics ---\n");
    printf("Global Average: %.2f\n", global_avg);
    printf("Global Minimum: %d\n", global_min);
    printf("Global Maximum: %d\n", global_max);
    
    int sorted_array[ARRAY_SIZE];
    int indices[m];
    for (int i = 0; i < m; i++) {
        indices[i] = i * segment_size;
    }
    
    for (int i = 0; i < ARRAY_SIZE; i++) {
        int min_val = 999;
        int min_idx = -1;
        
        for (int j = 0; j < m; j++) {
            if (indices[j] < (j + 1) * segment_size) {
                if (global_array[indices[j]] < min_val) {
                    min_val = global_array[indices[j]];
                    min_idx = j;
                }
            }
        }
        
        sorted_array[i] = min_val;
        indices[min_idx]++;
    }
    
    printf("\nFinal Sorted Array: ");
    for (int i = 0; i < ARRAY_SIZE; i++) {
        printf("%d ", sorted_array[i]);
    }
    printf("\n");
    return 0;
}