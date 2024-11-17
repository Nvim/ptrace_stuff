#ifndef MY_DB_H

#define MY_DB_H

#include <stddef.h>
#include <sys/types.h>
#include <sys/user.h>

typedef enum {
  STATUS_STOPPED,
  STATUS_RUNNING,
  STATUS_KILLED,
  STATUS_EXITED,
} status;

typedef enum {
  CMD_QUIT,
  CMD_KILL,
  CMD_CONTINUE,
  CMD_REGISTERS,
  CMD_INVALID
} command;

struct context {
  pid_t child_pid;
  struct user_regs_struct *child_regs;
  status child_status;
};

struct command_func {
  const char *name;
  int (*func)(struct context *ctx);
};

struct command_token {
  command name;
  const char **args;
  struct command_func *func;
};

const char *cmds[] = {
    "quit",
    "kill",
    "continue",
    "registers",
};

struct command_token *parse_line(char *line, const long len);
void print_command(const struct command_token *cmd);

#endif // !MY_DB_H
