#include	<sys/types.h>
#include	<sys/stat.h>
#include	<string.h>
#include	<stdlib.h>
#include    <unistd.h>
#include	<dirent.h>
#include    <getopt.h>
#include	<stdio.h>
#include	<errno.h>
#include    <time.h>
#include	<pwd.h>
#include	<grp.h>
/**
 **	ls version 2.0
 **		purpose  list contents of directory or directories
 **		action   if no args, use .  else list files in args
 **/
#ifndef S_IFMT
#define S_IFMT 0170000
#endif
#ifndef S_IFREG
#define S_IFREG 0100000
#endif
#ifndef S_IFBLK
#define S_IFBLK 0060000
#endif
#ifndef S_IFDIR
#define S_IFDIR 0040000
#endif
#ifndef S_IFCHR
#define S_IFCHR 0020000
#endif

int dir_cmp(const void *a,const void *b)
{
    return strcmp((*(struct dirent **)a)->d_name,(*(struct dirent **)b)->d_name);
}

char *uid_to_name(short uid)
/* 
 *	returns pointer to logname associated with uid, uses getpw()
 */	
{
	struct passwd *getpwuid(),*pw_ptr;

	if ((pw_ptr = getpwuid(uid)) == NULL)
		return "Unknown";
	else
		return pw_ptr->pw_name;
}

char *gid_to_name(short gid)
/*
 *	returns pointer to group number gid. used getgrgid(3)
 */
{
	struct group *getgrgid(),*grp_ptr;

	if ((grp_ptr = getgrgid(gid)) == NULL)
		return "Unknown";
	else
		return grp_ptr->gr_name;
}

void permbits(int permval,char *string)
/*
 *	convert bits in permval into chars rw and x
 */
{
	if (permval & 4)
		string[0] = 'r';
	if (permval & 2)
		string[1] = 'w';
	if (permval & 1)
		string[2] = 'x';
} 

char *filemode(int mode)
/*
 *	returns string of mode info
 *	default to ------- and then turn on bits
 */
{
	static	char	bits[11];
	char	type;

	strcpy(bits, "----------");

	switch (mode & S_IFMT){			// mask for type
		case S_IFREG:	type = '-';	break;	// stays a dash
		case S_IFDIR: 	type = 'd';	break;	// put a d there
		case S_IFCHR:	type = 'c';	break;	// char i/o dev
		case S_IFBLK:	type = 'b';	break;	// blk. i/o dev
		default:	type = '?';	break;	// fifo, socket..
	}
	bits[0] = type;
    

	/* do SUID, SGID, and SVTX later */

	permbits(mode >> 6,bits + 1);			/* owner	*/
	permbits(mode >> 3,bits + 4);			/* group	*/
	permbits(mode,bits + 7);			/* world	*/

	return bits;
}

void show_file_info(char *filename,struct stat *info_p,int maxlength)
/*
 * display the info about 'filename'.  The info is stored in struct at *info_p
 */
{
	char	*uid_to_name(short uid),*gid_to_name(short gid),*filemode(int mode);

    struct tm *mtime = gmtime(&info_p->st_mtime);
	printf("%s ",filemode(info_p->st_mode));
	printf("%d ",(int) info_p->st_nlink);	/* links */
	printf("%s ",uid_to_name(info_p->st_uid));
	printf("%s ",gid_to_name(info_p->st_gid));
	printf("%*ld ",maxlength,(long)info_p->st_size);	/* size  */
	printf("%d-%02d-%02d ",(mtime->tm_year) + 1900,(mtime->tm_mon) + 1,mtime->tm_mday);
	printf("%02d:%02d ",mtime->tm_hour,mtime->tm_min);
	printf("%s \n",filename);			/* print name	 */
}

void do_stat(struct dirent **entrylist,int size)
{
    long maxsize = 0,cur = 0;
    int length = 0;
    char *filename;
    for (int i = 0;i < size;i++)
    {
        struct stat info;
        filename = entrylist[i]->d_name;
        if (stat(filename,&info) == -1)
            perror(filename);
        else
        {
            cur = (long)(&info)->st_size;
            if (cur > maxsize)
                maxsize = cur;
        }
    }
    while (maxsize)
    {
        length++;
        maxsize /= 10;
    }
    for (int i = 0;i < size;i++)
    {
        struct stat info;
        filename = entrylist[i]->d_name;
        if (stat(filename,&info) == -1)
            perror(filename);
        else
            show_file_info(filename,&info,length);
    }
}

void do_ls(char *dirname,int mode)
/*
 *	list files in directory called dirname
 */
{
	DIR *dir_ptr;		/* the directory */
	struct dirent *direntp,**entrylist,**tmplist;		/* each entry	 */
    int index,size;

	if ((dir_ptr = opendir(dirname)) == NULL)
		fprintf(stderr,"ls2: cannot open %s\n",dirname);
	else
	{
        size = 100;
        index = 0;
        entrylist = (struct dirent **)malloc(sizeof(struct dirent *) * size);
		while ((direntp = readdir(dir_ptr)) != NULL)
        {
            if (direntp->d_name[0] == '.')
                continue;
            else
            {
                if (index >= size)
                {
                    tmplist = (struct dirent **)malloc(sizeof(struct dirent *) * size * 2);
                    for (int i = 0;i < index;i++)
                        tmplist[i] = entrylist[i];
                    entrylist = tmplist;
                }
                entrylist[index++] = direntp;
            }
        }
        qsort(entrylist,index,sizeof(entrylist[0]),dir_cmp);
        if (mode == 0)
            for (int i = 0;i < index;i++)
                printf("%s\n",entrylist[i]->d_name);
        else
        {
            chdir(dirname);
            do_stat(entrylist,index);
        }
		closedir(dir_ptr);
	}
}

int main(int argc, char *argv[])
{
    int next_option;
    /* A string listing valid short options letters.  */
    const char* const short_options = "l";
    /* An array describing valid long options. */
    const struct option long_options[] = {
        {"l",0,NULL,'l'},
        {NULL,0,NULL,0}   /* Required at end of array.  */
    };
    
    int opt_count = 0,i = 0,j = 0;
              
    do {
        next_option = getopt_long(argc,argv,short_options,
                                  long_options, NULL);
        switch (next_option)
        {
            case 'l':   /* -l */
                if (argc == 2)
                    do_ls(".",1);
                else
                    while (--argc){
                        if ((*++argv)[0] == '-')
                            continue;
                        printf("%s:\n",*argv);
                        do_ls(*argv,1);
                    }
                opt_count++;
                break;
            case -1:    /* Done with options.  */
                if (opt_count > 0)
                    break;
                if (argc == 1)
                    do_ls(".",0);
                else
                {
                    while (--argc){
                        if ((*++argv)[0] == '-')
                            continue;
                        printf("%s:\n",*argv);
                        do_ls(*argv,0);
                    }
                }
                break;
            default:    /* Something else: unexpected.  */
                abort ();
        };
    }
    while (next_option != -1);
    return 0;
}
