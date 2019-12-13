#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <linux/seccomp.h>

int main(int argc, char** argv) {
  printf("pid: %d\n", getpid());

  printf("setting restrictions...\n");
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);

  printf("running with restrictions. Allowed system calls are");
  printf("read(), write(), exit()\n");

  printf("try calling getpid()\n");
  pid_t pid = getpid();
  return 0;
}
