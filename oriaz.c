#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>



char prompt[1024] = "hello: ";
int status;


char** parse_pipe(char* command, char* by, int* n) {
    char** coms = NULL;
    // *n = 0;
    int i = 0;

    /* parse command line */
    char* token = strtok(command, by);
    while (token != NULL) {
        coms = (char**)realloc(coms, (i + 1) * sizeof(char*) );
        coms[i++] = token;
        token = strtok(NULL, by);

        if( n ) {
            (*n)++;
        }
    }
    coms = (char**)realloc(coms, (i + 1) * sizeof(char*));
    coms[i] = NULL;

    return coms;
}

char ***parser(char* command, int* num_of_pipes) {

    char*** coms = NULL;
    int i = 0;
    char by = '|';
    char** parsed_pipes = parse_pipe(command, &by, num_of_pipes);
   

    by = ' ';
    char** parsed_pipe;
    while( parsed_pipes[i] ) {
        // printf("parsed_pipes[i]: %s\n", parsed_pipes[i]);
        parsed_pipe = parse_pipe(parsed_pipes[i], &by, NULL);
        coms = (char***)realloc(coms, (i + 1) * sizeof(char**) );
        coms[i++] = parsed_pipe;
    }

    coms = (char***)realloc(coms, (i + 1) * sizeof(char**));
    coms[i] = NULL;

    return coms;
}

int exec(char*** pipes, int num_of_pipes) {

    if (fork() == 0) { 

        int pipefd[num_of_pipes][2];

        for (int i = 0; i < num_of_pipes; i++) {
            pipe(pipefd[i]);
        }

        for (int p = 0; p < num_of_pipes; p++) {
            if (fork() == 0) { /* child process */
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
                perror("execvp failed");
                exit(EXIT_FAILURE);
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
    } else {
        wait(NULL);
        
    }

    

    return 0;
}

int control_flow() {

    /* input of the control flow */
    int cc = 0, tc = 0;
    int ec = 0;
    char** cond_commands = NULL; //(char**)malloc(sizeof(char*));
    char** then_commands = NULL; //(char**)malloc(sizeof(char*));
    char** else_commands = NULL; //(char**)malloc(sizeof(char*));

    printf("> ");
    fflush(stdout);
    char condition[1024];
    fgets(condition, 1024, stdin);
    condition[strlen(condition) - 1] = '\0';
    int is_then = strcmp(condition, "then");

    if( ! is_then ) { exit(2); }

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
        is_then = strcmp(condition, "then");
    }

    printf("> ");
    fflush(stdout);
    char cmd[1024];
    fgets(cmd, 1024, stdin);
    cmd[strlen(cmd) - 1] = '\0';
    int is_else = strcmp(cmd, "else");

    if( ! is_else ) { exit(2); }

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

    if( ! is_fi ) { exit(2); }

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

    printf("status: %d\n", status);
    if( ! status ) {
        for(int i = 0; i < tc; i++) {
            num_of_pipes = 0;
            pipes = parser(then_commands[0], &num_of_pipes);
            exec(pipes, num_of_pipes);
        }

    } else {

        for(int i = 0; i < ec; i++) {
            
            num_of_pipes = 0;
            pipes = parser(else_commands[i], &num_of_pipes);
            exec(pipes, num_of_pipes);
        }

    }
    

    return 0;
}


/* free the piped command */
int free_pipes(char*** pipes) {
    return 0;
}



int main() {

    char command[1024];
    char*** pipes;

    while (1) {
        printf("%s", prompt);
        fgets(command, 1024, stdin);
        command[strlen(command) - 1] = '\0';

        if( ! strncmp(command, "if", 2) ) {
            control_flow();
        } else {
            int num_of_pipes = 0;
            pipes = parser(command, &num_of_pipes);

            exec(pipes, num_of_pipes);
            free_pipes(pipes);
        }
    }

    return 0;
}
