#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "../include/executer.h"
#include "../include/builtin.h"
#include "../include/jobs.h"


static int apply_redirection(command_t *cmd)
{
    int fd;

    /* Input redirection: < */
    if (cmd->input[0] != '\0')
    {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror("open input");
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2 input");
            close(fd);
            return -1;
        }

        close(fd);
    }

    /* Output redirection: > or >> */
    if (cmd->output[0] != '\0')
    {
        if (cmd->append)
        {
            fd = open(cmd->output,
                      O_WRONLY | O_CREAT | O_APPEND,
                      0644);
        }
        else
        {
            fd = open(cmd->output,
                      O_WRONLY | O_CREAT | O_TRUNC,
                      0644);
        }

        if (fd < 0)
        {
            perror("open output");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2 output");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}


int execute_command(command_t *cmd)
{
    if (cmd == NULL || cmd->argc == 0)
        return -1;

    /* Built-in command */
    if (is_builtin(cmd))
    {
        int saved_stdin = dup(STDIN_FILENO);
        int saved_stdout = dup(STDOUT_FILENO);

        if (saved_stdin < 0 || saved_stdout < 0)
        {
            perror("dup");
            return -1;
        }

        if (apply_redirection(cmd) < 0)
        {
            dup2(saved_stdin, STDIN_FILENO);
            dup2(saved_stdout, STDOUT_FILENO);

            close(saved_stdin);
            close(saved_stdout);

            return -1;
        }

        int result = execute_builtin(cmd);

        dup2(saved_stdin, STDIN_FILENO);
        dup2(saved_stdout, STDOUT_FILENO);

        close(saved_stdin);
        close(saved_stdout);

        return result;
    }


    /* External command */
    pid_t pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }

    if (pid == 0)
    {
        if (apply_redirection(cmd) < 0)
            exit(EXIT_FAILURE);

        char *args[MAX_ARGS + 1];

        for (int i = 0; i < cmd->argc; i++)
            args[i] = cmd->argv[i];

        args[cmd->argc] = NULL;

        execvp(args[0], args);

        perror("Shellforge");
        exit(EXIT_FAILURE);
    }


    /* Background command */
    if (cmd->background)
    {
        if (setpgid(pid, pid) < 0)
            perror("setpgid");

        int job_id = jobs_add(pid, cmd->argv[0]);

        if (job_id > 0)
        {
            printf("[%d] Running\t%s\n",
                   job_id,
                   cmd->argv[0]);
        }

        return 0;
    }


    /* Foreground command */
    waitpid(pid, NULL, 0);

    return 0;
}


int execute_pipeline(pipeline_t *pipeline)
{
    if (pipeline == NULL || pipeline->command_count == 0)
        return -1;

    int previous_read = -1;
    pid_t pids[MAX_COMMANDS];

    for (int i = 0; i < pipeline->command_count; i++)
    {
        int pipefd[2];

        if (i < pipeline->command_count - 1)
        {
            if (pipe(pipefd) == -1)
            {
                perror("pipe");
                return -1;
            }
        }

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
            return -1;
        }

        /* CHILD */
        if (pid == 0)
        {
            command_t *cmd = &pipeline->commands[i];

            if (previous_read != -1)
            {
                if (dup2(previous_read, STDIN_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                close(previous_read);
            }

            if (i < pipeline->command_count - 1)
            {
                close(pipefd[0]);

                if (dup2(pipefd[1], STDOUT_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                close(pipefd[1]);
            }

            if (apply_redirection(cmd) < 0)
                exit(EXIT_FAILURE);

            if (is_builtin(cmd))
            {
                int result = execute_builtin(cmd);
                exit(result);
            }

            char *args[MAX_ARGS + 1];

            for (int j = 0; j < cmd->argc; j++)
                args[j] = cmd->argv[j];

            args[cmd->argc] = NULL;

            execvp(args[0], args);

            perror("Shellforge");
            exit(EXIT_FAILURE);
        }

        /* PARENT */
        pids[i] = pid;

        if (i == 0)
        {
            if (setpgid(pid, pid) < 0)
                perror("setpgid");
        }
        else
        {
            if (setpgid(pid, pids[0]) < 0)
                perror("setpgid");
        }

        if (previous_read != -1)
            close(previous_read);

        if (i < pipeline->command_count - 1)
        {
            close(pipefd[1]);
            previous_read = pipefd[0];
        }
    }

    if (previous_read != -1)
        close(previous_read);


    /* Background pipeline */
    if (pipeline->commands[0].background)
    {
        int job_id = jobs_add(
            pids[0],
            pipeline->commands[0].argv[0]
        );

        if (job_id > 0)
        {
            printf("[%d] Running\t%s\n",
                   job_id,
                   pipeline->commands[0].argv[0]);
        }

        return 0;
    }


    /* Foreground pipeline */
    for (int i = 0; i < pipeline->command_count; i++)
    {
        waitpid(pids[i], NULL, 0);
    }

    return 0;
}
