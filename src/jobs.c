#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <signal.h>
#include "../include/jobs.h"

static job_t jobs[MAX_JOBS];
static int job_count = 0;

void jobs_init(void)
{
    job_count = 0;

    for (int i = 0; i < MAX_JOBS; i++)
        jobs[i].id = 0;
}

int jobs_add(pid_t pgid, const char *command)
{
    if (job_count >= MAX_JOBS)
        return -1;

    int index = -1;

    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].id == 0)
        {
            index = i;
            break;
        }
    }

    if (index == -1)
        return -1;

    jobs[index].id = index + 1;
    jobs[index].pgid = pgid;
    jobs[index].state = JOB_RUNNING;

    strncpy(jobs[index].command, command,
            sizeof(jobs[index].command) - 1);

    jobs[index].command[sizeof(jobs[index].command) - 1] = '\0';

    job_count++;

    return jobs[index].id;
}

void jobs_remove(int id)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].id == id)
        {
            jobs[i].id = 0;
            job_count--;
            return;
        }
    }
}

job_t *jobs_get(int id)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].id == id)
            return &jobs[i];
    }

    return NULL;
}

void jobs_list(void)
{
    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].id != 0)
        {
            const char *state;

            if (jobs[i].state == JOB_RUNNING)
                state = "Running";
            else if (jobs[i].state == JOB_STOPPED)
                state = "Stopped";
            else
                state = "Done";

            printf("[%d] %s\t%s\n",
                   jobs[i].id,
                   state,
                   jobs[i].command);
        }
    }
}

void jobs_update(void)
{
    int status;

    for (int i = 0; i < MAX_JOBS; i++)
    {
        if (jobs[i].id != 0)
        {
            pid_t result = waitpid(-jobs[i].pgid,
                                   &status,
                                   WNOHANG);

            if (result > 0)
            {
                jobs[i].state = JOB_DONE;
            }
        }
    }
}
