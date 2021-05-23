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
        printf("CSE4100-SP-P#4> ");                   
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
    int bg;              /* Should the job run in bg or fg? */
    pid_t pid;           /* Process id */
    int status;
    char path[MAXARGS] = "/bin/";
    
    strcpy(buf, cmdline);
    bg = myshell_parseline(buf, argv); 
    if (argv[0] == NULL)  
	    return;   /* Ignore empty lines */
    if (!builtin_command(argv)) { //quit -> exit(0), & -> ignore, other -> run
        if(Fork() == 0){
            strcat(path, argv[0]);
            if (execve(path, argv, environ) < 0) {	//ex) /bin/ls ls -al &
                printf("%s: Command not found.\n", argv[0]);
                exit(0);
            }
        }else{
            Wait(&status);
        }
	/* Parent waits for foreground job to terminate */
	/*if (!bg){ 
	    //wait and kill
	}
	else//when there is backgrount process!
	    printf("%d %s", pid, cmdline);
    }*/
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


