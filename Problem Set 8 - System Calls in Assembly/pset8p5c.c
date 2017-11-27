//pset8 Problem 5c
//Eric Ramos

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>

#define lim 60000000

int main(int argc, char *argv[])
{
	int i;
	struct timespec begin, end;
	printf("About to loop getuid() syscall with %d iterations\n", lim);
	clock_gettime(CLOCK_REALTIME, &begin);
	for(i = 0; i < lim; i++)
	{
		getuid();
	}
	clock_gettime(CLOCK_REALTIME, &end);
	long nstart_time = begin.tv_sec*1000000000 + begin.tv_nsec;
	long nend_time = end.tv_sec*1000000000 + end.tv_nsec;
	printf("Total elapsed time in nanoseconds: %ld\n", nend_time - nstart_time);
	printf("Average time in nanoseconds to complete one empty function call: %ld\n", (nend_time - nstart_time)/lim);
	return 0;
}