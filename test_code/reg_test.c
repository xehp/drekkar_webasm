/*
reg_test.c

This is an extended version of the hello world program.

Compile this with emscripten to then run in the WebAsm virtual machine.
sudo apt-get install binaryen emscripten gcc-multilib g++-multilib libedit-dev:i386
emcc reg_test.c

To read compiled the binary code do:
wasm-dis a.out.wasm

Run it in node:
node a.out.js

Or to test native:
gcc reg_test.c -lm

To run int in our runtime:
./drekkar_webasm_runtime ../test_code/a.out.wasm

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


/*****************************************************************************\ 
crc32.c

Functions to calculate 32 bit CRC checksums.

This is a reflected CRC-32 algorithm reportedly used in PKZip, AUTODIN II, Ethernet, and FDDI.

This code was created by Henrik Bjorkman using examples
from crc_v3.txt written by Ross N. Williams
You are free to use the code as long as you keep a
comment about where you got it.

History:

1.0 Created by Henrik Bjorkman 1996-04-06
1.1 Cleanup by Henrik Bjorkman 1996-04-30


\*****************************************************************************/

/* version information string */
//static char version[]="@(#) crc32.c version 1.1 ";

/*****************************************************************************/


/*****************************************************************************/


#ifndef ASSERT
#define ASSERT assert
#endif


#if (defined __AVR_ATmega328P__)
// This was needed on ATMEGA since it does not have enough RAM to keep the array.
//#define OPTIMIZE_FOR_SMALL_CODE_SIZE
#endif


// The STM32 can do CRC in HW, see chapter 14 "Cyclic redundancy check calculation unit (CRC)" in reference manual
// DocID027295 Rev 3, http://www.st.com/content/ccc/resource/technical/document/reference_manual/group0/b0/ac/3e/8f/6d/21/47/af/DM00151940/files/DM00151940.pdf/jcr:content/translations/en.DM00151940.pdf



// Reflection of input and output is only needed if an ethernet compatible checksum is needed.
#define IGNORE_REFLECTION

/* macro for debug printouts */
#define D(x)

/* returnerar biten paa position p i variabel v */
#define GETBIT(v,p) (((v)>>(p))&1)

/* Saetter biten paa pos p i variabel v till b. (b faar bara vara 0 eller 1) */
#define SETBIT(v,p,b) {v&=~(1<<(p));v|=(b)<<(p);}

/* This is the polynomial used for calculating the checksum. */
#define CRC32_POLYNOMIAL 0x04C11DB7L

/* Don't try to mixture with this. */
#define CRC32_COMPUTE(crc,ch) ((crc<<8)^CRC32_CODE(((crc>>24)^ch)&0xff))


/*****************************************************************************/

/* speedup tables */
#ifndef OPTIMIZE_FOR_SMALL_CODE_SIZE
static const uint32_t CRC32_CODE_TABLE[256] = {
		0u,79764919u,159529838u,222504665u, 319059676u,398814059u,445009330u,507990021u,
		638119352u,583659535u,797628118u,726387553u,890018660u,835552979u,1015980042u,944750013u,
		1276238704u,1221641927u,1167319070u,1095957929u,1595256236u,1540665371u,1452775106u,1381403509u,
		1780037320u,1859660671u,1671105958u,1733955601u,2031960084u,2111593891u,1889500026u,1952343757u,
		2552477408u,2632100695u,2443283854u,2506133561u,2334638140u,2414271883u,2191915858u,2254759653u,
		3190512472u,3135915759u,3081330742u,3009969537u,2905550212u,2850959411u,2762807018u,2691435357u,
		3560074640u,3505614887u,3719321342u,3648080713u,3342211916u,3287746299u,3467911202u,3396681109u,
		4063920168u,4143685023u,4223187782u,4286162673u,3779000052u,3858754371u,3904687514u,3967668269u,
		881225847u,809987520u,1023691545u,969234094u,662832811u,591600412u,771767749u,717299826u,
		311336399u,374308984u,453813921u,533576470u,25881363u,88864420u,134795389u,214552010u,
		2023205639u,2086057648u,1897238633u,1976864222u,1804852699u,1867694188u,1645340341u,1724971778u,
		1587496639u,1516133128u,1461550545u,1406951526u,1302016099u,1230646740u,1142491917u,1087903418u,
		2896545431u,2825181984u,2770861561u,2716262478u,3215044683u,3143675388u,3055782693u,3001194130u,
		2326604591u,2389456536u,2200899649u,2280525302u,2578013683u,2640855108u,2418763421u,2498394922u,
		3769900519u,3832873040u,3912640137u,3992402750u,4088425275u,4151408268u,4197601365u,4277358050u,
		3334271071u,3263032808u,3476998961u,3422541446u,3585640067u,3514407732u,3694837229u,3640369242u,
		1762451694u,1842216281u,1619975040u,1682949687u,2047383090u,2127137669u,1938468188u,2001449195u,
		1325665622u,1271206113u,1183200824u,1111960463u,1543535498u,1489069629u,1434599652u,1363369299u,
		622672798u,568075817u,748617968u,677256519u,907627842u,853037301u,1067152940u,995781531u,
		51762726u,131386257u,177728840u,240578815u,269590778u,349224269u,429104020u,491947555u,
		4046411278u,4126034873u,4172115296u,4234965207u,3794477266u,3874110821u,3953728444u,4016571915u,
		3609705398u,3555108353u,3735388376u,3664026991u,3290680682u,3236090077u,3449943556u,3378572211u,
		3174993278u,3120533705u,3032266256u,2961025959u,2923101090u,2868635157u,2813903052u,2742672763u,
		2604032198u,2683796849u,2461293480u,2524268063u,2284983834u,2364738477u,2175806836u,2238787779u,
		1569362073u,1498123566u,1409854455u,1355396672u,1317987909u,1246755826u,1192025387u,1137557660u,
		2072149281u,2135122070u,1912620623u,1992383480u,1753615357u,1816598090u,1627664531u,1707420964u,
		295390185u,358241886u,404320391u,483945776u,43990325u,106832002u,186451547u,266083308u,
		932423249u,861060070u,1041341759u,986742920u,613929101u,542559546u,756411363u,701822548u,
		3316196985u,3244833742u,3425377559u,3370778784u,3601682597u,3530312978u,3744426955u,3689838204u,
		3819031489u,3881883254u,3928223919u,4007849240u,4037393693u,4100235434u,4180117107u,4259748804u,
		2310601993u,2373574846u,2151335527u,2231098320u,2596047829u,2659030626u,2470359227u,2550115596u,
		2947551409u,2876312838u,2788305887u,2733848168u,3165939309u,3094707162u,3040238851u,2985771188u};
