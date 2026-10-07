#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>

#include "../include/job_control.h"
#include "../include/jobs.h"

static pid_t shell_pgid;
static struct termios shell_tmodes;


/* ---------------------------------------------------------
   Initialize shell job control
   --------------------------------------------------------- */
void init_job_control(void)
{
    /*
     * Ignore signals that could stop the shell while it
     * manipulates the terminal.
     */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);

    shell_pgid = getpid();

    /*
     * Put the shell into its own process group.
     */
    if (setpgid(shell_pgid, shell_pgid) < 0)
    {
        /* Already a process-group leader is okay. */
        if (getpgrp() != shell_pgid)
            perror("setpgid");
    }

    /*
     * Give the terminal to the shell.
     */
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp");

    /*
     * Save terminal settings.
     */
    if (tcgetattr(STDIN_FILENO, &shell_tmodes) < 0)
        perror("tcgetattr");
}


/* ---------------------------------------------------------
   Give terminal to foreground job
   --------------------------------------------------------- */
void put_job_foreground(pid_t pgid)
{
    if (tcsetpgrp(STDIN_FILENO, pgid) < 0)
        perror("tcsetpgrp");
}


/* ---------------------------------------------------------
   Continue a job in background
   --------------------------------------------------------- */
void put_job_background(pid_t pgid)
{
    if (kill(-pgid, SIGCONT) < 0)
        perror("SIGCONT");
}


/* ---------------------------------------------------------
   Wait for foreground job
   --------------------------------------------------------- */
void wait_for_job(pid_t pgid)
{
    int status;

    /*
     * Wait for any process in the job's process group.
     */
    while (1)
    {
        pid_t result = waitpid(
            -pgid,
            &status,
            WUNTRACED
        );

        if (result < 0)
        {
            perror("waitpid");
            break;
        }

        /*
         * Job stopped.
         */
        if (WIFSTOPPED(status))
        {
            break;
        }

        /*
         * Job terminated normally.
         */
        if (WIFEXITED(status))
        {
            break;
        }

        /*
         * Job terminated by a signal.
         */
        if (WIFSIGNALED(status))
        {
            break;
        }
    }

    /*
     * Give terminal control back to ShellForge.
     */
    if (tcsetpgrp(STDIN_FILENO, shell_pgid) < 0)
        perror("tcsetpgrp");

    /*
     * Restore shell terminal settings.
     */
    if (tcsetattr(
            STDIN_FILENO,
            TCSADRAIN,
            &shell_tmodes
        ) < 0)
    {
        perror("tcsetattr");
    }
}


/* ---------------------------------------------------------
   Bring job to foreground
   --------------------------------------------------------- */
void fg_job(int job_id)
{
    job_t *job = jobs_get(job_id);

    if (job == NULL)
    {
        printf("fg: job not found: %d\n", job_id);
        return;
    }

    if (job->state == JOB_DONE)
    {
        printf(
            "fg: job %d has already completed\n",
            job_id
        );

        jobs_remove(job_id);
        return;
    }

    printf("%s\n", job->command);

    job->state = JOB_RUNNING;

    /*
     * Give terminal to the job.
     */
    put_job_foreground(job->pgid);

    /*
     * Continue the job.
     */
    if (kill(-job->pgid, SIGCONT) < 0)
    {
        perror("SIGCONT");

        /*
         * Take terminal back if SIGCONT fails.
         */
        tcsetpgrp(STDIN_FILENO, shell_pgid);
        return;
    }

    /*
     * Wait until the job finishes or stops.
     */
    wait_for_job(job->pgid);

    /*
     * Remove completed foreground job.
     */
    jobs_remove(job_id);
}


/* ---------------------------------------------------------
   Continue job in background
   --------------------------------------------------------- */
void bg_job(int job_id)
{
    job_t *job = jobs_get(job_id);

    if (job == NULL)
    {
        printf("bg: job not found: %d\n", job_id);
        return;
    }

    if (job->state == JOB_DONE)
    {
        printf(
            "bg: job %d has already completed\n",
            job_id
        );

        jobs_remove(job_id);
        return;
    }

    job->state = JOB_RUNNING;

    put_job_background(job->pgid);

    printf(
        "[%d] Running\t%s\n",
        job->id,
        job->command
    );
}
