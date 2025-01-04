/*
hello_arg.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc hello_arg.c -g -o hello_arg.wasm
../drekkar_webasm_runtime/drekkar_webasm_runtime hello_arg.wasm hello world


To read compiled the binary code do:
wasm-dis a.out.wasm

Or to test native:
gcc hello_arg.c -lm

Or use node
node a.out.js
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

int main(int argc, char** args)
{
    printf("argc: %d\n", argc);
    for(int i=0;i<argc;i++)
    {
        printf("args[%d] %s\n", i, args[i]);
    }
    
    if (argc == 3)
    {
        printf("argc == 3\n");
    }
    else
    {
        printf("argc != 3\n");
	goto end;
    }

    if (argc != 3)
    {
        printf("argc != 3\n");
	goto end;
    }
    else
    {
        printf("argc == 3\n");
    }

    printf("So far so good.\n");
    end:

    return 1;
}

