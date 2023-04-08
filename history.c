#include "history.h"


int history_init(History* h) {
    h->history_counter = 0;
    return 0;
}

int history_add(History* h, char* cmd) {
    if (h->history_counter < HISTORY_SIZE) {
        strcpy(h->history[h->history_counter], cmd);
        h->history_counter++;
    } else {
        for (int j = 0; j < HISTORY_SIZE - 1; j++) {
            strcpy(h->history[j], h->history[j + 1]);
        }
        strcpy(h->history[HISTORY_SIZE - 1], cmd);
    }

    return 0;
}

int history_print(History* h) {
    printf("history: \n");
    for(int i = 0; i < h->history_counter; i++) {
        printf("%d. %s\n", i, h->history[i]);
    }
    printf("end of history\n");

    return 0;
}
