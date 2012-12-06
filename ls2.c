#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<sys/types.h>
#include	<dirent.h>
/**
 **	ls version 2.0
 **		purpose  list contents of directory or directories
 **		action   if no args, use .  else list files in args
 **/

int dir_cmp(const void *a,const void *b)
{
    return strcmp((*(struct dirent **)a)->d_name,(*(struct dirent **)b)->d_name);
}

void do_ls(char *dirname)
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
        for (int i = 0;i < index;i++)
            printf("%s\n",entrylist[i]->d_name);
		closedir(dir_ptr);
	}
}

int main(int argc, char *argv[])
{
	if (argc == 1)
		do_ls(".");
	else
		while (--argc){
			printf("%s:\n",*++argv);
			do_ls(*argv);
		}
    return 0;
}
