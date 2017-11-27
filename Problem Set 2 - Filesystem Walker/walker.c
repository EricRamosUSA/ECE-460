//walker.c
//Eric Ramos
//ECE460 PS2

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <ctype.h>
#include <complex.h>

//Global variables
char *pathname = NULL;
char *username = NULL;
char *target = NULL;
int global_uid = 0;
unsigned long m_time = 0;
unsigned int user_opt = 0x0;
int mounted_fs = 0;
int hardlink_counter = 0;
long inode_table[1024];
long devname_table[1024];
char *dirname_table[1024];

//User options
enum options
{
	OPT_u = 0x01,
	OPT_m = 0x02,
	OPT_x = 0x04,
	OPT_l = 0x08,
};

//Function Prototypes
char *get_user(uid_t st_uid, char *user);
char *get_group(gid_t st_gid, char *group);
int get_type(mode_t st_mode, char *perm_buf);
int get_perms(mode_t st_mode, char *perm_buf);
int get_info(struct stat *st, char *perm_buf, char *user, char *group, char *npath);
int print_info(struct stat *st, char *perm_buf, char *user, char *group, char *npath);
int dir_reader(struct stat *st, char *pathname);
int uid_detect(char *username);
int optu_detect(struct stat *st, char *user);
int optm_detect(struct stat *st, long st_time_days);
int optx_detect(struct stat *st, char *npath);
int optl_detect(struct stat *st, char *npath);
int hardloop_detect(struct stat *st, struct dirent *de, char *npath);

//For getopt
static const char *optstring="u:m:x:l:";

//Main function
int main(int argc, char *argv[])
{
	if(argc==1)
	{
		printf("Error: Must specify pathname.\n");
		exit(-1);
	}

	int start=1;
	int opt=getopt(argc,argv,optstring);
	while(opt!=-1)
	{
		switch(opt)
		{
			case 'u': //Nodes with this user
				user_opt |= OPT_u;
				username = optarg;
				start=start+2;
				break;
			case 'm': //Nodes not modified in this time
				user_opt |= OPT_m;
				m_time = atol(optarg);
				m_time = m_time/60/60/24;
				start=start+2;
				break;
			case 'x': //Stay in initial volume
				user_opt |= OPT_x;
				start=start+1;
				break;
			case 'l': //Only print nodes who are simlinks to target
				user_opt |= OPT_l;
				target = strdup(optarg);
				start=start+2;
				break;
		}
		opt=getopt(argc,argv,optstring);
	}
	struct stat st;
	pathname=argv[start];
	dir_reader(&st, pathname);
	return 0;
}

//Functions
//Gets user id
char *get_user(uid_t st_uid, char *user)
{
	struct passwd *usr_struct = getpwuid(st_uid);
	if(!usr_struct)
	{
		printf("A failure has occured while retieving user");
		exit(-1);
	}
	user = strdup(usr_struct->pw_name);
	return user;
}

//Gets group id
char *get_group(gid_t st_gid, char *group)
{
	struct group *grp_struct = getgrgid(st_gid);
	if(!grp_struct)
	{
		printf("A failure has occured while retieving group");
		exit(-1);
	}
	group = strdup(grp_struct->gr_name);
	return group;
}

//Prints the file info in a format similar to ls
int print_info(struct stat *st, char *perm_buf, char *user, char *group, char *npath)
{
	char date_buf[30];
	strftime(date_buf,30,"%m-%d-%Y %H:%M:%S",localtime(&(st->st_mtime)));	
	if(((perm_buf[0])=='l'))
	{
		char *symlink_buf;
		symlink_buf = malloc(st->st_size + 1);
		readlink(npath, symlink_buf, st->st_size+1);
		printf("%-7x/%-10ld %s\t %s\t %s\t %-4ld\t %s\t %-8ld %-8o %s -> %s\n",st->st_dev,(long) st->st_ino, perm_buf, user, group,(long) st->st_nlink, date_buf,(long) st->st_size,st->st_mode, npath, symlink_buf);
		free(symlink_buf);
	}
	if(((perm_buf[0])=='b')||((perm_buf[0])=='c'))
	{
		printf("%-7x/%-10ld %s\t %s\t %s\t %-4ld\t %s\t %-8ld %-8o %s\n",st->st_dev,(long) st->st_ino, perm_buf, user, group,(long) st->st_nlink, date_buf,(long) st->st_size,st->st_mode, npath);
	}
	if((!((perm_buf[0])=='l'))&&(!((perm_buf[0])=='b'))&&(!((perm_buf[0])=='c')))
	{
		printf("%-7x/%-10ld %s\t %s\t %s\t %-4ld\t %s\t %-8ld %-8o %s\n",st->st_dev,(long) st->st_ino, perm_buf, user, group,(long) st->st_nlink, date_buf,(long) st->st_size,st->st_mode, npath);
	}
	return 0;
}

