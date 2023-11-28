/*
hello_arg.c

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc hello_arg.c

To read compiled the binary code do:
wasm-dis a.out.wasm

Or to test native:
gcc hello_arg.c -lm

This is an extended version of the hello world program.

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>

static int hex(int ch)
{
     ch = ch & 0xF;
     return (ch < 10 ? ch + '0': ch - 10  + 'a');
}

static char tmp[256]={0};

static void log_hex_8(const char* msg, uint32_t d)
{
   tmp[0] = hex(d >> 4);
   tmp[1] = hex(d);
   tmp[2] = 0;
   printf("%s %s\n", msg, tmp);
}

static void log_hex_32(const char* msg, uint32_t d)
{
   tmp[0] = hex(d >> 28);
   tmp[1] = hex(d >> 24);
   tmp[2] = hex(d >> 20);
   tmp[3] = hex(d >> 16);
   tmp[4] = hex(d >> 12);
   tmp[5] = hex(d >> 8);
   tmp[6] = hex(d >> 4);
   tmp[7] = hex(d);
   tmp[8] = 0;
   printf("%s %s\n", msg, tmp);
}

/*******************************************************************************
 * sha1digest: https://github.com/CTrabant/teeny-sha1
 *
 * Calculate the SHA-1 value for supplied data buffer
 *
 * Based on https://github.com/jinqiangshou/EncryptionLibrary, credit
 * goes to @jinqiangshou, all new bugs are mine.
 *
 * @input:
 *    data      -- data to be hashed
 *    databytes -- bytes in data buffer to be hashed
 *
 * @output:
 *    digest    -- the result, MUST be at least 20 bytes
 *
 * At least one of the output buffers must be supplied.  The other, if not
 * desired, may be set to NULL.
 *
 * @return: 0 on success and non-zero on error.
 ******************************************************************************/
