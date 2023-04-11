
#ifndef __MY_SHELL__
#define __MY_SHELL__

int print_hc(char* str1, char* str2);

int handle_arrows(History* h, char* cmd);

char** parse_pipe(char* command, char* by, int* n);

char ***parser(char* command, int* num_of_pipes);

int exec(char*** pipes, int num_of_pipes);

int control_flow();

void free_cte(char** pipes);

void free_pipes(char*** pipes);


#endif