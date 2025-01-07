/*
read_test.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc read_test.c -o read_test.wasm

For testing try also compiling to JS and native:
emcc read_test.c -o read_test.js
gcc read_test.c -o read_test

To read compiled the binary code do:
wasm-dis read_test.wasm > read_test.wat

Or to test native:
gcc read_test.c -lm -o read_test

Or use node to run the JS file:
node read_test.js

Or run the native build:
./read_test

NOTE!!!
With more recent emscripten (have tried 3.1.6 & 3.1.74) this
test no longer works. fopen always fails with errno 63.
Perhaps one of these issues:
https://github.com/emscripten-core/emscripten/issues/5237
https://github.com/emscripten-core/emscripten/issues/17167

It works as native code but it also fails with node (but
with a different errno) so it's perhaps not an error in 
Drekkar webasm but some setting in emscripten.


Did also try 'checksummer.c' from here:
https://github.com/emscripten-core/emscripten/blob/main/test/checksummer.c
Same result in our environment as in node. Native code by gcc work as expected.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __EMSCRIPTEN__
#include <wasi/api.h>
#include <wasi/wasi-helpers.h>
#include <emscripten.h>
#endif

int main (int argc, char** args)
{
	const char* file_path = "test_input.txt";
	printf("read_test '%s'\n", file_path);

	/*#ifdef __EMSCRIPTEN__
	// Perhaps it's possible to get it working on node with this trick:
	// https://github.com/emscripten-core/emscripten/blob/1.29.12/tests/fs/test_nodefs_rw.c
	EM_ASM(
	  FS.mkdir('/hack');
	  FS.mount(NODEFS, { root: '/' }, '/hack');
	);
	#endif*/

	FILE *fp = fopen(file_path, "r+");
	if (fp == NULL) 
	{
		// errno -l | column -t -l 3
		//  2   ENOENT            No such file or directory
		// 44   ECHRNG            Channel number out of range
		// 63   ENOSR             Out of streams resources
		printf("fopen failed '%s' %d\n", file_path, errno);
		return 0;
	}

	int n = 0;
	for(;;)
	{
		const int ch = fgetc(fp);
		if (ch < 0)
		{
			break;
		}
		printf(" %02x", ch);
		if ((++n%16)==0) printf("\n");
	}

	fclose(fp);

	if (n%16!=0) printf("\n");
	printf("done\n");
	return 0;
}