#define SHA1ROTATELEFT(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))
int sha1digest(uint8_t *digest, const uint8_t *data, size_t databytes)
{
  uint32_t W[80];
  uint32_t H[] = {0x67452301,
                  0xEFCDAB89,
                  0x98BADCFE,
                  0x10325476,
                  0xC3D2E1F0};

  uint32_t a;
  uint32_t b;
  uint32_t c;
  uint32_t d;
  uint32_t e;
  uint32_t f = 0;
  uint32_t k = 0;

  uint32_t idx;
  uint32_t lidx;
  uint32_t widx;
  uint32_t didx = 0;

  int32_t wcount;
  uint32_t temp;
  uint64_t databits = ((uint64_t)databytes) * 8;
  uint32_t loopcount = (databytes + 8) / 64 + 1;
  uint32_t tailbytes = 64 * loopcount - databytes;
  uint8_t datatail[128] = {0};

  if (!digest)
    return -1;

  if (!data)
    return -1;

  /* Pre-processing of data tail (includes padding to fill out 512-bit chunk):
     Add bit '1' to end of message (big-endian)
     Add 64-bit message length in bits at very end (big-endian) */
  datatail[0] = 0x80;
  datatail[tailbytes - 8] = (uint8_t) (databits >> 56 & 0xFF);
  datatail[tailbytes - 7] = (uint8_t) (databits >> 48 & 0xFF);
  datatail[tailbytes - 6] = (uint8_t) (databits >> 40 & 0xFF);
  datatail[tailbytes - 5] = (uint8_t) (databits >> 32 & 0xFF);
  datatail[tailbytes - 4] = (uint8_t) (databits >> 24 & 0xFF);
  datatail[tailbytes - 3] = (uint8_t) (databits >> 16 & 0xFF);
  datatail[tailbytes - 2] = (uint8_t) (databits >> 8 & 0xFF);
  datatail[tailbytes - 1] = (uint8_t) (databits >> 0 & 0xFF);


  /* Process each 512-bit chunk */
  for (lidx = 0; lidx < loopcount; lidx++)
  {
    /* Compute all elements in W */
    memset (W, 0, 80 * sizeof (uint32_t));

    /* Break 512-bit chunk into sixteen 32-bit, big endian words */
    for (widx = 0; widx <= 15; widx++)
    {
      wcount = 24;

      /* Copy byte-per byte from specified buffer */
      while (didx < databytes && wcount >= 0)
      {
        W[widx] += (((uint32_t)data[didx]) << wcount);
        didx++;
        wcount -= 8;
      }
      /* Fill out W with padding as needed */
      while (wcount >= 0)
      {
        W[widx] += (((uint32_t)datatail[didx - databytes]) << wcount);
        didx++;
        wcount -= 8;
      }
    }

    /* Extend the sixteen 32-bit words into eighty 32-bit words, with potential optimization from:
       "Improving the Performance of the Secure Hash Algorithm (SHA-1)" by Max Locktyukhin */
    for (widx = 16; widx <= 31; widx++)
    {
      W[widx] = SHA1ROTATELEFT((W[widx - 3] ^ W[widx - 8] ^ W[widx - 14] ^ W[widx - 16]), 1);
    }
    for (widx = 32; widx <= 79; widx++)
    {
      W[widx] = SHA1ROTATELEFT((W[widx - 6] ^ W[widx - 16] ^ W[widx - 28] ^ W[widx - 32]), 2);
    }


    /* Main loop */
    a = H[0];
    b = H[1];
    c = H[2];
    d = H[3];
    e = H[4];


    for (idx = 0; idx <= 79; idx++)
    {
      if (idx <= 19)
      {
        f = (b & c) | ((~b) & d);
        k = 0x5A827999;
      }
      else if (idx >= 20 && idx <= 39)
      {
        f = b ^ c ^ d;
        k = 0x6ED9EBA1;
      }
      else if (idx >= 40 && idx <= 59)
      {
        f = (b & c) | (b & d) | (c & d);
        k = 0x8F1BBCDC;
      }
      else if (idx >= 60 && idx <= 79)
      {
        f = b ^ c ^ d;
        k = 0xCA62C1D6;
      }
      temp = SHA1ROTATELEFT(a, 5) + f + e + k + W[idx];

      e = d;
      d = c;
      c = SHA1ROTATELEFT (b, 30);
      b = a;
      a = temp;
    }

    H[0] += a;
    H[1] += b;
    H[2] += c;
    H[3] += d;
    H[4] += e;
  }

  /* Store binary digest in supplied buffer */
  if (digest)
  {
    for (idx = 0; idx < 5; idx++)
    {
      digest[idx * 4 + 0] = (uint8_t) (H[idx] >> 24);
      digest[idx * 4 + 1] = (uint8_t) (H[idx] >> 16);
      digest[idx * 4 + 2] = (uint8_t) (H[idx] >> 8);
      digest[idx * 4 + 3] = (uint8_t) (H[idx]);
    }
  }


  return 0;
}  /* End of sha1digest() */


static char text[256]="Hello hello so much hello and all this message still need to be longer........";

int test_sha1digest()
{
    uint8_t digest[20];
    int n = 0;

    n = sha1digest(digest, (const unsigned char *)text, strlen(text));
    if (n != 0)
    {
        printf("fail %u\n", n);
    }
    else
    {
      printf("digest\n");
      for(int i=0; i<20; i++)
      {
         log_hex_8(" ", digest[i]);
         n += digest[i];
      }
      if (n != 2867)
      {
          printf("FAIL %d != 2867\n", n);
      }
      else
      {
          printf("sha1digest OK, n=%u\n", n);
      }
    }
    return n;
}



#define ASIZE 20



static long long hll[ASIZE] = {5};


static long long test_long_long(long long a)
{
	long long r = a;

	for(int i=0; i<ASIZE; i++)
	{
		hll[i]= hll[i] + i*i;
	}

	for(int i=0; i<ASIZE; i++)
	{
	    //unsigned char ch = hll[i];
	    unsigned int ch = hll[i] & 0xFF;
	    r += ch;
	}

	r = r & 0x7f;

	// Expected return value is: 34 or 0x22.
	if (r == 34)
	{
	     printf("test_long_long OK, %lld %llx\n",r, r);
	}
	else
	{
	     printf("test_long_long failed, %lld %llx\n",r, r);
	}

	return r;
}



int test_malloc(size_t size)
{
    int r = 0;
    char *name;
    name = malloc(size);
    memset(name, 0, size);
    strncpy(name, "hello using malloc", size);
    r = printf("malloc: %s %zu\n", name, size); // wanted use %zx but then strange things happen.    
    free(name);
    return r;
}

