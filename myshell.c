#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>

#include "history.h"
#include "myshell.h"

#define RE_OUT 1
#define RE_ERR 2
#define RE_IN 3
#define RE_APPEND 11

#define UP_ARROW_KEY "\033[A"
#define DOWN_ARROW_KEY "\033[B"
#define LINE_UP "\033[1A"
#define DELETE_LINE "\x1b[2K"
#define EMPTY_STRING ""

char prompt[1024] = "hello: ";
int status;

/* help handle_arrows to print properly */
int print_hc(char* str1, char* str2) {
    printf(LINE_UP);
    printf(DELETE_LINE);
    printf("%s%s", str1, str2);
    fflush(stdout);
    return 0;
}

/* handle the arrows keys */
int handle_arrows(History* h, char* cmd) {

    if (h->history_counter == 0 ) {
        print_hc(prompt, "There Are No Previous Commands\n");
        return 1;
    }

    int history_counter = h->history_counter;
    int history_pos = history_counter - 1;
    int b = 1;

    char temp[1024];
    strcpy(temp, cmd);

    while (1) {

        if ( ! strcmp(cmd, UP_ARROW_KEY) ) {
            if( b == -1 ) { history_pos--; }
            b = 1;
           
            if( history_pos >= 0) {
                strncpy(temp, h->history[ history_pos ], 1024);
                history_pos--;
                print_hc(prompt, temp);
            } else {
                print_hc(prompt, temp);
            }
           
        } else if ( ! strcmp(cmd, DOWN_ARROW_KEY) ) {
            if( b == 1 ) { history_pos++; }
            b = -1;
           
            if (history_pos < history_counter - 1) {
                history_pos++;
                strncpy(temp, h->history[ history_pos ], 1024);
                print_hc(prompt, temp);
            } else {
                print_hc("", "");
                return 1;
            }
        } 

        fgets(cmd, 1024, stdin);
        cmd[strlen(cmd) - 1] = '\0';

        if (strcmp(cmd, "") == 0) {
            strncpy(cmd, temp, 1024);
            break;
        }
    }

    return 0;
}

/* parsing the command according to by param to an arrays of string */
char** parse_pipe(char* command, char* by, int* n) { 
    char** coms = NULL;
    // *n = 0;
    int i = 0;

    /* parse command line */
    char* token = strtok(command, by);
    while (token != NULL) {
        
        coms = (char**)realloc(coms, (i + 1) * sizeof(char*) );
        coms[i] = token;
        i++;
        token = strtok(NULL, by);

        if( n ) {
            (*n)++;
        }
    }
    coms = (char**)realloc(coms, (i + 1) * sizeof(char*));
    coms[i] = NULL;

    return coms;
}

/* parsing the command into an array of arrays of strings */
char ***parser(char* command, int* num_of_pipes) {

    char*** coms = NULL;
    int i = 0;
    char by = '|';
    char** parsed_pipes = parse_pipe(command, &by, num_of_pipes);
   

    by = ' ';
    char** parsed_pipe;
    while( parsed_pipes[i] ) {
        parsed_pipe = parse_pipe(parsed_pipes[i], &by, NULL);
        coms = (char***)realloc(coms, (i + 1) * sizeof(char**) );
        coms[i++] = parsed_pipe;
    }
    coms = (char***)realloc(coms, (i + 1) * sizeof(char**));
    coms[i] = NULL;

    free(parsed_pipes);

    return coms;
}

