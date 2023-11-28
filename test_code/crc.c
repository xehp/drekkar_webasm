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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <string.h>


/*****************************************************************************/


#ifndef ASSERT
#define ASSERT assert
#endif


#if (defined __AVR_ATmega328P__)
// This was needed on ATMEGA since it does not have enough RAM to keep the array.
#define OPTIMIZE_FOR_SMALL_CODE_SIZE
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

int main()
{
    uint32_t c = crc32_calculate((const unsigned char *)text, strlen(text));
    printf("%u\n", c);
    return c;
}

/***************************** end of file ***********************************/