struct test_struct
{
    long x, y, z;
};

int print_test_struct(struct test_struct *t)
{
    return printf("xyz %ld %ld %ld\n", t->x, t->y, t->z);
}

struct test_struct make_test_struct()
{
    struct test_struct t = {4,5,6};
    return t;
}

int test_struct()
{
    int r = 0;

    struct test_struct t = {1,2,3};
    r += print_test_struct(&t);

    t = make_test_struct();
    r += print_test_struct(&t);

    return r;
}



static uint32_t rotl32(uint32_t value, unsigned int shift)
{
	shift &= (32 - 1);
	return (value << shift) | ((value) >> (32 - (shift)));
}

static uint32_t rotr32(uint32_t value, unsigned int shift)
{
	shift &= (32 - 1);
	return (value >> shift) | (value << (32 - (shift)));
}

static uint64_t rotl64(uint64_t value, unsigned int shift)
{
	shift &= (64 - 1);
	return (value << shift) | (value >> (64 - (shift)));
}

static uint64_t rotr64(uint64_t value, unsigned int shift)
{
	shift &= (64 - 1);
	return (value >> shift) | (value << (64 - (shift)));
}

static int test_rotation_32(int32_t a)
{
    printf("start 0x%08x\n", a);
    for(int i = 0; i < 36; ++i)
    {
        printf("rotl32 %d 0x%08x\n", i, rotl32(a, i));
    }
    for(int i = 0; i < 36; ++i)
    {
        printf("rotr32 %d 0x%08x\n", i, rotr32(a, i));
    }
    return rotl32(a, 7);
}

static int test_rotation_64(int64_t a)
{
    printf("start 0x%08llx\n", (long long)a);
    for(int i = 0; i < 36; ++i)
    {
        printf("rotl32 %d 0x%016llx\n", i, (long long)rotl64(a, i));
    }
    for(int i = 0; i < 36; ++i)
    {
        printf("rotr32 %d 0x%016llx\n", i, (long long)rotr64(a, i));
    }
    return rotr32(a, 7);
}


// Trying some recursive calls.
int fibonacci(int depth, int a, int b)
{
    if (depth)
    {
	return fibonacci(depth-1, b, a+b);
    }
    return a+b;
}

int test_fibonacci()
{
    int r = 0;
    printf("fibonacci\n");
    printf("  %d, %d\n", 0, 0);
    printf("  %d, %d\n", 1, 1);
    for(int i=2; i < 20; i++)
    {
       const int f = fibonacci(i-2, 0, 1);
       printf("  %d, %d\n", i, f);
       r += f;
    }
    return r;
}

int is_prime(long long a)
{
    long long b = 2;
    while((b*b) <= a)
    {
	if ((a%b) == 0)
        {
            //printf("a/b %lld %lld\n", a, b);
	    return 1;
        }
        b++;
    }
    printf("prime %lld\n", a);
    return 0;
}

int test_primes()
{
    int r = 0;
    for(long long i=2; i < 100; i++)
    {
        r += is_prime(i);
    }
    return r;
}

static void float_to_string_one_decimal(char *buf, int size, float a)
{
   char sign = ' ';
   if (a<0) {a = -a; sign = '-';}
   a = (a * (float)10.0 + (float)0.5);
   unsigned int b = a;
   snprintf(buf, size, "%c%u.%u", sign, b/10, b%10);
}

static void double_to_string_one_decimal(char *buf, int size, double a)
{
   char sign = ' ';
   if (a<0) {a = -a; sign = '-';}
   a = (a * 10.0 + 0.5);
   unsigned long long b = a;
   snprintf(buf, size, "%c%llu.%llu", sign, b/10, b%10);
}

