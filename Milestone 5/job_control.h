#ifndef JOB_CONTROL_H
#define JOB_CONTROL_H

#include <sys/types.h>

void init_job_control(void);

void put_job_foreground(pid_t pgid);
void put_job_background(pid_t pgid);
void wait_for_job(pid_t pgid);

void fg_job(int job_id);
void bg_job(int job_id);

#endif
