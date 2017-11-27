//sem.h
//Eric Ramos

#ifndef SEM_H_
#define SEM_H_

#define N_PROC 64

extern int tas(volatile char *lock);

struct sem
{
	int sem_count;
	char spinlock;
	int proc_pid[N_PROC];
	int proc_vid[N_PROC];
	int proc_status[N_PROC];
};

void sem_init(struct sem *s, int count); //Initialize sem s

int lock_proc(volatile char *lock); //Lock process

int sem_try(struct sem *s); //Attempt to perform P

void sem_wait(struct sem *s); //Perform P

void sem_inc(struct sem *s); //Perform V

#endif // SEM_H_