static int test_more_i32(int a)
{
    // Assume starting with say i = 255.

    a = a & 0x1;
    //printf("a is now 1 %lld\n", a); 

    a = a ^ 0xFFFF;
    // a is now 0xFFFE

    a <<= 16;
    // a is now 0xFFFE0

    a |= 2;
    // a is now 0xFFFE2

    a &= 0xF;
    // a is now 2

    a = 2 * 3;
    // a is now 6

    a <<= 2;
    // a is now 24

    a >>= 1;
    // a is now 12

    a /= 2;
    // a is now 6, so binary ...00000110.

    a ^= 0xF;
    // a is now binary ...00001001 so 9.

    a++;
    // i is now 10, binary ...00001010

    a |= 0x10;
    // a is now 26

    if (a < 27)
    {
       a = 27;
    }

    if (a > 26)
    {
       a = 26;
    }

    if (a <= 77)
    {
       a = 77;
    }

    if (a >= 56)
    {
       a = 56;
    }

    a -= 2;

    a /= 2;

    --a;

    a = a % 52;

    a = a << 16;

    a ^= 0xFFFF0000;

    a = a >> 16;

    a = ~a;

    if ((a > 24) && (a < 28))
    {
       if ((a >= 26) && (a <= 26))
       {
           printf("OK, a is now 26 %d\n", a);
       }
       else
       {
           printf("Fail, a should be 26, but is %d.\n", a);
       }
    }
    else
    {
           printf("Fail, a should be >24 && <28, but is %d.\n", a);
    }

    return a;
}

static unsigned int test_more_u32(unsigned int a)
{
    // Assume starting with say i = 255.

    a = a & 0x1;
    //printf("a is now 1 %lld\n", a); 

    a = a ^ 0xFFFF;
    // a is now 0xFFFE

    a <<= 16;
    // a is now 0xFFFE0

    a |= 2;
    // a is now 0xFFFE2

    a &= 0xF;
    // a is now 2

    a = 2 * 3;
    // a is now 6

    a <<= 2;
    // a is now 24

    a >>= 1;
    // a is now 12

    a /= 2;
    // a is now 6, so binary ...00000110.

    a ^= 0xF;
    // a is now binary ...00001001 so 9.

    a++;
    // i is now 10, binary ...00001010

    a |= 0x10;
    // a is now 26

    if (a < 27)
    {
       a = 27;
    }

    if (a > 26)
    {
       a = 26;
    }

    if (a <= 77)
    {
       a = 77;
    }

    if (a >= 56)
    {
       a = 56;
    }

    a -= 2;

    a /= 2;

    --a;

    a = a % 52;

    a = a << 16;

    a ^= 0xFFFF0000;

    a = a >> 16;

    a ^= 0x0000FFFF;

    if ((a > 24) && (a < 28))
    {
       if ((a >= 26) && (a <= 26))
       {
           printf("OK, a is now 26 %u\n", a);
       }
       else
       {
           printf("Fail, a should be 26, but is %u.\n", a);
       }
    }
    else
    {
           printf("Fail, a should be >24 && <28, but is %u.\n", a);
    }

    return a;
}



static long long test_more_i64(long long a)
{
    // Assume starting with say i = 255.

    a = a & 0x1;
    //printf("a is now 1 %lld\n", a); 

    a = a ^ 0xFFFF;
    // a is now 0xFFFE

    a <<= 16;
    // a is now 0xFFFE0

    a |= 2;
    // a is now 0xFFFE2

    a &= 0xF;
    // a is now 2

    a = 2 * 3;
    // a is now 6

    a <<= 2;
    // a is now 24

    a >>= 1;
    // a is now 12

    a /= 2;
    // a is now 6, so binary ...00000110.

    a ^= 0xF;
    // a is now binary ...00001001 so 9.

    a++;
    // i is now 10, binary ...00001010

    a |= 0x10;
    // a is now 26

    if (a < 27)
    {
       a = 27;
    }

    if (a > 26)
    {
       a = 26;
    }

    if (a <= 77)
    {
       a = 77;
    }

    if (a >= 56)
    {
       a = 56;
    }

    a -= 2;

    a /= 2;

    --a;

    a = a % 52;

    a = a << 32;

    a ^= 0xFFFFFFFF00000000;

    a = a >> 32;

    a = ~a;

    if ((a > 24) && (a < 28))
    {
       if ((a >= 26) && (a <= 26))
       {
           printf("OK, a is now 26 %lld\n", a);
       }
       else
       {
           printf("Fail, a should be 26, but is %lld.\n", a);
       }
    }
    else
    {
           printf("Fail, a should be >24 && <28, but is %lld.\n", a);
    }

    return a;
}