//Puts the file type into a buffer
int get_type(mode_t st_mode, char *perm_buf)
{
	switch (st_mode & S_IFMT)
	{
		case S_IFBLK:
			perm_buf[0]='b'; //Block Device
			break;
		case S_IFCHR:
			perm_buf[0]='c'; //Character Device
			break;
		case S_IFDIR:
			perm_buf[0]='d'; //Directory
			break;
		case S_IFIFO:
			perm_buf[0]='p'; //FIFO/pipe
			break;
		case S_IFLNK:
			perm_buf[0]='l'; //Symlink
			break;
		case S_IFREG:
			perm_buf[0]='r'; //Regular File
			break;
		case S_IFSOCK:
			perm_buf[0]='s'; //Socket
			break;
		default:
			perm_buf[0]='u'; //Unknown file (error)
			printf("Error: inode is not valid type");
			break;
	}
	return 0;
}

//Put permissions in permission buf
int get_perms(mode_t st_mode, char *perm_buf)
{
	int i;
	int bits=(st_mode &(S_IRWXU|S_IRWXG|S_IRWXO));
	for(i=1;i<10;i+=3)
	{
		if((bits&(1<<(9-i)))==(1<<(9-i)))
		{
			perm_buf[i]='r';
		}
		if((bits&(1<<(8-i)))==(1<<(8-i)))
		{
			perm_buf[i+1]='w';
		}
		if((bits&(1<<(7-i)))==(1<<(7-i)))
		{
			perm_buf[i+2]='x';
		}
	}
	return 0;
}

//Get permission buffer, user, group, and send to print_info, then recurse if necessary
int get_info(struct stat *st, char *perm_buf, char *user, char *group, char *npath)
{
	get_type(st->st_mode,perm_buf);
	get_perms(st->st_mode,perm_buf);

	user = get_user(st->st_uid, user);
	group = get_group(st->st_gid, group);

	print_info(st,perm_buf, user, group, npath);
	if((st->st_mode & S_IFMT) == S_IFDIR)
	{
		dir_reader(st, npath);
	}

	//Free memory
	free(npath);
	free(user);
	free(group);

	return 0;
}

