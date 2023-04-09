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

#define UP_ARROW_KEY "\033[A"
#define DOWN_ARROW_KEY "\033[B"
#define LINE_UP "\033[1A"
#define DELETE_LINE "\x1b[2K"
#define EMPTY_STRING ""

char prompt[1024] = "hello: ";
int status;

/*
1. fix bug of quit in case of control flow and ordinary pipes
    by understanding the diff between return exit and _exit
2. fill the rest of the assignment 
*/

/* help handle_arrows print properly */
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

    int piping[2];
    pipe(piping);

    char buffer[10];

    if (fork() == 0) { 

        char status_str[20];

        if(num_of_pipes == 1) {
            if (fork() == 0) { 
                execvp(pipes[0][0], pipes[0]);
            } else {
                wait(&status);
                printf("internal fork exit with: %d\n", status);

                // char status_str[20];
                // sprintf(status_str, "%d", status);
                // write(piping[1], status_str, sizeof(status_str));
                // exit(status);
            }
        } else {

            int pipefd[num_of_pipes][2];

            for (int i = 0; i < num_of_pipes; i++) {
                pipe(pipefd[i]);
            }

            for (int p = 0; p < num_of_pipes; p++) {
                if (fork() == 0) { 
                    if (p == 0) { /* first command in pipeline */
                        close(STDOUT_FILENO); /* close stdout */
                        dup2(pipefd[p][1], STDOUT_FILENO); /* redirect stdout to write end of first pipe */
                        close(pipefd[p][0]); /* close read end of first pipe */
                    } else if (pipes[p+1] == NULL) { /* last command in pipeline */
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
                    // perror("execvp failed");
                    // exit(EXIT_FAILURE);
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

        printf("internal cec3rvcrevcwvvr3cv fork exit with: %d\n", status);
        sprintf(status_str, "%d", status);
        printf("status_str  exit with: %s\n", status_str);
        write(piping[1], status_str, sizeof(status_str));
        exit(status);
    } else {
        wait(&status);
        read(piping[0], buffer, sizeof(buffer));
        status = atoi(buffer);
        printf("I got sts: %d\n", status);
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
int control_flow() {

    if( fork() == 0 ) {
        /* input of the control flow */
        int cc = 0, tc = 0;
        int ec = 0;
        char** cond_commands = NULL;
        char** then_commands = NULL;
        char** else_commands = NULL;

        printf("> ");
        fflush(stdout);
        char condition[1024];
        fgets(condition, 1024, stdin);
        condition[strlen(condition) - 1] = '\0';
        if( (! strcmp(condition, "then")) || (! strcmp(condition, "else")) || (! strcmp(condition, "fi")) ) {
            printf("bash: syntax error near unexpected token `%s'\n", condition);
            exit(0);
        }
        int is_then = strcmp(condition, "then");

        while( is_then ) {
            cond_commands = (char**)realloc(cond_commands, (cc + 1) * sizeof(char*));
            char* cond = (char*)malloc(strlen(condition));
            strcpy(cond, condition);
            cond_commands[cc] = cond;
            cc++;

            printf("> ");
            fflush(stdout);
            fgets(condition, 1024, stdin);
            condition[strlen(condition) - 1] = '\0';
            if( (! strcmp(condition, "else")) || (! strcmp(condition, "fi")) ) {
                printf("bash: syntax error near unexpected token `%s'\n", condition);
                exit(0);
            }
            is_then = strcmp(condition, "then");
        }

        printf("> ");
        fflush(stdout);
        char cmd[1024];
        fgets(cmd, 1024, stdin);
        cmd[strlen(cmd) - 1] = '\0';
        if( (! strcmp(cmd, "else")) || (! strcmp(cmd, "fi")) ) {
            printf("bash: syntax error near unexpected token `%s'\n", cmd);
            exit(0);
        }
        int is_else = strcmp(cmd, "else");

        while( is_else ) {
            then_commands = (char**)realloc(then_commands, (tc + 1) * sizeof(char*));
            char* then = (char*)malloc(strlen(cmd));
            strcpy(then, cmd);
            then_commands[tc] = then;
            tc++;

            printf("> ");
            fflush(stdout);
            fgets(cmd, 1024, stdin);
            cmd[strlen(cmd) - 1] = '\0';
            is_else = strcmp(cmd, "else");
        }

        printf("> ");
        fflush(stdout);
        char cmd2[1024];
        fgets(cmd2, 1024, stdin);
        cmd2[strlen(cmd2) - 1] = '\0';
        int is_fi = strcmp(cmd2, "fi");

        if( ! is_fi ) { 
            printf("bash: syntax error near unexpected token `%s'\n", cmd2);
            exit(2); 
        }

        while( is_fi ) {
            else_commands = (char**)realloc(else_commands, (ec + 1) * sizeof(char*));
            char* _else = (char*)malloc(strlen(cmd2));
            strcpy(_else, cmd2);
            else_commands[ec] = _else;
            ec++;

            printf("> ");
            fflush(stdout);
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
                pipes = parser(then_commands[0], &num_of_pipes);
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

int main() {


    char command[1024];
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

        /* add command to history */
        history_add(&history, command);

        
        /* quit command */ 
        if(! strncmp(command, "quit", 4)) { break; }

        /* handle control flow */
        if( ! strncmp(command, "if", 2) ) {
            control_flow();
        } else {
            int num_of_pipes = 0;
            pipes = parser(command, &num_of_pipes);

            exec(pipes, num_of_pipes);
            // printf("ssss: %d\n", status);
            free_pipes(pipes);
            free(pipes);
        }
    }

    return 0;
}

