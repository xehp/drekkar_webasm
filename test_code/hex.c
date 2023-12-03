// hex.c
//
// Read a file to memory and print it in hex.
//
// Compare output with:
// od -t x1 test.wasm
// NOTE od gives position in octal not hex.

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
#ifdef __EMSCRIPTEN__
#include <wasi/api.h>
#include <wasi/wasi-helpers.h>
#endif





static void log_hex(const uint8_t *ptr, size_t n)
{
	size_t i = 0;
	while(i < n)
	{
		printf("%08zx", i);
		for(int j = 0 ; j< 16 && i < n; j++)
		{
			printf(" %02x", ptr[i]);
			i++;
		}
		printf("\n");
	}
	if (i%16) printf("\n");
}

static size_t load_file(uint8_t *storage, size_t s, const char* file_path)
{
	FILE *fp = fopen(file_path, "r+");
	if (fp == NULL) {
		printf("File not found '%s'\n", file_path);
		return 0;
	}

	size_t n = 0;
	while(n < s)
	{
		const int ch = fgetc(fp);
		if (ch < 0)
		{
			break;
		}
		storage[n] = ch;
		n++;
	}

	fclose(fp);
	return n;
}

uint8_t buf[0x10000];

int main(int argc, char** argv)
{
	assert(argc >= 2);
	printf("hex '%s'\n", argv[1]);
	size_t n = load_file(buf, sizeof(buf), argv[1]);
	printf("n %zu\n", n);
	log_hex(buf, n);
}