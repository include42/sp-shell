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

int bgSize = 1;
BG bgList[MAXBG];

/* Function prototypes */
pid_t createProcess(char *cmdline);
void myshell_parseline(char *buf, char **argv);
int builtin_command(char **argv, int exitFlag); 

int buildin_command_bg(char* cmdline);

void myshell_piping(char *cmdline);
pid_t createPipeProcess(char *cmdline, int* fd, int pipetype);

void sigchild_handler(int signo, siginfo_t *info, void* context) 
{
    int pid = (*info).si_pid;
    for(int i=1;i<bgSize;i++) {
        for(int j=0;j<bgList[i].pidSize;j++) {
            if(bgList[i].pid[j] == pid) {
                bgList[i].pid[j] = bgList[i].pid[bgList[i].pidSize - 1];
                bgList[i].pidSize--;
                if(bgList[i].pidSize <= 0) {
                    bgList[i].state = TERMINATED;
                    Waitpid(pid, 0, 0);
                    printf("[%d]\tDone\t\t%s\n", bgList[i].num, bgList[i].name);
                    return;
                }
            }
        }
    }
    return;
}

/* $begin shellmain */
int main() 
{
    char cmdline[MAXLINE]; /* Command line */
    struct sigaction action;

    action.sa_flags = SA_NOCLDSTOP | SA_RESTART | SA_SIGINFO;
    action.sa_handler = sigchild_handler;

    sigaction(SIGCHLD, &action, 0);

    while (1) {
	    /* Read */
        usleep(100000);
        printf("CSE4100-SP-P4> "); 
        Fgets(cmdline, MAXLINE, stdin); 
        if(!strcmp(cmdline, "\n")) continue;
        if (feof(stdin)){
            exit(0);
        }
    	myshell_piping(cmdline);
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

char* getStateStr(enum STATE state) {
    if(state == RUNNING) return "Running";
    if(state == STOPPED) return "Stopped";
    return "Terminated";
}

int getBGAgrn(char* str) {
    int size = strlen(str);
    int number = 0;
    
    if(str[0] != '%') return -1;
    for(int i=1;i<size;i++) {
        if(str[i] < '0' || str[i] > '9') return -1;
        number = (number * 10) + (str[i] - '0');
    }
    if(number <= 0)
        return -1;
    return number;
}

int buildin_command_bg(char *cmdline) {
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    strcpy(buf, cmdline);
    myshell_parseline(buf, argv);

    if(argv[0] == NULL) return 1;

    if (!strcmp(argv[0], "jobs")) {
	    for(int i=1;i<bgSize;i++) {
            if(bgList[i].state == TERMINATED) continue;
            printf("[%d]\t%s\t\t%s\n", bgList[i].num, getStateStr(bgList[i].state), bgList[i].name);
        }
        return 1;
    }

    if (!strcmp(argv[0], "bg")) {
        int id = getBGAgrn(argv[1]);
        if(id <= 0 || id >= bgSize || bgList[id - 1].state == STOPPED) {
            printf("잘못된 인자가 전달되었습니다.\n");
            return 1;
        }
        for(int i=0;i<bgList[id].pidSize; i++) {
            kill(bgList[id].pid[i], SIGTSTP);
        } 
        bgList[id].state = STOPPED;
        return 1;
    }

    if (!strcmp(argv[0], "fg")) {
        int id = getBGAgrn(argv[1]);
        if(id <= 0 || id >= bgSize || bgList[id].state == RUNNING) {
            printf("잘못된 인자가 전달되었습니다.\n");
            return 1;
        }
        for(int i=0;i<bgList[id].pidSize; i++) {
            kill(bgList[id].pid[i], SIGCONT);
        } 
        bgList[id].state = RUNNING;
        return 1;
    }

    if (!strcmp(argv[0], "kill")) {
        int id = getBGAgrn(argv[1]);
        if(id <= 0 || id >= bgSize || bgList[id].state == TERMINATED) {
            printf("잘못된 인자가 전달되었습니다.\n");
            return 1;
        }
        for(int i=0;i<bgList[id].pidSize; i++) {
            kill(bgList[id].pid[i], SIGINT);
        } 
        bgList[id].state = TERMINATED;
        return 1;
    }

    return 0;  /* Not a builtin command */
}

/* myshell_piping은 명령어의 pipe 유무를 확인하고, 그 경우에 따라 올바른 연산을 수행하도록 한다. */
void myshell_piping(char *cmdline) 
{
    int fd[MAXPIPES][2];
    char buf[MAXLINE];   /* Holds modified command line */    
    char *seperatedCmdLine;
    char *temp;
    int pipeCount,i;

    int isBg=0;
    pid_t pids[MAXPIPES]={0,};
    int pidSize = 0;

    isBg = isBackground(cmdline);
    strcpy(buf, cmdline);
    if(buildin_command_bg(buf)) {
        return;
    }
    pipeCount = findPipeCount(buf);
    

    if(isBg) {
        //백그라운드 실행일 경우, bgList에 정보를 넣는다.
        //이후, Wait하지 않도록 임의로 조정한다.
        
        //구조상 사이즈가 꽉차면 문제 발생. 예외 처리
        if(bgSize >= MAXBG) {
            unix_error("Too Many background processes...");
            exit(0);
        }

        bgList[bgSize].num = bgSize;
        strncpy(bgList[bgSize].name, buf, MAXBG_NAME);
        if(strlen(buf) >= MAXBG_NAME) {
            bgList[bgSize].name[MAXBG_NAME - 1] = NULL;
        }
        bgList[bgSize].state = RUNNING;
    }
    
    if(pipeCount == 0) {
        pid_t pid = createProcess(cmdline);
        if(!isBg){
            Wait(0);
        } else {
            bgList[bgSize].pid[0] = pid;
            bgList[bgSize].pidSize = 1;
            printf("[%d] %d\n", bgSize++, pid);
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

    if(!isBg) {
        for(i=0;i<=pidSize;i++){
            Wait(0);
        }
    } else {
        //여기는 bg일때, 마지막 연산(파이프의 끝)에 대해 bg 처리하는 부분임.
        bgList[bgSize].pidSize = pidSize + 1;
        for(i=0;i<pidSize;i++) {
            bgList[bgSize].pid[i] = pids[i];
        }
        bgList[bgSize].pid[i] = lastPid;
        printf("[%d] %d\n", bgSize++, lastPid);
    }
}

pid_t createPipeProcess(char *cmdline, int* fd, int pipetype)
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    
    strcpy(buf, cmdline);
    myshell_parseline(buf, argv); 
    /* Ignore empty lines */
    if (argv[0] == NULL) {
	    return -1;   
    }
    //exitFlag를 마지막인지 여부로 전달. 그렇지 않으면 exit에 대해 대응하지 않음.
    if (!builtin_command(argv, (pipetype == PIPEIN))) {
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
    pid_t pid;           /* Process id */
    int status;            /* child Process status */
    char path[MAXARGS] = "/bin/"; /* path for find execve() path */
    
    strcpy(buf, cmdline);
    myshell_parseline(buf, argv); 
    /* Ignore empty lines */
    if (argv[0] == NULL) {
	    return -1;   
    }

    if (!builtin_command(argv, 1)) { //quit -> exit(0), & -> ignore, other -> run
        if((pid = Fork()) == 0){
            //child process
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
int builtin_command(char **argv, int exitFlag) 
{
    if (!strcmp(argv[0], "cd")) {
	    chdir(argv[1]);
        return 1;
    }
    if (exitFlag && !strcmp(argv[0], "exit")) {    
	    exit(0);
    }
    if (!strcmp(argv[0], "&")) {    
	    return 1;
    }
    return 0;                     /* Not a builtin command */
}
/* $end createProcess block */

/* $begin myshell_parseline */
/* myshell_parseline - Parse the command line and build the argv array */
void myshell_parseline(char *buf, char **argv) 
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

    return;
}
/* $end myshell_parseline */
