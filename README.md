# Custom Shell Implementation (w25shell)

![Language: C](https://img.shields.io/badge/Language-C-green.svg)
![Category: System Programming](https://img.shields.io/badge/Category-System%20Programming-orange.svg)
![Platform: Linux](https://img.shields.io/badge/Platform-Linux-purple.svg)

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Installation](#installation)
- [Usage](#usage)
- [Command Syntax](#command-syntax)
- [Special Features](#special-features)
  - [Built-in Commands](#built-in-commands)
  - [Piping Operations](#piping-operations)
  - [Reverse Piping](#reverse-piping)
  - [File Operations](#file-operations)
  - [I/O Redirection](#io-redirection)
  - [Sequential Execution](#sequential-execution)
  - [Conditional Execution](#conditional-execution)
- [Implementation Details](#implementation-details)
  - [Command Parsing](#command-parsing)
  - [Process Creation](#process-creation)
  - [File Descriptor Management](#file-descriptor-management)
  - [Signal Handling](#signal-handling)
- [Code Structure](#code-structure)
- [Technical Concepts](#technical-concepts)
- [Challenges and Solutions](#challenges-and-solutions)
- [Performance Considerations](#performance-considerations)
- [Future Enhancements](#future-enhancements)
- [Acknowledgements](#acknowledgements)

## 🔍 Overview

w25shell is a custom implementation of a Unix/Linux shell written in C that provides a command-line interface for interacting with the operating system. This shell supports various features including command execution, piping, redirection, sequential and conditional execution, and several special file operations. The implementation focuses on system programming concepts such as process creation, inter-process communication, and file descriptor management.

This project demonstrates advanced systems programming techniques using the Linux API, particularly focusing on:
- Process creation and management using `fork()` and `exec()`
- Inter-process communication with pipes
- File I/O and redirection
- Signal handling
- Command parsing and execution

## ✨ Features

w25shell offers a rich set of features that mimic and extend traditional Unix/Linux shells:

- **Command Execution**: Execute standard Unix/Linux commands
- **Built-in Commands**:
  - `killterm`: Terminates the current shell instance
  - `killallterms`: Terminates all running w25shell instances
- **Piping Operations**: Support for up to 5 pipe operations (`|`)
- **Reverse Piping**: Unique feature to pipe commands in reverse order (`=`)
- **File Operations**:
  - Append text between two files (`~`)
  - Count words in text files (`#`)
  - Concatenate multiple text files (`+`)
- **I/O Redirection**: Input (`<`), output (`>`), and append output (`>>`)
- **Sequential Execution**: Run multiple commands in sequence (`;`)
- **Conditional Execution**: Execute commands based on success/failure of previous commands (`&&`, `||`)
- **Argument Limitation**: Enforces 1-5 arguments per command as per requirements

## 🏗️ System Architecture

The w25shell follows a modular architecture with specialized functions for different command types:

```
                                       ┌─────────────────┐
                                       │   Main Shell    │
                                       │     Loop        │
                                       └─────────┬───────┘
                                                 │
                                                 ▼
                   ┌─────────────────────────────────────────────────┐
                   │               Command Detection                 │
                   │     Identify special characters and operations  │
                   └──────┬──────┬──────┬───────┬───────┬────────────┘
                          │      │      │       │       │
       ┌──────────────────┘      │      │       │       └───────────────────┐
       │                         │      │       │                           │
       ▼                         ▼      ▼       ▼                           ▼
┌─────────────┐  ┌────────────────┐  ┌─────┐  ┌──────────────┐  ┌─────────────────┐
│   Regular   │  │     Piping     │  │ File │  │ Sequential/  │  │    Built-in     │
│  Command    │  │   Operations   │  │ Ops  │  │ Conditional  │  │    Commands     │
│  Execution  │  │                │  │      │  │  Execution   │  │                 │
└──────┬──────┘  └───────┬────────┘  └──┬───┘  └─────┬────────┘  └────────┬────────┘
       │                 │              │            │                     │
       │                 │              │            │                     │
       ▼                 ▼              ▼            ▼                     ▼
┌────────────────────────────────────────────────────────────────────────────────┐
│                             Process Creation and Management                     │
│              (fork, exec, waitpid, pipe, dup2, signal handling)                │
└────────────────────────────────────────────────────────────────────────────────┘
```

## 🔧 Installation

### Prerequisites

- Linux operating system
- GCC compiler
- Make (optional, for easier compilation)

### Compilation

1. Clone the repository:
   ```bash
   git clone https://github.com/Arshnoor-Singh-Sohi/Custom-Shell-Implementation.git
   cd Custom-Shell-Implementation
   ```

2. Compile the shell:
   ```bash
   gcc -o w25shell W25shell.c
   ```

   Or with additional flags for debugging:
   ```bash
   gcc -Wall -g -o w25shell W25shell.c
   ```

3. Make the executable accessible from anywhere (optional):
   ```bash
   chmod +x w25shell
   sudo cp w25shell /usr/local/bin/
   ```

## 🚀 Usage

1. Start the shell:
   ```bash
   ./w25shell
   ```

2. Use the shell like any standard Unix/Linux shell:
   ```
   w25shell$ ls -l
   w25shell$ pwd
   w25shell$ grep "pattern" file.txt
   ```

3. Exit the shell:
   ```
   w25shell$ killterm
   ```

## 📝 Command Syntax

The w25shell enforces specific rules for command execution:

1. **Argument Limitation**:
   - Each command can have between 1 and 5 arguments (including the command itself)
   - This limitation applies to each command in multi-command operations

2. **Basic Command Format**:
   ```
   w25shell$ command [args]
   ```
   Examples:
   ```
   w25shell$ pwd                    # 1 argument
   w25shell$ ls -l                  # 2 arguments
   w25shell$ grep pattern file.txt  # 3 arguments
   w25shell$ ls -l -t -a            # 4 arguments
   ```

3. **Special Character Usage**:
   - Piping: `command1 | command2`
   - Reverse Piping: `command1 = command2`
   - File Append: `file1.txt ~ file2.txt`
   - Word Count: `# file.txt`
   - File Concatenation: `file1.txt + file2.txt + file3.txt`
   - I/O Redirection: `command < infile.txt`, `command > outfile.txt`, `command >> appendfile.txt`
   - Sequential Execution: `command1 ; command2 ; command3`
   - Conditional Execution: `command1 && command2 || command3`

## ⚙️ Special Features

### Built-in Commands

#### killterm

Terminates the current shell instance.

```
w25shell$ killterm
```

Implementation details:
- Directly calls `exit(0)` to terminate the current process
- Displays a goodbye message before termination

#### killallterms

Terminates all running instances of w25shell.

```
w25shell$ killallterms
```

Implementation details:
- Uses `pgrep -f w25shell` to find all running w25shell processes
- Sends `SIGTERM` signal to each process using `kill()`
- Keeps track of the current process ID to avoid early self-termination
- Finally terminates itself

### Piping Operations

The shell supports piping up to 5 operations, allowing output from one command to be used as input for another.

```
w25shell$ ls -l | grep ".txt" | wc -l
```

Implementation details:
- Creates a separate process for each command using `fork()`
- Establishes pipes between processes using `pipe()`
- Redirects standard output and input using `dup2()`
- Each command operates on the output of the previous command
- Maximum of 5 pipe operations supported

Piping Execution Flow:

```
       ┌──────────┐     ┌───────────┐     ┌──────────┐
       │ Command1 │     │ Command2  │     │ Command3 │
       │ Process  │     │ Process   │     │ Process  │
       └────┬─────┘     └─────┬─────┘     └────┬─────┘
            │                 │                 │
            │     pipe 1      │     pipe 2      │
stdout ─────┼─────────────────┼─────────────────┼────► stdin
            │                 │                 │
    ┌───────┘         ┌───────┘         ┌───────┘
    │                 │                 │
    ▼                 ▼                 ▼
┌─────────┐      ┌─────────┐      ┌─────────┐
│ fd[0][1]│─────►│ fd[0][0]│      │ fd[1][0]│
│ (write) │      │ (read)  │      │ (read)  │
└─────────┘      └─────────┘      └─────────┘
                      │                 ▲
                      │                 │
                      │                 │
                      └─────────────────┘
                           fd[1][1]
                           (write)
```

### Reverse Piping

A unique feature of w25shell, reverse piping allows commands to be executed in reverse order, with each command's output feeding into the previous one, and the final output going to stdout.

```
w25shell$ wc -w = wc = ls -l
```

In the example above, the output of `ls -l` feeds into the first `wc`, the output of which feeds into the second `wc -w`, with the final result displayed on the terminal.

Implementation details:
- Similar to regular piping, but processes are created in reverse order
- Processes are connected through pipes in reverse order
- Command to the right of '=' feeds its output to the command on the left
- Maximum of 5 reverse pipe operations supported

Reverse Piping Execution Flow:

```
       ┌──────────┐     ┌───────────┐     ┌──────────┐
       │ Command1 │     │ Command2  │     │ Command3 │
       │ Process  │     │ Process   │     │ Process  │
       └────┬─────┘     └─────┬─────┘     └────┬─────┘
            │                 │                 │
            │     pipe 1      │     pipe 2      │
stdout ◄────┼─────────────────┼─────────────────┼────── stdin
            │                 │                 │
    ┌───────┘         ┌───────┘         ┌───────┘
    │                 │                 │
    ▼                 ▼                 ▼
┌─────────┐      ┌─────────┐      ┌─────────┐
│ fd[0][0]│◄─────│ fd[0][1]│      │ fd[1][1]│
│ (read)  │      │ (write) │      │ (write) │
└─────────┘      └─────────┘      └─────────┘
                      │                 ▲
                      │                 │
                      │                 │
                      └─────────────────┘
                           fd[1][0]
                           (read)
```

### File Operations

#### Append Text Operation (~)

Appends the contents of two text files to each other. The content of the second file is appended to the first file, and the content of the first file is appended to the second file.

```
w25shell$ file1.txt ~ file2.txt
```

Implementation details:
- Opens both files and reads their contents into memory
- Appends the content of each file to the end of the other
- Both files must have .txt extension
- Changes are saved to both files

File Append Operation Process:

```
┌───────────┐                         ┌───────────┐
│           │                         │           │
│  file1.txt│                         │ file2.txt │
│           │                         │           │
└─────┬─────┘                         └─────┬─────┘
      │                                     │
      │ Read                                │ Read
      ▼                                     ▼
┌─────────────┐                     ┌─────────────┐
│ file1 data  │                     │ file2 data  │
│ in memory   │                     │ in memory   │
└─────┬───────┘                     └───────┬─────┘
      │                                     │
      │                                     │
      │     ┌───────────────────────┐       │
      └────►│   Append Operation    │◄──────┘
            └───────────┬───────────┘
               │                  │
               │                  │
               ▼                  ▼
┌───────────────────────┐   ┌───────────────────────┐
│                       │   │                       │
│  file1.txt + file2data│   │  file2.txt + file1data│
│                       │   │                       │
└───────────────────────┘   └───────────────────────┘
```

#### Word Count Operation (#)

Counts the number of words in a text file.

```
w25shell$ # file.txt
```

Implementation details:
- Opens the specified file in read mode
- Reads the file character by character
- Counts word transitions (non-word character to word character)
- File must have .txt extension
- Displays the word count on standard output

Word Counting Algorithm:

```
Start with:
- word_count = 0
- in_word = 0 (not currently in a word)

For each character in the file:
┌───────────────────┐
│ Read character    │
└─────────┬─────────┘
          │
          ▼
┌────────────────────────┐     Yes    ┌───────────────────┐
│ Is character a         │────────────►│ Set in_word = 0   │
│ space/tab/newline?     │            └───────────────────┘
└────────────┬───────────┘
             │ No
             ▼
┌────────────────────────┐     No     ┌───────────────────┐
│ Was previously in      │────────────►│ word_count += 1   │
│ a word? (in_word==1)   │            │ Set in_word = 1   │
└────────────────────────┘            └───────────────────┘

Final word_count = Number of words in file
```

#### File Concatenation Operation (+)

Concatenates the contents of multiple text files and displays the result on standard output.

```
w25shell$ file1.txt + file2.txt + file3.txt
```

Implementation details:
- Parses the command to identify each file
- Opens each file in sequence
- Reads and outputs each file's content to standard output
- All files must have .txt extension
- Maximum of 5 files can be concatenated
- Files are processed in the order specified

File Concatenation Process:

```
┌───────────┐   ┌───────────┐   ┌───────────┐
│ file1.txt │   │ file2.txt │   │ file3.txt │
└─────┬─────┘   └─────┬─────┘   └─────┬─────┘
      │               │               │
      │ Read          │ Read          │ Read
      ▼               ▼               ▼
┌───────────┐   ┌───────────┐   ┌───────────┐
│ Content1  │   │ Content2  │   │ Content3  │
└─────┬─────┘   └─────┬─────┘   └─────┬─────┘
      │               │               │
      ▼               ▼               ▼
┌──────────────────────────────────────────┐
│                                          │
│      Concatenated Output to stdout       │
│                                          │
│ Content1 + Content2 + Content3           │
│                                          │
└──────────────────────────────────────────┘
```

### I/O Redirection

The shell supports standard input/output redirection, allowing commands to read from files and write results to files.

#### Input Redirection (<)

Redirects file content as input to a command.

```
w25shell$ grep "pattern" < file.txt
```

Implementation details:
- Opens the specified file in read mode
- Duplicates the file descriptor to standard input using `dup2()`
- Command reads from the file instead of keyboard

#### Output Redirection (>)

Redirects command output to a file, overwriting existing content.

```
w25shell$ ls -l > listing.txt
```

Implementation details:
- Opens the specified file in write mode with truncate option
- Duplicates the file descriptor to standard output using `dup2()`
- Command output goes to the file instead of terminal

#### Append Output Redirection (>>)

Redirects command output to a file, appending to existing content.

```
w25shell$ ls -l >> listing.txt
```

Implementation details:
- Opens the specified file in write mode with append option
- Duplicates the file descriptor to standard output using `dup2()`
- Command output is appended to the end of the file

I/O Redirection Diagram:

```
Input Redirection (<):
┌────────────┐        ┌─────────────┐        ┌─────────────┐
│            │        │             │        │             │
│  file.txt  │───────►│ dup2(fd, 0) │───────►│  Command    │
│            │        │             │        │             │
└────────────┘        └─────────────┘        └─────────────┘

Output Redirection (>):
┌────────────┐        ┌─────────────┐        ┌─────────────┐
│            │        │             │        │             │
│  Command   │───────►│ dup2(fd, 1) │───────►│  file.txt   │
│            │        │             │        │ (overwrite) │
└────────────┘        └─────────────┘        └─────────────┘

Append Output Redirection (>>):
┌────────────┐        ┌─────────────┐        ┌─────────────┐
│            │        │             │        │             │
│  Command   │───────►│ dup2(fd, 1) │───────►│  file.txt   │
│            │        │             │        │  (append)   │
└────────────┘        └─────────────┘        └─────────────┘
```

### Sequential Execution

Run multiple commands in sequence, separated by semicolons (`;`). Each command executes regardless of whether the previous command succeeded or failed.

```
w25shell$ date ; pwd ; ls -l
```

Implementation details:
- Parses the input string to identify command segments separated by `;`
- Executes each command in order
- Creates a new process for each command
- Waits for each command to complete before executing the next
- Maximum of 4 commands can be executed sequentially

Sequential Execution Process:

```
┌────────────┐   ┌────────────┐   ┌────────────┐
│ Command1   │   │ Command2   │   │ Command3   │
└──────┬─────┘   └──────┬─────┘   └──────┬─────┘
       │                │                │
       │                │                │
       ▼                ▼                ▼
┌──────────────────────────────────────────────┐
│                  Timeline                    │
├──────────────┬──────────────┬──────────────┬─┘
│  Execute     │  Execute     │  Execute     │
│  Command1    │  Command2    │  Command3    │
│              │              │              │
└──────────────┴──────────────┴──────────────┘
```

### Conditional Execution

Execute commands based on the success or failure of previous commands, using `&&` (AND) and `||` (OR) operators.

```
w25shell$ mkdir test && cd test || echo "Failed to create and enter directory"
```

Implementation details:
- Parses the input to identify command segments and operators
- Executes commands from left to right
- For `&&` (AND), the right command executes only if the left command succeeds
- For `||` (OR), the right command executes only if the left command fails
- Can combine AND and OR operators in a single command line
- Maximum of 5 conditional operators supported

Conditional Execution Logic:

```
     Command1
        │
        ▼
┌──────────────┐
│  Execution   │
└──────┬───────┘
       │
       ▼
┌──────────────┐
│  Success?    │─────────────┐
└──────┬───────┘   No (fail) │
       │ Yes                 │
       ▼                     ▼
┌──────────────┐     ┌──────────────┐
│ Operator &&? │     │ Operator ||? │
└──────┬───────┘     └──────┬───────┘
       │ Yes                │ Yes
       ▼                    ▼
┌──────────────┐     ┌──────────────┐
│   Execute    │     │   Execute    │
│  Next Command│     │ Next Command │
└──────────────┘     └──────────────┘
```

## 🔬 Implementation Details

### Command Parsing

The w25shell parses user input through several steps:

1. **Initial Input Collection**:
   - Uses `fgets()` to safely read user input from standard input
   - Removes the trailing newline character
   - Checks for empty commands

2. **Command Type Detection**:
   - Examines the input string for special characters (`|`, `=`, `~`, `#`, etc.)
   - Routes the command to the appropriate handler function

3. **Argument Parsing**:
   - Splits commands into individual arguments using `strtok()`
   - Enforces the argument limit (1-5 arguments per command)
   - Properly handles whitespace between arguments

4. **Special Command Handling**:
   - Each special command type has its own parsing logic
   - For complex commands, multiple parsing stages may be used

The following code shows the core of the command parsing function:

```c
int parse_command(char *input, char **args)
{
    // Keep track of how many words we found
    int word_count = 0;

    // This will hold each word as we find it
    char *current_word;

    // Get the first word from input - strtok breaks string by spaces
    current_word = strtok(input, " ");

    // Loop until we run out of words or hit our limit
    while (current_word != NULL)
    {
        if (word_count < MAX_ARGS)
        {
            args[word_count] = current_word;
        }
        word_count++;
        current_word = strtok(NULL, " ");
    }

    // Add NULL terminator
    if (word_count <= MAX_ARGS)
    {
        args[word_count] = NULL;
    }
    else
    {
        args[MAX_ARGS] = NULL;
        // Return a special value to indicate too many arguments
        return -1;
    }

    // Return how many words we found
    return word_count;
}
```

### Process Creation

The shell creates new processes for command execution using the following approach:

1. **Process Cloning**:
   - Uses `fork()` to create a child process
   - The child process is an exact copy of the parent at the time of forking

2. **Command Loading**:
   - Child process uses `execvp()` to replace itself with the requested command
   - If successful, the child process is completely replaced
   - If `execvp()` fails, an error message is displayed

3. **Process Synchronization**:
   - Parent uses `waitpid()` to wait for child process completion
   - Captures exit status to determine command success/failure

Process Creation Flow:

```
                   ┌────────────────┐
                   │  Parent Shell  │
                   └────────┬───────┘
                            │
                            │ fork()
                            │
              ┌─────────────┴─────────────┐
              │                           │
              ▼                           ▼
    ┌────────────────┐           ┌────────────────┐
    │  Child Process │           │  Parent Process│
    └────────┬───────┘           └────────┬───────┘
             │                            │
             │ execvp()                   │ waitpid()
             │                            │ (waits for child)
             ▼                            │
    ┌────────────────┐                    │
    │   Command      │                    │
    │  Execution     │                    │
    └────────┬───────┘                    │
             │                            │
             │ exit()                     │
             │                            │
             └─────────────────┐          │
                               ▼          ▼
                      ┌────────────────────────┐
                      │    Shell continues     │
                      └────────────────────────┘
```

### File Descriptor Management

The shell manages file descriptors for various operations using the following techniques:

1. **Pipe Creation**:
   - Creates pipes using `pipe()` system call
   - Each pipe has a read end (fd[0]) and a write end (fd[1])

2. **File Descriptor Duplication**:
   - Uses `dup2()` to redirect standard input/output
   - For piping: connects stdout of one process to stdin of another
   - For I/O redirection: connects files to standard streams

3. **File Descriptor Cleanup**:
   - Properly closes unused file descriptors to prevent leaks
   - Ensures parent process closes pipes after child creation

File Descriptor Management Example:

```c
// For output redirection (e.g., command > file.txt)
int output_fd = open(output_filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
if (output_fd < 0) {
    perror("Couldn't open output file");
    exit(EXIT_FAILURE);
}

// Redirect standard output to our file
if (dup2(output_fd, STDOUT_FILENO) < 0) {
    perror("Output redirection failed");
    exit(EXIT_FAILURE);
}

// Don't need the original file descriptor anymore
close(output_fd);
```

### Signal Handling

The shell implements signal handling for the following purposes:

1. **Process Termination**:
   - For `killterm` command: terminates the current shell process
   - For `killallterms` command: finds and terminates all w25shell processes

2. **Signal Sending**:
   - Uses `kill()` function to send signals to processes
   - Primarily uses `SIGTERM` for graceful termination

Signal Handling for `killallterms`:

```c
// First get our own process ID so we don't kill ourselves too early
pid_t my_own_pid = getpid();

// Find all w25shell processes
FILE *process_list = popen("pgrep -f w25shell", "r");

// Read each process ID and terminate it
while (fgets(process_id_text, sizeof(process_id_text), process_list) != NULL) {
    int process_id_number = atoi(process_id_text);
    
    // Don't kill ourselves yet
    if (process_id_number != my_own_pid) {
        int kill_result = kill(process_id_number, SIGTERM);
        // Check kill result
    }
}

// Now terminate our own process
exit(0);
```

## 📁 Code Structure

The w25shell is organized into the following major components:

| Component | Description | Functions |
|-----------|-------------|-----------|
| **Main Shell Loop** | Core command loop that drives the shell | `main()` |
| **Command Detection** | Identifies command types based on special characters | Main `while` loop in `main()` |
| **Command Parsing** | Breaks commands into arguments | `parse_command()` |
| **Regular Command Execution** | Handles standard commands | `execute_command()` |
| **Special Command Handling** | Implements special built-in commands | `handle_special_commands()` |
| **Piping Operations** | Implements standard and reverse piping | `handle_multi_pipe()`, `handle_reverse_pipe()` |
| **File Operations** | Handles file-specific operations | `handle_append()`, `handle_word_count()`, `handle_concat()` |
| **I/O Redirection** | Manages input/output redirection | `handle_redirection()` |
| **Sequential Execution** | Implements sequential command execution | `handle_sequential()` |
| **Conditional Execution** | Implements conditional command execution | `handle_conditional()` |

The code uses a modular approach with specialized functions for each command type, promoting code organization and maintainability.

## 🧠 Technical Concepts

The w25shell implementation demonstrates several important systems programming concepts:

### Process Management

- **Process Creation**: Creating new processes using `fork()`
- **Process Replacement**: Loading new programs using `execvp()`
- **Process Synchronization**: Waiting for processes using `waitpid()`
- **Process Termination**: Ending processes using `exit()` and `kill()`

### Inter-Process Communication

- **Pipes**: Connecting processes using `pipe()`
- **File Descriptors**: Managing I/O with file descriptors
- **Standard Streams**: Redirecting stdin, stdout, and stderr

### File Operations

- **File I/O**: Reading and writing files using `open()`, `read()`, `write()`, etc.
- **File Manipulation**: Appending, counting, and concatenating file contents
- **File Redirection**: Connecting files to process streams using `dup2()`

### Signal Handling

- **Signal Sending**: Sending signals to processes with `kill()`
- **Process Discovery**: Finding processes with `popen()` and `pgrep`

### String Manipulation

- **String Parsing**: Breaking strings into tokens using `strtok()`
- **String Searching**: Finding characters and substrings using `strchr()`, `strstr()`
- **String Handling**: Managing strings with `strcpy()`, `strlen()`, etc.

## 🛠️ Challenges and Solutions

Throughout the implementation of w25shell, several challenges were encountered and solved:

### Challenge 1: Piping Multiple Commands

**Problem**: Implementing a system that can pipe multiple commands together, with each command's output feeding into the next command's input.

**Solution**: Created a multi-stage approach:
1. Parse the input to identify each command segment
2. Create the necessary pipes between processes
3. For each command, fork a new process
4. Configure stdin/stdout for each process to connect through pipes
5. Execute each command in its own process
6. Properly close unused pipe ends to prevent hanging

### Challenge 2: Reverse Piping Implementation

**Problem**: Implementing reverse piping, where commands execute in reverse order with output flowing right to left.

**Solution**: Modified the piping algorithm to:
1. Parse commands from right to left
2. Create processes in reverse order
3. Configure pipes to connect processes in reverse
4. Handle file descriptor redirection appropriately

### Challenge 3: Conditional Execution Logic

**Problem**: Implementing the complex logic of AND (&&) and OR (||) operators that determine whether subsequent commands execute based on the success/failure of previous commands.

**Solution**: Created a system that:
1. Parses the command line to identify command segments and operators
2. Executes each command in sequence
3. Captures the exit status of each command
4. Uses the exit status to determine whether to execute the next command
5. Handles combinations of AND and OR operators

### Challenge 4: Preventing Resource Leaks

**Problem**: Ensuring all file descriptors, memory allocations, and processes are properly cleaned up to prevent resource leaks.

**Solution**: Implemented careful resource management:
1. Tracking all file descriptors created
2. Closing unused file descriptors promptly
3. Properly freeing allocated memory
4. Waiting for child processes to complete
5. Checking error codes for all system calls

### Challenge 5: Proper Signal Handling

**Problem**: Implementing the `killallterms` command to find and terminate all instances of w25shell without causing issues.

**Solution**: Developed a careful termination process:
1. Using `pgrep` to find all w25shell processes
2. Storing the current process ID to avoid early self-termination
3. Sending `SIGTERM` signals to all other w25shell processes
4. Finally terminating the current process

## ⚙️ Performance Considerations

The w25shell implementation takes into account several performance considerations:

1. **Buffer Sizes**: Uses appropriately sized buffers to optimize memory usage
2. **File Reading Strategy**: For file operations, reads files in chunks rather than byte-by-byte for better performance
3. **Pipe Handling**: Closes unused pipe ends promptly to prevent hanging processes
4. **Process Management**: Creates only the necessary number of processes for each operation
5. **Error Handling**: Implements comprehensive error checking to fail gracefully

## 🔮 Future Enhancements

Several potential enhancements could be added to the w25shell in the future:

1. **Command History**: Implement command history and recall functionality
2. **Tab Completion**: Add tab completion for commands and filenames
3. **Environment Variables**: Support for environment variables and variable expansion
4. **Shell Scripting**: Add scripting capabilities with control structures
5. **Job Control**: Implement background processes and job control functions
6. **Wildcard Expansion**: Support for filename wildcards (*, ?, etc.)
7. **Multiple I/O Redirection**: Support for multiple redirections in a single command
8. **Error Redirection**: Support for stderr redirection (2>)
9. **Custom Prompt**: Configurable shell prompt
10. **Command Aliasing**: Support for command aliases

## 🙏 Acknowledgements

This project was developed as part of the COMP-8567 (Advanced Systems Programming) course, which provided the foundation for understanding the complex systems programming concepts implemented in this shell.

Special thanks to:
- The course instructor for providing the project requirements and guidance
- The Linux programming community for documentation and examples
- Contributors to the GNU C Library and Linux manpages for comprehensive system call documentation

---
