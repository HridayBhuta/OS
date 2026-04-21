#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_ROVERS 5

typedef enum { EXPLORING, WAITING, CHARGING, DEAD } State;

State state[NUM_ROVERS];
int battery[NUM_ROVERS];
pthread_mutex_t mtx;
pthread_cond_t cond[NUM_ROVERS];

void print_state(int id, const char* msg) {
    printf("Rover %d %s (Battery: %d%%)\n", id, msg, battery[id]);
}

void test(int i) {
    int left = (i + 4) % NUM_ROVERS;
    int right = (i + 1) % NUM_ROVERS;

    int neighbor_emergency = (battery[left] <= 20 && state[left] == WAITING) || 
                             (battery[right] <= 20 && state[right] == WAITING);
    int my_emergency = (battery[i] <= 20);

    if (state[i] == WAITING && state[left] != CHARGING && state[right] != CHARGING) {
        if (my_emergency || !neighbor_emergency) {
            state[i] = CHARGING;
            pthread_cond_signal(&cond[i]);
        }
    }
}

void pickup_cables(int id) {
    pthread_mutex_lock(&mtx);
    state[id] = WAITING;
    print_state(id, "is waiting for cables");
    
    if (battery[id] <= 20) print_state(id, "ENTERING EMERGENCY STATE");

    while (state[id] == WAITING) {
        test(id);
        if (state[id] == CHARGING) break;

        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1; 

        int res = pthread_cond_timedwait(&cond[id], &mtx, &ts);
        if (res != 0) { 
            battery[id] -= 5;
            if (battery[id] <= 0) {
                state[id] = DEAD;
                printf("Rover %d STRANDED: Battery depleted to 0%% while waiting.\n", id);
                pthread_mutex_unlock(&mtx);
                pthread_exit(NULL);
            }
        }
    }
    
    if (state[id] == CHARGING) print_state(id, "is charging");
    pthread_mutex_unlock(&mtx);
}

void putdown_cables(int id) {
    pthread_mutex_lock(&mtx);
    state[id] = EXPLORING;
    print_state(id, "finished charging");
    print_state(id, "is exploring");
    
    test((id + 4) % NUM_ROVERS);
    test((id + 1) % NUM_ROVERS);
    pthread_mutex_unlock(&mtx);
}

void* rover_thread(void* arg) {
    int id = *((int*)arg);
    unsigned int seed = time(NULL) ^ id;

    while (1) {
        int explore_time = (rand_r(&seed) % 5) + 1;
        for (int i = 0; i < explore_time; i++) {
            sleep(1);
            pthread_mutex_lock(&mtx);
            battery[id] -= 10;
            if (battery[id] <= 0) {
                state[id] = DEAD;
                printf("Rover %d STRANDED: Battery depleted to 0%% during exploration.\n", id);
                pthread_mutex_unlock(&mtx);
                pthread_exit(NULL);
            }
            pthread_mutex_unlock(&mtx);
        }

        pickup_cables(id);

        int charge_time = (rand_r(&seed) % 3) + 1;
        for (int i = 0; i < charge_time; i++) {
            sleep(1);
            pthread_mutex_lock(&mtx);
            battery[id] += 15;
            if (battery[id] > 100) battery[id] = 100;
            pthread_mutex_unlock(&mtx);
        }

        putdown_cables(id);
    }
    return NULL;
}

int main() {
    pthread_t rovers[NUM_ROVERS];
    int ids[NUM_ROVERS];

    pthread_mutex_init(&mtx, NULL);
    for (int i = 0; i < NUM_ROVERS; i++) {
        battery[i] = 50;
        state[i] = EXPLORING;
        pthread_cond_init(&cond[i], NULL);
        ids[i] = i;
        pthread_create(&rovers[i], NULL, rover_thread, &ids[i]);
    }

    for (int i = 0; i < NUM_ROVERS; i++) pthread_join(rovers[i], NULL);
    return 0;
}