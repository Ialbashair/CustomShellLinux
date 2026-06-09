#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

#define MAX_ARGS 64

static int tokenize(char * line, char *argv[], int max_args){
  int argc = 0;
  char *tok = strtok(line, " \t\r\n");
  while (tok != NULL && argc < max_args - 1){
    argv[argc++] = tok;
    tok = strtok(NULL, " \t\r\n");
  }
  argv[argc] = NULL;
  return argc;
}

static int run_builtin(char *argv[], int argc, int *should_exit){
  if (argc == 0) return 1; // Empty line: handled, do nothing.

  if (strcmp(argv[0], "exit") == 0){
    *should_exit = 1;
    return 1;
  }

  if (strcmp(argv[0], "cd") == 0) {
    const char *dir = (argc > 1) ? argv[1] : getenv("HOME");
    if (dir == NULL) dir = "/";
    if (chdir(dir) != 0) perror("cd");
    return 1;
  }

  if (strcmp(argv[0], "pwd") == 0) {
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL){
      printf("%s\n", cwd);
    } else perror("pwd");
    return 1;
  }

  return  0;
}

static void run_external(char * argv[]) {
  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return;
  }
  if (pid == 0) {
    execvp(argv[0], argv);
    fprintf(stderr, "mysh: %s: %s\n", argv[0],strerror(errno));
    _exit(127);
  }
  int status;
  if (waitpid(pid, &status, 0) < 0) perror("waitpid");
}

int main(void) {
  char *line = NULL;
  size_t cap = 0;
  char *argv[MAX_ARGS];

  for (;;){
    printf("mysh> ");
    fflush(stdout);

    ssize_t n = getline(&line, &cap, stdin);
    if (n < 0) {
      printf("\n");
      break;
    }

    int argc = tokenize(line, argv, MAX_ARGS);
    
    int should_exit = 0;

    if (run_builtin(argv, argc, &should_exit)) {
      if (should_exit) break;
      continue;
    }

    run_external(argv);
  }

  free(line);
  return 0;
}
