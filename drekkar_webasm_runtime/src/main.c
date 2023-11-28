/*
main.c

Copyright (C) 2023 Henrik Bjorkman www.eit.se/hb.
All rights reserved etc etc...

To compile the test scripts some tools may be needed.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386

https://github.com/xehp/drekkar_webasm.git
*/

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <sys/stat.h>
#include "drekkar_env.h"



// https://stackoverflow.com/questions/3828192/checking-if-a-directory-exists-in-unix-system-call
static int does_folder_exist(const char* pathname)
{
	struct stat sb;
	if (stat(pathname, &sb) == 0 && S_ISDIR(sb.st_mode))
	{
	    return 1;
	}
	return 0;
}




static void print_help(const char* name) {
	printf("Usage: %s [options] <filename>\n", name);
	printf("Options:\n");
	printf("  --help                 Display this information.\n");
	printf("  --version              Display the version and copyright info.\n");
	printf("Where <filename> shall be the name of a \".wasm\" file.\n");
}

static void print_version(const char* name) {
	printf("%s : %s\n", name, DREKKAR_VERSION_STRING);
	printf("Drekkar WebAsm runtime environment\n");
	printf("http://www.drekkar.com/\n");
	printf("https://github.com/xehp/drekkar_webasm.git\n");
	printf("\n");
	printf("Copyright (C) 2023\n");
	printf("Henrik Bjorkman http://www.eit.se/hb\n");
	printf("\n");
	printf("GNU General Public License v3\n");
	printf("https://www.gnu.org/licenses/gpl-3.0.en.html\n");
	printf("\n");
	printf("IMPORTANT NOTICE! This version of this project is released under GPLv3.\n");
	printf("If your project is not open source you can't use this version!\n");
	printf("You will need to buy a closed source license from Drekkar AB.\n");
	printf("\n");
	printf("CREDITS\n");
	printf("This project owes a lot to the WAC project, ref [3]. It's a lot easier\n");
	printf("to have a working code example to look at than to only have the\n");
	printf("specifications. You may do any changes to this code but must make sure\n");
	printf("to mention that in history. Also to keep a reference to the originals.\n");
	printf("Not just this project but also to the WAC project. Thanks also to W3C\n");
	printf("and Mozilla.\n");
	printf("\n");
	printf("To compile the test scripts some tools may be needed.\n");
	printf("sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386\n");
	printf("\n");
	printf("References:\n");
	printf("[1] WebAssembly Core Specification Editor’s Draft, 7 November 2023\n");
	printf("    https://webassembly.github.io/spec/core/bikeshed/\n");
	printf("     https://webassembly.github.io/spec/core/_download/WebAssembly.pdf\n");
	printf(" [2] https://developer.mozilla.org/en-US/docs/WebAssembly/Reference\n");
	printf(" [3] https://github.com/kanaka/wac/tree/master\n");
	printf("\n");
	printf(" History:\n");
	printf(" Created November 2023 by Henrik Bjorkman.\n");

}


/* Find test_code dir, start at current directory and search upwards until it is found. */
static int find_root_dir(const char* test_code_dir_name, char *actual_path, size_t actual_path_size)
{


	int n = 0;
	char publicPath[PATH_MAX+1]="";
	char publicPathAndName[PATH_MAX+1]="";
	for(;;)
	{
		#pragma GCC diagnostic push
		#pragma GCC diagnostic ignored "-Wpragmas"
		#pragma GCC diagnostic ignored "-Wformat-truncation"

		snprintf(publicPathAndName, sizeof(publicPathAndName), "%s%s", publicPath, test_code_dir_name);
		if (does_folder_exist(publicPathAndName))
		{
			// https://stackoverflow.com/questions/229012/getting-absolute-path-of-a-file
			assert(actual_path_size >= PATH_MAX);
			const char *ptr = realpath(publicPathAndName, actual_path);
			//printf("Relative: '%s'\n", publicPathAndName);
			printf("Found: '%s'\n", ptr);
			break;
		}
		// Not found yet. Go up one level.
		strncpy(publicPathAndName, publicPath, sizeof(publicPathAndName));

		snprintf(publicPath, sizeof(publicPath), "../%s",publicPathAndName);
		if ((!does_folder_exist(publicPath)) || (n > 10))
		{
			printf("Did not find public folder. Gave up at '%s'\n", publicPath);
			return -1;
		}
		n++;
		#pragma GCC diagnostic pop
	}
	return 0;
}


static int test_drekkar_webasm_runtime(drekkar_wa_env_type *e)
{
	if (e->file_name[0]==0)
	{
		char path[PATH_MAX];
		find_root_dir("test_code", path, sizeof(path));

		#ifdef RUN_TEST_WASM
		snprintf(e->file_name, sizeof(e->file_name), "%s/" RUN_TEST_WASM ".wasm", path);
		#else
		snprintf(e->file_name, sizeof(e->file_name), "%s/a.out.wasm", path);
		#endif

	}

	long r = drekkar_wa_env_init(e);
	if (r != 0)
	{
		printf("wa_env_init failed %ld\n", r);
	}
	else
	{
		r = drekkar_wa_env_tick(e);
		if (r != 0)
		{
			printf("wa_env_tick failed %ld\n", r);
		}
		drekkar_wa_env_deinit(e);
	}
	return 0;
}

int main(int argc, char** argv)
{
	drekkar_wa_env_type e = {0};

	/* Parse the command line arguments. */
	int n = 1;
	while (n < argc)
	{
		const char *arg = argv[n++];
		if ((strcmp(arg, "--help") == 0) || (strcmp(arg, "-h") == 0))
		{
			print_help(argv[0]);
			return 0;
		}
		else if (strcmp(arg, "--version") == 0)
		{
			print_version(argv[0]);
			return 0;
		}
		else
		{
			snprintf(e.file_name, sizeof(e.file_name), "%s", arg);
		}
	}

	return test_drekkar_webasm_runtime(&e);
}
