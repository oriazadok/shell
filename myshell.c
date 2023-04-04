#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "stdio.h"
#include "errno.h"
#include "stdlib.h"
#include "unistd.h"
#include <string.h>

#define RE_OUT 1
#define RE_ERR 2
#define RE_APPEND 11


char command[1024], last_command[1024], myread[1024];
char *token;
int i;
char *outfile;
int fd, errfd, amper, redirect, piping, retid, status, argc1;
// int fildes[2];
char *argv1[10], *argv2[10];
char*** pipes;
// char* history;
char prompt[1024] = "hello: ";

// void sighandler(int signum){
//     printf("You typed Control-C!\n");
// // signal (sigint,SIG_IGN);
// // signal (sigint,SIG_DFL);
//     printf("%s", prompt);
//     fflush(stdout);
// }

/**
 * clause 4
 * clause 9
 * we stopped at clause 10 we first need to pipe and then replace the vars in the commands with their values
 * 
 * need to verify ^C doesnot kill the father
 * 
 * echo aa > out // check thus out
 * 
 * arrow + enter
 * 
 */


char** parse_pipe(char* command, char* by) {
    char** coms = NULL;
    int i = 0;

    /* parse command line */
    char* token = strtok(command, by);
    while (token != NULL) {
        coms = (char**)realloc(coms, (i + 1) * sizeof(char*) );
        coms[i++] = token;
        // printf("token: %s\n", token);
        token = strtok(NULL, by);
    }
    coms = (char**)realloc(coms, (i + 1) * sizeof(char*));
    coms[i] = NULL;

    return coms;
}

char ***parser(char* command) {
    char*** coms = NULL;
    int i = 0;
    char by = '|';
    char** parsed_pipes = parse_pipe(command, &by);

    by = ' ';
    char** parsed_pipe;
    while( parsed_pipes[i] ) {
        // printf("parsed_pipes[i]: %s\n", parsed_pipes[i]);
        parsed_pipe = parse_pipe(parsed_pipes[i], &by);
        coms = (char***)realloc(coms, (i + 1) * sizeof(char**) );
        coms[i++] = parsed_pipe;
    }

    coms = (char***)realloc(coms, (i + 1) * sizeof(char**));
    coms[i] = NULL;

    return coms;
}

int print_pipes(char*** coms) {

    printf("the commad issss: \n");

    int i = 0;
    while( coms[i] ) {
        int j = 0;
        printf("pip\n");
        while( coms[i][j] ) {
            printf("wow %s ", coms[i][j]);
            j++;
        }
        i++;
    }

    printf("\n");
    return 0;
}

int exec(char*** pipes) {
    // signal (SIGINT,SIG_DFL);

    if (fork() == 0) { 
        /* redirection of IO ? */
        // if (redirect == RE_OUT) {
        //     fd = creat(outfile, 0660); 
        //     close (STDOUT_FILENO) ; 
        //     dup(fd); 
        //     close(fd); 
        //     /* stdout is now redirected */
        // } else if (redirect == RE_ERR) {
        //     errfd = creat(outfile, 0660); 
        //     close (STDERR_FILENO) ; 
        //     dup(errfd); 
        //     close(errfd); 
        //     /* stderr is now redirected */
        // } else if (redirect == RE_APPEND) {
        //     fd = open(outfile, O_CREAT | O_APPEND | O_RDWR, 0660); 
        //     close (STDOUT_FILENO) ; 
        //     dup(fd); 
        //     close(fd); 
        //     /* stdout is now redirected */
        // } 
        int pipefd[2];

        int p = 0;
        pipe(pipefd);
        
        while( pipes[p] ) {

            if (fork() == 0) { 

                if( p ) {
                    close(STDIN_FILENO); 
                    // dup(pipefd[0]);
                    dup2(pipefd[0], 0); 

                } else if( pipes[p + 1] ) {
                    close(STDOUT_FILENO); 
                    // dup(pipefd[1]); 
                    dup2(pipefd[1], 1); 
                }

                close(pipefd[1]); 
                close(pipefd[0]); 
                /* stdout now goes to pipe */ 
                execvp(pipes[p][0], pipes[p]);
            } else {
                wait(NULL);
                p++;
            }
            
        } 
    } else {
        wait(NULL);
    }

    
                
    /* parent continues over here... */
    /* waits for child to exit if required */
    if (amper == 0) {
        retid = wait(&status);
    }

    printf("returning\n");

    return 0;
}

