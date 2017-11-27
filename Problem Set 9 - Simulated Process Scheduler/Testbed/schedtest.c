//Modified schedtest.c
//Changes to the program were made
//at lines with "//"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include "sched.h"
#include <signal.h>
#include <unistd.h>

#define DELAY_FACTOR 29
struct sched_waitq_head *wq1, *wq2; //
extern struct sched_waitq_head *parent_wait; //
init_fn()
{
	int x;
	sched_waitq_init(wq1); //
	sched_waitq_init(wq2); //
	fprintf(stderr,"Hooray made it to init_fn, stkaddr %p\n",&x);
	switch (sched_fork())
	{
	 case -1:
		fprintf(stderr,"fork failed\n");
		return -1;
	 case 0:
		fprintf(stderr,"<<in child addr %p>>\n",&x);
		child_fn1();
		fprintf(stderr,"!!BUG!! at %s:%d\n",__FILE__,__LINE__);
		return 0;
	 default:
		fprintf(stderr,"<<in parent addr %p>>\n",&x);
		parent_fn();
		break;
	}
	exit(0);
}

child_fn1()
{
 int y;
	fprintf(stderr,"Start pass1, child_fn1 &y=%p\n",&y);
	for(y=1;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 1,child_fn1 y=%d\n",y);
	sched_sleep(wq1); //
	fprintf(stderr,"Resuming child_fn1\n");
	for(y=1;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 2,child_fn1 y=%d\n",y);
	sched_exit(22);
}

parent_fn()
{
	int y,p;
	fprintf(stderr,"Wow, made it to parent, stkaddr=%p\n",&y);
	switch(sched_fork())
	{
	 case -1:fprintf(stderr,"Fork failed\n");
		return;
	 case 0:
		child_fn2();
		sched_exit(11);
		fprintf(stderr,"!!BUG!! at %s:%d\n",__FILE__,__LINE__);
		return;
	 default:
		while ((p=sched_wait(&y))>0)
			fprintf(stderr,"Child pid %d return code %d\n",p,y);;
		return;
	}
}

child_fn2()
{
	int y;
	sched_nice(4);
	fprintf(stderr,"Start pass1, child_fn2 &y=%p\n",&y);
	for(y=0;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 1,child_fn2 y=%d\n",y);
	sched_sleep(wq2);
	fprintf(stderr,"Resuming child_fn2\n");
	for(y=0;y<1<<DELAY_FACTOR;y++)
		;
	fprintf(stderr,"Done pass 2,child_fn2 y=%d\n",y);
	
}

void wakeup_handler(int sig) //
{
	if (sig==SIGUSR1)
	{//
		fprintf(stderr, "Recieved SIGUSR1\n");//
		sched_wakeup(wq1);
	}//
	else
	{//
		fprintf(stderr, "Recieved SIGUSR2\n");//
		sched_wakeup(wq2);
	}//
}

abrt_handler(int sig)
{
	sched_ps();
}

main()
{
	printf("UNIX PROCESS PID = %d\n", getpid()); //
	struct sigaction sa;
	fprintf(stderr,"Starting\n");
	sa.sa_flags=0;
	sa.sa_handler=wakeup_handler;
	sigemptyset(&sa.sa_mask);
	sigaction(SIGUSR1,&sa,NULL);
	sigaction(SIGUSR2,&sa,NULL);
	//sa.sa_handler=abrt_handler;
	//sigaction(SIGABRT,&sa,NULL);
	sched_init(init_fn);
	fprintf(stderr,"Whoops\n");
}