//sched.c
//Eric Ramos

//LIST OF QUEUES AND ARRAYS
//[wait queue] - sched_wait, SIGUSR1, SIGUSR2
//[run queue]
//[global process array]
//Individual child lists

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <signal.h>
#include <sys/time.h>
#include "sched.h"
#include <string.h>
#include <math.h>

#define STACK_SIZE 65536
//Store the current process
struct sched_proc *current;


//For task switching
long long unsigned tick = 0;
char NEED_RESCHED = 0; //"TIF_NEED_RESCHED" flag
int load_weight;
int sched_latency = 20;
int NR = 0; //Runnable task no.

//Keep track of all pid's ever created
int global_pid_array_val = 0; //Last PID created
struct sched_proc *proc_array[SCHED_NPROC];

//Queues
struct sched_waitq_head *r_head; //runqueue head node
struct sched_waitq_head *parent_wait; //parent waitqueue head node
extern struct sched_waitq_head *wq1, *wq2; //SIGUSR wq's

//Context switching
extern int savectx(struct savectx *ctx); //"setjmp"
extern void restorectx(struct savectx *ctx, int retval); //"longjmp"

//Misc Functions
void sched_tick(int signo);
extern void abrt_handler(int sig);
extern void wakeup_handler(int sig);
void sched_ps();
void child_fn();
void idle_fn();
void enqueue(struct sched_waitq_head *q_head, struct sched_proc *task_ptr);
struct sched_proc *dequeue(struct sched_waitq_head *q_head, char NR);
void wait_enqueue(struct sched_waitq_head *q_head, struct sched_proc *task_ptr);
void wait_dequeue(struct sched_waitq_head *q_head, struct sched_proc *proc);
struct sched_proc *create_new_pid();
void sched_updatepriority(struct sched_proc *proc);
void sched_printqueue(struct sched_waitq_head *q_head);
void sched_printvruntime(struct sched_waitq_head *q_head);
void sched_mask(sigset_t *mask);
void sched_unmask(sigset_t *mask);
int sched_checktime();
void sched_weight(struct sched_proc *proc);
void sched_vruntime(struct sched_proc *proc);
void sched_timeslice(struct sched_proc *proc);
void sched_adoption();
void sched_free();
void push_child(struct childlist *cl, struct sched_proc *c);
struct list_node *remove_child(struct childlist *cl, int child_pid);
struct list_node *find_child(struct childlist *cl, int child_pid);

//Initialize the queue
void sched_waitq_init(struct sched_waitq_head *q_head)
{
	//q_head needs 2 mallocs: for itself and the struct node
	//sched_waitq_head malloc is in init
	q_head->task_list = (struct node *)malloc(sizeof(struct node));
	q_head->task_list->prev = q_head->task_list;
	q_head->task_list->next = q_head->task_list;
	q_head->task_list->container = NULL;
}

//Push process to the head of runqueue
void enqueue(struct sched_waitq_head *q_head, struct sched_proc *task_ptr)
{
	NR++; //Update no. runnable
	struct sched_waitq *new_proc = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
	//Linking Nodes
	new_proc->task_list = (struct node *)malloc(sizeof(struct node));
	if(q_head->task_list->next == q_head->task_list)
	{
		new_proc->task_list->next = q_head->task_list->next;
		new_proc->task_list->prev = q_head->task_list;
		q_head->task_list->next->prev = new_proc->task_list;
		q_head->task_list->next = new_proc->task_list;
	}
	else
	{
		//Keep track of current process in queue
		struct sched_waitq *temp = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
		temp = q_head->task_list->next->container;
		while(1)
		{
			//Check if the node priority <= new proc's priority
			//Originally this was >=, before the low vruntime criterion
			if(temp->task->dynam_pr <= task_ptr->dynam_pr)
			{
				//If the prev node was the head
				if(temp->task_list->prev == q_head->task_list)
				{
					new_proc->task_list->next = q_head->task_list->next;
					new_proc->task_list->prev = q_head->task_list;
					q_head->task_list->next->prev = new_proc->task_list;
					q_head->task_list->next = new_proc->task_list;
					break;
				}
				else
				{
					temp = temp->task_list->prev->container; //Get the last process container
					new_proc->task_list->next = temp->task_list->next;
					new_proc->task_list->prev = temp->task_list;
					temp->task_list->next->prev = new_proc->task_list;
					temp->task_list->next = new_proc->task_list;
					break;
				}
			}
			//If the next node is the head
			if(temp->task_list->next == r_head->task_list)
			{
				new_proc->task_list->next = temp->task_list->next;
				new_proc->task_list->prev = temp->task_list;
				temp->task_list->next->prev = new_proc->task_list;
				temp->task_list->next = new_proc->task_list;
				break;
			}
			//Set the next node for comparison
			temp = temp->task_list->next->container;
		}
	}
	//Make sure the node references the container, set the task to supplied task
	new_proc->task_list->container = new_proc;
	new_proc->task = task_ptr;
}

