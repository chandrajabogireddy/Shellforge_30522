#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <readline/readline.h>

#include "history.h"
#include "token.h"
#include "lexer.h"
#include "parser.h"
#include "expand.h"
#include "executer.h"

int main(void)
{
    printf("=====================================\n");
    printf("      Shellforge\n");
    printf(" A Unix Style Shell written in C\n");
    printf("=====================================\n");

    token_list_t tokens;
    pipeline_t pipeline;
    char *line;

    while (1)
    {
        line = readline("shellforge$ ");

        if (line == NULL)
        {
            printf("\nGoodbye!\n");
            break;
        }

        if (strlen(line) == 0)
        {
            free(line);
            continue;
        }

        if (strcmp(line, "history") == 0)
        {
            show_history();
            free(line);
            continue;
        }

        /* Save command in Shellforge history */
        shellforge_add_history(line);

        /* Lexical analysis */
        lexer(line, &tokens);

        /* Parsing */
        if (parser(&tokens, &pipeline))
        {
            /* Expand environment variables */
            expand_variables(&pipeline);

            /*
             * Handle exit in the parent process.
             * This is important because execute_pipeline()
             * uses child processes for pipelines.
             */
            if (pipeline.command_count == 1 &&
                pipeline.commands[0].argc > 0 &&
                strcmp(pipeline.commands[0].argv[0], "exit") == 0)
            {
                free(line);
                break;
            }

            /* Execute command or pipeline */
            execute_pipeline(&pipeline);
        }

        free(line);
    }

    return 0;
}
