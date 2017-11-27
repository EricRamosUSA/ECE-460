//fifo.h
//Eric Ramos

#ifndef FIFO_H_
#define FIFO_H_

#define MYFIFO_BUFSIZ 4096

struct fifo
{
	struct sem *reader_sem;
	struct sem *deadlock_sem;
	struct sem *writer_sem;
	int head;							 //Head index
	int tail;							 //Tail index
	int size;							 //Current size
	int max_fsize;						 //Max size
	unsigned long myfifo[MYFIFO_BUFSIZ]; //The fifo
};

void fifo_init(struct fifo *f);

void fifo_wr(struct fifo *f, unsigned long d);

unsigned long fifo_rd(struct fifo *f);

//FIFO dequeue, enqueue

unsigned long dequeue(struct fifo *f);

void enqueue(struct fifo *f, unsigned long push_val);

#endif // FIFO_H_