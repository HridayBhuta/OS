#ifndef CPU_BOUND_H
#define CPU_BOUND_H

static inline void cpu_bound_task() {
    unsigned long long limit = 1500000000ULL; 
    volatile unsigned long long i = 0;
    while(i < limit) {
        i++;
    }
}

#endif