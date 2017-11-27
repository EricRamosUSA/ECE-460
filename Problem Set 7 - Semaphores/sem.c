//sem.c
//Eric Ramos

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include "sem.h"

extern int my_procnum;

static struct sem *s;
void handler(int signo);

void sem_init(struct sem *s, int count)
{
	s->sem_count = count;
	s->spinlock = 0;
	signal(SIGUSR1, handler);
}

int lock_proc(volatile char *lock)
{
	while(tas(lock)!=0);
	return 1;
}

//Basic algorithm:
//Mask interrupts
//Lock memory bus
//Perform operation
//Release memory bus
//Unmask interrupts

int sem_try(struct sem *s)
{
	int isblocked = 0; //If operation blocks, return 0;
	sigset_t oldmask, newmask;
	sigfillset(&newmask); //Set all masks
	sigprocmask(SIG_BLOCK, &newmask, &oldmask); //Block
	if(lock_proc(&(s->spinlock)) == 1)
	{
		if(s->sem_count>0)
		{
			s->sem_count--;
			isblocked = 1;
		}
		else
		{
			isblocked = 0;
		}
		s->spinlock = 0;
		sigprocmask(SIG_SETMASK, &oldmask, NULL); //Restore
	}
	return isblocked;
}

void sem_wait(struct sem *s)
{
	sigset_t oldmask, newmask;
	sigfillset(&newmask);
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	while(1)
	{
		if(lock_proc(&(s->spinlock)) == 1)
		{
			s->proc_pid[my_procnum] = getpid();
			if(s->sem_count>0)
			{
				s->sem_count--;
				s->proc_status[my_procnum] = 0; //Unblock
				sigprocmask(SIG_BLOCK, &oldmask, NULL);
				s->spinlock = 0;
				break;
			}
			else //Add proc to list + suspend
			{
				s->proc_status[my_procnum] = 1;
				sigset_t wait_mask;
				sigfillset(&wait_mask);
				sigdelset(&wait_mask, SIGUSR1); //Setting mask to full except for SIGUSR1
				s->spinlock = 0; //Release lock so semaphore is accessible in sem_inc
				sigsuspend(&wait_mask); //Sleep until get SIGUSR1
			}
		}
	}
}

void sem_inc(struct sem *s)
{
	sigset_t oldmask, newmask;
	sigfillset(&newmask);
	sigprocmask(SIG_BLOCK, &newmask, &oldmask);
	if(lock_proc(&(s->spinlock)) == 1)
	{
		s->sem_count++;
		int i;
		for(i = 0; i < N_PROC; i++)
		{
			if(s->proc_status[i]==1)
			{
				pid_t wake_pid = s->proc_pid[i];
				if(kill(wake_pid, SIGUSR1)<0){perror("kill failed");};
			}
		}
		sigprocmask(SIG_BLOCK, &oldmask, NULL);
		s->spinlock=0;
	}
}

//Signal handler
void handler(int signo)
{
	if(signo == SIGUSR1)
	{
		return;
	}
}