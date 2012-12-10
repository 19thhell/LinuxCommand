#include	<sys/types.h>
#include    <sys/ioctl.h>
#include	<sys/stat.h>
#include    <termios.h>
#include	<strings.h>
#include	<string.h>
#include	<stdlib.h>
#include    <unistd.h>
#include	<dirent.h>
#include    <getopt.h>
#include    <iconv.h>
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
    return strcasecmp((*(struct dirent **)a)->d_name,(*(struct dirent **)b)->d_name);
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

void show_file_info(char *filename,struct stat *info_p,int maxlink,int maxsize)
/*
 * display the info about 'filename'.  The info is stored in struct at *info_p
 */
{
	char	*uid_to_name(short uid),*gid_to_name(short gid),*filemode(int mode);

    struct tm *mtime = gmtime(&info_p->st_mtime);
	printf("%s ",filemode(info_p->st_mode));
	printf("%*d ",maxlink,(int)info_p->st_nlink);	/* links */
	printf("%s ",uid_to_name(info_p->st_uid));
	printf("%s ",gid_to_name(info_p->st_gid));
	printf("%*ld ",maxsize,(long)info_p->st_size);	/* size  */
	printf("%d-%02d-%02d ",(mtime->tm_year) + 1900,(mtime->tm_mon) + 1,mtime->tm_mday);
	printf("%02d:%02d ",mtime->tm_hour,mtime->tm_min);
	printf("%s \n",filename);			/* print name	 */
}

int code_convert(char *from_charset,char *to_charset,char *inbuf,int inlen,char *outbuf,int outlen)  
{  
        iconv_t cd; 
        int rc;  
        char **pin = &inbuf;  
        char **pout = &outbuf;  
  
        cd = iconv_open(to_charset,from_charset);  
        if (cd==0)  
                return -1;  
        memset(outbuf,0,outlen);  
        if (iconv(cd,pin,&inlen,pout,&outlen) == -1)  
                return -1;  
        iconv_close(cd);  
        return 0;  
}  

int get_row(struct dirent **entrylist,int n,int minlen)
{
    struct winsize ws;
    ioctl(STDIN_FILENO,TIOCGWINSZ,&ws);
    int ws_col = ws.ws_col,inlen,outlen,row,col,sum,flag,p,curl,maxlen;
    for (row = 1;row <= n;row++)
    {
        col = (n - 1) / row + 1;
        flag = 0;
        sum = 0;
        for (int i = 0;i < col;i++)
        {
            maxlen = 0;
            for (int j = 0;j < row;j++)
            {
                p = i * row + j;
                if (p >= n)
                    break;
                else
                {
                    inlen = strlen(entrylist[p]->d_name);
                    outlen = 255;
                    char *inbuf = entrylist[p]->d_name;
                    char outbuf[255];
                    code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
                    curl = strlen(outbuf) + 2;
                    if (curl > maxlen)
                        maxlen = curl;
                }
            }
            sum += maxlen;
            if (sum >= ws_col)
            {
                flag = 0;
                break;
            }
            else
            {
                flag = 1;
            }
        }
        if (flag == 1)
            break;
    }
    return row;
}

void print_list(struct dirent **entrylist,int n,int row)
{
    int col = (n - 1) / row + 1,total = 0,curl,inlen,outlen,p;
    int maxlen[col];
    for (int i = 0;i < col;i++)
    {
        curl = 0;
        maxlen[i] = 0;
        for (int j = 0;j < row;j++)
        {
            p = i * row + j;
            if (p >= n)
                break;
            else
            {
                inlen = strlen(entrylist[p]->d_name);
                outlen = 255;
                char *inbuf = entrylist[p]->d_name;
                char outbuf[255];
                code_convert("utf-8","gb2312",inbuf,inlen,outbuf,outlen);
                curl = strlen(outbuf) + 2;
                if (curl > maxlen[i])
                    maxlen[i] = curl;
            }
        }
        if (i > 0)
            maxlen[i] += maxlen[i - 1];
    }
    for (int i = 0;i < row;i++)
    {
        for (int j = 0;j < col;j++)
        {
            p = j * row + i;
            if (p >= n)
                break;
            else
            {
               if (j > 0)
                    printf("\033[%dC",maxlen[j - 1]);
                printf("%s  \n",entrylist[p]->d_name);
            }
            if (j < col - 1)
                printf("\033[%dA",1);
        }
    }
//    printf("\033[%dB\n",row * 2 - n % row - 1);
}

