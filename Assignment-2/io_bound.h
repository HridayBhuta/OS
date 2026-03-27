#ifndef IO_BOUND_H
#define IO_BOUND_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>

static inline void io_bound_task() {
    char filename[64];
    snprintf(filename, sizeof(filename), "/tmp/io_test_%d.dat", getpid());
    
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);

    char buffer[4096]; 
    for (int i = 0; i < 1500; i++) {
        write(fd, buffer, sizeof(buffer));
        fsync(fd); 
    }

    close(fd);
    unlink(filename); 
}

#endif