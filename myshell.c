/* $begin my-shell */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128
#define MAXPIPES 20

/* Function prototypes */
void createProcess(char *cmdline);
int myshell_parseline(char *buf, char **argv);
int builtin_command(char **argv); 

void myshell_piping(char *cmdline);
void createStartProcess(char *cmdline, int* fd);
void createPipeProcess(char *cmdline, int fd_in[2], int fd_out[2]);
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

int findPipeCount(char* cmdline) {
    int length = strlen(cmdline);
    int i, cnt=0;

    for(i=0;i<length;i++) {
        if(cmdline[i] == '|') cnt++;
    }
    return cnt;
}

void myshell_piping(char *cmdline) 
{
    int fd[MAXPIPES][2];
    char buf[MAXLINE];   /* Holds modified command line */    
    char *seperatedCmdLine;
    char *temp;
    int pipeCount,i;

    strcpy(buf, cmdline);
    pipeCount = findPipeCount(buf);
    
    if(pipeCount == 0) {
        createProcess(cmdline);
        return;
    } else if(pipeCount >= MAXPIPES) { // 파이프가 너무 많을 경우, 해당 명령어를 실행하지 않고 종료한다.
        unix_error("Too Many Pipes...");
        exit(0);
    }
    //----
    for(i=0;i<pipeCount;i++) {
        if(pipe(fd[i]) < 0) {
            unix_error("Pipe error");
            exit(0);
        }
    }
    seperatedCmdLine = strtok(buf, "|");
    createStartProcess(seperatedCmdLine, fd[0]);
    Close(fd[0][1]);
    for(i=1;i<pipeCount;i++) { //pipe의 개수 +1개만큼 조각이 나뉨. 그러나 마지막은 건너뛰므로 pipeCount까지
        seperatedCmdLine = strtok(NULL, "|");
        createPipeProcess(seperatedCmdLine, fd[i-1], fd[i]);
        Close(fd[i][1]);
    }
    seperatedCmdLine = strtok(NULL, "|");
    createEndProcess(seperatedCmdLine, fd[i-1]);

    /*
    자식 프로세스들을 만들어 준 뒤, 부모 프로세스는 사용하지 않는 fd를 닫아준다.
    이후 프로세스 전체가 끝날 때까지 Wait한다.
    Wait하는 중 만약 exit 명령이 들어오면, 
    모든 프로세스를 리핑한 뒤 exit해준다.
    */
    for(i=0;i<pipeCount;i++){
        Close(fd[i][0]);
    }
    int status;
    int exitFlag = 0;
    for(i=0;i<=pipeCount;i++){
        Wait(&status);
        if(WIFSIGNALED(status)) {
            exitFlag = 1;
        }
    }
    if(exitFlag) {
        /* 
        자식 프로세스에서 시그널이 발생한 경우, 오류를 처리해 준다. 
        phase1에서 시그널이 생기는 경우는 exit 처리 경우뿐이므로
        exit(0)를 해주도록 단순 처리하였다.

        또한, 파이프 연산의 경우 마지막 연산이 exit인 경우에만 exit가 유효하다.
        이외의 연산은 piping 과정의 일부이므로, 부모 프로세스에 영향을 주지 않는다.
        그렇기 때문에, exit에 대한 대응은 endProcess에 한하도록 하였다.
        */
        exit(0);
    }
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


    if (!builtin_command(argv)) {
        if(Fork() == 0){
            Dup2(fd[1], STDOUT_FILENO);
            if (!strcmp(argv[0], "exit")) { /* exit command */
                exit(0);
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

void createPipeProcess(char *cmdline, int* fd_in, int* fd_out)
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
    if (!builtin_command(argv)) {
        if(Fork() == 0){
            Dup2(fd_in[0], STDIN_FILENO); //이거 문제일수도!!! ls | cat | ls -al
            Dup2(fd_out[1], STDOUT_FILENO); //지금 이게 안되서, 표준출력으로 걍 나가버림;;;
            Close(fd_in[0]);
            Close(fd_out[1]);
            //child process        
            if (!strcmp(argv[0], "exit")) { /* exit command */
                exit(0);
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
    if (!builtin_command(argv)) {
        if(Fork() == 0){
            Dup2(fd[0], STDIN_FILENO);
            Close(fd[0]);
            
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
