#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include "executer.h"
#include "builtin.h"


/*
 * Apply input/output redirection.
 */
static int apply_redirection(command_t *cmd)
{
    int fd;

    /* Input redirection: < file */
    if (cmd->input[0] != '\0')
    {
        fd = open(cmd->input, O_RDONLY);

        if (fd < 0)
        {
            perror("input");
            return -1;
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    /* Output redirection: > file or >> file */
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
            perror("output");
            return -1;
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("dup2");
            close(fd);
            return -1;
        }

        close(fd);
    }

    return 0;
}


/*
 * Execute one command.
 */
int execute_command(command_t *cmd)
{
    pid_t pid;
    int status;

    if (cmd == NULL || cmd->argc == 0)
    {
        return -1;
    }

    /*
     * Built-in commands must run in the parent
     * so that cd can change the shell directory.
     */
    if (is_builtin(cmd))
    {
        int saved_stdin = -1;
        int saved_stdout = -1;

        /*
         * Save stdin when input redirection is used.
         */
        if (cmd->input[0] != '\0')
        {
            saved_stdin = dup(STDIN_FILENO);

            if (saved_stdin < 0)
            {
                perror("dup");
                return -1;
            }
        }

        /*
         * Save stdout when output redirection is used.
         */
        if (cmd->output[0] != '\0')
        {
            saved_stdout = dup(STDOUT_FILENO);

            if (saved_stdout < 0)
            {
                perror("dup");

                if (saved_stdin != -1)
                    close(saved_stdin);

                return -1;
            }
        }

        /*
         * Apply redirection.
         */
        if (apply_redirection(cmd) < 0)
        {
            if (saved_stdin != -1)
                close(saved_stdin);

            if (saved_stdout != -1)
                close(saved_stdout);

            return -1;
        }

        /*
         * Execute builtin.
         */
        int result = execute_builtin(cmd);

        /*
         * Flush output before restoring stdout.
         */
        fflush(stdout);

        /*
         * Restore stdin.
         */
        if (saved_stdin != -1)
        {
            dup2(saved_stdin, STDIN_FILENO);
            close(saved_stdin);
        }

        /*
         * Restore stdout.
         */
        if (saved_stdout != -1)
        {
            dup2(saved_stdout, STDOUT_FILENO);
            close(saved_stdout);
        }

        return result;
    }


    /*
     * External command.
     */
    pid = fork();

    if (pid < 0)
    {
        perror("fork");
        return -1;
    }


    /*
     * CHILD PROCESS
     */
    if (pid == 0)
    {
        char *args[MAX_ARGS + 1];

        /*
         * Apply redirection.
         */
        if (apply_redirection(cmd) < 0)
        {
            exit(EXIT_FAILURE);
        }

        /*
         * Prepare argv for execvp().
         */
        for (int i = 0; i < cmd->argc; i++)
        {
            args[i] = cmd->argv[i];
        }

        args[cmd->argc] = NULL;

        /*
         * Execute external command.
         */
        execvp(args[0], args);

        /*
         * execvp() only returns on failure.
         */
        perror("Shellforge");
        exit(EXIT_FAILURE);
    }


    /*
     * PARENT PROCESS
     */
    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return -1;
    }

    if (WIFEXITED(status))
    {
        return WEXITSTATUS(status);
    }

    if (WIFSIGNALED(status))
    {
        fprintf(stderr,
                "Process terminated by signal %d\n",
                WTERMSIG(status));

        return -1;
    }

    return 0;
}


/*
 * Execute a complete pipeline.
 *
 * Example:
 *
 *     ls | wc -l
 */
int execute_pipeline(pipeline_t *pipeline)
{
    if (pipeline == NULL || pipeline->command_count == 0)
    {
        return -1;
    }

    /*
     * Single command.
     */
    if (pipeline->command_count == 1)
    {
        return execute_command(&pipeline->commands[0]);
    }


    int previous_read = -1;
    pid_t pids[MAX_COMMANDS];


    /*
     * Create processes for every command.
     */
    for (int i = 0; i < pipeline->command_count; i++)
    {
        int pipefd[2];


        /*
         * Create pipe except for the last command.
         */
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


        /*
         * CHILD PROCESS
         */
        if (pid == 0)
        {
            command_t *cmd = &pipeline->commands[i];


            /*
             * Connect previous pipe to stdin.
             */
            if (previous_read != -1)
            {
                if (dup2(previous_read, STDIN_FILENO) < 0)
                {
                    perror("dup2");
                    exit(EXIT_FAILURE);
                }

                close(previous_read);
            }


            /*
             * Connect current pipe to stdout.
             */
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


            /*
             * Apply explicit redirection.
             */
            if (apply_redirection(cmd) < 0)
            {
                exit(EXIT_FAILURE);
            }


            /*
             * Built-in inside a pipeline.
             */
            if (is_builtin(cmd))
            {
                int result = execute_builtin(cmd);
                exit(result);
            }


            /*
             * Prepare argv.
             */
            char *args[MAX_ARGS + 1];

            for (int j = 0; j < cmd->argc; j++)
            {
                args[j] = cmd->argv[j];
            }

            args[cmd->argc] = NULL;


            /*
             * Execute external command.
             */
            execvp(args[0], args);


            perror("Shellforge");
            exit(EXIT_FAILURE);
        }


        /*
         * PARENT PROCESS
         */
        pids[i] = pid;


        /*
         * Close previous pipe.
         */
        if (previous_read != -1)
        {
            close(previous_read);
        }


        /*
         * Save current pipe for next command.
         */
        if (i < pipeline->command_count - 1)
        {
            close(pipefd[1]);
            previous_read = pipefd[0];
        }
    }


    /*
     * Close final pipe.
     */
    if (previous_read != -1)
    {
        close(previous_read);
    }


    /*
     * Wait for all pipeline processes.
     */
    int status;
    int last_status = 0;

    for (int i = 0; i < pipeline->command_count; i++)
    {
        if (waitpid(pids[i], &status, 0) == -1)
        {
            perror("waitpid");
            return -1;
        }

        if (i == pipeline->command_count - 1 &&
            WIFEXITED(status))
        {
            last_status = WEXITSTATUS(status);
        }
    }

    return last_status;
}