#define CRC32_CODE(i) CRC32_CODE_TABLE[i]
#else
static uint32_t crc32_code(uint8_t i)
{
	uint32_t crc=((uint32_t) i << 24);
	int j;
    for(j=0; j<8; j++)
    {
      if (crc & 0x80000000L)
      {
        crc=(crc<<1)^CRC32_POLYNOMIAL;
      }
      else
      {
        crc=crc<<1;
      }
    }
    return crc;
}
#define CRC32_CODE(i) crc32_code(i)
#endif


#ifndef IGNORE_REFLECTION

#ifndef OPTIMIZE_FOR_SMALL_CODE_SIZE
static const uint8_t CRC32_REFLECT8BIT_TABLE[256] = {
		0,128,64,192,32,160,96,224,16,144,80,208,48,176,112,240,
		8,136,72,200,40,168,104,232,24,152,88,216,56,184,120,248,
		4,132,68,196,36,164,100,228,20,148,84,212,52,180,116,244,
		12,140,76,204,44,172,108,236,28,156,92,220,60,188,124,252,
		2,130,66,194,34,162,98,226,18,146,82,210,50,178,114,242,
		10,138,74,202,42,170,106,234,26,154,90,218,58,186,122,250,
		6,134,70,198,38,166,102,230,22,150,86,214,54,182,118,246,
		14,142,78,206,46,174,110,238,30,158,94,222,62,190,126,254,
		1,129,65,193,33,161,97,225,17,145,81,209,49,177,113,241,
		9,137,73,201,41,169,105,233,25,153,89,217,57,185,121,249,
		5,133,69,197,37,165,101,229,21,149,85,213,53,181,117,245,
		13,141,77,205,45,173,109,237,29,157,93,221,61,189,125,253,
		3,131,67,195,35,163,99,227,19,147,83,211,51,179,115,243,
		11,139,75,203,43,171,107,235,27,155,91,219,59,187,123,251,
		7,135,71,199,39,167,103,231,23,151,87,215,55,183,119,247,
		15,143,79,207,47,175,111,239,31,159,95,223,63,191,127,255};
#define CRC32_REFLECT8BIT(i) CRC32_REFLECT8BIT_TABLE[i]
#else
static uint8_t crc32_reflect8(uint8_t in)
{
  uint8_t i;
  uint8_t out=0;
  for (i=0;i<8;i++)
  {
    SETBIT(out,i,GETBIT(in,7-i));
  }
  return(out);
}
#define CRC32_REFLECT8BIT(i) crc32_reflect8(i)
#endif

/*****************************************************************************/