static unsigned long long test_more_u64(unsigned long long a)
{
    // Assume starting with say i = 255.

    a = a & 0x1;
    //printf("a is now 1 %lld\n", a); 

    a = a ^ 0xFFFF;
    // a is now 0xFFFE

    a <<= 16;
    // a is now 0xFFFE0

    a |= 2;
    // a is now 0xFFFE2

    a &= 0xF;
    // a is now 2

    a = 2 * 3;
    // a is now 6

    a <<= 2;
    // a is now 24

    a >>= 1;
    // a is now 12

    a /= 2;
    // a is now 6, so binary ...00000110.

    a ^= 0xF;
    // a is now binary ...00001001 so 9.

    a++;
    // i is now 10, binary ...00001010

    a |= 0x10;
    // a is now 26

    if (a < 27)
    {
       a = 27;
    }

    if (a > 26)
    {
       a = 26;
    }

    if (a <= 77)
    {
       a = 77;
    }

    if (a >= 56)
    {
       a = 56;
    }

    a -= 2;

    a /= 2;

    --a;

    a = a % 52;


    a = a << 32;

    a ^= 0xFFFFFFFF00000000;

    a = a >> 32;

    a ^= 0x00000000FFFFFFFF;

    if ((a > 24) && (a < 28))
    {
       if ((a >= 26) && (a <= 26))
       {
           printf("OK, a is now 26 %llu\n", a);
       }
       else
       {
           printf("Fail, a should be 26, but is %llu.\n", a);
       }
    }
    else
    {
           printf("Fail, a should be >24 && <28, but is %llu.\n", a);
    }

    return a;
}


#define SHA1ROTATELEFT32(value, bits) (((value) << (bits)) | ((value) >> (32 - (bits))))

static long long c = -5;

static int g[20] = {0};
static long long h[20] = {5};

static long long test_negative_i64(long long a)
{
	c = c + a + 3;

        printf("test_negative_i64 -9=%lld -11=%lld\n", a, c);


        unsigned int d = c;
        d = SHA1ROTATELEFT32(d, 7);

        printf("SHA1ROTATELEFT 0x%llx 7 0x%x 0x%x\n", (long long)c, d, d >> 7);

	for(int i=0; i<20; i++)
        {
		g[i]= g[i] + i;
        }

	for(int i=0; i<20; i++)
        {
		h[i]= h[i] + g[i]*i;
        }

	for(int i=0; i<20; i++)
        {
            printf("h[%d] = %lld\n", i, h[i]);
        }

	for(int i=0; i<20; i++)
        {
            unsigned char ch = h[i] &0xFF;
            printf("h[%d] = %lld %d\n", i, h[i], (int)ch);
        }

	uint32_t k;

        k = 0x8F1BBCDC;

        log_hex_32("k", k);

        printf("k %08x\n", k);

        return c;
}