//Main directory reader function
int dir_reader(struct stat *st, char *pathname)
{
	DIR *dirp;
	struct dirent *de;
	if(!(dirp=opendir(pathname)))
	{
		fprintf(stderr,"Error in opening directory %s: %s\n",pathname,strerror(errno));
		exit(-1);
	}
	while((de=readdir(dirp)))
	{
	    char *user = NULL;
	    char *group = NULL;
	    char perm_buf[11];
	    int i;
	    for(i=0;i<10;i++){perm_buf[i]='-';}
	    perm_buf[10]='\0';
		if((strcmp(de->d_name,".")) && (strcmp(de->d_name,"..")))
		{
			char *npath=malloc(sizeof(pathname)+sizeof(de->d_name)+2);
			if(!npath)
			{
				fprintf(stderr, "Insufficient memory for malloc\n");
				exit(-1);
			}
			sprintf(npath, "%s/%s", pathname, de->d_name);
			if(lstat(npath,st)==-1)
			{
				fprintf(stderr, "System call: lstat failed for path %s with inode number %d: %s\n", npath, (int) st->st_ino, strerror(errno));
				free(npath);
				continue;
			}
			if(hardloop_detect(st, de, npath)) //Check for looping hardlinks
			{
				continue;
			}
			if(mounted_fs == 0)
			{
				mounted_fs = st->st_dev; //Initial device
			}

			//Below are option cases
			//MOST OF THIS IS UNNECESSARY
			long st_time_days = (st->st_mtime)/60/60/24;
			//User
			if(user_opt==OPT_u)
			{
				if(optu_detect(st, user)==1)
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Mod time
			else if(user_opt==OPT_m)
			{
				if(optm_detect(st, st_time_days)==1)
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Volume
			else if(user_opt==OPT_x)
			{
				if(optx_detect(st, npath)==1)
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Symlink
			else if(user_opt==OPT_l)
			{
				if(optl_detect(st, npath)==1)
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and mod time
			else if((user_opt==(OPT_u | OPT_m)))
			{
				if((optu_detect(st, user)==1)&&(optm_detect(st, st_time_days)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and volume
			else if((user_opt==(OPT_u | OPT_x)))
			{
				if((optu_detect(st, user)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Mod time and volume
			else if((user_opt==(OPT_m | OPT_x)))
			{
				if((optm_detect(st, st_time_days)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and symlink
			else if((user_opt==(OPT_u | OPT_l)))
			{
				if((optu_detect(st, user)==1)&&(optl_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Symlink and volume
			else if((user_opt==(OPT_l | OPT_x)))
			{
				if((optl_detect(st, npath)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Symlink and mod time
			else if((user_opt==(OPT_l | OPT_m)))
			{
				if((optl_detect(st, npath)==1)&&(optm_detect(st, st_time_days)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and mod time and volume
			else if((user_opt==(OPT_u | OPT_m | OPT_x)))
			{
				if((optu_detect(st, user)==1)&&(optm_detect(st, st_time_days)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and mod time and symlink
			else if((user_opt==(OPT_u | OPT_m | OPT_l)))
			{
				if((optu_detect(st, user)==1)&&(optm_detect(st, st_time_days)==1)&&(optl_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and symlink time and volume
			else if((user_opt==(OPT_u | OPT_l | OPT_x)))
			{
				if((optu_detect(st, user)==1)&&(optl_detect(st, npath)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//Symlink and mod time and volume
			else if((user_opt==(OPT_l | OPT_m | OPT_x)))
			{
				if((optl_detect(st, npath)==1)&&(optm_detect(st, st_time_days)==1)&&(optx_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			//User and mod time and volume
			else if((user_opt==(OPT_u | OPT_m | OPT_x | OPT_l)))
			{
				if((optu_detect(st, user)==1)&&(optm_detect(st, st_time_days)==1)&&(optx_detect(st, npath)==1)&&(optl_detect(st, npath)==1))
				{
					get_info(st, perm_buf, user, group, npath);
				}
			}
			else if(user_opt==0x0) //If no options are indicated
			{
				get_info(st, perm_buf, user, group, npath);
			}
        }
	}
	mounted_fs = st->st_dev;
	closedir(dirp);
	return 0;
}

//See if the uid is an integer
int uid_detect(char *username)
{
	int i = 0;
	while(username[i]!='\0')
	{
		if(isdigit(username[i])==0)
		{
			return -1;
		}
		i++;
	}
	return 0;
}

//Function for -u
int optu_detect(struct stat *st, char *user)
{
	if(uid_detect(username)==0)
	{
		global_uid=atoi(username);
		if(global_uid==(st->st_uid))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if(strcmp(username,get_user(st->st_uid, user))==0)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

//Function for -m
int optm_detect(struct stat *st, long st_time_days)
{
	if(m_time>=0)
	{
		if(m_time>=st_time_days)
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else if(m_time<0)
	{
		if((abs(m_time)<=(st_time_days)))
		{
			return 1;
		}
		else
		{
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//Function for -x
int optx_detect(struct stat *st, char *npath)
{
	if(st->st_dev==mounted_fs)
	{
		return 1;
	}
	else
	{
		printf("Note: not crossing mount point at %s\n", npath);
		return 0;
	}
}

//Function for -l
int optl_detect(struct stat *st, char *npath)
{
	if((st->st_mode & S_IFMT)==S_IFLNK)
	{
		char *symlink_comp;
		symlink_comp = malloc(st->st_size + 1);
		readlink(npath, symlink_comp, st->st_size + 1);
		if(!(strcmp(target, symlink_comp)))
		{
			free(symlink_comp);
			return 1;
		}
		else
		{
			free(symlink_comp);
			return 0;
		}
	}
	else
	{
		return 0;
	}
}

//Hardlink loop detector
//This stores the inode, device number, and full path to directories opened in dir_reader
//If a directory is opened with a matching inode and device number, then dir_reader does
//not open that directory, instead printing the statement below.
int hardloop_detect(struct stat *st, struct dirent *de, char *npath)
{
	int i;
	inode_table[hardlink_counter]=de->d_ino;
	devname_table[hardlink_counter]=st->st_dev;
	dirname_table[hardlink_counter]=npath;

	for(i=0;i<(hardlink_counter-1);i++)
	{
		if(((de->d_ino)==inode_table[i])&&((st->st_dev)==devname_table[i]))
		{
			inode_table[hardlink_counter]=0;
			devname_table[hardlink_counter]=0;
			dirname_table[hardlink_counter]=NULL;
			printf("Note: loop detected at %s, with dev=%x and inode=%lu, previously seen as %s\n", npath, st->st_dev,(long) st->st_ino, dirname_table[i]);
			hardlink_counter--;
			free(npath);
			return 1;
		}
	}
	hardlink_counter++;
	return 0;
}