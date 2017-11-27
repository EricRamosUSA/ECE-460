//pset8 Problem 5
//Eric Ramos

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define lim 60000000
#define sec2ns 1000000000

//Empty function
void empty_func();

int main(int argc, char *argv[])
{
	//Looping
	int i;
	struct timespec begin, end;
	printf("About to loop with %d iterations\n", lim);
	clock_gettime(CLOCK_MONOTONIC, &begin);
	for(i = 0; i < lim; i++)
	{
		//Do nothing
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	unsigned long long nstart_time = begin.tv_sec*sec2ns + begin.tv_nsec;
	unsigned long long nend_time = end.tv_sec*sec2ns + end.tv_nsec;
	printf("Total elapsed time in nanoseconds: %llu\n", nend_time - nstart_time);
	long double looptime = (long double)(nend_time - nstart_time)/lim;
	printf("Average time in nanoseconds to complete one loop iteration: %.2Lf\n", looptime);

	//Empty function
	printf("About to loop empty function with %d iterations\n", lim);
	clock_gettime(CLOCK_MONOTONIC, &begin);
	for(i = 0; i < lim; i++)
	{
		empty_func();
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	nstart_time = begin.tv_sec*sec2ns + begin.tv_nsec;
	nend_time = end.tv_sec*sec2ns + end.tv_nsec;
	printf("Total elapsed time in nanoseconds: %llu\n", nend_time - nstart_time);
	long double functime = (long double)(nend_time - nstart_time)/lim;
	functime = fabs(functime - looptime); //Subtract the time to complete one loop
	printf("Average time in nanoseconds to complete one empty function call: %.2Lf\n", functime);

	//System call
	printf("About to loop getuid() syscall with %d iterations\n", lim);
	clock_gettime(CLOCK_MONOTONIC, &begin);
	for(i = 0; i < lim; i++)
	{
		getuid();
	}
	clock_gettime(CLOCK_MONOTONIC, &end);
	nstart_time = begin.tv_sec*sec2ns + begin.tv_nsec;
	nend_time = end.tv_sec*sec2ns + end.tv_nsec;
	printf("Total elapsed time in nanoseconds: %llu\n", nend_time - nstart_time);
	long double systime = (long double)(nend_time - nstart_time)/lim - looptime;
	printf("Average time in nanoseconds to complete one empty function call: %.2Lf\n", systime);
	return 0;
}

void empty_func(){}