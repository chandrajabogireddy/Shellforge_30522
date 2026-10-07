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
#include "jobs.h"
#include "job_control.h"

int main(void)
{
    printf("=====================================\n");
printf("      Shellforge\n");
printf(" A Unix Style Shell written in C\n");
printf("=====================================\n");
    jobs_init();
    init_job_control();

    token_list_t tokens;
    pipeline_t pipeline;
    char *line;

    while (1)
    {
        jobs_update();

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

        if (strcmp(line, "jobs") == 0)
        {
            jobs_list();
            free(line);
            continue;
        }

        if (strncmp(line, "fg ", 3) == 0)
        {
            int job_id = atoi(line + 3);
            fg_job(job_id);

            free(line);
            continue;
        }

        if (strncmp(line, "bg ", 3) == 0)
        {
            int job_id = atoi(line + 3);
            bg_job(job_id);

            free(line);
            continue;
        }

        shellforge_add_history(line);

        lexer(line, &tokens);

        if (parser(&tokens, &pipeline))
        {
            expand_variables(&pipeline);

            if (pipeline.command_count == 1 &&
                pipeline.commands[0].argc > 0 &&
                strcmp(pipeline.commands[0].argv[0], "exit") == 0)
            {
                free(line);
                break;
            }

            execute_pipeline(&pipeline);
        }

        free(line);
    }

    return 0;
}
