/*
hello_world.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386

Compile to wasm (and js)
emcc hello_world.c -o hello_world.wasm
emcc hello_world.c -o hello_world.js

Run the WASM:
~/git/drekkar_webasm/drekkar_webasm_runtime/drekkar_webasm_runtime hello_world.wasm

Run the JS (to compare result):
node hello_world.js

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char** args)
{
    printf("hello, world\n");
    return 0;
}
