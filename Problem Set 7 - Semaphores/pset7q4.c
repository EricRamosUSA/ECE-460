//pset7q4.c
//Eric Ramos
//ECE460 PS7 Question 4 Part 2

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
#define N_WRITER 40
#define N_READER 1
#define lim 1000
#define CUR 1
#define PREV 0
int proc_writer_pid[N_WRITER];
int ii; //Keep track of writer pid's, status
int proc_reader_pid;
unsigned long *val_store[2];	//Keep track of all read values from procs
								//As well as last set of values

//Function Prototypes
void spawn_writer();
void spawn_reader();
void check_integrity();
void check_status(pid_t pid, int status);

//Main Function
int main(int argc, char *argv[])
{
	//Part 2
	if((test_fifo = (struct fifo *)mmap(NULL, sizeof(struct fifo), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	int i;
	for(i = 0; i < 2; i++)
	{
		if((val_store[i] = (unsigned long *)mmap(NULL, N_WRITER*sizeof(unsigned long), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
		{
			perror("mmap error");
		}
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
	unsigned long i; //increment
	unsigned long data_val; //push value
	//Write to FIFO
	for(my_procnum = N_READER; my_procnum < N_WRITER+N_READER; my_procnum++)
	{
		if((write_pid = fork())<0){perror("forking failed");exit(-1);}
		if(write_pid == 0)
		{
			for(i = 0; i < lim; i++)
			{
				data_val = i<<16;	//Store in i in datum ***use half of 32 bits for this
				data_val = data_val | (my_procnum - N_READER); //Store sequence number
				fifo_wr(test_fifo, data_val);
			}
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
	int i;
	unsigned long parse;
	unsigned long hex_mask = 0xFFFF; //Want the 16 least significant bytes for procnum
	//Read from FIFO
	for(my_procnum = 0; my_procnum < N_READER; my_procnum++)
	{
		if((read_pid = fork())<0){perror("forking failed");exit(-1);}
		if(read_pid == 0)
		{
			for(i = 0; i < lim*N_WRITER; i++)
			{
				//Popped item is int + my_procnum
				//index = i & (00....0011(12 more..)11)
				parse = fifo_rd(test_fifo);
				val_store[CUR][(parse & hex_mask)] = (parse >> 16);
				check_integrity();
			}
			exit(0);
		}
		else
		{
			printf("Created reading process %d\n", read_pid);
			proc_reader_pid = read_pid;
		}
	}
}

//Detect if values in val_store have changed by more than 1
void check_integrity()
{
	int j;
	for(j = 0; j < N_WRITER; j++)
	{
		printf("%lu ", val_store[CUR][j]);
	}
	for(j = 0; j < N_WRITER; j++)
	{
		if((val_store[CUR][j] - val_store[PREV][j]) > 1)
		{
			printf("\nData has deviated in vid %d by more than one", j);
		}
		val_store[PREV][j] = val_store[CUR][j];
	}
	printf("\n");
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