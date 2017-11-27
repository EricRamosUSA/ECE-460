//catgrepmore.c
//Eric Ramos
//ECE460 PS4

//Input commands in the form:
//catgrepmore pattern infile1 [...infile2...]

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/signal.h>

//Function Prototypes
int file_open(char *fname);
int read_write(int fd_in, int buf_size, int fd_out);
int file_close(int fd);
void set_pipe(int fd_pipe[]);
int pipe_exec(int pipe_fd1[2], int pipe_fd2[2]);
int redirector(int fd_in, int fd_out);
void status_check(int status, char *command);
void handler(int signo);

//Global Vars
long buf_size = 4096; //Set default buf_size to 4K bytes
int infile_fd;
char *pattern = NULL;
int files_processed = 0;
int bytes_processed = 0;

int main(int argc, char *argv[])
{
	if(argc < 3)
	{
		printf("catgrepmore: not enough arguments\n");
		exit(-1);
	}
	signal(SIGINT, handler); //Detect SIGINT from user
	signal(SIGPIPE, handler); //Ignore sigpipe

	pattern = argv[1];
	for(int i = 2; i<argc; i++)
	{
		files_processed++;
		int pipe_fd1[2];
		int pipe_fd2[2];
		set_pipe(pipe_fd1);
		set_pipe(pipe_fd2);
		infile_fd = file_open(argv[i]);
		pipe_exec(pipe_fd1, pipe_fd2);
	}
	return 0;
}

//file_open opens the filename from '*fname'
int file_open(char *fname)
{
	int fd;
	if((fd=open(fname,O_RDONLY))<0)
	{
		fprintf(stderr,"Can’t open file '%s' for intput: %s\n",fname,strerror(errno));
		exit(-1);
	}
	return fd;
}

//read_write takes the input file descriptor 'fd', buffer size, and output file descriptor 'fd_out'
int read_write(int fd_in, int buf_size, int fd_out)
{
	//Read from text file
	char buf[buf_size];
	int j, lim;
	lim = sizeof buf;
	while((j = read(fd_in, buf, lim)) != 0)
	{
		if(j < 0)
		{
			perror("An error has occured in 'read' for infile: No data was read.");
			exit(-1);
		}
		else if(j == 0) //The end of file has been reached, break out of while
		{
			break;
		}
		int k;
		k = write(fd_out, buf, j); //Write the bytes that are stored in buf
		bytes_processed += k;
		if(k <= 0)
		{
			if(errno==EPIPE)
			{
				break;
			}
			else
			{
				perror("An error has occured for 'write': No data was written.");
				exit(-1);
			}
		}
		else if(k < j)
		{
			perror("A partial 'write' error to output file has occured");
			exit(-1);
		}
	}
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

//Creates a pipe
void set_pipe(int fd_pipe[])
{
	if(pipe(fd_pipe)<0)
	{
		perror("Pipe creation failed");
		exit(-1);
	}
}

//pipe_exec is a execute function for exec'ing grep and more
//This forks grep and more successively, while closing
//the file descriptors not used by each child and parent process
int pipe_exec(int pipe_fd1[2], int pipe_fd2[2])
{
	pid_t pid_grep, pid_more;
	int status_grep, status_more;

	if((pid_grep = fork()) == -1)
	{
		perror("Forking failed for command 'grep'");
		exit(-1);
	}
	else if(pid_grep == 0) //grep child process 
	{
		file_close(infile_fd); //close infile_fd
		file_close(pipe_fd1[1]); //close write end of pipe1
		file_close(pipe_fd2[0]); //close read end of pipe2
		redirector(pipe_fd1[0], STDIN_FILENO); //redirect stdin to read end of pipe1
		redirector(pipe_fd2[1], STDOUT_FILENO); //redirect stdout to write end of pipe2
		if(execlp("grep", "grep", pattern, NULL) < 0)
		{
			perror("Failed to exec grep");
			exit(-1);
		}
	}

	if((pid_more = fork()) == -1)
	{
		perror("Forking failed for command 'more'");
		exit(-1);
	}
	else if(pid_more == 0) //more child process 
	{
		file_close(infile_fd);
		file_close(pipe_fd1[1]); //close write end of pipe1
		file_close(pipe_fd1[0]); //close read end of pipe1
		file_close(pipe_fd2[1]); //close write end of pipe2
		redirector(pipe_fd2[0], STDIN_FILENO); //redirect stdin to read end of pipe2
		if(execlp("more", "more", NULL) < 0)
		{
			perror("Failed to exec more");
			exit(-1);
		}
	}

	if(pid_grep != 0 && pid_more != 0) //Parent process
	{
		file_close(pipe_fd1[0]); //close read end of pipe1
		file_close(pipe_fd2[1]); //close write end of pipe2
		file_close(pipe_fd2[0]); //close read end of pipe2
		read_write(infile_fd, buf_size, pipe_fd1[1]); //read from infd, to write end of pipe1
		file_close(pipe_fd1[1]); //close write end of pipe1 
		file_close(infile_fd); //close infile_fd
		waitpid(pid_grep, &status_grep, 0);
		waitpid(pid_more, &status_more, 0);
		status_check(status_grep, "grep");
		status_check(status_more, "more");
	}
	return 0;
}

//For pipe redirections
int redirector(int fd_in, int fd_out)
{
	if(dup2(fd_in,fd_out)<0)
	{
		perror("Can't dup2");
		return -1;
	}
	file_close(fd_in);
	return 0;
}

//Check the status of the child processes
void status_check(int status, char *command)
{
	if(status!=0)
	{
		if(WIFSIGNALED(status))
		{
			if(WTERMSIG(status)!=SIGPIPE)
			{
				fprintf(stderr, "%s exited with signal %d\n", command, WTERMSIG(status));
			}
		}
		else
		{
			fprintf(stderr, "%s exited with nonzero return value %d\n", command, WEXITSTATUS(status));
		}
	}
	else
	{
		printf("\n%s exited with no errors\n", command);
	}
}

//Signal handler
void handler(int signo)
{
	if(signo==SIGPIPE)
	{
		return; //Ignore sigpipe, more to next infile
	}
	fprintf(stderr, "\nProcess terminated by SIGINT\n");
	fprintf(stderr, "NUMBER OF FILES: %d\n", files_processed);
	fprintf(stderr, "NUMBER OF BYTES: %d\n", bytes_processed);
	exit(-1);
}