//Pop from the runqueue tail
struct sched_proc *dequeue(struct sched_waitq_head *q_head, char NR)
{
	if(NR == 1) //This is so that dequeue can be used for non-runqueues
	{
		NR--; //Took off one task
	}
	struct sched_proc *proc_out = (struct sched_proc *)malloc(sizeof(struct sched_proc));
	//Unlink Nodes
	proc_out = q_head->task_list->prev->container->task;
	q_head->task_list->prev->prev->next = q_head->task_list;
	q_head->task_list->prev = q_head->task_list->prev->prev;
	return proc_out;
}

//Push to the waitq head
void wait_enqueue(struct sched_waitq_head *q_head, struct sched_proc *task_ptr)
{
	struct sched_waitq *new_proc = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
	//Linking Nodes
	new_proc->task_list = (struct node *)malloc(sizeof(struct node));
	new_proc->task_list->next = q_head->task_list->next;
	new_proc->task_list->prev = q_head->task_list;
	q_head->task_list->next->prev = new_proc->task_list;
	q_head->task_list->next = new_proc->task_list;
	//Container
	new_proc->task_list->container = new_proc;
	new_proc->task = task_ptr;
}

//Pop the supplied task from the waitq
void wait_dequeue(struct sched_waitq_head *q_head, struct sched_proc *proc)
{
	//Unlink Nodes
	struct sched_waitq *temp = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
	if(q_head->task_list->next == q_head->task_list)
	{
		fprintf(stderr, "waitq is empty\n");
		return;
	}
	temp = q_head->task_list->next->container;
	while(1)
	{
		if(temp->task == proc)
		{
			temp->task_list->next->prev = temp->task_list->prev;
			temp->task_list->prev->next = temp->task_list->next;
			proc->my_wait = NULL;
			return;
		}
		temp = q_head->task_list->next->container;
	}
}

//Push onto the child list
void push_child(struct childlist *cl, struct sched_proc *c)
{
	struct list_node *node = malloc(sizeof(struct list_node));
	node->child = c;
	node->next = node->prev = NULL;

	if(cl->num_child == 0)	//push into empty list
		cl->head = cl->tail = node;
	else
	{
		cl->tail->next = node;
		node->prev = cl->tail;
		cl->tail = node;
	}
	cl->num_child += 1;
}

//Find a child
struct list_node *find_child(struct childlist *cl, int child_pid)
{
	if(cl->num_child == 0)
	{
		fprintf(stderr, "child_list is empty.\n");
		return NULL;
	}	
	struct list_node *lwalker;
	for(lwalker = cl->head; lwalker != NULL; lwalker = lwalker->next)
	{
		if(lwalker->child->pid == child_pid) //found the child
			return lwalker;
	}
	fprintf(stderr, "child not found\n");
	return NULL;
}

