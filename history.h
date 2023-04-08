
#ifndef __HISTORY__
#define __HISTORY__

#include <stdio.h>
#include <string.h>

#define HISTORY_SIZE 20

typedef struct _History {
    char history[HISTORY_SIZE][1024];
    int history_counter;
} History;

int history_init(History* h);

int history_add(History* h, char* cmd);

int history_print(History* h);

#endif