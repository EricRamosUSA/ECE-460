//shell.c
//Eric Ramos
//ECE460 PS3

//Commands are in the form "command {arg {arg...}} {redir_op {redir_op...}}"
//shell.c forks to create a new process to run command
//Sets up I/O redirection, if applicable
//Execs the program
//Wait for and report the exit status
//Report the real, user and system time consumed by the command

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>

struct timeval prog_begin;
struct timeval prog_end;

//Max three redirect operations
char *fname_table[3] = {NULL, NULL, NULL};
char *operation_table[3] = {NULL, NULL, NULL};

int get_line(char *cmd_buf);
int token_detect(char **new_argv, char *cmd_buf);
int execute_command(char **new_argv);
void reset_ary(char **arg, int lim);
void reset_buf(char *cmd_buf);
int redirector();
int print_times(struct rusage ru, int status);

int main(int argc, char *argv[])
{
	char cmd_buf[1024]; //MAX_CANON is 1024, according to limits.h
	char *new_argv[1025]; //+1 for terminating NULL
	int script_detect = 0;
	if(argc==2)
	{
		script_detect = 1;
		//Do stdin redirection
		fname_table[0] = argv[1]; //Script Name
		operation_table[0] = "<"; //stdin
		redirector();
	}
	while(1)
	{
		if(script_detect!=1)
		{
			printf("myshell > ");
		}
		reset_ary(new_argv, 1024);
		reset_buf(cmd_buf);
		reset_ary(fname_table, 3);
		reset_ary(operation_table, 3);
		int script_end = get_line(cmd_buf);
		int tok_test = token_detect(new_argv, cmd_buf);
		if(script_detect!=1)
		{
			if(strcmp(new_argv[0],"exit")==0)
			{
				printf("Exiting myshell...\n");
				return 0;
			}
		}
		if(tok_test>0)
		{
			execute_command(new_argv);
		}
		if((script_end==0)&&(script_detect==1))
		{
			break;
		}
	}
	return 0;
}

//This function collects the line from the user input
int get_line(char *cmd_buf)
{
	int j, c;
	for(j = 0; j<1023 && (c = getchar()); j++)
	{
		if(c=='\n')
		{
			cmd_buf[j] = '\0';
			return 1;
		}
		else if(c==EOF)
		{
			cmd_buf[j] = '\0';
			printf("\n");
			return 0;
		}
		else
		{
			cmd_buf[j] = c;
		}
	}
}

//This function forks and execs command from new_argv
int execute_command(char **new_argv)
{
	printf("Executing command '%s' ", new_argv[0]);
	if(new_argv[1]!=NULL)
	{
		printf("with arguments");
		int j;
		for(j=1;new_argv[j]!=NULL;j++)
		{
			printf(" '%s'", new_argv[j]);
		}
		printf("\n");
	}
	else
	{
		printf("with no arguments\n");
	}
	gettimeofday(&prog_begin, NULL);
	pid_t pid;
	struct rusage ru;
	int status;
	pid = fork();
	switch(pid)
	{
		case -1:
			perror("Forking Failed");
			exit(-1);
			break;
		case 0: //Child Process
			if(fname_table[0]!=NULL)
			{
				redirector();
			}
			if(execvp(*new_argv, new_argv) < 0)
			{
				perror("Exec Failed\n");
				exit(-1);
			}
			break;
		default: //Parent Process
			if(wait3(&status, 0, &ru)==-1)
			{
			     perror("wait3 failed");
			}
			else
			{
				if(status!=0)
				{
					if(WIFSIGNALED(status))
					{
						fprintf(stderr, "Exited with signal %d\n", WTERMSIG(status));
					}
					else
					{
						fprintf(stderr, "Exited with return value %d\n", WEXITSTATUS(status));
					}
				}
				else
				{
					print_times(ru, status);
				}
			}
			break;
	}
	return 0;
}