//Pop from child list
struct list_node *remove_child(struct childlist *cl, int child_pid)
{
	if(cl->num_child == 0)
	{
		fprintf(stderr, "Error removing child: child_list is empty.\n");
		return NULL;
	}
	struct list_node *pop_node;
	if((pop_node = find_child(cl, child_pid)) != NULL)
	{
		if(cl->num_child == 1)  //only 1 node 
		{
			cl->head = cl->tail = NULL;
			cl->num_child -= 1;
			return pop_node;
		}	
		if(pop_node == cl->head)
		{
			cl->head = pop_node->next;
			cl->head->prev = NULL;
			cl->num_child -= 1;
			return pop_node;
		}
		else if(pop_node == cl->tail)
		{
			cl->tail = pop_node->prev;
			cl->tail->next = NULL;
			cl->num_child -= 1;
			return pop_node;
		}
		else
		{
			pop_node->prev->next = pop_node->next;
			pop_node->next->prev = pop_node->prev;
			cl->num_child -= 1;
			return pop_node;
		}
	}
}

//Create a new sched_proc with a new PID value
struct sched_proc *create_new_pid()
{
	struct sched_proc *task = (struct sched_proc *)malloc(sizeof(struct sched_proc));
	global_pid_array_val++;
	task->pid = global_pid_array_val;
	task->ppid = 0;
	task->status = SCHED_READY;
	task->niceval = 0;
	task->static_pr = task->niceval + 20; //Want 0<=prio<=39
	task->accumtime = 0;
	task->dynam_pr = task->static_pr;
	if((task->sp=(void *)mmap(0, STACK_SIZE, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0))==MAP_FAILED)
	{
		perror("mmap failed");
		exit(1);
	}
	task->ctx = (struct savectx *)malloc(sizeof(struct savectx));
	task->prev_sum_exec_runtime = 0;
	proc_array[global_pid_array_val-1] = task;
	task->num_child = 0;
	return task;
}

void sched_init(int (*init_fn) ()) //Changed from void
{
	/*
	Called once by test program
	Initialize scheduling system
	Set up periodic interval timer
	Establish sched_tick as the sig handler
	Create initial task with a pid of 1
	Make pid 1 runnable and transfer execution to it
	*/

	//Set up waitq's
	r_head = (struct sched_waitq_head *)malloc(sizeof(struct sched_waitq_head));
	parent_wait = (struct sched_waitq_head *)malloc(sizeof(struct sched_waitq_head));
	wq1 = (struct sched_waitq_head *)malloc(sizeof(struct sched_waitq_head));
	wq2 = (struct sched_waitq_head *)malloc(sizeof(struct sched_waitq_head));

	//Set up interval timer, signal handler
	struct sigaction sa1;
	sigemptyset(&sa1.sa_mask);
	sa1.sa_flags = 0;
	sa1.sa_handler = sched_tick;
	if(sigaction(SIGVTALRM, &sa1, NULL) < 0)
	{
		perror("Error with sigaction\n");
		exit(1);
	}
	struct itimerval timer;
	timer.it_value.tv_sec = 0;
	timer.it_value.tv_usec = 250000; //250 milliseconds
	timer.it_interval = timer.it_value;
	if(setitimer(ITIMER_VIRTUAL, &timer, NULL) < 0)
	{
		perror("Error setting interval timer");
		exit(1);
	}
	fprintf(stderr, "Set SIGVTALRM handler\n");

	//Set up SIGABRT handler
	struct sigaction sa2;
	sigemptyset(&sa2.sa_mask);
	sa2.sa_flags = 0;
	sa2.sa_handler = abrt_handler;
	if(sigaction(SIGABRT, &sa2, NULL) < 0)
	{
		perror("Error with sigaction\n");
		exit(1);
	}

	//Create runqueue and waitqueue
	sched_waitq_init(r_head);
	sched_waitq_init(parent_wait);

	//Create first process
	current = create_new_pid();
	current->status = SCHED_RUNNING;
	current->prev_sum_exec_runtime = sched_gettick();

	current->child_list.num_child = 0;
	current->child_list.head = current->child_list.tail = NULL;

	//Initial timeslice
	load_weight = 0; //Set up load weight
	NR++; //Increase the number of runnable tasks
	sched_weight(current);
	load_weight = current->weight;
	sched_timeslice(current);
	//sched_nice(-5);

	//Set up context
	savectx(current->ctx);
	//Need to set (...)->regs[JB_SP] = newsp + STACK_SIZE;
	current->ctx->regs[JB_BP] = current->sp + STACK_SIZE;
	current->ctx->regs[JB_SP] = current->sp + STACK_SIZE;
	current->ctx->regs[JB_PC] = init_fn;
	restorectx(current->ctx, 0);
}

