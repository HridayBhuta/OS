#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

static void xread(int fd, void *buf, size_t n){
  for(size_t o=0;o<n;){
    ssize_t r=read(fd,(char*)buf+o,n-o);
    if(r==0) exit(1);
    if(r<0){ if(errno==EINTR) continue; perror("read"); exit(1); }
    o+=(size_t)r;
  }
}
static void xwrite(int fd, const void *buf, size_t n){
  for(size_t o=0;o<n;){
    ssize_t w=write(fd,(char*)buf+o,n-o);
    if(w<0){ if(errno==EINTR) continue; perror("write"); exit(1); }
    o+=(size_t)w;
  }
}

int main(void){
  int n,k,r;
  printf("Enter N (seconds), K (processes), R (repetitions): ");
  if(scanf("%d %d %d",&n,&k,&r)!=3 || n<=0||k<=0||r<=0) return 1;

  int p2c[2], c2p[2];
  if(pipe(p2c)||pipe(c2p)){ perror("pipe"); return 1; }

  pid_t child=fork();
  if(child<0){ perror("fork"); return 1; }

  if(child==0){
    close(p2c[1]); close(c2p[0]);

    while(1){
      for(int i=0;i<r;i++){
        pid_t p=fork();
        if(p==0){
          char cmd[256];
          snprintf(cmd,sizeof(cmd),
            "ps axo user,pid,pmem,time | { read -r h; echo \"$h\"; sort -rk3; } | head -n %d",
            k+1);
          execlp("sh","sh","-c",cmd,(char*)0);
          _exit(127);
        }
        while(waitpid(p,0,0)<0 && errno==EINTR){}
        sleep((unsigned)n);
      }

      char R='R'; xwrite(c2p[1],&R,1);

      int pid; xread(p2c[0],&pid,sizeof(pid));
      if(pid==-2) _exit(0);
      if(pid!=-1) kill(pid,SIGKILL);
    }
  }

  close(p2c[0]); close(c2p[1]);

  for(;;){
    char R; xread(c2p[0],&R,1);

    int pid;
    printf("Enter PID to kill (-1 skip, -2 exit): ");
    fflush(stdout);
    if(scanf("%d",&pid)!=1) pid=-2;

    xwrite(p2c[1],&pid,sizeof(pid));
    if(pid==-2) break;
  }

  while(waitpid(child,0,0)<0 && errno==EINTR){}
  return 0;
}
