#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "sched.h"
#include <signal.h>
#include <unistd.h>

int init_fn();
struct sched_waitq_head *wq1, *wq2;

extern int global_pid_array_val;

abrt_handler(int sig)
{
	sched_ps();
}

wakeup_handler(int sig)
{
	if(sig==SIGUSR1)
	{
		fprintf(stderr, "About to wake up queue 1\n");
		sched_wakeup(wq1);
	}
	else
	{
		fprintf(stderr, "About to wake up queue 2\n");
		sched_wakeup(wq2);
	}
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	fprintf(stderr,"Starting\n");
	sa.sa_flags=0;
	sa.sa_handler=wakeup_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1,&sa,NULL);
	sigaction(SIGUSR2,&sa,NULL);
	sched_init(init_fn);
	return 0;
}

int init_fn()
{
	printf("Successfully init'd\n");
	sched_waitq_init(wq1);
	sched_waitq_init(wq2);
	while(global_pid_array_val<3)
	{
		if(sched_fork() == 0)
		{
			//In child
			sched_nice(-1*sched_getpid());	
			break;
		}
		else
		{
			//In parent
		}
	}
	while(1);
	exit(0);
}