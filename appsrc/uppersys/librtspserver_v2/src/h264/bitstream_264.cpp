#include "bitstream_264.h"
#ifndef WIN32
#include <stdint.h>
#else
#include <stdio.h>



typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
#endif
/*****************************************************************************
 * Constants
 ****************************************************************************/

unsigned int BitstreamPos(const Bitstream * const bs);


static char ue_data[256]={ 8,//0
                          7,            //1
                          6,6,           //2~3
                          5,5,5,5,           //4~7
                          4,4,4,4,4,4,4,4,           //8~15
                          3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,           //16~31
                          2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,          //32~63
                          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
                          1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,//64~127
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0//128~255
};

static const int stuffing_codes[8] =
{
	        /* nbits     stuffing code */
	0,		/* 1          0 */
	1,		/* 2          01 */
	3,		/* 3          011 */
	7,		/* 4          0111 */
	0xf,	/* 5          01111 */
	0x1f,	/* 6          011111 */
	0x3f,   /* 7          0111111 */
	0x7f,	/* 8          01111111 */
};


 void  eg_write(Bitstream* const bs, int code_num);
/* initialise bitstream structure */
 void BitstreamInit(Bitstream * const bs,
			  void *const bitstream,
			  unsigned int length)
{
	unsigned int tmp;
	unsigned int bitpos;
	uint64_t adjbitstream = (uint64_t)bitstream;

	/*
	 * Start the stream on a unsigned int boundary, by rounding down to the
	 * previous unsigned int and skipping the intervening bytes.
	 */
	bitpos = ((sizeof(unsigned int)-1) & ((unsigned int)(uint64_t)bitstream));
	adjbitstream = adjbitstream - bitpos;
	bs->start = bs->tail = (unsigned int *) adjbitstream;
    bs->end = (unsigned int *)((unsigned char *)bitstream + length);

	tmp = *bs->start;
 	BSWAP(tmp);
 	bs->bufa = tmp;

	tmp = *(bs->start + 1);
 	BSWAP(tmp);
 	bs->bufb = tmp;

	bs->buf = 0;
	bs->pos = bs->initpos = bitpos*8;
	bs->length = length;
}
 unsigned int BitstreamLeftBits(Bitstream *const bs)
 {
     return bs->length * 8 - BitstreamPos(bs);
 }

/* reset bitstream state */

 void BitstreamReset(Bitstream * const bs)
{
	unsigned int tmp;

	bs->tail = bs->start;

	tmp = *bs->start;
 	BSWAP(tmp);
 	bs->bufa = tmp;

	tmp = *(bs->start + 1);
 	BSWAP(tmp);
 	bs->bufb = tmp;

	bs->buf = 0;
	bs->pos = bs->initpos;
}


/* reads n bits from bitstream without changing the stream pos */




 unsigned int BitstreamShowBits(Bitstream * const bs,const unsigned int bits)
{
	int nbit = (bits + bs->pos) - 32;
	if (nbit > 0) {
		return ((bs->bufa & (0xffffffff >> bs->pos)) << nbit) | (bs->bufb >> (32 -nbit));
	} else {
		return (bs->bufa & (0xffffffff >> bs->pos)) >> (32 - bs->pos - bits);
	}
}


/* skip n bits forward in bitstream */

 void BitstreamSkip(Bitstream * const bs,
			  const unsigned int bits)
{
    int left;
    left = (uint64_t)bs->end - (uint64_t)bs->tail;
    if (BitstreamLeftBits(bs) <= 0)
    {
        return;
    }
	bs->pos += bits;

	if (bs->pos >= 32) {
		unsigned int tmp;

		bs->bufa = bs->bufb;
        if (left > 8)
        {
            tmp = *((unsigned int *) bs->tail + 2);
            BSWAP(tmp);
            bs->bufb = tmp;
        }
            bs->tail++;
        
		
		bs->pos -= 32;
	}
}

/* read n bits from bitstream */
#if 1
 unsigned int BitstreamGetBits(Bitstream * const bs,
				 const unsigned int n)
{
    unsigned int ret,tmp;
    int nbit;
    int left;
    left = (uint64_t)bs->end - (uint64_t)bs->tail;
    if (BitstreamLeftBits(bs) <= 0)
    {
        return 0;
    }
    nbit = (n + bs->pos) - 32;
    tmp = (bs->bufa & (0xffffffff >> bs->pos));
	if (nbit > 0) {
		ret= (tmp << nbit) | (bs->bufb >> (32 -nbit));
	} else {
		ret= tmp >> (32 - bs->pos - n);
	}
    bs->pos += n;

	if (bs->pos >= 32) {


		bs->bufa = bs->bufb;
        if (left > 8)
        {
		tmp = *((unsigned int *) bs->tail + 2);
 		BSWAP(tmp);
 		bs->bufb = tmp;
        }
		bs->tail++;
		bs->pos -= 32;
	}
	return ret;
}
#else

