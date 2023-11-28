/*
hello_world.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc hello_arg.c
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char** args)
{
    printf("hello, world\n");
    return 0;
}
