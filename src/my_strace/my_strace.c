#include "my_strace.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/syscall.h>
#include <sys/user.h>
#include <sys/wait.h>
#include <unistd.h>

#define syscode_case(x)                                                        \
  case x:                                                                      \
    return #x;

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s [CMD]\n", argv[0]);
    return 1;
  }

  const char *cmd = argv[1];

  pid_t pid = fork();
  if (pid == 0) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    printf("Executing %s:\n", cmd);
    execvp(cmd, argv + 1);
  }

  wait(NULL); // sync with exec
  /* Kill child along with parent: */
  ptrace(PTRACE_SETOPTIONS, pid, 0, PTRACE_O_EXITKILL);

  for (;;) {
    /* Stop when entering syscall: */
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
      perror("Error entering syscall: ");
      return 1;
    }
    wait(NULL);

    /* Get registers: */
    struct user_regs_struct regs;
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
      perror("Error reading registers before entering syscall: ");
    }
    printf("%llu - %s() => ", regs.orig_rax, get_syscall_name(regs.orig_rax));

    /* Continue syscall */
    if (ptrace(PTRACE_SYSCALL, pid, 0, 0) == -1) {
      perror("Error running syscall: ");
      return 1;
    }
    wait(NULL);

    /* Get registers after syscall. Fails if process exited */
    if (ptrace(PTRACE_GETREGS, pid, 0, &regs) == -1) {
      if (errno == ESRCH) {
        printf("Child exited.\n");
        exit(regs.rdi);
      }
      fprintf(stderr, "Error getting result from syscall\n");
    }
    printf("%llu\n", regs.rax);
  }

  return 0;
}

const char *get_syscall_name(const long long unsigned int number) {
  if (number < SYSCALL_COUNT) {
    return SYSCALL_MAP[number];
  }
  return "UNKNOWN";
}
