/*
 * mysh - a minimal custom shell for Linux
 *
 * Implemented so far:
 *   - REPL: print prompt, read a line, parse it, run it, repeat
 *   - Built-in commands: cd, pwd, exit
 *   - External commands run via fork + execvp + waitpid
 *   - Piping between commands: cmd1 | cmd2 | cmd3 ...
 *
 * Not yet implemented (your next checklist items):
 *   - Background jobs              (the & operator)
 *   - Signal handling / job control (Ctrl-C, SIGCHLD reaping)
 *   - I/O redirection             (> and <), quoting in the parser
 *
 * Build:  gcc -Wall -Wextra -std=c11 -o mysh main.c
 * Run:    ./mysh
 */

#define _POSIX_C_SOURCE 200809L   /* getline, strtok_r */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>

#define MAX_ARGS 64
#define MAX_CMDS 32   /* max segments in a pipeline */

/* Split a segment into a NULL-terminated argv array by whitespace.
 * Returns the token count. Modifies the segment in place. */
static int tokenize(char *seg, char *argv[], int max_args) {
    int argc = 0;
    char *tok = strtok(seg, " \t\r\n");
    while (tok != NULL && argc < max_args - 1) {
        argv[argc++] = tok;
        tok = strtok(NULL, " \t\r\n");
    }
    argv[argc] = NULL;
    return argc;
}

/* Split a line on '|' into segment pointers. Returns the segment count. */
static int split_pipeline(char *line, char *segs[], int max_cmds) {
    int n = 0;
    char *save = NULL;
    char *tok = strtok_r(line, "|", &save);
    while (tok != NULL && n < max_cmds) {
        segs[n++] = tok;
        tok = strtok_r(NULL, "|", &save);
    }
    return n;
}

/* Returns 1 if argv[0] was a built-in (and has been handled), else 0.
 * Sets *should_exit to 1 when the shell should terminate.
 * Only meaningful for a single, non-piped command (runs in the parent). */
static int run_builtin(char *argv[], int argc, int *should_exit) {
    if (argc == 0) return 1;                 /* empty line: handled, do nothing */

    if (strcmp(argv[0], "exit") == 0) {
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
        if (getcwd(cwd, sizeof(cwd)) != NULL) printf("%s\n", cwd);
        else perror("pwd");
        return 1;
    }

    return 0;                                /* not a built-in */
}

/* Run a single external program and wait for it (no pipe). */
static void run_external(char *argv[]) {
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return; }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "mysh: %s: %s\n", argv[0], strerror(errno));
        _exit(127);
    }
    int status;
    if (waitpid(pid, &status, 0) < 0) perror("waitpid");
}

/* Run a pipeline of `nseg` commands (nseg >= 2), wiring each command's
 * stdout into the next command's stdin via pipe(). */
static void run_pipeline(char *segs[], int nseg) {
    int npipes = nseg - 1;
    int pipefd[MAX_CMDS - 1][2];

    for (int i = 0; i < npipes; i++) {
        if (pipe(pipefd[i]) < 0) { perror("pipe"); return; }
    }

    pid_t pids[MAX_CMDS];
    for (int i = 0; i < nseg; i++) {
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); break; }

        if (pid == 0) {
            /* child: connect stdin/stdout to the surrounding pipes */
            if (i > 0)        dup2(pipefd[i - 1][0], STDIN_FILENO);
            if (i < npipes)   dup2(pipefd[i][1], STDOUT_FILENO);

            /* a child must close EVERY pipe fd after dup2'ing what it needs */
            for (int j = 0; j < npipes; j++) {
                close(pipefd[j][0]);
                close(pipefd[j][1]);
            }

            char *argv[MAX_ARGS];
            int argc = tokenize(segs[i], argv, MAX_ARGS);
            if (argc == 0) _exit(0);
            execvp(argv[0], argv);
            fprintf(stderr, "mysh: %s: %s\n", argv[0], strerror(errno));
            _exit(127);
        }
        pids[i] = pid;
    }

    /* The parent must close ALL pipe fds, or the last command never sees
     * EOF on its stdin and the pipeline hangs forever. */
    for (int j = 0; j < npipes; j++) {
        close(pipefd[j][0]);
        close(pipefd[j][1]);
    }

    for (int i = 0; i < nseg; i++) {
        int status;
        waitpid(pids[i], &status, 0);
    }
}

int main(void) {
    char *line = NULL;
    size_t cap = 0;
    int running = 1;

    while (running) {
        printf("mysh> ");
        fflush(stdout);

        ssize_t n = getline(&line, &cap, stdin);
        if (n < 0) {                         /* EOF (Ctrl-D) or read error */
            printf("\n");
            break;
        }

        char *segs[MAX_CMDS];
        int nseg = split_pipeline(line, segs, MAX_CMDS);
        if (nseg == 0) continue;             /* blank line */

        if (nseg == 1) {
            char *argv[MAX_ARGS];
            int argc = tokenize(segs[0], argv, MAX_ARGS);
            int should_exit = 0;
            if (run_builtin(argv, argc, &should_exit)) {
                if (should_exit) running = 0;
                continue;
            }
            run_external(argv);
        } else {
            run_pipeline(segs, nseg);
        }
    }

    free(line);
    return 0;
}
