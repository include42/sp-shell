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

## Phase II: Redirection and Piping in Your Shell
### Task Specifications:
In this phase, you will be extending the functionality of the simple of the simple shell example that you programmed in project phase I.
Start by creating a new process for each command in the pipeline and making the parent wait for the last command. This will allow running simple commands such as "ls -al | grep filename".
The key idea is; passing output of one process as input to another, Note that, you can have multiple chains of pipes as your command line argument.

### Evaluation:
Following shell commands with piping can be evaluated, e.g.,

- ls -al | grep filename
- cat filename | less
- cat filename | grep -v "abc" | sort -r
