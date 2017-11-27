//pset7q2.c
//Eric Ramos
//ECE460 PS7 Question 2

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/mman.h>
#include <signal.h>
#include "sem.h"

//Global vars
static long int *addr;
static struct sem *s;
pid_t pid;
int my_procnum;
int procnum_status[N_PROC];

//Function Prototypes
void proc_spawn();
void semaphore_adder();

//Main Function
int main(int argc, char *argv[])
{
	if((s = (struct sem *)mmap(NULL, sizeof(struct sem), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	my_procnum = 0;
	sem_init(s, 1);
	if((addr = (long *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	addr[0] = 0x00;
	proc_spawn();
	printf("Processes finished\n");
	printf("Result is: %lu\n", addr[0]);
	return 0;
}

void proc_spawn()
{
	pid_t pid;
	int status;
	int i;
	for(my_procnum = 0; my_procnum < 15; my_procnum++)
	{
		if((pid = fork())<0){perror("forking failed");exit(-1);}
		if(pid == 0)
		{
			semaphore_adder();
			exit(0);
		}
	}
	while(1)
	{
		wait(&status); //Waiting for all children
		if(errno == ECHILD){break;}
	}
}

void semaphore_adder()
{
	int i;
	for(i = 0; i < 1000000; i++)
	{
		sem_wait(s);
		addr[0]++;
		sem_inc(s);
	}
	printf("The process pid %d/vid %d has finished\n", s->proc_pid[my_procnum], my_procnum);
}