unsigned int BitstreamGetBits(Bitstream * const bs,
                 const unsigned int n)
{
    register int nbit;
    register int bfill;
    register unsigned int tmp;
    register unsigned int ret;
    register unsigned int bufa,bufb,pos,*tail;
    bufa = bs->bufa;
    bufb = bs->bufb;
    pos = bs->pos;
    tail = bs->tail;
    nbit = (n + pos) - 32;
    ret = (bufa & (0xffffffff >> pos));
    if (nbit > 0) {
        ret=(ret  << nbit) | (bufb >> (32 -nbit));
    } else {
        ret= ret >> (32 - pos - n);
    }
    pos += n;
    bfill = pos >= 32;
   

        if (bfill)bufa = bufb;
        tmp = *((unsigned int *) tail + 2);
        //BSWAP(tmp);
        __asm{ 
            mov eax,tmp
                bswap eax
                mov tmp,eax
        }

        if (bfill)bufb = tmp;
        if (bfill)pos -= 32;
 
    bs->bufa=bufa;
    bs->bufb=bufb;
    bs->pos=pos;
    if (bfill)bs->tail=tail+1;
    return ret;
}
#endif
 unsigned int BitstreamTerminate(Bitstream * const bs)
{
	if (bs->start + bs->length <= bs->tail)
	{
		return bs->length;
	}
    return 0;
};
 

/* read single bit from bitstream */
#if 1
unsigned int BitstreamGetBit1(Bitstream * const bs)
{
    unsigned int ret;
    unsigned int tmp;
    int nbit;
    int left;
    left = (uint64_t)bs->end - (uint64_t)bs->tail;
    if (BitstreamLeftBits(bs) <= 0)
    {
        return 0;
    }
    nbit = bs->pos - 31;
    tmp = (bs->bufa & (0xffffffff >> bs->pos));
	if (nbit > 0) {
		ret= (tmp << nbit) | (bs->bufb >> (32 -  nbit));
	} else {
		ret=tmp  >> (31 - bs->pos);
	}
    bs->pos += 1;
    

	if (bs->pos >= 32) {
		

		bs->bufa = bs->bufb;
        if (left > 8)
        {
            tmp = *((unsigned int *) bs->tail + 2);
            BSWAP(tmp);
            bs->bufb = tmp;
        }
		
		bs->tail++;
		bs->pos -= 32;
	}
	return ret;
}
#else
 unsigned int BitstreamGetBit1(Bitstream * const bs)
{
    register int nbit;
    register int bfill;
    register unsigned int tmp;
    register unsigned int ret;
    register unsigned int bufa,bufb,pos,*tail;
    bufa = bs->bufa;
    bufb = bs->bufb;
    pos = bs->pos;
    tail = bs->tail;
    nbit = pos - 31;
    ret = (bufa & (0xffffffff >> pos));
    if (nbit > 0) {
        ret=(ret  << nbit) | (bufb >> (32 -nbit));
    } else {
        ret= ret >> (31 - pos);
    }
    pos++;
    bfill = pos >= 32;


    if (bfill)bufa = bufb;
    tmp = *((unsigned int *) tail + 2);
    BSWAP(tmp);

    if (bfill)bufb = tmp;
    if (bfill)pos -= 32;

    bs->bufa=bufa;
    bs->bufb=bufb;
    bs->pos=pos;
    if (bfill)bs->tail=tail+1;
    return ret;
}
#endif
#if 0
 unsigned int eg_read(Bitstream* const bs)
{
    int m,m1, info;
    unsigned int tmp;
    int len;
    tmp=BitstreamShowBits(bs,8);
   if(tmp!=0)
   {
        
      m=ue_data[tmp];
      if(m==0)
      {
          BitstreamSkip(bs,1);
          return 0;
      }
     // BitstreamSkip(bs,m+1);
     // info = BitstreamGetBits(bs, m);
        info = BitstreamGetBits(bs, m+m+1);
    return (1<<m)-1+(info&(0xFFFFFFFF>>(32-m)));


   }else
    {
         m = 0;
    
        while (!BitstreamGetBit1(bs))
        {
            m ++;
        }
    
    } 
    if (m == 0)
        return 0;

    info = BitstreamGetBits(bs, m);
    
    return (1 << m) + info - 1;
    
    
}
#endif
#if 1
 int eg_read(Bitstream* const bs)

{

    int m, info;
    unsigned int val;
    m = 0;
    while (1)
    {
        if (BitstreamLeftBits(bs) <= 0)
        {
            break;
        }
        val = BitstreamGetBit1(bs);
        if (val)
        {
            break;
        }
        m ++;
        
    }

    if (m == 0)
        return 0;


     if(BitstreamLeftBits(bs) < m)
     {
         return 0;
     }
    info = BitstreamGetBits(bs, m);

    return (1 << m) + info - 1;

}
#endif  