int main() {
    

    // signal(SIGINT, sighandler);

    while (1) {
        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        piping = 0;

        pipes = parser(command);
        // print_pipes(pipes);


        /* !! command */ 
        if(! strcmp(command, "!!")) {
            strcpy(command, last_command);
        }
        strcpy(last_command, command);

        /* parse command line */
        i = 0;
        token = strtok (command," ");
        while (token != NULL) {
            argv1[i] = token;
            token = strtok (NULL, " ");
            i++;
            if (token && ! strcmp(token, "|")) {
                piping = 1;
                break;
            }
        }
        argv1[i] = NULL;
        argc1 = i;

        /* Is command empty */
        if (argv1[0] == NULL)
            continue;

        /* Does command contain pipe */
        if (piping) {
            i = 0;
            while (token!= NULL) {
                token = strtok (NULL, " ");
                argv2[i] = token;
                i++;
            }
            argv2[i] = NULL;
        }

        

        /* change the prompt */ 
        if(! strcmp(argv1[0], "prompt")) {
            strcpy(prompt, argv1[2]);
            strcat(prompt, " ");
            continue;
        }

        /* myread command */
        if(!strcmp(argv1[0], "myread")) {
            if (argv1[1]){
                fgets(myread, 1024, stdin);
                if (setenv(argv1[1], myread, 1));
                continue;
            }
        }

        /* Does command line end with & */ 
        if (! strcmp(argv1[argc1 - 1], "&")) {
            amper = 1;
            argv1[argc1 - 1] = NULL;
        } else { 
            amper = 0; 
        }
        
        if (argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">")) {
            redirect = RE_OUT;
            argv1[argc1 - 2] = NULL;
            outfile = argv1[argc1 - 1];
        } else if (argc1 > 1 && ! strcmp(argv1[argc1 - 2], "2>")) {
            redirect = RE_ERR;
            argv1[argc1 - 2] = NULL;
            outfile = argv1[argc1 - 1];
        } else if (argc1 > 1 && ! strcmp(argv1[argc1 - 2], ">>")) {
            redirect = RE_APPEND;
            argv1[argc1 - 2] = NULL;
            outfile = argv1[argc1 - 1];
        } else { 
            redirect = 0; 
        }

        

        /* echo command */ 
        if(! strcmp(argv1[0], "echo")) {

            if( ( argv1[1] != NULL ) && (! strcmp(argv1[1], "$?")) ) {
                printf("%d\n", retid);
                continue;
            }
            int i = 1;
            char *env_var;
            while(argv1[i]) {
                if(argv1[i][0] =='$') {
                    if( (env_var = getenv(argv1[i] + 1)) ) {
                        strcpy(argv1[i], env_var);
                    }
                }
                printf("%s ", argv1[i]);
                i++;
            }
            // printf("check\n");
            printf("\n");
            continue;
        }

        /* cd command */ 
        if(! strcmp(argv1[0], "cd")) {
            chdir(argv1[1]);
            continue;
        }
        
        /* quit command */ 
        // if(! strcmp(argv1[0], "quit")) {
        //     return 0;
        // }

        /* add vars */ 
        if(argv1[0][0] =='$') {
            if(! argv1[1] || ! argv1[2]) {
                continue;
            }
            if(! strcmp(argv1[1], "=")) {
                if ( setenv(argv1[0] + 1, argv1[2], 1));
                continue;
            }
        }
        

        exec(pipes);
        /* for commands not part of the shell command language */ 
    //     if (fork() == 0) { 


    //         // signal (SIGINT,SIG_DFL);

    //         /* redirection of IO ? */
    //         if (redirect == RE_OUT) {
    //             fd = creat(outfile, 0660); 
    //             close (STDOUT_FILENO) ; 
    //             dup(fd); 
    //             close(fd); 
    //             /* stdout is now redirected */
    //         } else if (redirect == RE_ERR) {
    //             errfd = creat(outfile, 0660); 
    //             close (STDERR_FILENO) ; 
    //             dup(errfd); 
    //             close(errfd); 
    //             /* stderr is now redirected */
    //         } else if (redirect == RE_APPEND) {
    //             fd = open(outfile, O_CREAT | O_APPEND | O_RDWR, 0660); 
    //             close (STDOUT_FILENO) ; 
    //             dup(fd); 
    //             close(fd); 
    //             /* stdout is now redirected */
    //         } 

    //         int pipefd[2];
    //         pipe(pipefd);
    //         int p = 0;
    //         if ( pipes[p] ) {
    //             pipe (fildes);
    //             if (fork() == 0) { 
    //                 /* first component of command line */ 
    //                 close(STDOUT_FILENO); 
    //                 dup(pipefd[1]); 
    //                 close(pipefd[1]); 
    //                 close(pipefd[0]); 
    //                 /* stdout now goes to pipe */ 
    //                 /* child process does command */ 
    //                 execvp(pipes[p][0], pipes[p]);
    //             } 

    //             p++;
    //         } 
            
    //     }
    //     /* parent continues over here... */
    //     /* waits for child to exit if required */
    //     if (amper == 0) {
    //         retid = wait(&status);
    //     }
    }

    return 0;
}