int sched_fork()
{
	/*
	Create a new simulated task which is a copy of the caller
	Make new pid runnable and eligible to be scheduled
	Returns 0 to the child and child's pid to parent
	Do not need to duplicate entire address
	*/
	if(global_pid_array_val == SCHED_NPROC)
	{
		fprintf(stderr, "Cannot create another process\n");
		sched_switch();
	}
	sigset_t mask;
	sched_mask(&mask);
	struct sched_proc *child_proc = create_new_pid();
	fprintf(stderr, "Created new child process\n");
	child_proc->ppid = current->pid;
	memcpy(child_proc->sp, current->sp, STACK_SIZE);
	void *lim0 = child_proc->sp;				//Pointer to stack
	void *lim1 = child_proc->sp + STACK_SIZE;	//Pointer to top of stack
	child_proc->ctx->regs[JB_BP] = lim1;
	child_proc->ctx->regs[JB_SP] = lim1;
	child_proc->ctx->regs[JB_PC] = child_fn;
	unsigned long adj = lim0 - current->sp; //STACK_SIZE;
	
	//Timeslicing
	sched_weight(child_proc);
	sched_timeslice(child_proc);
	child_proc->vruntime = current->vruntime; //Child inherits parent's vruntime
	
	//Modifying parent children list
	push_child(&current->child_list, child_proc);

	//Set up ctx
	adjstack(lim0, lim1, adj);
	if(savectx(child_proc->ctx)==0)
	{
		child_proc->ctx->regs[JB_BP] += adj;
		child_proc->ctx->regs[JB_SP] += adj;
		load_weight += child_proc->weight;
		enqueue(r_head, child_proc);
		sched_unmask(&mask);
		return child_proc->pid;
	}
	else
	{
		sched_unmask(&mask);
		return 0;
	}
}

void sched_exit(int code)
{
	/*
	Terminate current task, making it zombie, store exit code
	If parent is sleeping, wake it up and return exit code to it
	No SIGCHILD, sched_exit will not return
	Another runnable process will be scheduled
	*/
	sigset_t mask;
	sched_mask(&mask);
	if(current->pid == 1)
	{
		printf("Cannot kill init\n");
	}
	else
	{
		//Terminate Child
		current->status = SCHED_ZOMBIE;
		current->code = code;
		load_weight -= current->weight;
		//Send exit code to parent
		if((proc_array[(current->ppid)-1])->status == SCHED_SLEEPING &&
			(proc_array[(current->ppid)-1])->my_wait == parent_wait)
		{
				wait_dequeue((proc_array[(current->ppid)-1])->my_wait, (proc_array[(current->ppid)-1]));
				(proc_array[(current->ppid)-1])->status = SCHED_READY;
				load_weight += (proc_array[(current->ppid)-1])->weight;
				(proc_array[(current->ppid)-1])->my_wait = NULL;
				enqueue(r_head, (proc_array[(current->ppid)-1]));
		}
		sched_unmask(&mask);
		sched_switch();
	}
}

int sched_wait(int *code)
{
	/*
	Return exit code of zombie child and free resources to child
	If multiple zombies, order is not defined
	If no zombies, place caller in SLEEPING, wake up when child
	calls sched_exit()
	If no children, return -1
	Exit code is integer from sched_exit
	*/
	sigset_t mask;
	sched_mask(&mask);
	savectx(current->ctx);
	int child_pid;
	if(current->child_list.num_child == 0)
	{
		//printf("Current process has no children to sched_wait for\n");
		sched_unmask(&mask);
		return -1;
	}
	
	struct list_node *i;
	struct list_node *removed;
	for(i = current->child_list.head; i != NULL; i = i->next)
	{
		if(i->child->status == SCHED_ZOMBIE)
		{
			*code = i->child->code;
			child_pid = i->child->pid;
			removed = remove_child(&current->child_list, child_pid);
			//Free resources
			sched_free(removed->child);
			sched_unmask(&mask);
			return child_pid;
		}
	}
	sched_sleep(parent_wait);
	load_weight -= current->weight;
	sched_unmask(&mask);
	sched_switch();
}

