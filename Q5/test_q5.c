#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

long myfork(void) {
    return syscall(467);
}

int main() {
    long result = myfork();

    printf("Hello world!!\n");

    return 0;
}