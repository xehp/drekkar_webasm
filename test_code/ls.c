// ls.c
//
// List files in folder.
//
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <dirent.h>
#include <stdio.h>
#ifndef __WIN32
#include <sys/statvfs.h>
#endif
#ifdef __EMSCRIPTEN__
#include <wasi/api.h>
#include <wasi/wasi-helpers.h>
#endif







// https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
int ls(const char* path)
{
	DIR           *d;
	struct dirent *dir;
	int n=0;

	d = opendir(path);
	if (d)
	{
		while ((dir = readdir(d)) != NULL)
		{
			printf("'%s'\n", dir->d_name);
			++n;
		}

		closedir(d);
	}
	else
	{
		printf("Failed to open '%s'\n", path);
	}
	return(n);
}



int main(int argc, char** argv)
{
	if (argc >= 2)
	{
		size_t n = ls(argv[1]);
		printf("n %zu\n", n);
	}
	else
	{
		size_t n = ls(".");
		printf("n %zu\n", n);
	}
}