static void float_test()
{
    char tmp[256];
    printf("float_test\n");

    float a = 25.56;
    float b = 0.4;
    float c = a*b;
    // Test floating point, something is not working here,
    //printf("float %g * %g = %g\n", a, b, c);
    float_to_string_one_decimal(tmp, sizeof(tmp), a);
    printf("a = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), b);
    printf("b = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), c);
    printf("c = %s\n", tmp);


    a = 35.49;
    b = 0.65;
    c = a*b;
    //printf("float %g * %g = %g\n", a, b, c);

    float_to_string_one_decimal(tmp, sizeof(tmp), a);
    printf("a = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), b);
    printf("b = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), c);
    printf("c = %s\n", tmp);


    a = 9.4;
    b = 2.0;
    c = 14.8;
    float_to_string_one_decimal(tmp, sizeof(tmp), a);
    printf("a = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), b);
    printf("b = %s\n", tmp);
    float_to_string_one_decimal(tmp, sizeof(tmp), c);
    printf("c = %s\n", tmp);
   

    if ((a - b) * 2.0 == c)
    {
        printf("nearly equal\n");
    }
    if ((a - b) * 2.0 != c)
    {
        printf("not nearly equal\n");
    }

    printf("%g * %g = %g\n", a, c, a * c);
    printf("%g / %g = %g\n", a, c, a / c);
    printf("fmin(%g, %g) = %g\n", a, c, fmin(a,c));
    printf("fmax(%g, %g) = %g\n", a, c, fmax(a,c));

    if(a>b) {printf("a>b\n");}
    if(a<b) {printf("a<b\n");}
    if(a>=b) {printf("a>=b\n");}
    if(a<=b) {printf("a<=b\n");}

    printf("sqrt(%g) = %g\n", b, sqrt(b));
    printf("ceil(%g) = %g\n", a, ceil(a));
    printf("rint(%g) = %g\n", a, rint(a));
    printf("trunc(%g) = %g\n", a, trunc(a));
    printf("floor(%g) = %g\n", a, floor(a));
    printf("ceil(%g) = %g\n", c, ceil(c));
    printf("rint(%g) = %g\n", c, rint(c));
    printf("trunc(%g) = %g\n", c, trunc(c));
    printf("floor(%g) = %g\n", c, floor(c));
}


static void double_test()
{
    char tmp[256];
    printf("double_test\n");

    double a = 35.49;
    double b = 0.65;

    double c = a*b;
    // Test floating point, something is not working here,
    printf("double %g * %g = %g\n", a, b, c);

    double_to_string_one_decimal(tmp, sizeof(tmp), a);
    printf("a = %s\n", tmp);
    double_to_string_one_decimal(tmp, sizeof(tmp), b);
    printf("b = %s\n", tmp);
    double_to_string_one_decimal(tmp, sizeof(tmp), c);
    printf("c = %s\n", tmp);


    a = 9.4;
    b = 2.0;
    c = 14.8;
    double_to_string_one_decimal(tmp, sizeof(tmp), a);
    printf("a = %s\n", tmp);
    double_to_string_one_decimal(tmp, sizeof(tmp), b);
    printf("b = %s\n", tmp);
    double_to_string_one_decimal(tmp, sizeof(tmp), c);
    printf("c = %s\n", tmp);


    if ((a - b) * 2.0 == c)
    {
        printf("nearly equal\n");
    }
    if ((a - b) * 2.0 != c)
    {
        printf("not nearly equal\n");
    }

    printf("%g * %g = %g\n", a, c, a * c);
    printf("%g / %g = %g\n", a, c, a / c);
    printf("fmin(%g, %g) = %g\n", a, c, fmin(a,c));
    printf("fmax(%g, %g) = %g\n", a, c, fmax(a,c));


    if(a>b) {printf("a>b\n");}
    if(a<b) {printf("a<b\n");}
    if(a>=b) {printf("a>=b\n");}
    if(a<=b) {printf("a<=b\n");}

    printf("sqrt(%g) = %g\n", b, sqrt(b));
    printf("ceil(%g) = %g\n", a, ceil(a));
    printf("rint(%g) = %g\n", a, rint(a));
    printf("trunc(%g) = %g\n", a, trunc(a));
    printf("floor(%g) = %g\n", a, floor(a));
    printf("ceil(%g) = %g\n", c, ceil(c));
    printf("rint(%g) = %g\n", c, rint(c));
    printf("trunc(%g) = %g\n", c, trunc(c));
    printf("floor(%g) = %g\n", c, floor(c));
}


static int32_t a = 0;
static int64_t b = 0;

static int test_load()
{
   a = -7;
   b = a;
   printf("test_load %d %lld\n", a, (long long)b);
   return a+b;
}

int log_arguments(int argc, char** args)
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

int main(int argc, char** args) 
{
    // Start "r" at some value so that in the end we have zero left.
    // If result is non zero then something is fail.
    int r = 1996243706;

    r += log_arguments(argc, args);

    r += test_malloc(20);

    r += test_malloc(0x1000);

    // Test using ridicules amount of memory.
    //test_malloc(0x10000000);

    r += test_struct();

    r += test_primes();

    r += test_fibonacci();

    r += test_more_u32(255);
    r += test_more_u64(255LL);
    r += test_more_i32(255);
    r += test_more_i64(255LL);
    r += test_rotation_32(0x01020304);
    r += test_rotation_64(0x01020304);

    float_test();
    double_test();

    r += test_load();

    r += test_long_long(-9);

    r += test_sha1digest();

    r += test_negative_i64(-9);

    printf("result %d\n", r);

    assert(r == 0);

    return r & 0x7f;
}
