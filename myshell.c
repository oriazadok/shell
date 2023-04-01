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
 * setenv
 * 
 */

int* parser(char* command) {

    int* files = NULL;
    int i = 0;
    char int_str[20];
    char *token;

    /* parse command line */
    token = strtok(command, "|");           ls | grep f | d | e | t | w | p | m     
    while (token != NULL) {

        sprintf(int_str, "%d", i);
        printf("Var: %s", int_str);
        int fd = open(int_str, O_CREAT | O_APPEND | O_RDWR, 0660);
        write(fd, token, strlen(token) + 1);
        files = realloc(files, sizeof(int) * (i + 1));
        files[i++] = fd;

        token = strtok(NULL, "|");
        printf("tok: %s\n", token);
    }
    files = realloc(files, sizeof(int) * (i + 1));
    files[i] = 0;

    return files;
}
void print_pipes(int* pipes) {
    int i = 0;
    char com[1024] = { 0 };
    char int_str[20];
    int fd;

    
    while( pipes[i] ) {

        sprintf(int_str, "%d", i++);
        fd = open(int_str, O_RDONLY);
        size_t size = read(fd, com, 1024);
        printf("size: %ld, com: %s\n", size, com);
        memset(com,1 ,1024);
        close(fd);
    }
}

int main() {
    char command[1024], last_command[1024], read[1024];
    char *token;
    int i;
    char *outfile;
    int fd, errfd, amper, redirect, piping, retid, status, argc1;
    int fildes[2];
    char *argv1[10], *argv2[10];

    // signal(SIGINT, sighandler);

    while (1) {
        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';
        piping = 0;

        int* pipes = parser(command);
        print_pipes(pipes);


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

        /* read command */
        if(!strcmp(argv1[0], "read")) {
            if (argv1[1]){
                fgets(read, 1024, stdin);
                if (setenv(argv1[1], read, 1));
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
        

        /* for commands not part of the shell command language */ 
        if (fork() == 0) { 

            // to vdok
            // signal (SIGINT,SIG_DFL);

            /* redirection of IO ? */
            if (redirect == RE_OUT) {
                fd = creat(outfile, 0660); 
                close (STDOUT_FILENO) ; 
                dup(fd); 
                close(fd); 
                /* stdout is now redirected */
            } else if (redirect == RE_ERR) {
                errfd = creat(outfile, 0660); 
                close (STDERR_FILENO) ; 
                dup(errfd); 
                close(errfd); 
                /* stderr is now redirected */
            } else if (redirect == RE_APPEND) {
                fd = open(outfile, O_CREAT | O_APPEND | O_RDWR, 0660); 
                close (STDOUT_FILENO) ; 
                dup(fd); 
                close(fd); 
                /* stdout is now redirected */
            } 
            if (piping) {
                pipe (fildes);
                if (fork() == 0) { 
                    /* first component of command line */ 
                    close(STDOUT_FILENO); 
                    dup(fildes[1]); 
                    close(fildes[1]); 
                    close(fildes[0]); 
                    /* stdout now goes to pipe */ 
                    /* child process does command */ 
                    execvp(argv1[0], argv1);
                } 
                /* 2nd command component of command line */ 
                close(STDIN_FILENO);
                dup(fildes[0]);
                close(fildes[0]); 
                close(fildes[1]); 
                /* standard input now comes from pipe */ 
                execvp(argv2[0], argv2);
            } else {
                execvp(argv1[0], argv1);
            }
        }
        /* parent continues over here... */
        /* waits for child to exit if required */
        if (amper == 0) {
            retid = wait(&status);
        }
    }

    return 0;
}
