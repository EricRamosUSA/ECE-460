//pset7q3.c
//Eric Ramos
//ECE460 PS7 Question 4 Part 1

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/mman.h>
#include "sem.h"
#include "fifo.h"

//Global vars
int my_procnum;
struct fifo *test_fifo;
#define N_WRITER 1
#define N_READER 1
#define lim 10000
int proc_writer_pid[N_WRITER];
int ii; //Keep track of writer pid's, status
int proc_reader_pid;

//Function Prototypes
void spawn_writer();
void spawn_reader();
void check_status(pid_t pid, int status);

//Main Function
int main(int argc, char *argv[])
{
	//Part 1
	if((test_fifo = (struct fifo *)mmap(NULL, sizeof(struct fifo), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	fifo_init(test_fifo);
	spawn_reader();
	spawn_writer();

	int status;
	for(ii = 0; ii < N_WRITER; ii++)
	{
		waitpid(proc_writer_pid[ii], &status, 0);
		printf("Writer ");
		check_status(proc_writer_pid[ii], status);
	}
	waitpid(proc_reader_pid, &status, 0);
	printf("Reader ");
	check_status(proc_reader_pid, status);
	return 0;
}

void spawn_writer()
{
	pid_t write_pid;
	unsigned long j; //datum
	//Write to FIFO
	for(my_procnum = N_READER; my_procnum < N_WRITER+N_READER; my_procnum++)
	{
		if((write_pid = fork())<0){perror("forking failed");exit(-1);}
		if(write_pid == 0)
		{
			for(j = 0; j < lim; j++)
			{
				fifo_wr(test_fifo, j);
			}
			printf("Pid %d wrote %lu numbers to FIFO\n", getpid(), j);
			exit(0);
		}
		else
		{
			printf("Created writing process %d\n", (int)write_pid);
			proc_writer_pid[ii] = write_pid;
			ii++;
		}
	}
}

void spawn_reader()
{
	pid_t read_pid;
	unsigned long i;
	//Read from FIFO
	for(my_procnum = 0; my_procnum < N_READER; my_procnum++)
	{
		if((read_pid = fork())<0){perror("forking failed");exit(-1);}
		unsigned long store_val[lim*N_WRITER];
		if(read_pid == 0)
		{
			for(i = 0; i < lim*N_WRITER; i++)
			{
				store_val[i] = fifo_rd(test_fifo);
				printf("The #%lu popped val is %lu\n", i+1, store_val[i]);
			}
			printf("Pid %d read %lu numbers from FIFO\n", getpid(), i);
			exit(0);
		}
		else
		{
			printf("Created reading process %d\n", read_pid);
			proc_reader_pid = read_pid;
		}
	}
}

void check_status(pid_t pid, int status)
{
	if(status!=0)
	{
		if(WIFSIGNALED(status))
		{
			fprintf(stderr, "Process %d exited with signal %d\n", (int)pid, WTERMSIG(status));
		}
		else
		{
			fprintf(stderr, "Process %d exited with return value %d\n", (int)pid, WEXITSTATUS(status));
		}
	}
	else
	{
		printf("Process %d exited with no errors\n", (int)pid);
	}
}