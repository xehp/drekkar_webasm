/*
array_of_long_long.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc array_of_long_long.c

Then run it like this:
node a.out.js ; echo $?

To read compiled the binary code do:
wasm-dis a.out.wasm

Or to test native:
gcc array_of_long_long.c -lm
Then run it like this:
./a.out ; echo $?

This is an extended version of the hello world program.

*/
#include <stdio.h>

#define ASIZE 20



static long long h[ASIZE] = {5};

static int hex(int ch)
{
     ch = ch & 0xF;
     return (ch < 10 ? ch + '0': ch - 10  + 'a');
}

static long long test_negative_i64(long long a)
{
        long long r = a;

	for(int i=0; i<ASIZE; i++)
        {
		h[i]= h[i] + i*i;
        }

	for(int i=0; i<ASIZE; i++)
        {
            //unsigned char ch = h[i];
            unsigned int ch = h[i] & 0xFF;
            r += ch;
        }

        printf("%lld %llx\n",r, r);

        return r & 0x7f;
}



int main (int argc, char** args) 
{
    // Expected return value is: 34 or 0x22.
    return test_negative_i64(-9) ;
}
