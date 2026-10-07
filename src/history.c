#include <stdio.h>
#include <string.h>
#include "../include/history.h"

#define MAX_HISTORY 100
#define MAX_CMD 256

char history[MAX_HISTORY][MAX_CMD];
int count = 0;

void shellforge_add_history(char *cmd){
    if (count < MAX_HISTORY)
    {
        strcpy(history[count], cmd);
        count++;
    }
}

void show_history()
{
    printf("\n------------ Command History ----------\n");
    for (int i = 0; i < count; i++)
    {
        printf("%2d   %s\n", i + 1, history[i]);
    }
    printf("---------------------------------------\n");
}
