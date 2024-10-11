#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include "LineParser.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#define S_IRUSR 0x400
#define S_IWUSR 0x200
#define S_IRGRP 0x40
#define S_IROTH 0x4

#define STDIN 0
#define STDOUT 1

int change_dir(cmdLine* cmdline){
    if(strncmp("cd",cmdline->arguments[0],2) ==0){   
        if(cmdline->argCount !=2){
            fprintf(stderr, "cd error - number of arguments is not 2\n");
        }
        else if (chdir(cmdline->arguments[1]) == -1){
		    fprintf(stderr, "chdir failed\n");
		}
        return 1;
    }
    return 0;
}

int cat(cmdLine *pCmdLine, int *pipe_fd){
    if(pCmdLine->inputRedirect != NULL){
        pipe_fd[0] = open(pCmdLine->inputRedirect, O_RDONLY | O_CREAT,0644);
        dup(pipe_fd[0]);
        close(pipe_fd[0]);
    }
    if(pCmdLine->outputRedirect != NULL){
        pipe_fd[1] = open(pCmdLine->outputRedirect, O_WRONLY | O_CREAT,0644);
        dup(pipe_fd[1]);
        close(pipe_fd[1]);
    }
}

void execute(cmdLine *pCmdLine , int debugmode, int blocking_child, int flag, int *pipe_fd){
    if(!flag){
        int state;
        int pid;
        int pipe_fd[2];
        pid = fork;
        if(debugmode == 1){
            fprintf(stderr, "the pid of the child is : %ld , the exection command : %s \n",pid , pCmdLine->arguments);
            if(pid != 0){   
                fprintf(stderr,"fork fails! the pid is: %ld , the excution command is: %s \n",pid , pCmdLine->arguments);
            }
        }
        if(pid != 0){
            cat(pCmdLine,pipe_fd);
            execvp(pCmdLine->arguments[0],pCmdLine->arguments);
            perror("Error the execvp fails: ");
            exit(1);
        }
        else{
            if(blocking_child){
                waitpid(pid,&state,0); 
            }
        }
        freeCmdLines(pCmdLine);
    }
    else{
        int pipe_fd1[2];
        int state = 0;
        int n1;
        int o1;
        FILE * in;
        FILE * out;
        if (debugmode){
            fprintf(stderr, "parent_process>forking\n");
        }
        pipe(pipe_fd1);
        int pid1 = fork();
        
        if (debugmode){
            fprintf(stderr, "parent_process>created process with id: %d\n", pid1);
        }
        if(pid1 == 0){
            // printf("child!!!!!!!!!!!!\n");
            close(1);
            dup(pipe_fd1[1]);
            // if (debugmode){
            //     fprintf(stderr, "child1>redirecting stdout to the write end of the pipe\n");
            // }
            close(pipe_fd1[1]);
            if (debugmode){
                fprintf(stderr, "child1>going to execute cmd: %s \n", pCmdLine->arguments[0]);
            }
            dprintf(STDOUT,"before\n");
            if(pCmdLine->inputRedirect!=NULL){
                close(STDIN);
                //dup2()
                in = fopen(pCmdLine->inputRedirect,"r");
            }
            if(pCmdLine->outputRedirect!=NULL){
                close(STDOUT);
                out = fopen(pCmdLine->outputRedirect,"o");
            }
            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if(pCmdLine->inputRedirect!=NULL){
                close(in);
            }
            if(pCmdLine->outputRedirect!=NULL){
                close(out);
            }
            close(pipe_fd1[1]);
            exit(0);

        }
        else{
            if (debugmode){
                fprintf(stderr, "parent_process>forking\n");
            }
            waitpid(pid1, &state, 0);
            close(pipe_fd1[1]);
            int pid2 = fork();
            if (debugmode){
                fprintf(stderr, "parent_process>created process with id: %d\n", pid2);
            }
            if(pid2 == 0){
                close(STDIN);
                dup(pipe_fd1[0]);
                if (debugmode){
                    fprintf(stderr, "child1>redirecting stdout to the write end of the pipe\n");
                }
                close(pipe_fd1[0]);
                if (debugmode){
                    fprintf(stderr, "child1>going to execute cmd: %s %s \n", pCmdLine->arguments[0], pCmdLine->next->arguments[1]);
                }
                if(pCmdLine->inputRedirect!=NULL){
                    close(STDIN);
                    in = fopen(pCmdLine->inputRedirect,"r");
                }
                if(pCmdLine->outputRedirect!=NULL){
                    close(STDOUT);
                    out = fopen(pCmdLine->outputRedirect,"o");
                }
                execvp(pCmdLine->arguments[0], pCmdLine->arguments);
                if(pCmdLine->inputRedirect!=NULL){
                    close(in);
                }
                if(pCmdLine->outputRedirect!=NULL){
                    close(out);
                }
                exit(0);
            }
            else{
                close(pipe_fd1[0]);
                
                waitpid(pid2, &state, 0);
                freeCmdLines(pCmdLine);
            }
        }
        /*waitpid(pid1, &state, 0);
        // close(pipe_fd[1]);
        if (debugmode){
            fprintf(stderr, "parent_process>forking\n");
        }
        int pid2 = fork();
        if (debugmode){
            fprintf(stderr, "parent_prputRedirect!=NULL){
                close(in);
            }
            if(pCmdLine->outputRedirect!=NULL){
                close(out);
            }
            close(pipe_fd[1]);
            exit(0);

        }
        else{
            if (debugmode){
            if (debugmode){
                fprintf(stderr, "child1>redirecting stdout to the write end of the pipe\n");
            }
            close(pipe_fd[0]);
            if (debugmode){
                fprintf(stderr, "child1>going to execute cmd: %s %s \n", pCmdLine->arguments[0], pCmdLine->next->arguments[1]);
            }
            if(pCmdLine->inputRedirect!=NULL){
                close(STDIN);
                in = fopen(pCmdLine->inputRedirect,"r");
            }
            if(pCmdLine->outputRedirect!=NULL){
                close(STDOUT);
                out = fopen(pCmdLine->outputRedirect,"o");
            }
            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            if(pCmdLine->inputRedirect!=NULL){
                close(in);
            }
            if(pCmdLine->outputRedirect!=NULL){
                close(out);
            }
            exit(0);
        }8*/
       
    }
}

int main(int argc , char **argv){
    char cwd[PATH_MAX];
    char input[2048];
    cmdLine* cmd;
    int chd = 0;
    int debug_mode = 0;
    int flag;
    int blocking_child ;
    int pipe_fd[2];
    for(int i = 0; i< argc; ++i){
        if(strncmp("-d",argv[i],2) == 0){
            debug_mode = 1;
        }
    }
    while(1){
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("Current working dir: %s\n", cwd);
        }
        if(fgets(input,2048,stdin)!=NULL){
            cmd = parseCmdLines(input);
            if(cmd->next !=NULL){
                //pipe(pipe_fd);
				flag = 1;
            }
            chd = change_dir(cmd);
            if(!cmd->blocking){
                blocking_child = 0;
            }
            if(cmd!=NULL && chd == 0){
                execute(cmd,debug_mode,blocking_child,flag,&pipe_fd);
            }
            chd = 0;
            if(strncmp("quit", input, 4) == 0){
                break;
            }
        }

    }
    return 0;
}