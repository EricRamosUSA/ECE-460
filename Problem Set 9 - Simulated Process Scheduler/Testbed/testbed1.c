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
	printf("UNIX PROCESS PID = %d\n", getpid());
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
	while(global_pid_array_val<6)
	{
		if(sched_fork() == 0)
		{
			//In child
			break;
		}
		else
		{
			//In parent
		}
	}
	if(sched_getpid()==1)
	{
		int code1, code2, code3, code4, code5;
		int pid1 = sched_wait(&code1);
		int pid2 = sched_wait(&code2);
		int pid3 = sched_wait(&code3);
		int pid4 = sched_wait(&code4);
		int pid5 = sched_wait(&code5);
		fprintf(stderr, "PID %d exited with code %d\n", pid1, code1);
		fprintf(stderr, "PID %d exited with code %d\n", pid2, code2);
		fprintf(stderr, "PID %d exited with code %d\n", pid3, code3);
		fprintf(stderr, "PID %d exited with code %d\n", pid4, code4);
		fprintf(stderr, "PID %d exited with code %d\n", pid5, code5);
	}
	if(sched_getpid()==3)
	{
		fprintf(stderr, "About to exit pid %d\n", sched_getpid());
		sched_exit(22);
	}
	if(sched_getpid()!=1)
	{
		fprintf(stderr, "About to exit pid %d\n", sched_getpid());
		sched_exit(11);
	}
	printf("PID %d about to enter while(1) loop\n", sched_getpid());
	while(1);
	exit(0);
}