//Place the current task on the specified waitq
void sched_sleep(struct sched_waitq_head *waitq)
{
	sigset_t mask;
	sched_mask(&mask);

	//Put task to sleep, schedule another process
	current->status = SCHED_SLEEPING;
	current->my_wait = waitq;
	wait_enqueue(waitq, current);
	//sched_printqueue(parent_wait);
	sched_unmask(&mask);
	if(waitq == parent_wait){return;}
	if(savectx(current->ctx) == 0)
	{
		sched_switch();
	}
}

void sched_wakeup(struct sched_waitq_head *waitq)
{
	//Wake up all tasks in the waitq
	
	struct node *temp;
	sigset_t mask;
	sched_mask(&mask);
	temp = waitq->task_list;
	int make_current_ready = 0;
	
	while(temp->prev != waitq->task_list)
	{
		temp = waitq->task_list->prev;
		sched_vruntime(temp->container->task);
		sched_updatepriority(temp->container->task);
		temp->container->task->status = SCHED_READY;
		temp->container->task->my_wait = NULL;
		load_weight += temp->container->task->weight;
		if(temp->container->task->dynam_pr < current->dynam_pr &&
			current->status == SCHED_RUNNING)
		{
			make_current_ready = 1;
		}
		//Push to the runqueue
		enqueue(r_head, temp->container->task);
		dequeue(waitq, 0);
	}
	
	if(make_current_ready == 1)
	{
		make_current_ready = 0;
		current->status = SCHED_READY;
		enqueue(r_head, current);
		sched_unmask(&mask);
		sched_switch();
	}
	sched_unmask(&mask);
}

void sched_nice(int niceval)
{
	/*
	Set nice value to supplied parameter
	Nice vals vary from 19 (least preferred
	static priority) to -20 (most preferred)
	Out of range values set to min/max
	*/
	sigset_t mask;
	sched_mask(&mask);
	load_weight -= current->weight;
	if(niceval <= 19 && niceval >= -20)
	{
		current->niceval = niceval;
		current->static_pr = current->niceval + 20;
	}
	else if(niceval < -20)
	{
		current->niceval = -20;
		current->static_pr = current->niceval + 20;
	}
	else if(niceval > 19)
	{
		current->niceval = 19;
		current->static_pr = current->niceval + 20;
	}
	sched_weight(current);
	load_weight += current->weight;
	sched_unmask(&mask);
}

int sched_getpid()
{
	//Get current task's pid
	return current->pid;
}

int sched_getppid()
{
	//Return parent pid of current task
	return current->ppid;
}

int sched_gettick()
{
	//Return the number of timer ticks since startup
	return tick;
}

void sched_ps()
{
	/*
	Output to stout listing of all current tasks
	Include: pid, ppid, current state, base addr of private
	stack area, static priority, dynamic priority, total
	CPU time used (in ticks)
	Establish sched_ps() as the signal handler for SIGABRT
	*/
	sigset_t mask;
	sched_mask(&mask);
	fprintf(stderr, "Recieved SIGABRT\n");
	fprintf(stderr, "Starting process dump\n");
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "PID \tPPID\tSTATUS\t    STACKPTR\t  STAT_PRIO\tDYN_PRIO    CPUTIME\t\tWAITQ\n");
	int i;
	for(i = 0; i < global_pid_array_val; i++)
	{
		struct sched_proc *proc = proc_array[i];
		if(proc == NULL) //Check if proc has been erased by sched_wait/sched_free
		{
			continue;
		}
		fprintf(stderr, "%4d\t", proc->pid);
		fprintf(stderr, "%4d\t", proc->ppid);
		fprintf(stderr, "%6x\t  ", proc->status);
		fprintf(stderr, "%13p", proc->sp);
		fprintf(stderr, "%11d\t", proc->static_pr);
		fprintf(stderr, "%8d\t", proc->dynam_pr);
		fprintf(stderr, "%3d\t", proc->prev_sum_exec_runtime);
		if(proc->status == SCHED_SLEEPING)
		{
			fprintf(stderr, "%13p", proc->my_wait);
		}
		fprintf(stderr, "\n");
	}
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "End of process list\n");
	sched_unmask(&mask);
}

