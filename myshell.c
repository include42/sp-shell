/* $begin my-shell */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void createProcess(char *cmdline);
int myshell_parseline(char *buf, char **argv);
int builtin_command(char **argv); 

void myshell_piping(char *cmdline);
void createStartProcess(char *cmdline, int* fd);
void createPipeProcess(char *cmdline, int* fd);
void createEndProcess(char *cmdline, int* fd);

/* $begin shellmain */
int main() 
{
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	    /* Read */
        printf("CSE4100-SP-P4> ");                   
        Fgets(cmdline, MAXLINE, stdin); 
        if (feof(stdin)){
            exit(0);
        }
    	myshell_piping(cmdline);
    } 
}
/* $end shellmain */

void myshell_piping(char *cmdline) 
{
    int fd[2];
    char buf[MAXLINE];   /* Holds modified command line */    
    char *seperatedCmdLine;
    char *temp;
    int processCnt = 0;

    strcpy(buf, cmdline);
    
    if(strstr(buf, "|") == NULL) {
        createProcess(cmdline);
        return;
    }

/*
아이디어: 파이프의 여닫기를 프로세스 쪼개고 나서 해야해!! 그게 이슈의 이유인듯!!!!!!
혹은, 아예 함수마다 파이프 따던가
*/
    if(pipe(fd) < 0) {
        unix_error("Pipe error");
        exit(0);
    }
    seperatedCmdLine = strtok(buf, "|");
    //printf("first str is %s buf %s\n", seperatedCmdLine, buf);
    createStartProcess(seperatedCmdLine, fd);
    processCnt++;
    Wait(0);
    seperatedCmdLine = strtok(NULL, "|");
    while(1) { //이걸 어떻게 마지막인걸 알지????????
        temp = strtok(NULL, "|");
        if(temp == NULL) break;
        
        //printf("seperate piping now... : %s\n", seperatedCmdLine);
        createPipeProcess(seperatedCmdLine, fd);
        processCnt++;
        Wait(0);
        seperatedCmdLine = temp;
    }
    //printf("seperate end... : %s\n", seperatedCmdLine);
    createEndProcess(seperatedCmdLine, fd);
    Wait(0);
    processCnt++;

    Close(fd[0]);
    Close(fd[1]);
    // printf("wait %d processes...\n",processCnt);
    // while(processCnt > 0) {
    //     printf("wait first is end\n");
    //     Wait(0);
    //     processCnt--;
    // }
}
void createStartProcess(char *cmdline, int* fd)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    
    strcpy(buf, cmdline);
    bg = myshell_parseline(buf, argv); 
    /* Ignore empty lines */
    if (argv[0] == NULL) {
	    return;   
    }


    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if(Fork() == 0){
            Dup2(fd[1], STDOUT_FILENO);
            Close(fd[0]);
            Close(fd[1]);
            //child process
            if (!strcmp(argv[0], "exit")) { /* exit command */
                pid = getpid();
                kill(pid, SIGINT);     /* 인터럽트 시그널을 발생시킨다. */
            }
            //---
            strcat(path, argv[0]);
            if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        } 
    }
    return;
}

void createPipeProcess(char *cmdline, int* fd)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    
    strcpy(buf, cmdline);
    bg = myshell_parseline(buf, argv); 
    /* Ignore empty lines */
    if (argv[0] == NULL) {
	    return;   
    }
    if(Fork() == 0){
        Dup2(fd[0], STDIN_FILENO);
        Dup2(fd[1], STDOUT_FILENO);
        Close(fd[0]);
        Close(fd[1]);
        //child process
        if (!strcmp(argv[0], "exit")) { /* exit command */
            pid = getpid();
            kill(pid, SIGINT);     /* 인터럽트 시그널을 발생시킨다. */
        }
        //---
        strcat(path, argv[0]);
        if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }
    return;
}

void createEndProcess(char *cmdline, int* fd)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    char stdBuf[MAXLINE];
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    

    strcpy(buf, cmdline);
    bg = myshell_parseline(buf, argv); 
    /* Ignore empty lines ls*/
    if (argv[0] == NULL) {
	    return;   
    }
    if(Fork() == 0){
        Dup2(fd[0], STDIN_FILENO);
        Close(fd[0]);
        Close(fd[1]);
        //child process
        if (!strcmp(argv[0], "exit")) { /* exit command */
            pid = getpid(); 
            kill(pid, SIGINT);     /* 인터럽트 시그널을 발생시킨다. */
        }
        //---
        strcat(path, argv[0]);
        if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
            printf("%s: Command not found.\n", argv[0]);
            exit(0);
        }
    }
    return;
}

/* $begin createProcess */
/* createProcess - Evaluate a command line and fork chile process */
void createProcess(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    
    strcpy(buf, cmdline);
    bg = myshell_parseline(buf, argv); 
    /* Ignore empty lines */
    if (argv[0] == NULL) {
	    return;   
    }

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if(Fork() == 0){
            //child process
            if (!strcmp(argv[0], "exit")) { /* exit command */
                pid = getpid();   
                kill(pid, SIGINT);     /* 인터럽트 시그널을 발생시킨다. */
            }
            //---
            strcat(path, argv[0]);
            if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        } else {
            //parent
            Wait(&status);
            if(WIFSIGNALED(status)) {
                /* 
                자식 프로세스에서 시그널이 발생한 경우, 오류를 처리해 준다. 
                phase1에서 시그널이 생기는 경우는 exit 처리 경우뿐이므로
                exit(0)를 해주도록 단순 처리하였다.
                */
                exit(0);
            }
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "cd")) {    /* Ignore singleton & */
	    chdir(argv[1]);
        return 1;
    }
    if (!strcmp(argv[0], "&")) {    /* Ignore singleton & */
	    return 1;
    }
    return 0;                     /* Not a builtin command */
}
/* $end createProcess block */

/* $begin myshell_parseline */
/* myshell_parseline - Parse the command line and build the argv array */
int myshell_parseline(char *buf, char **argv) 
{
    char *delim;         /* Points to first space delimiter */
    int argc;            /* Number of args */
    int bg;              /* Background job? */

    buf[strlen(buf)-1] = ' ';  /* Replace trailing '\n' with space */
    while (*buf && (*buf == ' ')) /* Ignore leading spaces */
	buf++;

    /* Build the argv list */
    argc = 0;
    while ((delim = strchr(buf, ' '))) {
	argv[argc++] = buf;
	*delim = '\0';
	buf = delim + 1;
	while (*buf && (*buf == ' ')) /* Ignore spaces */
            buf++;
    }
    argv[argc] = NULL;
    
    if (argc == 0)  /* Ignore blank line */
	return 1;

    /* Should the job run in the background? */
    if ((bg = (*argv[argc-1] == '&')) != 0)
	argv[--argc] = NULL;

    return bg;
}
/* $end myshell_parseline */
