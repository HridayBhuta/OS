#include <linux/kernel.h>
#include <linux/syscalls.h>
#include <linux/sched.h>

SYSCALL_DEFINE0(myfork)
{
    struct kernel_clone_args args = {
        .exit_signal = SIGCHLD,
    };
    pr_info("myfork: Process %d is cloning itself.\n", current->pid);

    return kernel_clone(&args);
}