int eg_read_se(Bitstream* const bs)
{
    int k;
    int code_num = eg_read_ue(bs);
    
    k = (code_num & 1) ? (code_num + 1) >> 1 : -(code_num >> 1);
    
    return k;
}

 unsigned int BitstreamPos(const Bitstream * const bs)
{
	return((unsigned int)(8*((uint64_t)bs->tail - (uint64_t)bs->start) + bs->pos - bs->initpos));
}

void BitstreamFlush(Bitstream* const bs)
{
    if (bs->pos) {
        unsigned int b = bs->buf;

         BSWAP(b);
         *bs->tail = b;
    }
}

/*
 * flush the bitstream & return length (unit bytes)
 * NOTE: assumes no futher bitstream functions will be called.
 */

unsigned int BitstreamLength(Bitstream * const bs)
{
	unsigned int len = (unsigned int)((uint64_t)bs->tail - (uint64_t)bs->start);

    if (bs->pos)
        len += (bs->pos + 7) >>3;
	/* initpos is always on a byte boundary */
	if (bs->initpos)
		len -= bs->initpos>>3;

	return len;
}

 void BitstreamByteAlign(Bitstream * const bs)
{
	int n ;
	n = bs->pos & 7;
	 
	
	if (n) BitstreamSkip(bs, 8 - n);
}


/* move bitstream position forward by n bits and write out buffer if needed */

 void BitstreamForward(Bitstream * const bs,
				 const unsigned int bits)
{
	bs->pos += bits;

	if (bs->pos >= 32) {
		unsigned int b = bs->buf;

 		BSWAP(b);
 		*bs->tail++ = b;
		bs->buf = 0;
		bs->pos -= 32;
	}
}





#if 0
 void eg_write_se(Bitstream* const bs, int k)
 {
     int code_num;

     code_num = (k > 0) ? (k << 1) - 1 : (-k) << 1;

     eg_write_ue(bs, code_num);    
 }
/* pad bitstream to the next byte boundary */

 void BitstreamPad(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	if (bits < 8)
		BitstreamPutBits(bs, stuffing_codes[bits - 1], bits);
}


/*
 * pad bitstream to the next byte boundary
 * alway pad: even if currently at the byte boundary
 */

 void BitstreamPadAlways(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	BitstreamPutBits(bs, stuffing_codes[bits - 1], bits);
}

 void BitstreamPadZero(Bitstream * const bs)
{
    int bits = 8 - (bs->pos % 8);
    if (bits < 8)
        BitstreamPutBits(bs, 0, bits);
}
//for CABAC
static void __inline
BitstreamPadOneA(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	if (bits<=8 && bits>0)
		BitstreamPutBits(bs, 1<<(bits-1), bits);
}
static void __inline
BitstreamPadOne(Bitstream * const bs)
{
	int bits = 8 - (bs->pos % 8);
	if (bits<8 && bits>0)
		BitstreamPutBits(bs, 1<<(bits-1), bits);
}
////////////////////////////////////////////////////////////
// exp-golomb
 void eg_write(Bitstream* const bs, int code_num)
{
    int tmp = code_num + 1;
    int m, info;
  
    if (code_num != 0)
    {
        m = 0;
       
        while (tmp)
        {
    	    tmp >>= 1;
    	    m ++;
        }

        m --;
        
        info = code_num + 1 - (1 << m);
        BitstreamPutBits(bs, 1, m + 1);
        BitstreamPutBits(bs, info, m);
    }
    else
    {
        BitstreamPutBits(bs, 1, 1);
    }
}
#endif
#if 0
static const uint8_t ff_golomb_vlc_len[512]={
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,3,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

static const uint8_t ff_ue_golomb_vlc_code[512]={
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,
 7, 7, 7, 7, 8, 8, 8, 8, 9, 9, 9, 9,10,10,10,10,11,11,11,11,12,12,12,12,13,13,13,13,14,14,14,14,
 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};
static const uint8_t ff_log2_tab[256]={
        0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
        5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};


static __inline int av_log2(unsigned int v)
{
    int n;

    n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}
static int __inline eg_read1(Bitstream* const bs)
{
    unsigned int m, info;
    int log;
    
    m = BitstreamShowBits(bs,32);
    
   if(m>=(1<<27))
   {
       m>>=32-9;
       BitstreamSkip(bs,ff_golomb_vlc_len[m]);
       return ff_ue_golomb_vlc_code[m];
   }else
   {
       log= 2*av_log2(m) - 31;
        m>>= log;
        m--;
        BitstreamSkip(bs, 32 - log);
        return m;
   }
}
#endif





