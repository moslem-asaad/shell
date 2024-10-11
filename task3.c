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

int ** createPipes(int nPipes){
    int** pipes;
    pipes=(int**) calloc(nPipes, sizeof(int));

    for (int i=0; i<nPipes;i++){
        pipes[i]=(int*) calloc(2, sizeof(int));
        pipe(pipes[i]);
    }  
    return pipes;
}
void releasePipes(int **pipes, int nPipes){
        for (int i=0; i<nPipes;i++){
            free(pipes[i]);
        
        }  
    free(pipes);
}
int *leftPipe(int **pipes, cmdLine *pCmdLine){
    if (pCmdLine->idx == 0) return NULL;
    return pipes[pCmdLine->idx -1];
}

int *rightPipe(int **pipes, cmdLine *pCmdLine){
    if (pCmdLine->next == NULL) return NULL;
    return pipes[pCmdLine->idx];
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

void execute(cmdLine *pCmdLine , int debugmode, int blocking_child){
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

void execute_pipe(cmdLine *pCmdLine , int debugmode, int blocking_child){
    printf("in execute_pipe\n");
    int READ = 0;
    int WRITE = 1;
    int size = 0;
    cmdLine* cmd = pCmdLine;
    while(cmd->next!=NULL){
        size++;
        cmd = cmd->next;
    }
    printf("before creating pipes\n");
    int ** pipes = createPipes(size);
    printf("after creating pipes\n");
    for(int i = 0;i<=size;i++){
        cmd = pCmdLine;
        pCmdLine = pCmdLine->next;
        int id = fork();
        fprintf(stderr,"the running command is %s\n",cmd->arguments[0]);
        if(id == 0){
            fprintf(stderr,"in child\n");
            // in  child
            int* fd_curr = pipes[cmd->idx];
            int* fd_l = leftPipe(pipes,cmd);
            if(fd_l == NULL){
                close(STDOUT);
                close(fd_curr[READ]);
                dup2(fd_curr[WRITE],WRITE);
                if(execvp(cmd->arguments[0],cmd->arguments) == -1){
                    perror("the execv failed");
                    _exit(1);
                }
            }else{
                fprintf(stderr,"before right pipe\n");
                int* fd_r = rightPipe(pipes,cmd);
                if(fd_r!=NULL){
                    close(STDIN);
                    close(fd_l[WRITE]);
                    dup2(fd_l[READ],READ);
                    close(STDOUT);
                    close(fd_curr[READ]);
                    dup2(fd_curr[WRITE],WRITE);
                    if(execvp(cmd->arguments[0],cmd->arguments) == -1){
                        perror("the execv failed");
                        _exit(1);
                    }
                }
                else{
                    fprintf(stderr,"in last one\n");
                    close(STDIN);
                    close(fd_curr[WRITE]);
                    dup2(fd_curr[READ],READ);
                    if(execvp(cmd->arguments[0],cmd->arguments) == -1){
                        perror("the execv failed");
                        _exit(1);
                    }
                }
            }
        }else{
            waitpid(id,NULL,0);
        }
    }

    /*int fd[2];
    pipe(fd);
    while(pCmdLine -> next !=NULL){
        int pid2,pid1 = fork();
        if(pid1== 0){
            printf("in child 1");
            // child 1
            close(STDOUT);
            close(fd[READ]);
            dup2(fd[WRITE],WRITE);
            execvp(pCmdLine->arguments[0],pCmdLine->arguments);
            fprintf(stderr,"run ls yeeey!");
            perror("Error the execvp fails: ");
            exit(1);
        }else{
            printf("before in child 2\n");
            pid2 = fork();
            if(pid2 == 0){
                printf("in child 2");
                // child 2
                close(STDIN);
                close(fd[WRITE]);
                printf("before dup wc yeeey!");
                dup2(fd[READ],READ);
                printf("before run wc yeeey!");
                pCmdLine = pCmdLine ->next;
                execvp(pCmdLine->arguments[0],pCmdLine->arguments);
                printf("run wc yeeey!");
                perror("Error the execvp fails: ");
                exit(1);
            }
        }
        waitpid(pid1,NULL,0);
        close(fd[1]);
        waitpid(pid2,NULL,0);
    }*/
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
				flag = 0;
            }
            chd = change_dir(cmd);
            if(!cmd->blocking){
                blocking_child = 0;
            }
            if(cmd!=NULL && chd == 0 && flag== 0){
                execute_pipe(cmd,debug_mode,blocking_child);
            }
            if(cmd!=NULL && chd == 0 && flag== 1){
                int READ = 0;
                int WRITE = 1;
                int fd[2];
                pipe(fd);
                int pid2,pid1 = fork();
                if(pid1== 0){
                    // child 1
                    close(fd[READ]);
                    dup2(fd[WRITE],WRITE);
                    execvp(cmd->arguments[0],cmd->arguments);
                    perror("Error the execvp fails: ");
                    exit(1);
                }else{
                    pid2 = fork();
                    if(pid2 == 0){
                        // child 2
                        close(fd[WRITE]);
                        dup2(fd[READ],READ);
                        execvp(cmd->arguments[0],cmd->arguments);
                        perror("Error the execvp fails: ");
                        exit(1);
                    }
                }
                waitpid(pid1,NULL,0);
                close(fd[1]);
                waitpid(pid2,NULL,0);
            }
            chd = 0;
            if(strncmp("quit", input, 4) == 0){
                break;
            }
        }

    }
    return 0;
}