//pset7q1.c
//Eric Ramos
//ECE460 PS7 Question 1

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
extern int tas(volatile char *lock);

//Global vars
static long *addr;
int tas_mode;
int N_PROC = 64;

//Function Prototypes
void proc_spawn();
void check_status(pid_t pid, int status);
void mutex_add();

//Main Function
int main(int argc, char *argv[])
{
	if(argc!=2){printf("not enough args\n");exit(-1);}
	if(!strcmp(argv[1],"a")){tas_mode=0;printf("Running without tas protection\n");}
	else if(!strcmp(argv[1],"b")){tas_mode=1;printf("Running with tas protection\n");}
	else{printf("not valid arg\n");}
	if((addr = (long int *)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	addr[0] = 0x00; //Lock variable
	addr[1] = 0x00; //Adding variable
	printf("Adding 1 to 1e6 with 5 child proceses and parent to shared memory\n");
	printf("(Result should be 6000000)\n");
	proc_spawn();
	return 0;
}

//Spawn a bunch of processes
void proc_spawn()
{
	pid_t pid1, pid2, pid3, pid4, pid5;
	int status1, status2, status3, status4, status5;

	//Spawn 4 processes
	printf("Spawning 5 processes\n");
	//Proc 1
	if((pid1 = fork())<0){perror("forking failed for pid1");exit(-1);}

	//Proc 2
	if(pid1 > 0)
	{
		if((pid2 = fork())<0){perror("forking failed for pid2");exit(-1);}
	}

	//Proc 3
	if(pid1 > 0 && pid2 > 0)
	{
		if((pid3 = fork())<0){perror("forking failed for pid3");exit(-1);}
	}

	//Proc 4
	if(pid1 > 0 && pid2 > 0 && pid3 > 0)
	{
		if((pid4 = fork())<0){perror("forking failed for pid4");exit(-1);}
	}

	//Proc 5
	if(pid1 > 0 && pid2 > 0 && pid3 > 0 && pid4 > 0)
	{
		if((pid5 = fork())<0){perror("forking failed for pid5");exit(-1);}
	}
	
	//Adding 1 to addr millions of times
	if(!tas_mode)
	{
		long i; for(i=0; i< 1000000; i++){addr[1]++;}
	}
	else
	{
		mutex_add();
	}

	//Parent Process
	if(pid1!=0 && pid2 !=0 && pid3 !=0 && pid4 !=0 && pid5 !=0)
	{
		waitpid(pid1, &status1, 0);
		waitpid(pid2, &status2, 0);
		waitpid(pid3, &status3, 0);
		waitpid(pid4, &status4, 0);
		waitpid(pid5, &status5, 0);
		check_status(pid1, status1);
		check_status(pid2, status2);
		check_status(pid3, status3);
		check_status(pid4, status4);
		check_status(pid5, status5);
		printf("Processes finished\n");
		printf("Result is: %lu\n", addr[1]);
	}
}

void mutex_add()
{
	long i;
	for(i=0; i< 1000000; i++)
	{
		while(tas((char *)addr)!=0);
		addr[1]++;
		addr[0] = 0x00;
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