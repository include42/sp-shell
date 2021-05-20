# Project4. buiding shell
### Sogang University CSE4100: System Programming 

## Phase I: Building and Testing Your Shell
### Task Specifications:
Your fist task is to write a simple shell and starting processes is the main function of linux shells.
So, writing a shell means that you need to know exactly what is going on with processes and how they start.

Your shell should be able to execute the basic internal shell commands such as,

- cd, cd..: to navigate the directories in your shell
- ls: listing the directory contents
- mkdir, rmdir: create and remove directory using your shell
- touch, cat, echo: creating, reading, and printing the contents of a file
- exit: shell quits and returns to original program (parent process)

A command is always executed by the child process created via forking by the parent process.