/* execute the parsed command */
int exec(char*** pipes, int num_of_pipes) {

    char* outfile;
    int fd, redirect, errfd, amper;

    int n = 0;
    while (pipes[num_of_pipes - 1][n] != NULL) {
        n++;
    }
    size_t argc = n;


    /* Does command line end with & */ 
    if (! strcmp(pipes[num_of_pipes - 1][argc - 1], "&")) {
        amper = 1;
        pipes[num_of_pipes - 1][argc - 1] = NULL;
    }
    else {
        amper = 0; 
    }

    if (argc > 1 && ! strcmp(pipes[num_of_pipes - 1][argc - 2], "<")) {
        redirect = RE_IN;
        pipes[num_of_pipes - 1][argc - 2] = NULL;
        outfile = pipes[num_of_pipes - 1][argc - 1];
    } else if (argc > 1 && ! strcmp(pipes[num_of_pipes - 1][argc - 2], ">")) {
        redirect = RE_OUT;
        pipes[num_of_pipes - 1][argc - 2] = NULL;
        outfile = pipes[num_of_pipes - 1][argc - 1];
    } else if (argc > 1 && ! strcmp(pipes[num_of_pipes - 1][argc - 2], "2>")) {
        redirect = RE_ERR;
        pipes[num_of_pipes - 1][argc - 2] = NULL;
        outfile = pipes[num_of_pipes - 1][argc - 1];
    } else if (argc > 1 && ! strcmp(pipes[num_of_pipes - 1][argc - 2], ">>")) {
        redirect = RE_APPEND;
        pipes[num_of_pipes - 1][argc - 2] = NULL;
        outfile = pipes[num_of_pipes - 1][argc - 1];
    } else {
        redirect = 0; 
    }


    int piping[2];
    pipe(piping);

    char buffer[10];

    if (fork() == 0) { 

        // Set up signal handler
        struct sigaction sa;
        sa.sa_flags = SA_SIGINFO;
        sa.sa_sigaction = NULL;
        sigemptyset(&sa.sa_mask);

        if (sigaction(SIGINT, &sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }
        

        if(num_of_pipes == 1) {
            if (fork() == 0) { 
                // Return to default behavior
                struct sigaction default_sa;
                default_sa.sa_flags = 0;
                default_sa.sa_handler = SIG_DFL;
                sigemptyset(&default_sa.sa_mask);

                if (sigaction(SIGINT, &default_sa, NULL) == -1) {
                    perror("sigaction");
                    exit(1);
                }
                
                /* stdin is now redirected */
                if (redirect == RE_IN) {
                    fd = open(outfile, O_RDONLY); 
                    close (STDIN_FILENO); 
                    dup2(fd, STDIN_FILENO);
                    close(fd); 
                    /* stdout is now redirected */
                } else if (redirect == RE_OUT) {
                    fd = creat(outfile, 0660); 
                    close (STDOUT_FILENO) ; 
                    dup2(fd, STDOUT_FILENO); 
                    close(fd); 
                    /* stdout is now redirected */
                } else if (redirect == RE_ERR) {
                    errfd = creat(outfile, 0660); 
                    close (STDERR_FILENO) ; 
                    dup2(errfd, STDERR_FILENO); 
                    close(errfd); 
                    /* stderr is now redirected */
                } else if (redirect == RE_APPEND) {
                    fd = open(outfile, O_CREAT | O_APPEND | O_RDWR, 0660); 
                    close (STDOUT_FILENO) ; 
                    dup2(fd, STDOUT_FILENO); 
                    close(fd); 
                    /* stdout is now redirected */
                } 
                execvp(pipes[0][0], pipes[0]);
            } else {
                wait(&status);
            }
        } else {

            int pipefd[num_of_pipes][2];

            for (int i = 0; i < num_of_pipes; i++) {
                pipe(pipefd[i]);
            }

            for (int p = 0; p < num_of_pipes; p++) {
                if (fork() == 0) { 

                    // Return to default behavior
                    struct sigaction default_sa;
                    default_sa.sa_flags = 0;
                    default_sa.sa_handler = SIG_DFL;
                    sigemptyset(&default_sa.sa_mask);

                    if (sigaction(SIGINT, &default_sa, NULL) == -1) {
                        perror("sigaction");
                        exit(1);
                    }

                    if (p == 0) { /* first command in pipeline */
                        close(STDOUT_FILENO); /* close stdout */
                        dup2(pipefd[p][1], STDOUT_FILENO); /* redirect stdout to write end of first pipe */
                        close(pipefd[p][0]); /* close read end of first pipe */
                    } else if (pipes[p+1] == NULL) { /* last command in pipeline */
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
                        close(STDIN_FILENO); /* close stdin */
                        dup2(pipefd[p-1][0], STDIN_FILENO); /* redirect stdin to read end of previous pipe */
                        close(pipefd[p-1][1]); /* close write end of previous pipe */
                    } else { /* middle command in pipeline */
                        close(STDIN_FILENO); /* close stdin */
                        dup2(pipefd[p-1][0], STDIN_FILENO); /* redirect stdin to read end of previous pipe */
                        close(pipefd[p-1][1]); /* close write end of previous pipe */
                        close(STDOUT_FILENO); /* close stdout */
                        dup2(pipefd[p][1], STDOUT_FILENO); /* redirect stdout to write end of current pipe */
                        close(pipefd[p][0]); /* close read end of current pipe */
                    }

                    /* close all pipe file descriptors */
                    for (int i = 0; i < num_of_pipes; i++) {
                        close(pipefd[i][0]);
                        close(pipefd[i][1]);
                    }

                    execvp(pipes[p][0], pipes[p]);
                    return status;
                }
            }

            /* parent process */
            for (int c = 0; c < num_of_pipes; c++) {
                close(pipefd[c][0]); /* close read end of pipe */
                close(pipefd[c][1]); /* close write end of pipe */
            }

            for (int k = 0; k < num_of_pipes; k++) {
                wait(&status); /* wait for child processes to exit */
            }
        }

        char status_str[20];
        sprintf(status_str, "%d", status);
        write(piping[1], status_str, sizeof(status_str));
        exit(status);
    } else {
        if(amper == 0) {
            wait(&status);
            read(piping[0], buffer, sizeof(buffer));
            status = atoi(buffer);
        }
    }

    return status;
}

/* free the Command Then Else commands */
void free_cte(char** pipes) {

    int i = 0;
    while( pipes[i] ) {
        free( pipes[i] );
        i++;
    }
    free( pipes[i] );
}

/* handle the control flow (if else) */
int control_flow(char* command) {

    int tc = 0, ec = 0;
    char** cond_commands = NULL;
    char** then_commands = NULL;
    char** else_commands = NULL;

    if( fork() == 0 ) {
        // Return to default behavior
        struct sigaction default_sa;
        default_sa.sa_flags = 0;
        default_sa.sa_handler = SIG_DFL;
        sigemptyset(&default_sa.sa_mask);

        if (sigaction(SIGINT, &default_sa, NULL) == -1) {
            perror("sigaction");
            exit(1);
        }


        /* input of the control flow */

        cond_commands = (char**)realloc(cond_commands, ( 1 ) * sizeof(char*));
        char* cond = (char*)malloc(strlen(command));
        strcpy(cond, command);
        cond_commands[0] = cond;

        if( ! strncmp(command, "then", 4) ) { printf("bash: syntax error near unexpected token `then'\n"); exit(2); }
        if( ! strncmp(command, "else", 4) ) { printf("bash: syntax error near unexpected token `else'\n"); exit(2); }
        if( ! strncmp(command, "fi", 2) ) { printf("bash: syntax error near unexpected token `fi'\n"); exit(2); }
            
        // expecting "then" token
        char then_token[1024];
        fgets(then_token, 1024, stdin);
        then_token[strlen(then_token) - 1] = '\0';
        if( strcmp(then_token, "then") ) { printf("bash: syntax error expected token `then'\n"); exit(2); }


        char cmd[1024];
        fgets(cmd, 1024, stdin);
        cmd[strlen(cmd) - 1] = '\0';
        if( ! strncmp(cmd, "else", 4) ) { printf("bash: syntax error near unexpected token `else'\n"); exit(2); }
        if( ! strncmp(cmd, "fi", 2) ) { printf("bash: syntax error near unexpected token `fi'\n"); exit(2); }

        int is_else = strcmp(cmd, "else");

        while( is_else ) {
            then_commands = (char**)realloc(then_commands, (tc + 1) * sizeof(char*));
            char* then = (char*)malloc(strlen(cmd));
            strcpy(then, cmd);
            then_commands[tc] = then;
            tc++;

            fgets(cmd, 1024, stdin);
            cmd[strlen(cmd) - 1] = '\0';
            is_else = strncmp(cmd, "else", 4);
            if( ! strncmp(cmd, "fi", 2) ) { printf("bash: syntax error near unexpected token `fi'\n"); exit(2); }

        }

        char cmd2[1024];
        fgets(cmd2, 1024, stdin);
        cmd2[strlen(cmd2) - 1] = '\0';
        if( ! strncmp(cmd, "fi", 2) ) { printf("bash: syntax error near unexpected token `fi'\n"); exit(2); }

        int is_fi = strcmp(cmd2, "fi");

        while( is_fi ) {
            else_commands = (char**)realloc(else_commands, (ec + 1) * sizeof(char*));
            char* _else = (char*)malloc(strlen(cmd2));
            strcpy(_else, cmd2);
            else_commands[ec] = _else;
            ec++;

            fgets(cmd2, 1024, stdin);
            cmd2[strlen(cmd2) - 1] = '\0';
            is_fi = strcmp(cmd2, "fi");
        }


        /* execute of the control flow */
        char*** pipes;

        int num_of_pipes = 0;
        pipes = parser(cond_commands[0], &num_of_pipes);
        exec(pipes, num_of_pipes);
        // printf("status: %d\n", status);
        free_pipes(pipes);
        free(pipes);

        // printf("status: %d\n", status);
        if( ! status ) {
            for(int i = 0; i < tc; i++) {
                num_of_pipes = 0;
                pipes = parser(then_commands[i], &num_of_pipes);
                exec(pipes, num_of_pipes);
                free_pipes(pipes);
                free(pipes);
            }

        } else {

            for(int i = 0; i < ec; i++) {
                
                num_of_pipes = 0;
                pipes = parser(else_commands[i], &num_of_pipes);
                exec(pipes, num_of_pipes);
                free_pipes(pipes);
                free(pipes);
            }

        }

        free_cte(cond_commands);
        free_cte(then_commands);
        free_cte(else_commands);

        free(cond_commands);
        free(then_commands);
        free(else_commands);

        exit(status);
    } else {
        wait(&status);
    }

    return 0;
}

/* free the piped command */
void free_pipes(char*** pipes) {

    int i = 0;
    while( pipes[i] ) {
        free( pipes[i] );
        i++;
    }
    free( pipes[i] );
}

void handle_signal(int sig, siginfo_t *si, void *ucontext) {
    printf("You typed Control-C!\n");
}

int main() {

    // Set up signal handler
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = handle_signal;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }

    char command[1024], read[1024];;
    char*** pipes;
    
    History history;
    history_init(&history);

    while (1) {
        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';

        /* handle down arrow key */
        if ( ! strcmp(command, DOWN_ARROW_KEY) ) {
            printf(LINE_UP);
            printf(DELETE_LINE);
            continue;
        }

        /* handle up arrow key */
        if ( ! strcmp(command, UP_ARROW_KEY) ) {
            if ( handle_arrows(&history, command) ) {
                continue;
            }
        }

        /* empty command */
        if( ! strcmp(command, EMPTY_STRING) ) { continue; }

        /* last command */ 
        if(! strcmp(command, "!!")) {
            strcpy(command, history.history[history.history_counter - 1]);
        }

        /* add command to history */
        history_add(&history, command);

        /* handle control flow */
        if( ! strncmp(command, "if", 2) ) {
            control_flow(command + 3);
            continue;
        }

        int num_of_pipes = 0;
        pipes = parser(command, &num_of_pipes);

        /* change the prompt */ 
        if(! strcmp(pipes[0][0], "prompt")) {
            strcpy(prompt, pipes[0][2]);
            strcat(prompt, " ");
            continue;
        }

        /* add vars */ 
        if(pipes[0][0][0] =='$') {
            if(! pipes[0][1] || ! pipes[0][2]) {
                continue;
            }
            if(! strcmp(pipes[0][1], "=")) {
                if ( setenv(pipes[0][0] + 1, pipes[0][2], 1));
                continue;
            }
        }

        /* myread command */
        if(!strcmp(pipes[0][0], "read")) {
            if ( pipes[0][1] ){
                fgets(read, 1024, stdin);
                read[strlen(read) - 1] = '\0';
                if (setenv(pipes[0][1], read, 1));
                continue;
            }
        }

        /* change directory */ 
        if( (pipes[0][0]) && (! strcmp(pipes[0][0], "cd"))) {
            chdir(pipes[0][1]);
            continue;
        }

        /* echo command */ 
        if(! strcmp(pipes[0][0], "echo")) {

            /* echo the last returned status */ 
            if( (pipes[0][1]) && (! strcmp(pipes[0][1], "$?"))) {
                printf("%d\n", status);
                continue;
            }
            int i = 1;
            char *env_var;
            while(pipes[0][i]) {
                if(pipes[0][i][0] =='$') {
                    if( (env_var = getenv(pipes[0][i] + 1)) ) {
                        strcpy(pipes[0][i], env_var);
                    }
                }
                printf("%s ", pipes[0][i]);
                i++;
            }
            printf("\n");
            continue;
        }
        
        /* quit command */ 
        if(! strncmp(command, "quit", 4)) { break; }

        exec(pipes, num_of_pipes);

        free_pipes(pipes);
        free(pipes);
    }

    return 0;
}

