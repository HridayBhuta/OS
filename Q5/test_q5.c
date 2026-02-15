#include <unistd.h>
#include <sys/syscall.h>
#include <stdio.h>

/* * Wrapper for the system call.
 * 467 is the syscall number you registered in the syscall_64.tbl file.
 */
long myfork(void) {
    return syscall(467);
}

int main() {
    /* Call the custom implementation of fork */
    long result = myfork();

    /* * Both the parent and the child process will execute this.
     * If successful, you should see this message twice.
     */
    printf("Hello world!!\n");

    /* Optional: Print the return value to see PIDs (Parent gets child PID, Child gets 0) */
    // printf("Return value: %ld\n", result);

    return 0;
}