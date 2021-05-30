/* $begin my-shell */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

#define MAXBG 128
#define MAXBG_NAME 30

#define MAXPIPES 20
#define PIPEIN 1
#define PIPEOUT 2

enum STATE {
    RUNNING,
    STOPPED,
    TERMINATED
};

typedef struct _BG {
    int num;
    char name[MAXBG_NAME];
    enum STATE state;
    pid_t pid[MAXPIPES];
    int pidSize;
} BG;

/* Function prototypes */
pid_t createProcess(char *cmdline);
int myshell_parseline(char *buf, char **argv);
int builtin_command(char **argv); 

void myshell_piping(char *cmdline, int *bgSize, BG *bgList);
pid_t createPipeProcess(char *cmdline, int* fd, int pipetype);

/* $begin shellmain */
int main() 
{
    int bgSize = 0;
    BG bgList[MAXBG]={};
    char cmdline[MAXLINE]; /* Command line */

    while (1) {
	    /* Read */
        printf("CSE4100-SP-P4> ");                   
        Fgets(cmdline, MAXLINE, stdin); 
        if (feof(stdin)){
            exit(0);
        }
    	myshell_piping(cmdline, &bgSize, bgList);
    } 
}
/* $end shellmain */

/* findPipeCount는 pipe가 stdin으로 받은 데이터에 몇 개 있는지 확인하는 함수이다. */
int findPipeCount(char* cmdline) {
    int length = strlen(cmdline);
    int i, cnt=0;

    for(i=0;i<length;i++) {
        if(cmdline[i] == '|') cnt++;
    }
    return cnt;
}

int isBackground(char *cmdline) {
    int size = strlen(cmdline);

    if(size > 2 && cmdline[size-2] == '&') {
        cmdline[size-2] = NULL;
        return 1;
    }
    return 0;
}

/* myshell_piping은 명령어의 pipe 유무를 확인하고, 그 경우에 따라 올바른 연산을 수행하도록 한다. */
void myshell_piping(char *cmdline, int* bgSize, BG *bgList) 
{
    int fd[MAXPIPES][2];
    char buf[MAXLINE];   /* Holds modified command line */    
    char *seperatedCmdLine;
    char *temp;
    int pipeCount,i;

    int isBg=0;
    pid_t pids[MAXPIPES]={0,};
    int pidSize = 0;

    strcpy(buf, cmdline);
    pipeCount = findPipeCount(buf);
    isBg = isBackground(buf);

    if(isBg) {
        //백그라운드 실행일 경우, bgList에 정보를 넣는다.
        //이후, Wait하지 않도록 임의로 조정한다.
        bgList[(*bgSize)].num = (*bgSize);
        strncpy(bgList[(*bgSize)].name, buf, MAXBG_NAME);
        if(strlen(buf) >= MAXBG_NAME) {
            bgList[(*bgSize)].name[MAXBG_NAME - 1] = NULL;
        }
        bgList[(*bgSize)].state = RUNNING;
    }
    
    if(pipeCount == 0) {
        pid_t pid = createProcess(cmdline);
        int status;
        if(!isBg){
            Wait(&status);
            if(WIFSIGNALED(status)) {
                /* 
                자식 프로세스에서 시그널이 발생한 경우, 오류를 처리해 준다. 
                phase1에서 시그널이 생기는 경우는 exit 처리 경우뿐이므로
                exit(0)를 해주도록 단순 처리하였다.
                */
                exit(0);
            }
        } else {
            bgList[(*bgSize)].pid[0] = pid;
            bgList[(*bgSize)++].pidSize = 1;
            printf("[%d] %d\n", (*bgSize), pid);
        }
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
    pids[pidSize++] = createPipeProcess(seperatedCmdLine, fd[0], PIPEOUT);
    Close(fd[0][1]);
    for(i=1;i<pipeCount;i++) { //pipe의 개수 +1개만큼 조각이 나뉨. 그러나 마지막은 건너뛰므로 pipeCount까지
        seperatedCmdLine = strtok(NULL, "|");
        int tempFd[2] = {fd[i-1][0], fd[i][1]};
        pids[pidSize++] = createPipeProcess(seperatedCmdLine, tempFd, PIPEIN | PIPEOUT);
        Close(fd[i][1]);
    }
    seperatedCmdLine = strtok(NULL, "|");
    pid_t lastPid = createPipeProcess(seperatedCmdLine, fd[i-1], PIPEIN);

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
    if(!isBg) {
        for(i=0;i<=pidSize;i++){
            Wait(&status);
        }
        if(WIFSIGNALED(status)) {
            exitFlag = 1;
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
    } else {
        //여기는 bg일때, 마지막 연산(파이프의 끝)에 대해 bg 처리하는 부분임.
        bgList[(*bgSize)].pidSize = pidSize + 1;
        for(i=0;i<pidSize;i++) {
            bgList[(*bgSize)].pid[i] = pids[i];
        }
        bgList[(*bgSize)++].pid[i] = lastPid;
        printf("[%d] %d\n", (*bgSize), lastPid);
    }
}

pid_t createPipeProcess(char *cmdline, int* fd, int pipetype)
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
	    return -1;   
    }
    if (!builtin_command(argv)) {
        if((pid = Fork()) == 0){
            if(pipetype & PIPEIN) {
                Dup2(fd[0], STDIN_FILENO); 
                Close(fd[0]);
            }
            if(pipetype & PIPEOUT) {
                Dup2(fd[1], STDOUT_FILENO); 
                Close(fd[1]);
            }      
            if (!strcmp(argv[0], "exit")) { /* exit command */
                if(pipetype == PIPEIN){ // 마지막 파이프일 경우, exit의 결과로 인터럽트 시그널을 부모에게 전달
                    kill(pid, SIGINT);     /* 인터럽트 시그널을 발생시킨다. */
                }
                exit(0);
            }
            //---
            strcat(path, argv[0]);
            if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        } else{
            return pid;
        }
    }
    return -1;
}

/* $begin createProcess */
/* createProcess - Evaluate a command line and fork chile process */
pid_t createProcess(char *cmdline) 
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
	    return -1;   
    }

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if((pid = Fork()) == 0){
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
        }else {
            return pid;
        }
    }
    return -1;
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
