#include "my_db.h"
#include "signal.h"
#include <string.h>

#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <unistd.h>

static void tracer_main(struct context *ctx, int argc, char **argv);
static void print_regs(const struct user_regs_struct *regs);

int main(int argc, char *argv[]) {

  if (argc < 2) {
    fprintf(stderr, "Usage: %s command [args]\n", argv[0]);
    return 1;
  }

  struct context ctx = {
      .child_pid = -1,
      .child_regs = NULL,
      .child_status = STATUS_STOPPED,
  };

  ctx.child_pid = fork();
  if (ctx.child_pid == -1) {
    fprintf(stderr, "Fatal: couldn't fork\n");
    return 1;
  }
  if (ctx.child_pid == 0) {
    ptrace(PTRACE_TRACEME, 0, 0, 0);
    execvp(argv[1], argv + 1);
    perror("Couldn't execute command: ");
    exit(0);
  }

  tracer_main(&ctx, argc, argv);
  return EXIT_SUCCESS;
}

static void tracer_main(struct context *ctx, int argc, char **argv) {
  char *line = NULL;
  size_t n = 0;
  long len = -1;
  for (;;) {
    if ((len = getline(&line, &n, stdin)) == -1) {
      perror("Couldn't read from stdin");
    }
    struct command_token *cmd = parse_line(line, len);
    /* print_command(cmd); */
    if (cmd->name == CMD_QUIT) {
      break;
    } else if (cmd->name == CMD_KILL) {
      kill(ctx->child_pid, SIGKILL);
      ctx->child_status = STATUS_KILLED;
    } else if (cmd->name == CMD_CONTINUE) {
      /* Sync with exec */
      waitpid(ctx->child_pid, NULL, 0);
    } else if (cmd->name == CMD_REGISTERS) {
      if (ctx->child_status == STATUS_KILLED ||
          ctx->child_status == STATUS_EXITED) {
        fprintf(stderr, "Can't inspect registers. Process has terminated");
      } else {
        struct user_regs_struct regs;
        if (ptrace(PTRACE_GETREGS, ctx->child_pid, 0, &regs) == -1) {
          perror("Couldn't read process registers");
        }
        print_regs(&regs);
      }
    }
  }
}

// TODO: handle args and associate matching function ptr
struct command_token *parse_line(char *line, const long len) {
  struct command_token *cmd = malloc(sizeof(struct command_token));

  int i = 0;
  while (!isspace(line[i++])) {
  }
  if (strncmp("quit", line, i - 1) == 0) {
    cmd->name = CMD_QUIT;
  } else if (strncmp("kill", line, i - 1) == 0) {
    cmd->name = CMD_KILL;
  } else if (strncmp("continue", line, i - 1) == 0) {
    cmd->name = CMD_CONTINUE;
  } else if (strncmp("registers", line, i - 1) == 0) {
    cmd->name = CMD_REGISTERS;
  } else {
    cmd->name = CMD_INVALID;
  }
  cmd->args = NULL;
  cmd->func = NULL;

  return cmd;
}

void print_command(const struct command_token *cmd) {
  printf("== CMD %p ==\n", (void *)cmd);
  if (!cmd) {
    return;
  }
  printf("ID: %d\nArgs: %p\nFunc: %p", cmd->name, (void *)cmd->args,
         (void *)cmd->func);
}

static void print_regs(const struct user_regs_struct *regs) {
  if (!regs) {
    fprintf(stderr, "Trying to print an empty set of registers");
    return;
  }
  printf("R15: %llu\nR14: %llu\nR13: %llu\nR12: %llu\nRBP: %llu\nRBX: "
         "%llu\nR11: %llu\nR10: %llu\nR9: %llu\nR8: %llu\nRAX: %llu\nRCX: "
         "%llu\nRDX: %llu\nRSI: %llu\nRDI: %llu\nO_RAX: %llu\nRIP: "
         "%llu\nCS: %llu\nEFLAGS: %llu\nRSP: %llu\nSS: %llu\nFS_BASE: %llu\n"
         "GS_BASE: %llu\nDS: %llu\nES: %llu\nFS: %llu\nGS: %llu\n",
         regs->r15, regs->r14, regs->r13, regs->r12, regs->rbp, regs->rbx,
         regs->r11, regs->r10, regs->r9, regs->r8, regs->rax, regs->rcx,
         regs->rdx, regs->rsi, regs->rdi, regs->orig_rax, regs->rip, regs->cs,
         regs->eflags, regs->rsp, regs->ss, regs->fs_base, regs->gs_base,
         regs->ds, regs->es, regs->fs, regs->gs);
}