void sched_switch()
{
	/*
	Analogous to schedule()
	Place the current task on the run queue (Assuming READY)
	Select best ready task based on priority
	Selected task should be RUNNING, and do context switch
	*/
	//fprintf(stderr, "This is a context switch\n");
	sigset_t mask;
	sched_mask(&mask);
	//sched_printqueue(r_head);
	if(r_head->task_list->next == r_head->task_list)
	{
		fprintf(stderr, "RQ is empty, idle task until signal recieved\n");
		sigset_t susp_mask;
		sigfillset(&susp_mask);
		sigdelset(&susp_mask, SIGUSR1);
		sigdelset(&susp_mask, SIGUSR2);
		sigdelset(&susp_mask, SIGINT); //End with cntl-C
		sigsuspend(&susp_mask);
		fprintf(stderr, "Exiting sigsuspend\n");
	}
	if(NEED_RESCHED == 1)
	{
		NEED_RESCHED = 0;
	}
	//Remove the next_proc, make running
	current = dequeue(r_head, 1);
	current->status = SCHED_RUNNING;
	//Update the time the proc got the CPU
	current->prev_sum_exec_runtime = sched_gettick();
	//fprintf(stderr, "The new task pid is %d\n", sched_getpid());
	sched_unmask(&mask);
	restorectx(current->ctx, 1);
}

void sched_tick(int signo)
{
	/*
	Signal handler for SIGVTALRM generated by periodic timer
	Number of ticks since sched_init should be returned
	Examine currently running task and see if a time slice
	has expired, mark the task as READY, put it on the queue
	base on dynamic priority, and call sched_switch().
	NOTE: SIGVTALRM is masked by default.
	*/
	//fprintf(stderr, "Process has entered tick handler\n");
	sigset_t mask;
	sched_mask(&mask);
	tick++;
	//fprintf(stderr, "%llu tick(s) have elapsed\n", tick);
	current->accumtime++;
	sched_vruntime(current);
	sched_updatepriority(current);
	if(sched_checktime())
	{
		//Go to switch
		sched_timeslice(current);
		sched_switch();
	}
	sched_unmask(&mask);
}

//Child Function
void child_fn()
{
	fprintf(stderr, "I am a child\n");
	while(1);
}

//For debugging
void sched_printqueue(struct sched_waitq_head *q_head)
{
	//Print the queue
	struct sched_waitq *temp = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
	struct sched_proc *proc = (struct sched_proc *)malloc(sizeof(struct sched_proc));
	temp = q_head->task_list->next->container;
	fprintf(stderr, "PRINTING OUT ELEMENTS OF QUEUE\n");
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "PID   PPID   STATUS   STAT_PRIO   DYN_PRIO   NICEVAL   ACCUMTIME   CPUTIME\n");
	while(temp!=NULL)
	{
		proc = temp->task;
		fprintf(stderr, "%d", proc->pid);
		fprintf(stderr, "%*d", 7, proc->ppid);
		fprintf(stderr, "%*x", 7, proc->status);
		fprintf(stderr, "%*d", 7, proc->static_pr);
		fprintf(stderr, "%*d", 7, proc->dynam_pr);
		fprintf(stderr, "%*d", 7, proc->niceval);
		fprintf(stderr, "%*d", 7, proc->accumtime);
		fprintf(stderr, "%*d\n", 7, proc->prev_sum_exec_runtime);
		temp = temp->task_list->next->container;
	}
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "End of process list\n");
}

