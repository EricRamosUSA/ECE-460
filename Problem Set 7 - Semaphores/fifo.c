//fifo.c
//Eric Ramos

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/mman.h>
#include "sem.h"
#include "fifo.h"

void fifo_init(struct fifo *f)
{
	if((f->reader_sem = (struct sem *)mmap(NULL, sizeof(struct sem), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	if((f->deadlock_sem = (struct sem *)mmap(NULL, sizeof(struct sem), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	if((f->writer_sem = (struct sem *)mmap(NULL, sizeof(struct sem), PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, -1, 0))==NULL)
	{
		perror("mmap error");
	}
	sem_init(f->reader_sem, 0);
	sem_init(f->deadlock_sem, 1);
	sem_init(f->writer_sem, MYFIFO_BUFSIZ);
	f->head = -1;
	f->tail = -1;
	f->size = 0;
	f->max_fsize = MYFIFO_BUFSIZ;
}

void fifo_wr(struct fifo *f, unsigned long d)
{
	sem_wait(f->writer_sem); //Block unless things to write
	sem_wait(f->deadlock_sem); //Ensure that only one writer is writing to FIFO
	enqueue(f, d);
	sem_inc(f->deadlock_sem);
	sem_inc(f->reader_sem);
}

unsigned long fifo_rd(struct fifo *f)
{
	sem_wait(f->reader_sem); //Block until things to read
	sem_wait(f->deadlock_sem);
	unsigned long pop_val = dequeue(f);
	sem_inc(f->deadlock_sem);
	sem_inc(f->writer_sem);
	return pop_val;
}

unsigned long dequeue(struct fifo *f)
{
	unsigned long pop_val;
	f->size--;
	f->head++;
	pop_val = f->myfifo[f->head % f->max_fsize];
	return pop_val;
}

void enqueue(struct fifo *f, unsigned long push_val)
{
	f->size++;
	f->tail++;
	f->myfifo[f->tail % f->max_fsize] = push_val;
	return;
}