void do_stat(struct dirent **entrylist,int index)
{
    long maxlink = 0,maxsize = 0,curl = 0,curs = 0;
    int link = 0,size = 0;
    char *filename;
    for (int i = 0;i < index;i++)
    {
        struct stat info;
        filename = entrylist[i]->d_name;
        if (stat(filename,&info) == -1)
            perror(filename);
        else
        {
            curl = (int)(&info)->st_nlink;
            curs = (long)(&info)->st_size;
            if (curl > maxlink)
                maxlink = curl;
            if (curs > maxsize)
                maxsize = curs;
        }
    }
    while (maxlink)
    {
        link++;
        maxlink /= 10;
    }
    while (maxsize)
    {
        size++;
        maxsize /= 10;
    }
    for (int i = 0;i < index;i++)
    {
        struct stat info;
        filename = entrylist[i]->d_name;
        if (stat(filename,&info) == -1)
            perror(filename);
        else
            show_file_info(filename,&info,link,size);
    }
}

void do_ls(char *dirname,int mode)
/*
 *	list files in directory called dirname
 */
{
	DIR *dir_ptr;		/* the directory */
	struct dirent *direntp,**entrylist,**tmplist;		/* each entry	 */
    int index,size,length,minlen,maxlen;

	if ((dir_ptr = opendir(dirname)) == NULL)
    {
        length = strlen(dirname);
        int i,psize,fsize;
        char *path,*file,*filename;
        for (i = length - 1;i >= 0;i--)
            if (dirname[i] == '/')
                if (i < length - 1)
                    break;
                else
                {
                    fprintf(stderr,"ls2: cannot open %s\n",dirname);
                    return;
                }
        if (i > 0)
        {
            psize = i + 1;
            fsize = length - 1 - i;
            path = (char *)malloc(sizeof(char) * psize); 
            file = (char *)malloc(sizeof(char) * fsize); 
            filename = dirname + i + 1;
            memcpy(path,dirname,psize);
            memcpy(file,filename,fsize);
        }
        else
        {
            path = (char *)malloc(sizeof(char));
            path[0] = '.';
            file = dirname;
        }
        if (file[0] == '.')
            return;
        if ((dir_ptr = opendir(path)) == NULL)
            fprintf(stderr,"ls2: cannot open %s\n",dirname);
        else
        {
            while ((direntp = readdir(dir_ptr)) != NULL)
            {
                if (strcmp(direntp->d_name,file) == 0)
                {
                    entrylist = (struct dirent **)malloc(sizeof(struct dirent *));
                    entrylist[0] = direntp;
                    break;
                }
            }
            index = 1;
            minlen = fsize;
            if (mode == 0)
            {
                print_list(entrylist,index,get_row(entrylist,index,minlen));
            }
            else
            {
                chdir(path);
                do_stat(entrylist,index);
            }
            closedir(dir_ptr);
        }
        return;
    }
	else
	{
        size = 100;
        index = 0;
        minlen = 256;
        maxlen = 0;
        entrylist = (struct dirent **)malloc(sizeof(struct dirent *) * size);
		while ((direntp = readdir(dir_ptr)) != NULL)
        {
            if (direntp->d_name[0] == '.')
                continue;
            else
            {
                if (index >= size)
                {
                    size *= 2;
                    tmplist = (struct dirent **)malloc(sizeof(struct dirent *) * size);
                    for (int i = 0;i < index;i++)
                        tmplist[i] = entrylist[i];
                    entrylist = (struct dirent **)malloc(sizeof(struct dirent *) * size);
                    entrylist = tmplist;
                }
                entrylist[index++] = direntp;
                length = strlen(direntp->d_name);
                if (length > maxlen)
                    maxlen = length;
                if (length < minlen)
                    minlen = length;
            }
        }
        qsort(entrylist,index,sizeof(entrylist[0]),dir_cmp);
        if (mode == 0)
        {
            print_list(entrylist,index,get_row(entrylist,index,minlen));
        }
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
                        if (argc > 1)
                            printf("\n");
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
                        if (argc > 1)
                            printf("\n");
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
