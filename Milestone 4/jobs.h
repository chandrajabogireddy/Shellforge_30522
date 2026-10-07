#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

#define MAX_JOBS 32

typedef enum
{
    JOB_RUNNING,
    JOB_STOPPED,
    JOB_DONE
} job_state_t;

typedef struct
{
    int id;
    pid_t pgid;
    char command[256];
    job_state_t state;
} job_t;

void jobs_init(void);
int jobs_add(pid_t pgid, const char *command);
void jobs_remove(int id);
void jobs_list(void);
job_t *jobs_get(int id);
void jobs_update(void);

#endif
