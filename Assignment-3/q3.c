#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

sem_t rmutex;    
sem_t wmutex;    
sem_t readTry;   
sem_t resource;  


int shared_resource_value = 0;
int read_count = 0;
int write_count = 0;

void* writer_thread(void* arg) {
    int id = *((int*)arg);
    unsigned int seed = id;

    while (1) {
        sem_wait(&wmutex);
        write_count++;
        if (write_count == 1) sem_wait(&readTry); 
        sem_post(&wmutex);

        sem_wait(&resource);
        
        shared_resource_value += 5;
        printf("[Writer %d] Writing to resource: %d\n", id, shared_resource_value);
        usleep((rand_r(&seed) % 1500) * 1000); 
        
        sem_post(&resource);

        sem_wait(&wmutex);
        write_count--;
        if (write_count == 0) sem_post(&readTry); 
        sem_post(&wmutex);

        usleep((rand_r(&seed) % 5000) * 1000); 
    }
    return NULL;
}

void* reader_thread(void* arg) {
    int id = *((int*)arg);
    unsigned int seed = id + 10;

    while (1) {
        sem_wait(&readTry); 
        sem_wait(&rmutex);
        read_count++;
        if (read_count == 1) sem_wait(&resource); 
        sem_post(&rmutex);
        sem_post(&readTry);

        printf("[Reader %d] Reading resource: %d (Active Readers: %d)\n", id, shared_resource_value, read_count);
        usleep((rand_r(&seed) % 1000) * 1000); 
        
        sem_wait(&rmutex);
        read_count--;
        if (read_count == 0) sem_post(&resource); 
        sem_post(&rmutex);

        usleep((rand_r(&seed) % 3000) * 1000); 
    }
    return NULL;
}

int main() {
    pthread_t readers[4], writers[2];
    int reader_ids[4] = {1, 2, 3, 4};
    int writer_ids[2] = {1, 2};

    sem_init(&rmutex, 0, 1);
    sem_init(&wmutex, 0, 1);
    sem_init(&readTry, 0, 1);
    sem_init(&resource, 0, 1);

    for (int i = 0; i < 4; i++) pthread_create(&readers[i], NULL, reader_thread, &reader_ids[i]);
    for (int i = 0; i < 2; i++) pthread_create(&writers[i], NULL, writer_thread, &writer_ids[i]);

    for (int i = 0; i < 4; i++) pthread_join(readers[i], NULL);
    for (int i = 0; i < 2; i++) pthread_join(writers[i], NULL);

    return 0;
}