//This sets up the redirection tables (if necessary)
//It also stores the appropriate elements from the user
//input to new_argv. It also returns a nonzero integer
//if input commands are specified
int token_detect(char **new_argv, char *cmd_buf)
{
	if(!strncmp(cmd_buf,"#",1))
	{
		return 0;
	}
	int count = 0;
	int exec_ary_count = 0;
	char *delim = " \t";
	char *token, *saveptr;
	for(token=strtok_r(cmd_buf,delim,&saveptr);token&&(count!=3);token=strtok_r(NULL,delim,&saveptr))
	{
		char *tok_pos = NULL;
		//Start with the biggest substring, until '<'
		if((tok_pos = strstr(token,"2>>"))!=NULL)
		{
			fname_table[count]=(tok_pos+3);
			operation_table[count] = "2>>";
			count++;
			continue;
		}
		else if((tok_pos = strstr(token,">>"))!=NULL)
		{
			fname_table[count]=(tok_pos+2);
			operation_table[count] = ">>";
			count++;
			continue;
		}
		else if((tok_pos = strstr(token,"2>"))!=NULL)
		{
			fname_table[count]=(tok_pos+2);
			operation_table[count] = "2>";
			count++;
			continue;
		}
		else if((tok_pos = strstr(token,">"))!=NULL)
		{
			fname_table[count]=(tok_pos+1);
			operation_table[count] = ">";
			count++;
			continue;
		}
		else if((tok_pos = strstr(token,"<"))!=NULL)
		{
			fname_table[count]=(tok_pos+1);
			operation_table[count] = "<";
			count++;
			continue;
		}
		else
		{
			new_argv[exec_ary_count] = token;
			exec_ary_count++;
		}
	}
	new_argv[exec_ary_count] = NULL;
	if(count == 3)
	{
		printf("Max number of redirections reached\n");
		return 1;
	}
	return count+exec_ary_count;
}

//Loop throught fname_table and redirect ops table
int redirector()
{
	int j;
	int fd;
	int fd_source;
	for(j=0;j<3;j++)
	{
		if(operation_table[j]!=NULL)
		{
			if(!strcmp(operation_table[j],"<")) //stdin
			{
				fd=open(fname_table[j],O_RDONLY);
				fd_source = 0;
			}
			else if(!strncmp(operation_table[j],">",1)) //stdout
			{
				if(!strncmp(operation_table[j],">>",2))
				{
					fd=open(fname_table[j],O_WRONLY|O_CREAT|O_APPEND,0666);
				}
				else
				{
					fd=open(fname_table[j],O_WRONLY|O_CREAT|O_TRUNC,0666);
				}
				fd_source = 1;
			}
			else if(!strncmp(operation_table[j],"2>",2)) //stderr
			{
				if(!strncmp(operation_table[j],"2>>",3))
				{
					fd=open(fname_table[j],O_WRONLY|O_CREAT|O_APPEND,0666);
				}
				else
				{
					fd=open(fname_table[j],O_WRONLY|O_CREAT|O_TRUNC,0666);
				}
				fd_source = 2;
			}
			if(fd<0)
			{
				perror("Open failure");
			}
			if(dup2(fd,fd_source)<0)
			{
				perror("Can't dup2");
				return -1;
			}
			if(close(fd)<0)
			{
				perror("Can't close file");
			}			
		}
	}
	return 0;
}

int print_times(struct rusage ru, int status)
{
	gettimeofday(&prog_end, NULL);
	long user_time = prog_end.tv_sec - prog_begin.tv_sec;
	long user_utime = prog_end.tv_usec - prog_begin.tv_usec;
	printf("Command returned with return code %d\n", status);
	printf("Consuming %ld.%06ld real seconds, ", user_time, user_utime);
	printf("%ld.%06ld user, ", ru.ru_utime.tv_sec, ru.ru_utime.tv_usec);
	printf("%ld.%06ld system\n\n", ru.ru_stime.tv_sec, ru.ru_stime.tv_usec);
	return 0;
}

//Sets all elements of char** to NULL
void reset_ary(char **arg, int lim)
{
	int j;
	for(j=0;j<lim;j++)
	{
		arg[j]=NULL;
	}
}

//Resets cmd_buf to NULL
void reset_buf(char *cmd_buf)
{
	int j;
	while(j=0, j<1023, j++)
	{
		cmd_buf[j]='\0';
	}
}