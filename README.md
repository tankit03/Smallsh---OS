# README for smallsh Shell Assignment

## Introduction

This README outlines the usage and features of `smallsh`, a custom shell written in C. `smallsh` incorporates a subset of features from well-known shells like bash. This guide will help you compile, run, and effectively use `smallsh`.

## Features

- **Command Prompt:** Utilizes `:` as the command prompt symbol.
- **Command Handling:** Supports blank lines, comments (`#`), variable expansion (`$$`), built-in commands (`exit`, `cd`, `status`), external commands, input/output redirection, and foreground and background process execution.
- **Signal Handling:** Implements custom handlers for SIGINT and SIGTSTP signals.

## Learning Outcomes

Upon completing this assignment, you should understand the Unix process API, signal concepts, and I/O redirection. You'll also be able to write programs using these concepts.

## Usage

### Compilation

1. Navigate to the directory containing `smallsh.c`.
2. Compile the program using the following command:
   ```
   gcc -std=c99 smallsh.c -o smallsh
   ```

### Running the Program

1. Start the shell by entering `./smallsh` in the terminal.
2. The shell will prompt you with `:`, indicating it's ready for commands.

### Program Functionality

- **Command Line Syntax:** 
  `command [arg1 arg2 ...] [< input_file] [> output_file] [&]`
- **Comments and Blank Lines:** Lines starting with `#` are comments. Blank lines are ignored.
- **Variable Expansion:** `$$` expands to the process ID of `smallsh`.
- **Built-in Commands:** `exit`, `cd`, and `status`.
- **External Commands:** Handled using `fork()`, `exec()`, and `waitpid()`.
- **I/O Redirection:** Managed using `dup2()` before executing commands.
- **Foreground and Background Processes:** Commands with `&` are run in the background, others in the foreground.
- **Signal Handling:** `SIGINT` and `SIGTSP` are handled as per specifications.

### Sample Execution

Refer to the assignment description for a detailed example of `smallsh` in action, demonstrating various commands and behaviors.

## Hints & Resources

- **Command Prompt:** Use `fflush()` to ensure prompt visibility.
- **Command Parsing:** Consider using a struct for command elements.
- **Variable Expansion:** Test with different scenarios.
- **Built-in Commands:** Implement these before handling external commands.
- **External Commands:** Handle exec() failures correctly.
- **I/O Redirection:** Set up redirection in the child process.
- **Foreground/Background Processes:** Handle process termination and status updates correctly.
- **Signal Handling:** Ensure signal handlers are reentrant.

## Running the File

1. Use `cd` and `ls` to navigate to the correct directory.
2. Compile the program (`gcc -std=c99 smallsh.c -o smallsh`).
3. Run the executable (`./smallsh`).
4. Follow the on-screen prompts.

---

**Creator:** Tanish Hupare
**Assignment:** smallsh Shell Assignment (Assignment 3)
