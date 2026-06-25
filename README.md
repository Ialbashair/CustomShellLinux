# CustomShellLinux

A minimal, custom shell for Linux environments implemented in C. This project provides a foundational Read-Evaluate-Print Loop (REPL) and supports basic shell functionalities, including built-in commands, external command execution, and pipeline processing.

## Features

* **REPL Interface:** Continuously prompts for input, parses commands, and executes them in a loop.
* **Built-in Commands:** Supports essential built-ins such as `cd` (change directory), `pwd` (print working directory), and `exit` (terminate shell).
* **External Command Execution:** Utilizes `fork`, `execvp`, and `waitpid` to execute external Linux binaries.
* **Piping Support:** Allows chaining commands together using pipes (`|`), routing the standard output of one command into the standard input of the next.

## Prerequisites

* [cite_start]**Compiler:** GCC (GNU Compiler Collection).
* [cite_start]**Environment:** A Linux environment utilizing standard POSIX libraries (e.g., glibc)[cite: 2, 3].