//Check if a time slice has elapsed
int sched_checktime()
{
	int delta_exec = tick - current->prev_sum_exec_runtime;
	if(delta_exec >= current->timeslice)
	{
		if(savectx(current->ctx)==0) //Save the current process
		{
			if(current->status == SCHED_RUNNING)
			{
				current->status = SCHED_READY;
				enqueue(r_head, current);
			}
		}
		NEED_RESCHED = 1;
	}
	else
	{
		NEED_RESCHED = 0;
	}
	return NEED_RESCHED;
}

//Update the priority of the proc
void sched_updatepriority(struct sched_proc *proc)
{
	//The commented out portion was a previous implementation of
	//dynamic priority that never worked properly. This was based
	//on the formula presented in chapter 10 of the notes.
	/*
	int bonus = 0; //Based on how long proc was on the runqueue
	int tick_siz = 10; //Partition the bonus by 10
	float slope = 20/tick_siz; //Height is numerator
	fprintf(stderr, "slope %f\n", slope);
	int time_diff = tick - proc->prev_sum_exec_runtime;
	if(slope*time_diff > 10)
	{
		bonus = 10;
	}
	else
	{
		bonus = slope*time_diff;
	}
	proc->dynam_pr = ceil(max(0, min(proc->static_pr - bonus + 5, 45)));
	*/
	proc->dynam_pr = current->vruntime;
}

void sched_mask(sigset_t *mask)
{
	//Masks the signals
	sigemptyset(mask);
	sigaddset(mask, SIGVTALRM);
	sigaddset(mask, SIGABRT);
	sigaddset(mask, SIGUSR1);
	sigaddset(mask, SIGUSR2);
	sigprocmask(SIG_BLOCK, mask, 0);
}

void sched_unmask(sigset_t *mask)
{
	//Unmask all signals
	sigprocmask(SIG_UNBLOCK, mask, 0);
}

void sched_weight(struct sched_proc *proc)
{
	//Calculate the weight for proc
	proc->weight = 1024*pow(1.24, -1*(proc->niceval));
}

void sched_vruntime(struct sched_proc *proc)
{
	//Calculate vruntime
	int T; //Time difference since load was last examined
	T = tick - proc->prev_sum_exec_runtime;
	double relative_weight;
	relative_weight = (double)(256*proc->weight/load_weight);
	proc->vruntime += ceil((double)T/relative_weight/256);
}

void sched_timeslice(struct sched_proc *proc)
{
	//Calculate the timeslice
	double P = (double)(sched_latency/NR);
	double relative_weight = (double)(256*proc->weight/load_weight);
	int Sn = ceil(P*relative_weight/256);
	proc->timeslice = Sn;
}

void sched_adoption(struct sched_proc *proc)
{
	//Give all soon-to-be orphaned procs to the init process
	struct childlist *cl = &proc->child_list;
	if(cl->num_child == 0)
	{
		fprintf(stderr, "child_list is empty.\n");
		return;
	}
	struct list_node *lwalker;
	for(lwalker = cl->head; lwalker != NULL; lwalker = lwalker->next)
	{
		lwalker->child->ppid = current->pid;
		push_child(&current->child_list, lwalker->child);
	}
}

void sched_free(struct sched_proc *proc)
{
	//Free the resources of a given proc
	sched_adoption(proc);
	if(munmap(proc->sp, STACK_SIZE)<0)
	{
		perror("munmap failed");
		exit(1);
	}
	proc_array[proc->pid -1] = NULL;
	free(proc->ctx);
	free(proc);
}

//Print the vruntime for error checking
void sched_printvruntime(struct sched_waitq_head *q_head)
{
	sigset_t mask;
	sched_mask(&mask);
	struct sched_waitq *temp = (struct sched_waitq *)malloc(sizeof(struct sched_waitq));
	struct sched_proc *proc = (struct sched_proc *)malloc(sizeof(struct sched_proc));
	temp = q_head->task_list->next->container;
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "PID \tVRUNTIME\n");
	while(temp!=NULL)
	{
		proc = temp->task;
		fprintf(stderr, "%4d\t", proc->pid);
		fprintf(stderr, "%4d\t\n", proc->vruntime);
		temp = temp->task_list->next->container;
	}
	fprintf(stderr, "-------------------------------------\n");
	fprintf(stderr, "End of process list\n");
}