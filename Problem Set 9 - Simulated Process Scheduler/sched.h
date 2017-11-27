//sched.h
//Eric Ramos

#ifndef SCHED_H_
#define SCHED_H_

#include "jmpbuf-offsets64.h"

#define SCHED_NPROC		4096
#define SCHED_READY		0x00
#define SCHED_RUNNING	0x01
#define SCHED_SLEEPING	0x02
#define SCHED_ZOMBIE	0x03

#define max(a, b)((a > b)? a:b)
#define min(a, b)((a < b)? a:b)

struct savectx
{
	//.regs[JB_SP]
	//.regs[JB_BP]
	//.regs[JB_PC]
	void *regs[JB_SIZE];
};

struct list_node
{
	struct list_node *next, *prev;
	struct sched_proc *child;
};

struct childlist
{
	struct list_node *head, *tail;
	int num_child;
};

struct sched_proc
{
	//Pid's
	//Task state
	//Priorities/niceval
	//Accumulated time
	//CPU Time
	//Stack address
	//Context
	int pid;
	int ppid;
	int status;
	int priority;
	int static_pr;
	int dynam_pr;
	int niceval;
	int accumtime;
	void *sp;
	struct savectx *ctx;

	//CFS Related things below
	int weight;
	unsigned int vruntime;
	int prev_sum_exec_runtime;
	int sum_exec_runtime;
	unsigned int timeslice;

	//For wait
	struct childlist child_list;
	int num_child;
	struct sched_waitq_head *my_wait;
	int code;
};

struct node
{
	struct node *prev;
	struct node *next;
	struct sched_waitq *container;
};

struct sched_waitq_head
{
	//Sentinel node
	char spinlock;
	struct node *task_list;
};

struct sched_waitq
{
	//For event/wakeup/runqueue queue
	struct sched_proc *task;
	struct node *task_list;
};

void sched_waitq_init(struct sched_waitq_head *q_head);

void sched_init(int (*init_fn)());

int sched_fork();

void sched_exit(int code);

int sched_wait(int *code);

void sched_sleep(struct sched_waitq_head *waitq);

void sched_wakeup(struct sched_waitq_head *waitq);

void sched_nice(int niceval);

int sched_getpid();

int sched_getppid();

int sched_gettick();

void sched_switch();

#endif // SCHED_H_