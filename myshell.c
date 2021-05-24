/* $begin my-shell */
#include "csapp.h"
#include<errno.h>
#define MAXARGS   128

/* Function prototypes */
void myshell_execute(char *cmdline);
int myshell_parseline(char *buf, char **argv);
int builtin_command(char **argv); 

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
    	myshell_execute(cmdline);
    } 
}
/* $end shellmain */
  
/* $begin myshell_execute */
/* myshell_execute - Evaluate a command line and fork chile process */
void myshell_execute(char *cmdline) 
{
    char *argv[MAXARGS]; /* Argument list execve() */
    char buf[MAXLINE];   /* Holds modified command line */
    int pipes[2];       /* Simple Pipeline I/O Array */
    char pipe_buf[MAXLINE]; /* Buffer for pipeline */
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
    /* Open Pipe for cd operation */
    if(pipe(pipes) < 0) {
        unix_error("Pipe error");
        exit(0);
    }

    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if(Fork() == 0){
            //child process
            Close(pipes[0]);
            if(!strcmp(argv[0], "cd")) {
                //cd의 경우, chdir해준 뒤 pipe로 결과 전달
                chdir(argv[1]);
                getcwd(pipe_buf, MAXLINE);
                Write(pipes[1], pipe_buf, MAXLINE);
                exit(0);
            }
            //---
            strcat(path, argv[0]);
            if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }
	/* Parent waits for foreground job to terminate */
        if (!bg) { 
            int status; 
            Close(pipes[1]);
            Wait(status);
            if(!strcmp(argv[0], "cd")) {
                Read(pipes[0], pipe_buf, MAXLINE);
                chdir(pipe_buf);
            }
        } else{
            printf("%d %s", pid, cmdline);
        }
    }
    return;
}

/* If first arg is a builtin command, run it and return true */
int builtin_command(char **argv) 
{
    if (!strcmp(argv[0], "exit")) { /* quit command */
	    exit(0);  
    }
    if (!strcmp(argv[0], "&")) {    /* Ignore singleton & */
	    return 1;
    }
    return 0;                     /* Not a builtin command */
}
/* $end myshell_execute block */

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
