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

## Phase III: Run Processes in Background in Your Shell
### Task Specifications:
It is the last phase of your MyShell project, where you enable your shell to run processes in background. Linux shells support the notion of job control, which allows users to move jobs back and fore between background and foreground, and to change the process state (running, stopped, or terminated) of the processes in a job.

Your shell must start a command in the background if an '&' is given in the command line arguments. Besides, your shell must also provide various built-in commands that support job control.

For example: 
- jobs: List the running and stopped background jobs.
- bg <job>: Change a stopped background job to a running background job.
- fg <job>: Change a stopped or running background job to a running in the foreground.
- kill <job>: Terminated a job.

### Evaluation: 
- [ ] Any command from project phase II can be given with '&' at the end of commandline
- [ ] ls -al | grep filename &
- [ ] cat sample | grep -v a &
