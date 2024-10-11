#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "LineParser.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>


#define STDIN  0
#define STDOUT 1
#define STDERR 2

#define S_IRUSR 0x400
#define S_IWUSR 0x200
#define S_IRGRP 0x40
#define S_IROTH 0x4
#define S_IRWXU  00700



int cat(cmdLine *pCmdLine, int *pipe_fd){
    if(pCmdLine->inputRedirect!=NULL){
        //close std
        close(STDIN);
        pipe_fd[0] = open(pCmdLine->inputRedirect, O_RDONLY,0777); //0777
        dup(pipe_fd[0]);
    }
    if(pCmdLine->outputRedirect!=NULL){
        //close stdout
        close(STDOUT);
        pipe_fd[1] = open(pCmdLine->outputRedirect, O_WRONLY,0777);
        dup(pipe_fd[1]);
    }
} 

void execute(cmdLine *pCmdLine , int debug_mode){
    int status;
    long PID = fork();
    int pipe_fd[2];
    if(debug_mode == 1 ){
        fprintf(stderr,"- PID: %ld\n- Executing command: %s\n",PID,pCmdLine->arguments);
        if(PID != 0) {
            fprintf(stderr,"the fork proccess failes\n");
        }
    }
    if(PID == 0){
        cat(pCmdLine,pipe_fd);
        if(execvp(pCmdLine->arguments[0],pCmdLine->arguments) == -1){
            perror("the execv failed");
            _exit(1);
        }
    }
    else{
         if(pCmdLine->blocking){
            waitpid(PID,&status,0);
        } 
    }
    freeCmdLines(pCmdLine);
} 
int change_curr_dir(cmdLine *cmd){
    int chd = 0;
    if(strncmp("cd",cmd->arguments[0],2)==0){  
        chd = 1; 
        if(cmd->argCount !=2){
            fprintf(stderr, "cd error - number of arguments is not 2\n");
        }
        else if (chdir(cmd->arguments[1]) == -1){
		    fprintf(stderr, "cd error - fail in chdir\n");
		}
    }
    return chd;
}


int main(int argc , char **argv){
    char cwd[PATH_MAX];
    char input[2048];
    cmdLine *cmd ; 
    int infinite = 1;
    int debug_mode = 0;
    int chd = 0;
    for(int i = 0; i< argc; ++i){
        if(strncmp("-d",argv[i],2) == 0){
            debug_mode = 1;
        }
    }
    while(1){
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working dir: %s\n", cwd);
        }
        if(fgets(input,2048,stdin)!=NULL && strncmp("quit", input, 4) != 0){
            cmd = parseCmdLines(input);
            chd = change_curr_dir(cmd);
            if(cmd!=NULL && chd == 0){
                execute(cmd,debug_mode);
            }
            if(strncmp("quit",input , 4)==0){
                break;
            }
        } 

    }
    return 0;

}