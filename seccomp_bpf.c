#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/prctl.h>
#include <seccomp.h>

int main(int argc, char** argv) {
  printf("pid: %d\n", getpid());

  scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_KILL);
  // uncomment to trap the signal
  //scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_TRAP);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(read), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(write), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(sigreturn), 0);
  seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(exit_group), 0);
  // uncomment to allow the getpid() system call
  //seccomp_rule_add(ctx, SCMP_ACT_ALLOW, SCMP_SYS(getpid), 0);

  printf("setting restrictions...\n");
  seccomp_load(ctx);

  printf("running with restrictions. Allowed system calls are");
  printf("read(), write(), exit(), sigreturn\n");

  printf("try calling getpid()\n");
  pid_t pid = getpid();
  return 0;
}
