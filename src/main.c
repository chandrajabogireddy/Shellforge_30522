#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <readline/readline.h>
#include "../include/history.h"

int main()
{
    char *input;

    printf("=====================================\n");
    printf("      Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    while (1)
    {
        input = readline("shellforge$ ");

        if (input == NULL)
            break;

        if (strlen(input) == 0)
        {
            free(input);
            continue;
        }

        if (strcmp(input, "history") == 0)
        {
            show_history();
        }
        else
        {
            add_history(input);
            printf(" YOU ENTERED : %s\n", input);

            if (strcmp(input, "exit") == 0)
            {
                printf("Exiting...\n");
                free(input);
                break;
            }
        }

        free(input);
    }

    return 0;
}
