#define _GNU_SOURCE
#include <sched.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

static int child_func(void* arg) {
  // We can set the hostname for this namespace and the hostname
  // for the underlying host is not affected.
  sethostname("child_host", 11); 

  char hostname[1024];
  gethostname(hostname, 1024);
  printf("child hostname: %s\n", hostname);
  // Notice that this process id is 1.
  printf("child pid: %d\n", getpid());
  printf("child ppid: %d\n", getppid());
  return 0;
}

int main(int argc, char** argv) {
  const int STACK_SIZE = 65536;
  char* stack = malloc(STACK_SIZE);
  if (!stack) {
    perror("malloc");
    exit(1);
  }

  printf("parent pid: %d\n", getpid());

  unsigned long flags = 
    CLONE_NEWCGROUP |  // new cgroup namespace
    CLONE_NEWIPC    |  // new IPC namespace
    CLONE_NEWNET    |  // new network namespace
    CLONE_NEWNS     |  // new mount namespace
    CLONE_NEWPID    |  // new PID namespace
    CLONE_NEWUSER   |  // new user namespace
    CLONE_NEWUTS;      // new Unix Time Share namespace (separates hostname/domainname)

  if (clone(child_func, stack + STACK_SIZE, flags | SIGCHLD, NULL) == -1) {
    perror("clone");
    exit(1);
  }

  int status;
  if (wait(&status) == -1) {
    perror("wait");
    exit(1);
  }

  char hostname[1024];
  gethostname(hostname, 1024);
  printf("parent hostname: %s\n", hostname);
  return 0;
}
