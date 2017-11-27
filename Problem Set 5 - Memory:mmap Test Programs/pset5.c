//pset5.c
//Eric Ramos
//ECE460 PS5

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/mman.h>

//Function Prototypes
int file_open(char *fname);
int read_file(int fd_in, char *strcmpbuf, int startprint, int buf_size);
int file_close(int fd);
int execute_command();
void handler(int signo);
void pset5a();
void pset5b();
void pset5c();
void pset5d();
void pset5e();
void pset5f();

//Specify letter
int mode = 0;

int main(int argc, char *argv[])
{
	if((argc==1)||(argc>2))
	{
		printf("Invalid number of args\n");
		exit(-1);
	}
	if(!strcmp(argv[1],"a")){mode = 1;}
	else if(!strcmp(argv[1],"b")){mode = 2;}
	else if(!strcmp(argv[1],"c")){mode = 3;}
	else if(!strcmp(argv[1],"d")){mode = 4;}
	else if(!strcmp(argv[1],"e")){mode = 5;}
	else if(!strcmp(argv[1],"f")){mode = 6;}
	else{printf("Not a valid arg\n");exit(-1);}
	int i;
	for(i=1; i<35; i++)
	{
		if(i!=SIGCHLD)
		{
			signal(i, handler);
		}
	}
	execute_command();
	return 0;
}

//file_open opens the filename from '*fname'
int file_open(char *fname)
{
	int fd;
	if((fd=open(fname,O_RDWR))<0)
	{
		fprintf(stderr,"Can’t open file '%s' for intput: %s\n",fname,strerror(errno));
		exit(-1);
	}
	return fd;
}

//print the contents of the file byte-by-byte in <hex> format
int read_file(int fd_in, char *strcmpbuf, int startprint, int buf_size)
{
	int j;
	int count = 0;
	while((j = read(fd_in, strcmpbuf, buf_size)) != 0)
	{
		if(j < 0)
		{
			perror("An error has occured in 'read' for infile: No data was read.");
			exit(-1);
		}
		else if(j == 0)
		{
			break;
		}
	}
	for(count = 0; count < buf_size; count++)
	{
		if(startprint<=count){printf("<%x> ", strcmpbuf[count]);}
	}
	printf("<EOF>\n");
	return 0;
}

//file_close takes file descriptor 'fd' and closes
int file_close(int fd)
{
	if(fd!=0) //Close infile if not stdin
	{
		if((close(fd))<0)
		{
			perror("An error has occurred in closing input file.");
			exit(-1);
		}
	}
	return 0;
}

//This function forks and execs dd for file creation
int execute_command()
{
	pid_t pid;
	int status;
	pid = fork();
	switch(pid)
	{
		case -1:
			perror("Forking Failed");
			exit(-1);
			break;
		case 0: //Child Process
			if(mode == 5)
			{
				//Testing part e with a 3 page mmap
				printf("Creating a 8195 byte random text file\n");
				if(execlp("dd", "dd", "if=/dev/urandom", "of=test", "bs=8195", "count=1", NULL) < 0)
				{
					perror("Exec Failed\n");
					exit(-1);
				}
			}
			else
			{
				printf("Creating a 10 byte random text file\n");
				if(execlp("dd", "dd", "if=/dev/urandom", "of=test", "bs=10", "count=1", NULL) < 0)
				{
					perror("Exec Failed\n");
					exit(-1);
				}				
			}
			break;
		default: //Parent Process
			waitpid(pid, &status, 0);
			if(status!=0)
			{
				if(WIFSIGNALED(status))
				{
					fprintf(stderr, "Exited with nonzero return value %d\n", WEXITSTATUS(status));
				}
			}
			else
			{
				printf("Exited with no errors\n");
			}
			switch(mode)
			{
				case 1:
					pset5a();
					break;
				case 2:
					pset5b();
					break;
				case 3:
					pset5c();
					break;
				case 4:
					pset5d();
					break;
				case 5:
					pset5e();
					break;
				case 6:
					pset5f();
					break;
			}
			break;
	}
	return 0;
}

//Signal handler
void handler(int signo)
{
	printf("Signal received: %s\n", strsignal(signo));
	exit(-1);
}

