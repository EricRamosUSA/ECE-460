//copycat.c
//Eric Ramos
//ECE460 PS1

//copycat concatenates and copies files from arguments in the forms:
//copycat [-b ###] [-o outfile] infile1 [...infile2...]
//copycat [-b ###] [-o outfile]
//Where [...] denotes an optional argument
//If no outfile is designated, the program will output to stdout
//If no infile is designated, the program takes inputs from stdin
//If a hyphen '-' is designated as an infile, the program will take
//inputs for stdin, then continues if more infiles were specified.

#include <fcntl.h> //For open
#include <unistd.h> //For write, read, close, getopt
#include <sys/types.h> //For read
#include <sys/uio.h> //For read
#include <stdio.h> //fprintf, printf
#include <errno.h> //Errors
#include <string.h> //More Errors
#include <stdlib.h> //getopt

//file_open opens the filename from '*fname'
int file_open(char *fname, long buf_size)
{
	int fd;
	if ((fd=open(fname,O_RDONLY))<0)
	{
		printf("Error in 'open' for file input opening:\n");
		if (errno == ENOENT)
		{
			fprintf(stderr,"'%s' does not appear to be the name of a valid file: %s\n",fname,strerror(errno));
		}
		else
		{
			fprintf(stderr,"Can’t open file '%s' for intput: %s\n",fname,strerror(errno));
		}
		exit(-1);
	}
	return fd;
}

//read_write takes the input file descriptor 'fd', filename '*fname', buffer size, and output file descriptor 'fd_out'
int read_write(int fd, char *fname, long buf_size, int fd_out)
{
	//Read from text file
	char buf[buf_size];
	int j, lim;
	lim = sizeof buf;
	while((j = read(fd, buf, lim)) != 0)
	{
		if (j < 0)
		{
			fprintf(stderr,"An error has occured in 'read' for infile %s: No data was read: %s\n", fname, strerror(errno));
			return -1;
		}
		else if (j == 0) //The end of file has been reached, break out of while
		{
			break;
		}

		int k;
		k = write(fd_out, buf, j); //Write the bytes that are stored in buf

		if (k < 0)
		{
			fprintf(stderr,"An error has occured for 'write' to output file %s: No data was written: %s\n", fname, strerror(errno));
			exit(-1);
		}
		else if (k == 0)
		{
			fprintf(stderr,"An error has occured for 'write' to output file %s: No data was written: %s\n", fname, strerror(errno));
			exit(-1);
		}
		else if (k < j)
		{
			fprintf(stderr,"A partial 'write' error to output file %s has occured: Only %d bytes of data were written: %s\n", fname, k, strerror(errno));
			exit(-1);
		}
	}
	return 0;
}

//file_close takes the input file descriptor 'fd' and filename '*fname'
int file_close(int fd, char *fname)
{
	if(fd!=0) //Close infile if not stdin
	{
		if((close(fd))<0)
		{
			fprintf(stderr, "An error has occurred in closing input file '%s':\n%s", fname, strerror(errno));
			exit(-1);
		}
	}
	return 0;
}

static const char *optString = "b:o:"; //For getopt switch
long buf_size;

int main(int argc, char *argv[])
{
	//Setting output to terminal, buffer size
	int fd_in = 0;
	int fd_out = 1;
	buf_size = 1; //Set default buf_size to 1 byte
	char buf[buf_size];

	if(argc == 1) //If only copycat is called
	{
		while((read(fd_in, buf, buf_size))>0)
		{
			write(fd_out, buf, buf_size);
		}
	}

	int start = 1; //This stores where argv[count] starts in last for loop
	int opt = getopt(argc, argv, optString);
	while(opt != -1)
	{
		switch(opt)
		{
			case 'b':
				buf_size = atol(optarg);
				if (buf_size < 1)
				{
					printf("Error in setting buffer size: Invalid buffer size\n");
					printf("Buffer size will be set to default of 1 byte\n");
					buf_size = 1;
				}
				else
				{
					printf("Buffer size is set to: %lu byte(s)\n", buf_size);
				}
				start = start + 2; //skip -b and buf_size arguments in argv
				break;
			case 'o':
				//Open or create file, then give permissions for created file
				if((fd_out = open(optarg,O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))<0)
				{
					printf("Error in opening output file\n");
					fprintf(stderr,"Sorry, cannot 'open' %s for output writing: %s\n",optarg,strerror(errno));
					exit(-1);
				}
				printf("Opened file name %s for output writing\n", optarg);
				start = start + 2; //skip -o and outfile arguments in argv
				break;
		}
		opt = getopt(argc, argv, optString);
	}

	//Open infiles
	int count, fd;
	char *fname;
	if (argc > 1)
    {
    	for (count = start; count < argc; count++)
		{
			fname = argv[count];
			if(strcmp(argv[count],"-")==0) //Detect if hyphens are being used
			{
				read_write(0, fname, buf_size, fd_out);
			}
			else
			{
				fd = file_open(fname, buf_size);
				read_write(fd, fname, buf_size, fd_out);
				file_close(fd, fname);
			}
		}
	}
	return 0;
}