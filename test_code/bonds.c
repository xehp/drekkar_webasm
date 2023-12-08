/*
main.c
  Created on: Dec 7, 2023
    Author: henrik

3
25 60 100
13 0 50
12 70 90


ska ge: 9.1


Compile like this:
emcc -O0 -g bonds.c

To examine the generated code do:
wasm-dis a.out.wasm > a.out.wat

*/




#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define MAX_SIZE 20
static int size = 0;
static double m[MAX_SIZE][MAX_SIZE] = {0};
static int uy[MAX_SIZE] = {0};

static double calculate(int x)
{
	double b = 0;

	for(int y=0; y< size; y++)
	{
		if (!uy[y])
		{
			const double a = m[x][y];
			if (x+1 == size)
			{
				if (a > b)
				{
					b = a;
				}
			}
			else
			{
				uy[y]=1;

				double t = a*calculate(x+1);
				if (t > b)
				{
					b = t;
				}
				uy[y]=0;
			}
		}
	}
	return b;
}



int main(int argc, char** argv)
{
	scanf("%d", &size);
	if (size>20)
	{
		printf("0.0");
	}
	else
	{
		for(int x=0; x<size; x++)
		{
			for(int y=0; y<size; y++)
			{
				int v;
				scanf("%d", &v);
				m[x][y] = 0.01 * v;
			}
		}
	}
	printf("%g%%\n", calculate(0)*100.0);
}