//Do pset5a
void pset5a()
{
	char *addr;
	int fd;
	char *file_name = "test";
	fd = file_open(file_name);
	printf("About to mmap MAP_PRIVATE, with PROT_READ only\n");
	if((addr = (char*)mmap(NULL, 4096, PROT_READ, MAP_PRIVATE, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	const char *buf = "This is a string";
	printf("Attempting to write to memory\n");
	memcpy(addr, buf, strlen(buf)+1);
	printf("Successful: \n%s\n", addr);
	file_close(fd);
	if(munmap(addr, 4096)<0)
	{
		printf("munmap failed\n");
	}
}

//Do pset5b
void pset5b()
{
	char *addr;
	int fd;
	struct stat st;
	char *file_name = "test";
	fd = file_open(file_name);
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	printf("About to mmap MAP_SHARED\n");
	if((addr = (char*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	//Write some bytes to memory
	printf("Writing 5 bytes to memory starting at offset 0\n");
	int i;
	for(i = 0; i<5; i++)
	{
		addr[i] = 1;
	}
	printf("Contents from memory starting at offset 0:\n");
	int j;
	for(j = 0; j<10; j++)
	{
		printf("<%x> ", addr[j]);
	}
	printf("\nContents from file starting at offset 0:\n");
	char file_buf[4096];
	read_file(fd, file_buf, 0, st.st_size);
	int k;
	int diff = 0;
	for(k = 0; k < st.st_size; k++)
	{	
		if(file_buf[k]!=addr[k])
		{
			printf("The update is not visible in the file using read()\n");
			diff = 1;
			break;
		}
	}
	if(diff == 0)
	{
		printf("The update is visible in the file using read()\n");
	}
	file_close(fd);
	if(munmap(addr, 4096)<0)
	{
		printf("munmap failed\n");
	}
}

//Do pset5c
void pset5c()
{
	char *addr;
	int fd;
	struct stat st;
	char *file_name = "test";
	fd = file_open(file_name);
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	printf("About to mmap MAP_PRIVATE\n");
	if((addr = (char*)mmap(NULL, 4096, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	//Write some bytes to memory
	printf("Writing 5 bytes to memory starting at offset 0\n");
	int i;
	for(i = 0; i<5; i++)
	{
		addr[i] = 1;
	}
	printf("Contents from memory starting at offset 0:\n");
	int j;
	for(j = 0; j<10; j++)
	{
		printf("<%x> ", addr[j]);
	}
	printf("\nContents from file starting at offset 0:\n");
	char file_buf[4096];
	read_file(fd, file_buf, 0, st.st_size);
	int k;
	int diff = 0;
	for(k = 0; k < st.st_size; k++)
	{	
		if(file_buf[k]!=addr[k])
		{
			printf("The update is not visible in the file using read()\n");
			diff = 1;
			break;
		}
	}
	if(diff == 0)
	{
		printf("The update is visible in the file using read()\n");
	}
	file_close(fd);
	if(munmap(addr, 4096)<0)
	{
		printf("munmap failed\n");
	}
}

//Do pset5d
void pset5d()
{
	char *addr;
	int fd;
	struct stat st;
	char *file_name = "test";
	fd = file_open(file_name);
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	int oldsize = st.st_size;
	printf("The size from fstat before the operation is %d\n", oldsize);
	printf("About to mmap MAP_SHARED\n");
	if((addr = (char*)mmap(NULL, oldsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	//Write some bytes to memory
	printf("Writing 4 bytes to offset %d in memory\n", oldsize);;
	int i;
	for(i = 0; i<5; i++)
	{
		addr[oldsize + i] = 1;
	}
	printf("Contents from memory starting at offset 0:\n");
	int j;
	for(j = 0; j<(oldsize+10); j++)
	{
		printf("<%x> ", addr[j]);
	}
	printf("\nContents from file starting at offset 0:\n");
	char file_buf[4096];
	read_file(fd, file_buf, 0, oldsize);
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	int newsize = st.st_size;
	printf("The size from fstat after the operation is %d\n", newsize);
	if(newsize==oldsize)
	{
		printf("The file size does not change before and after the operation\n");
	}
	else
	{
		printf("The file size does change before and after the operation\n");
	}
	file_close(fd);
	if(munmap(addr, newsize)<0)
	{
		printf("munmap failed\n");
	}
}

//Do pset 5e
void pset5e()
{
	char *addr;
	int fd;
	struct stat st;
	char *file_name = "test";
	fd = file_open(file_name);
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	int oldsize = st.st_size;
	printf("About to mmap MAP_SHARED\n");
	if((addr = (char*)mmap(NULL, oldsize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	//Write some bytes to memory
	printf("Writing 4 bytes to offset %d in memory\n", oldsize);
	int i;
	for(i = 0; i<5; i++)
	{
		addr[oldsize + i] = 1;
	}
	//lseek to new end
	int push_end = 10;
	printf("lseek to new EOF at offset %d\n", oldsize+push_end);
	if(lseek(fd, push_end, SEEK_END)<0)
	{
		perror("lseek failed");
	}
	printf("Writing one byte to offset %d\n", oldsize+push_end);
	if(write(fd, "E", 1)>1)
	{
		perror("write failed");
	}
	printf("Contents from memory starting at offset %d:\n", oldsize-2);
	int j;
	for(j = (oldsize-2); j<(oldsize+push_end+2); j++)
	{
		printf("<%x> ", addr[j]);
	}
	printf("\nContents from file starting at offset %d:\n", oldsize-2);
	if(lseek(fd, 0, SEEK_SET)<0) //Bring the fpos back
	{
		perror("lseek failed");
	}
	char file_buf[4096*3]; //For 3 pages
	if(fstat(fd, &st) == -1)
	{
		perror("fstat error");
		exit(-1);
	}
	int newsize = st.st_size;
	read_file(fd, file_buf, oldsize-2, newsize);
	int k;
	int diff = 0;
	for(k = 0; k < newsize; k++)
	{	
		if(file_buf[k]!=addr[k])
		{
			printf("The data previously written to the 'hole' in memory is not visible in the file\n");
			diff = 1;
			break;
		}
	}
	if(diff == 0)
	{
		printf("The data previously written to the 'hole' in memory is visible in the file\n");
	}
	file_close(fd);
	if(munmap(addr, newsize)<0)
	{
		printf("munmap failed\n");
	}
}

//Do pset 5f
void pset5f()
{
	char *addr;
	int fd;
	char *file_name = "test";
	fd = file_open(file_name);
	printf("About to MAP_PRIVATE mmap two page\n");
	if((addr = (char*)mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0))==MAP_FAILED)
	{
		perror("mmap error");
		exit(-1);
	}
	printf("mmap with no errors\n");
	printf("Accessing first element of first page:\n");
	printf("<%x>\n", addr[0]);
	printf("Accessing first element of second page:\n");
	printf("<%x>\n", addr[4097]);
	file_close(fd);
	if(munmap(addr, 8192)<0)
	{
		printf("munmap failed\n");
	}
}