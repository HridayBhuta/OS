#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#define MAX_CHAIRS 5
#define MAX_PATIENTS 100


int n, k, max_visits, y;
int arrival_times[MAX_PATIENTS];


pthread_mutex_t chair_mutex;
sem_t dentist_sleep;
sem_t patient_ready[MAX_PATIENTS];


int free_chairs = MAX_CHAIRS;
int waiting_patients[MAX_PATIENTS] = {0}; 
int active_patients = 0;


typedef struct {
    int id;
    int status; 
    int visits;
    int timeouts;
    int turnaways;
} PatientStats;

PatientStats stats[MAX_PATIENTS];

void* dentist_thread(void* arg) {
    while (1) {
        sem_wait(&dentist_sleep); 
        
        pthread_mutex_lock(&chair_mutex);
        if (active_patients == 0) {
            pthread_mutex_unlock(&chair_mutex);
            break; 
        }

        // Find lowest ID patient
        int selected_patient = -1;
        for (int i = 0; i < n; i++) {
            if (waiting_patients[i]) {
                selected_patient = i;
                waiting_patients[i] = 0;
                free_chairs++;
                break;
            }
        }
        pthread_mutex_unlock(&chair_mutex);

        if (selected_patient != -1) {
            sem_post(&patient_ready[selected_patient]); 
            sleep(1); 
        }
        
        pthread_mutex_lock(&chair_mutex);
        int still_waiting = 0;
        for (int i = 0; i < n; i++) {
            if (waiting_patients[i]) still_waiting = 1;
        }
        if (still_waiting) {
            sem_post(&dentist_sleep);
        }
        pthread_mutex_unlock(&chair_mutex);
    }
    return NULL;
}

void* patient_thread(void* arg) {
    int id = *((int*)arg);
    sleep(arrival_times[id]);

    while (stats[id].visits < max_visits && stats[id].status == 0) {
        stats[id].visits++;
        
        pthread_mutex_lock(&chair_mutex);
        if (free_chairs == 0) {
            stats[id].turnaways++;
            pthread_mutex_unlock(&chair_mutex);
            sleep(y); // Sleep before retry
            continue;
        }


        free_chairs--;
        waiting_patients[id] = 1;
        
        if (free_chairs == MAX_CHAIRS - 1) {
            sem_post(&dentist_sleep); 
        }
        pthread_mutex_unlock(&chair_mutex);


        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += k;

        int res = sem_timedwait(&patient_ready[id], &ts);

        if (res == -1 && errno == ETIMEDOUT) {
            pthread_mutex_lock(&chair_mutex);
            if (waiting_patients[id] == 1) { 
                waiting_patients[id] = 0;
                free_chairs++;
                stats[id].timeouts++;
                pthread_mutex_unlock(&chair_mutex);
                sleep(y);
            } else {
                pthread_mutex_unlock(&chair_mutex);
                sem_wait(&patient_ready[id]); 
                stats[id].status = 1;
            }
        } else {
            // Treated
            stats[id].status = 1;
        }
    }

    pthread_mutex_lock(&chair_mutex);
    active_patients--;
    if (active_patients == 0) sem_post(&dentist_sleep); 
    pthread_mutex_unlock(&chair_mutex);
    
    return NULL;
}

int main() {
    FILE *fp = fopen("input.txt", "r");
    if (!fp) return 1;
    fscanf(fp, "%d %d %d %d", &n, &k, &max_visits, &y);
    for (int i = 0; i < n; i++) {
        fscanf(fp, "%d", &arrival_times[i]);
        stats[i].id = i;
        stats[i].status = 0;
        stats[i].visits = 0;
        stats[i].timeouts = 0;
        stats[i].turnaways = 0;
    }
    fclose(fp);

    active_patients = n;
    pthread_mutex_init(&chair_mutex, NULL);
    sem_init(&dentist_sleep, 0, 0);
    for (int i = 0; i < n; i++) sem_init(&patient_ready[i], 0, 0);

    pthread_t dentist;
    pthread_create(&dentist, NULL, dentist_thread, NULL);

    pthread_t patients[MAX_PATIENTS];
    int patient_ids[MAX_PATIENTS];
    for (int i = 0; i < n; i++) {
        patient_ids[i] = i;
        pthread_create(&patients[i], NULL, patient_thread, &patient_ids[i]);
    }

    for (int i = 0; i < n; i++) pthread_join(patients[i], NULL);
    pthread_join(dentist, NULL);

    int total_treated = 0;
    printf("\nCLINIC REPORT\n");
    for (int i = 0; i < n; i++) {
        printf("Patient %d - Status: %s | Visits: %d | Timeouts: %d | Turnaways: %d\n", 
               i + 1, stats[i].status ? "TREATED" : "UNTREATED", 
               stats[i].visits, stats[i].timeouts, stats[i].turnaways);
        if (stats[i].status) total_treated++;
    }
    printf("Total treated: %d\nTotal untreated: %d\n", total_treated, n - total_treated);

    return 0;
}