/* reflekterar bitarna i en variabel */
static uint32_t crc32_reflect32(uint32_t in)
{
  uint32_t out=0;
  uint8_t i;
  for (i=0;i<32;i++)
  {
    SETBIT(out,i,GETBIT(in,31-i));
  }
  return(out);
}
#define CRC32_REFLECT32BIT(i) crc32_reflect32(i)

#else
#define CRC32_REFLECT8BIT(i) i
#define CRC32_REFLECT32BIT(i) i
#endif

/*****************************************************************************/



/*****************************************************************************/ 


/* calculates a checksumm for a buffer at address "buf" of size "size" */
uint32_t crc32_calculate(const unsigned char *buf, int size)
{
  /* this crc starts with all ones. It would be possible to start with something else. */
  uint32_t crc=0xffffffffl; 

  ASSERT(buf);

  D(printf("crc32_calculate: %s %d\n",buf,size);)

  /* Update the checksum for all bytes in the buffer. */
  int i=0;
  for(i=0;i<size;i++)
  {
    // TODO We could make a version that does not use reflect since we do not need to use Ethernet compatible CRC.
    crc = CRC32_COMPUTE(crc, CRC32_REFLECT8BIT(*buf++));
  }

  /* reflect the bits in the checksum */
  crc=CRC32_REFLECT32BIT(crc);

  /* invert the checksum */
  crc=~crc;

  return(crc);

}

// CRC For this string is expected to be 0xa61d8a27 or 2786953767 in decimal.
static char text[256]="Hello hello so much hello and all this message still need to be longer........";

int test_crc32()
{
    uint32_t c = crc32_calculate((const unsigned char *)text, strlen(text));
    printf("%u\n", c);
    return c;
}

/***************************** end of file ***********************************/





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
    printf("test_rotation_32, start 0x%08x\n", a);
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
    printf("test_rotation_64, start 0x%08llx\n", (long long)a);
    for(int i = 0; i < 36; ++i)
    {
        printf("rotl64 %d 0x%016llx\n", i, (long long)rotl64(a, i));
    }
    for(int i = 0; i < 36; ++i)
    {
        printf("rotr64 %d 0x%016llx\n", i, (long long)rotr64(a, i));
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

    int c = a - 20;
    for(unsigned long long i = 0; i < 10; i++)
    {
        if (c <= i) 
        {
	    a += 3;
        }
    }            

    for(unsigned long long i = 0; i < 10; i++)
    {
        if (c < i) 
        {
	    a += 5;
        }
    }            

    for(unsigned long long i = 0; i < 10; i++)
    {
        if (c >= i) 
        {
	    a += 7;
        }
    }            

    for(unsigned long long i = 0; i < 10; i++)
    {
        if (c > i) 
        {
	    a += 11;
        }
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

    int c = a - 20;
    for(long long i = 0; i < 10; i++)
    {
        if (c <= i) 
        {
	    a += 3;
        }
    }            

    for(long long i = 0; i < 10; i++)
    {
        if (c < i) 
        {
	    a += 5;
        }
    }            

    for(long long i = 0; i < 10; i++)
    {
        if (c >= i) 
        {
	    a += 7;
        }
    }            

    for(long long i = 0; i < 10; i++)
    {
        if (c > i) 
        {
	    a += 11;
        }
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

    long long c = a - 20;
    for(int i = 0; i < 10; i++)
    {
        if (c <= i) 
        {
	    a += 3;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c < i) 
        {
	    a += 5;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c >= i) 
        {
	    a += 7;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c > i) 
        {
	    a += 11;
        }
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

    unsigned long long c = a - 20;
    for(int i = 0; i < 10; i++)
    {
        if (c <= i) 
        {
	    a += 3;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c < i) 
        {
	    a += 5;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c >= i) 
        {
	    a += 7;
        }
    }            

    for(int i = 0; i < 10; i++)
    {
        if (c > i) 
        {
	    a += 11;
        }
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

static int test_snprintf(int r)
{
    int m1 = 17;
    char tmp[33+1];
    int m2 = 19;
    uint32_t section_id = r;
    uint32_t section_len = r * r;
    long long_nr = section_len / 3;
    long long unsigned pos = r + 5;
    snprintf(tmp, sizeof(tmp), "snprintf %d, pos 0x%llx, len %d, %ld", section_id, pos, section_len, long_nr);
    size_t len =strlen(tmp);
    printf("%zu '%s'\n", len, tmp);

    for(int i = 0; i < len; ++i)
    {
       r += tmp[i]; 
    }
    return m1+m2+r;
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
    int r = -790710061-568-3422-36+923+10;

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

    //float_test();
    //double_test();

    r += test_load();

    r += test_long_long(-9);

    r += test_crc32();

    r += test_sha1digest();

    r += test_negative_i64(-9);

    r += test_snprintf(10);

    printf("result %d\n", r);

    assert(r == 0);

    return r & 0x7f;
}
