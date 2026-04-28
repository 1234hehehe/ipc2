
#include <string.h>
#include <stdio.h>
#include "libcommon_struct.h"
#include "libcommon_api.h"

#define DATA_ORDER_IS_BIG_ENDIAN 
#define SHA_LONG U32
#define MD32_REG_T int

#define SHA_LBLOCK	16
#define SHA_CBLOCK	(SHA_LBLOCK*4)	/* SHA treats input data as a
					 * contiguous array of 32 bit
					 * wide big-endian values. */
#define SHA_LAST_BLOCK  (SHA_CBLOCK-8)
#define SHA_DIGEST_LENGTH 20



typedef struct SHAstate_st
{
	SHA_LONG h0,h1,h2,h3,h4;
	SHA_LONG Nl,Nh;
	SHA_LONG data[SHA_LBLOCK];
	unsigned int num;
} SHA_CTX;

#ifndef HOST_c2l
#define HOST_c2l(c,l)	(l =(((unsigned long)(*((c)++)))<<24),		\
	l|=(((unsigned long)(*((c)++)))<<16),		\
	l|=(((unsigned long)(*((c)++)))<< 8),		\
	l|=(((unsigned long)(*((c)++)))    ),		\
	l)
#endif
#ifndef HOST_l2c
#define HOST_l2c(l,c)	(*((c)++)=(unsigned char)(((l)>>24)&0xff),	\
	*((c)++)=(unsigned char)(((l)>>16)&0xff),	\
	*((c)++)=(unsigned char)(((l)>> 8)&0xff),	\
	*((c)++)=(unsigned char)(((l)    )&0xff),	\
	l)
#endif

#define HASH_LENGTH_1             SHA_DIGEST_LENGTH
#define HASH_LONG               SHA_LONG
#define HASH_CTX_1                SHA_CTX
#define HASH_CBLOCK             SHA_CBLOCK
#define HASH_MAKE_STRING_1(c,s)   do {	\
	unsigned long ll;		\
	ll=(c)->h0; HOST_l2c(ll,(s));	\
	ll=(c)->h1; HOST_l2c(ll,(s));	\
	ll=(c)->h2; HOST_l2c(ll,(s));	\
	ll=(c)->h3; HOST_l2c(ll,(s));	\
	ll=(c)->h4; HOST_l2c(ll,(s));	\
} while (0)

#define INIT_DATA_h0 0x67452301UL
#define INIT_DATA_h1 0xefcdab89UL
#define INIT_DATA_h2 0x98badcfeUL
#define INIT_DATA_h3 0x10325476UL
#define INIT_DATA_h4 0xc3d2e1f0UL

#define K_00_19	0x5a827999UL
#define K_20_39 0x6ed9eba1UL
#define K_40_59 0x8f1bbcdcUL
#define K_60_79 0xca62c1d6UL

#ifdef WIN32
#define ROTATE(a,n)	_lrotl(a,n)
#else
#define ROTATE(a,n)     (((a)<<(n))|(((a)&0xffffffff)>>(32-(n))))
#endif

#define Xupdate(a,ix,ia,ib,ic,id)	( (a)=(ia^ib^ic^id),	\
	ix=(a)=ROTATE((a),1)	\
	)
/* As  pointed out by Wei Dai <weidai@eskimo.com>, F() below can be
 * simplified to the code in F_00_19.  Wei attributes these optimisations
 * to Peter Gutmann's SHS code, and he attributes it to Rich Schroeppel.
 * #define F(x,y,z) (((x) & (y))  |  ((~(x)) & (z)))
 * I've just become aware of another tweak to be made, again from Wei Dai,
 * in F_40_59, (x&a)|(y&a) -> (x|y)&a
 */
#define	F_00_19(b,c,d)	((((c) ^ (d)) & (b)) ^ (d)) 
#define	F_20_39(b,c,d)	((b) ^ (c) ^ (d))
#define F_40_59(b,c,d)	(((b) & (c)) | (((b)|(c)) & (d))) 
#define	F_60_79(b,c,d)	F_20_39(b,c,d)

#define BODY_00_15(i,a,b,c,d,e,f,xi) \
	(f)=xi+(e)+K_00_19+ROTATE((a),5)+F_00_19((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_16_19(i,a,b,c,d,e,f,xi,xa,xb,xc,xd) \
	Xupdate(f,xi,xa,xb,xc,xd); \
	(f)+=(e)+K_00_19+ROTATE((a),5)+F_00_19((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_20_31(i,a,b,c,d,e,f,xi,xa,xb,xc,xd) \
	Xupdate(f,xi,xa,xb,xc,xd); \
	(f)+=(e)+K_20_39+ROTATE((a),5)+F_20_39((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_32_39(i,a,b,c,d,e,f,xa,xb,xc,xd) \
	Xupdate(f,xa,xa,xb,xc,xd); \
	(f)+=(e)+K_20_39+ROTATE((a),5)+F_20_39((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_40_59(i,a,b,c,d,e,f,xa,xb,xc,xd) \
	Xupdate(f,xa,xa,xb,xc,xd); \
	(f)+=(e)+K_40_59+ROTATE((a),5)+F_40_59((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define BODY_60_79(i,a,b,c,d,e,f,xa,xb,xc,xd) \
	Xupdate(f,xa,xa,xb,xc,xd); \
	(f)=xa+(e)+K_60_79+ROTATE((a),5)+F_60_79((b),(c),(d)); \
	(b)=ROTATE((b),30);

#define	HASH_BLOCK_DATA_ORDER_1	sha1_block_data_order
#ifndef MD32_XARRAY
  /*
   * Originally X was an array. As it's automatic it's natural
   * to expect RISC compiler to accomodate at least part of it in
   * the register bank, isn't it? Unfortunately not all compilers
   * "find" this expectation reasonable:-( On order to make such
   * compilers generate better code I replace X[] with a bunch of
   * X0, X1, etc. See the function body below...
   *					<appro@fy.chalmers.se>
   */
# define X(i)	XX##i
#else
  /*
   * However! Some compilers (most notably HP C) get overwhelmed by
   * that many local variables so that we have to have the way to
   * fall down to the original behavior.
   */
# define X(i)	XX[i]
#endif

static void sha1_block_data_order(SHA_CTX *c, const void *p, size_t num)
{
	const unsigned char *data = (const unsigned char *)p;
	unsigned MD32_REG_T A,B,C,D,E,T,l;
#ifndef MD32_XARRAY
	unsigned MD32_REG_T	XX0, XX1, XX2, XX3, XX4, XX5, XX6, XX7,
		XX8, XX9,XX10,XX11,XX12,XX13,XX14,XX15;
#else
	SHA_LONG	XX[16];
#endif

	A=c->h0;
	B=c->h1;
	C=c->h2;
	D=c->h3;
	E=c->h4;

	for (;;)
	{
		const union { long one; char little; } is_endian = {1};

		if (!is_endian.little && sizeof(SHA_LONG)==4 && ((size_t)p%4)==0)
		{
			const SHA_LONG *W=(const SHA_LONG *)data;

			X( 0) = W[0];				X( 1) = W[ 1];
			BODY_00_15( 0,A,B,C,D,E,T,X( 0));	X( 2) = W[ 2];
			BODY_00_15( 1,T,A,B,C,D,E,X( 1));	X( 3) = W[ 3];
			BODY_00_15( 2,E,T,A,B,C,D,X( 2));	X( 4) = W[ 4];
			BODY_00_15( 3,D,E,T,A,B,C,X( 3));	X( 5) = W[ 5];
			BODY_00_15( 4,C,D,E,T,A,B,X( 4));	X( 6) = W[ 6];
			BODY_00_15( 5,B,C,D,E,T,A,X( 5));	X( 7) = W[ 7];
			BODY_00_15( 6,A,B,C,D,E,T,X( 6));	X( 8) = W[ 8];
			BODY_00_15( 7,T,A,B,C,D,E,X( 7));	X( 9) = W[ 9];
			BODY_00_15( 8,E,T,A,B,C,D,X( 8));	X(10) = W[10];
			BODY_00_15( 9,D,E,T,A,B,C,X( 9));	X(11) = W[11];
			BODY_00_15(10,C,D,E,T,A,B,X(10));	X(12) = W[12];
			BODY_00_15(11,B,C,D,E,T,A,X(11));	X(13) = W[13];
			BODY_00_15(12,A,B,C,D,E,T,X(12));	X(14) = W[14];
			BODY_00_15(13,T,A,B,C,D,E,X(13));	X(15) = W[15];
			BODY_00_15(14,E,T,A,B,C,D,X(14));
			BODY_00_15(15,D,E,T,A,B,C,X(15));

			data += SHA_CBLOCK;
		}
		else
		{
			HOST_c2l(data,l); X( 0)=l;		HOST_c2l(data,l); X( 1)=l;
			BODY_00_15( 0,A,B,C,D,E,T,X( 0));	HOST_c2l(data,l); X( 2)=l;
			BODY_00_15( 1,T,A,B,C,D,E,X( 1));	HOST_c2l(data,l); X( 3)=l;
			BODY_00_15( 2,E,T,A,B,C,D,X( 2));	HOST_c2l(data,l); X( 4)=l;
			BODY_00_15( 3,D,E,T,A,B,C,X( 3));	HOST_c2l(data,l); X( 5)=l;
			BODY_00_15( 4,C,D,E,T,A,B,X( 4));	HOST_c2l(data,l); X( 6)=l;
			BODY_00_15( 5,B,C,D,E,T,A,X( 5));	HOST_c2l(data,l); X( 7)=l;
			BODY_00_15( 6,A,B,C,D,E,T,X( 6));	HOST_c2l(data,l); X( 8)=l;
			BODY_00_15( 7,T,A,B,C,D,E,X( 7));	HOST_c2l(data,l); X( 9)=l;
			BODY_00_15( 8,E,T,A,B,C,D,X( 8));	HOST_c2l(data,l); X(10)=l;
			BODY_00_15( 9,D,E,T,A,B,C,X( 9));	HOST_c2l(data,l); X(11)=l;
			BODY_00_15(10,C,D,E,T,A,B,X(10));	HOST_c2l(data,l); X(12)=l;
			BODY_00_15(11,B,C,D,E,T,A,X(11));	HOST_c2l(data,l); X(13)=l;
			BODY_00_15(12,A,B,C,D,E,T,X(12));	HOST_c2l(data,l); X(14)=l;
			BODY_00_15(13,T,A,B,C,D,E,X(13));	HOST_c2l(data,l); X(15)=l;
			BODY_00_15(14,E,T,A,B,C,D,X(14));
			BODY_00_15(15,D,E,T,A,B,C,X(15));
		}

		BODY_16_19(16,C,D,E,T,A,B,X( 0),X( 0),X( 2),X( 8),X(13));
		BODY_16_19(17,B,C,D,E,T,A,X( 1),X( 1),X( 3),X( 9),X(14));
		BODY_16_19(18,A,B,C,D,E,T,X( 2),X( 2),X( 4),X(10),X(15));
		BODY_16_19(19,T,A,B,C,D,E,X( 3),X( 3),X( 5),X(11),X( 0));

		BODY_20_31(20,E,T,A,B,C,D,X( 4),X( 4),X( 6),X(12),X( 1));
		BODY_20_31(21,D,E,T,A,B,C,X( 5),X( 5),X( 7),X(13),X( 2));
		BODY_20_31(22,C,D,E,T,A,B,X( 6),X( 6),X( 8),X(14),X( 3));
		BODY_20_31(23,B,C,D,E,T,A,X( 7),X( 7),X( 9),X(15),X( 4));
		BODY_20_31(24,A,B,C,D,E,T,X( 8),X( 8),X(10),X( 0),X( 5));
		BODY_20_31(25,T,A,B,C,D,E,X( 9),X( 9),X(11),X( 1),X( 6));
		BODY_20_31(26,E,T,A,B,C,D,X(10),X(10),X(12),X( 2),X( 7));
		BODY_20_31(27,D,E,T,A,B,C,X(11),X(11),X(13),X( 3),X( 8));
		BODY_20_31(28,C,D,E,T,A,B,X(12),X(12),X(14),X( 4),X( 9));
		BODY_20_31(29,B,C,D,E,T,A,X(13),X(13),X(15),X( 5),X(10));
		BODY_20_31(30,A,B,C,D,E,T,X(14),X(14),X( 0),X( 6),X(11));
		BODY_20_31(31,T,A,B,C,D,E,X(15),X(15),X( 1),X( 7),X(12));

		BODY_32_39(32,E,T,A,B,C,D,X( 0),X( 2),X( 8),X(13));
		BODY_32_39(33,D,E,T,A,B,C,X( 1),X( 3),X( 9),X(14));
		BODY_32_39(34,C,D,E,T,A,B,X( 2),X( 4),X(10),X(15));
		BODY_32_39(35,B,C,D,E,T,A,X( 3),X( 5),X(11),X( 0));
		BODY_32_39(36,A,B,C,D,E,T,X( 4),X( 6),X(12),X( 1));
		BODY_32_39(37,T,A,B,C,D,E,X( 5),X( 7),X(13),X( 2));
		BODY_32_39(38,E,T,A,B,C,D,X( 6),X( 8),X(14),X( 3));
		BODY_32_39(39,D,E,T,A,B,C,X( 7),X( 9),X(15),X( 4));

		BODY_40_59(40,C,D,E,T,A,B,X( 8),X(10),X( 0),X( 5));
		BODY_40_59(41,B,C,D,E,T,A,X( 9),X(11),X( 1),X( 6));
		BODY_40_59(42,A,B,C,D,E,T,X(10),X(12),X( 2),X( 7));
		BODY_40_59(43,T,A,B,C,D,E,X(11),X(13),X( 3),X( 8));
		BODY_40_59(44,E,T,A,B,C,D,X(12),X(14),X( 4),X( 9));
		BODY_40_59(45,D,E,T,A,B,C,X(13),X(15),X( 5),X(10));
		BODY_40_59(46,C,D,E,T,A,B,X(14),X( 0),X( 6),X(11));
		BODY_40_59(47,B,C,D,E,T,A,X(15),X( 1),X( 7),X(12));
		BODY_40_59(48,A,B,C,D,E,T,X( 0),X( 2),X( 8),X(13));
		BODY_40_59(49,T,A,B,C,D,E,X( 1),X( 3),X( 9),X(14));
		BODY_40_59(50,E,T,A,B,C,D,X( 2),X( 4),X(10),X(15));
		BODY_40_59(51,D,E,T,A,B,C,X( 3),X( 5),X(11),X( 0));
		BODY_40_59(52,C,D,E,T,A,B,X( 4),X( 6),X(12),X( 1));
		BODY_40_59(53,B,C,D,E,T,A,X( 5),X( 7),X(13),X( 2));
		BODY_40_59(54,A,B,C,D,E,T,X( 6),X( 8),X(14),X( 3));
		BODY_40_59(55,T,A,B,C,D,E,X( 7),X( 9),X(15),X( 4));
		BODY_40_59(56,E,T,A,B,C,D,X( 8),X(10),X( 0),X( 5));
		BODY_40_59(57,D,E,T,A,B,C,X( 9),X(11),X( 1),X( 6));
		BODY_40_59(58,C,D,E,T,A,B,X(10),X(12),X( 2),X( 7));
		BODY_40_59(59,B,C,D,E,T,A,X(11),X(13),X( 3),X( 8));

		BODY_60_79(60,A,B,C,D,E,T,X(12),X(14),X( 4),X( 9));
		BODY_60_79(61,T,A,B,C,D,E,X(13),X(15),X( 5),X(10));
		BODY_60_79(62,E,T,A,B,C,D,X(14),X( 0),X( 6),X(11));
		BODY_60_79(63,D,E,T,A,B,C,X(15),X( 1),X( 7),X(12));
		BODY_60_79(64,C,D,E,T,A,B,X( 0),X( 2),X( 8),X(13));
		BODY_60_79(65,B,C,D,E,T,A,X( 1),X( 3),X( 9),X(14));
		BODY_60_79(66,A,B,C,D,E,T,X( 2),X( 4),X(10),X(15));
		BODY_60_79(67,T,A,B,C,D,E,X( 3),X( 5),X(11),X( 0));
		BODY_60_79(68,E,T,A,B,C,D,X( 4),X( 6),X(12),X( 1));
		BODY_60_79(69,D,E,T,A,B,C,X( 5),X( 7),X(13),X( 2));
		BODY_60_79(70,C,D,E,T,A,B,X( 6),X( 8),X(14),X( 3));
		BODY_60_79(71,B,C,D,E,T,A,X( 7),X( 9),X(15),X( 4));
		BODY_60_79(72,A,B,C,D,E,T,X( 8),X(10),X( 0),X( 5));
		BODY_60_79(73,T,A,B,C,D,E,X( 9),X(11),X( 1),X( 6));
		BODY_60_79(74,E,T,A,B,C,D,X(10),X(12),X( 2),X( 7));
		BODY_60_79(75,D,E,T,A,B,C,X(11),X(13),X( 3),X( 8));
		BODY_60_79(76,C,D,E,T,A,B,X(12),X(14),X( 4),X( 9));
		BODY_60_79(77,B,C,D,E,T,A,X(13),X(15),X( 5),X(10));
		BODY_60_79(78,A,B,C,D,E,T,X(14),X( 0),X( 6),X(11));
		BODY_60_79(79,T,A,B,C,D,E,X(15),X( 1),X( 7),X(12));

		c->h0=(c->h0+E)&0xffffffffL; 
		c->h1=(c->h1+T)&0xffffffffL;
		c->h2=(c->h2+A)&0xffffffffL;
		c->h3=(c->h3+B)&0xffffffffL;
		c->h4=(c->h4+C)&0xffffffffL;

		if (--num == 0) break;

		A=c->h0;
		B=c->h1;
		C=c->h2;
		D=c->h3;
		E=c->h4;

	}
}

S32 Common_Sha1_Create(Common_Sha1_T *phSha1)
{
	SHA_CTX *c = NULL;
	if (phSha1 == NULL)
	{
		return -1;
	}
	c = (SHA_CTX *)Common_Malloc(sizeof(SHA_CTX),0,__FUNCTION__,__LINE__);
	if (c == NULL)
	{
		return -1;
	}
	
	
	memset (c,0,sizeof(*c));
	c->h0=INIT_DATA_h0;
	c->h1=INIT_DATA_h1;
	c->h2=INIT_DATA_h2;
	c->h3=INIT_DATA_h3;
	c->h4=INIT_DATA_h4;
	*phSha1 = (Common_Sha1_T)c;
	return 0;

}
S32 Common_Sha1_Append(Common_Sha1_T hSha1,U8 *pData,S32 nDataLen)
{
	const unsigned char *data=pData;
	unsigned char *p;
	HASH_LONG l;
	size_t n;
	size_t len = nDataLen;
	HASH_CTX_1 *c = (HASH_CTX_1 *)hSha1;
	if (c == NULL)
	{
		return -1;
	}
	if (pData == NULL || nDataLen <= 0)
	{
		return 0;
	}
	
	


	l=(c->Nl+(((HASH_LONG)len)<<3))&0xffffffffUL;
	/* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
	 * Wei Dai <weidai@eskimo.com> for pointing it out. */
	if (l < c->Nl) /* overflow */
		c->Nh++;
	c->Nh+=(HASH_LONG)(len>>29);	/* might cause compiler warning on 16-bit */
	c->Nl=l;

	n = c->num;
	if (n != 0)
		{
		p=(unsigned char *)c->data;

		if (len >= HASH_CBLOCK || len+n >= HASH_CBLOCK)
			{
			memcpy (p+n,data,HASH_CBLOCK-n);
			HASH_BLOCK_DATA_ORDER_1 (c,p,1);
			n      = HASH_CBLOCK-n;
			data  += n;
			len   -= n;
			c->num = 0;
			memset (p,0,HASH_CBLOCK);	/* keep it zeroed */
			}
		else
			{
			memcpy (p+n,data,len);
			c->num += (unsigned int)len;
			return 0;
			}
		}

	n = len/HASH_CBLOCK;
	if (n > 0)
		{
		HASH_BLOCK_DATA_ORDER_1 (c,data,n);
		n    *= HASH_CBLOCK;
		data += n;
		len  -= n;
		}

	if (len != 0)
		{
		p = (unsigned char *)c->data;
		c->num = (unsigned int)len;
		memcpy (p,data,len);
		}
	return 0;
}
S32 Common_Sha1_Finish(Common_Sha1_T hSha1,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/)
{
	HASH_CTX_1 *c = (HASH_CTX_1 *)hSha1;
	unsigned char *md= NULL,md_mm[HASH_LENGTH_1];
	if (c == NULL)
	{
		return -1;
	}
	
	unsigned char *p = (unsigned char *)c->data;
	size_t n = c->num;

	md = md_mm;

	p[n] = 0x80; /* there is always room for one */
	n++;

	if (n > (HASH_CBLOCK-8))
	{
		memset (p+n,0,HASH_CBLOCK-n);
		n=0;
		HASH_BLOCK_DATA_ORDER_1 (c,p,1);
	}
	memset (p+n,0,HASH_CBLOCK-8-n);

	p += HASH_CBLOCK-8;
#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
	(void)HOST_l2c(c->Nh,p);
	(void)HOST_l2c(c->Nl,p);
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
	(void)HOST_l2c(c->Nl,p);
	(void)HOST_l2c(c->Nh,p);
#endif
	p -= HASH_CBLOCK;
	HASH_BLOCK_DATA_ORDER_1 (c,p,1);
	c->num=0;
	memset (p,0,HASH_CBLOCK);

#ifndef HASH_MAKE_STRING_1
#error "HASH_MAKE_STRING_1 must be defined!"
#else
	HASH_MAKE_STRING_1(c,md);
#endif
	if (pHexOut != NULL)
	{
		memcpy(pHexOut,md_mm,HASH_LENGTH_1);
	}
	if (pStringOut != NULL)
	{
		for(S32 i = 0;i<HASH_LENGTH_1;i++)
		{
			sprintf((S8 *)pStringOut + i * 2,"%02x",(U8)md_mm[i]);
		}
		pStringOut[HASH_LENGTH_1 * 2] = 0;
	}
	
	

	return 0;
}
S32 Common_Sha1_Destroy(Common_Sha1_T *pSha1)
{
	if (pSha1 == NULL || *pSha1 == NULL)
	{
		return -1;
	}
	Common_Free(*pSha1,__FUNCTION__,__LINE__);
	*pSha1 = NULL;
	return 0;
}
S32 Common_Sha1_Simply(U8 *pData,S32 nDataLen,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/)
{
	Common_Sha1_T hSha1 = NULL;
	if(Common_Sha1_Create(&hSha1))
	{
		return -1;
	}
	if(Common_Sha1_Append(hSha1,pData,nDataLen))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	if(Common_Sha1_Finish(hSha1,pHexOut,pStringOut))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	Common_Sha1_Destroy(&hSha1);
	return 0;
}



S32 Common_UsernameToken_CalcNonce(S8 HexNonce[20])
{
	int i;
	time_t r ,s;
	r = time(&s);
	memcpy(HexNonce, &r, 4);
	for (i = 4; i < 20; i += 4)
	{ 
		r = rand();
		memcpy(HexNonce + i, &r, 4);
	}
	return 0;

}

S32 Common_UsernameToken_CalcDigest(S8 *szCreated, S8  *HexNonce/*[20]*/, S32 nNonceLen, S8 *szPassword, S8 HexHash[20])
{
	Common_Sha1_T hSha1 = NULL;
	if(Common_Sha1_Create(&hSha1))
	{
		return -1;
	}
	if(Common_Sha1_Append(hSha1,(U8 *)HexNonce,nNonceLen))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	if(Common_Sha1_Append(hSha1,(U8 *)szCreated,strlen(szCreated)))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	if(Common_Sha1_Append(hSha1,(U8 *)szPassword,strlen(szPassword)))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	if(Common_Sha1_Finish(hSha1,(U8 *)HexHash,NULL))
	{
		Common_Sha1_Destroy(&hSha1);
		return -1;
	}
	Common_Sha1_Destroy(&hSha1);
	return 0;

}


// 256
#define SHA256_CBLOCK	(SHA_LBLOCK*4)	/* SHA-256 treats input data as a
					 * contiguous array of 32 bit
					 * wide big-endian values. */

#define SHA224_DIGEST_LENGTH 28
#define SHA256_DIGEST_LENGTH	32

#define HASH_LENGTH             SHA256_DIGEST_LENGTH

typedef struct SHA256state_st
{
	SHA_LONG h[8];
	SHA_LONG Nl,Nh;
	SHA_LONG data[SHA_LBLOCK];
	unsigned int num,md_len;
} SHA256_CTX;

#define	HASH_LONG		SHA_LONG
#define	HASH_CTX		SHA256_CTX
#define	HASH_CBLOCK		SHA_CBLOCK
/*
 * Note that FIPS180-2 discusses "Truncation of the Hash Function Output."
 * default: case below covers for it. It's not clear however if it's
 * permitted to truncate to amount of bytes not divisible by 4. I bet not,
 * but if it is, then default: case shall be extended. For reference.
 * Idea behind separate cases for pre-defined lenghts is to let the
 * compiler decide if it's appropriate to unroll small loops.
 */
#define	HASH_MAKE_STRING(c,s)	do {	\
	unsigned long ll;		\
	unsigned int  xn;		\
	switch ((c)->md_len)		\
	{   case SHA224_DIGEST_LENGTH:	\
		for (xn=0;xn<SHA224_DIGEST_LENGTH/4;xn++)	\
		{   ll=(c)->h[xn]; HOST_l2c(ll,(s));   }	\
		break;			\
	    case SHA256_DIGEST_LENGTH:	\
		for (xn=0;xn<SHA256_DIGEST_LENGTH/4;xn++)	\
		{   ll=(c)->h[xn]; HOST_l2c(ll,(s));   }	\
		break;			\
	    default:			\
		if ((c)->md_len > SHA256_DIGEST_LENGTH)	\
		    return 0;				\
		for (xn=0;xn<(c)->md_len/4;xn++)		\
		{   ll=(c)->h[xn]; HOST_l2c(ll,(s));   }	\
		break;			\
	}				\
	} while (0)

#define	HASH_BLOCK_DATA_ORDER	sha256_block_data_order

static const SHA_LONG K256[64] = {
	0x428a2f98UL,0x71374491UL,0xb5c0fbcfUL,0xe9b5dba5UL,
	0x3956c25bUL,0x59f111f1UL,0x923f82a4UL,0xab1c5ed5UL,
	0xd807aa98UL,0x12835b01UL,0x243185beUL,0x550c7dc3UL,
	0x72be5d74UL,0x80deb1feUL,0x9bdc06a7UL,0xc19bf174UL,
	0xe49b69c1UL,0xefbe4786UL,0x0fc19dc6UL,0x240ca1ccUL,
	0x2de92c6fUL,0x4a7484aaUL,0x5cb0a9dcUL,0x76f988daUL,
	0x983e5152UL,0xa831c66dUL,0xb00327c8UL,0xbf597fc7UL,
	0xc6e00bf3UL,0xd5a79147UL,0x06ca6351UL,0x14292967UL,
	0x27b70a85UL,0x2e1b2138UL,0x4d2c6dfcUL,0x53380d13UL,
	0x650a7354UL,0x766a0abbUL,0x81c2c92eUL,0x92722c85UL,
	0xa2bfe8a1UL,0xa81a664bUL,0xc24b8b70UL,0xc76c51a3UL,
	0xd192e819UL,0xd6990624UL,0xf40e3585UL,0x106aa070UL,
	0x19a4c116UL,0x1e376c08UL,0x2748774cUL,0x34b0bcb5UL,
	0x391c0cb3UL,0x4ed8aa4aUL,0x5b9cca4fUL,0x682e6ff3UL,
	0x748f82eeUL,0x78a5636fUL,0x84c87814UL,0x8cc70208UL,
	0x90befffaUL,0xa4506cebUL,0xbef9a3f7UL,0xc67178f2UL };

/*
 * FIPS specification refers to right rotations, while our ROTATE macro
 * is left one. This is why you might notice that rotation coefficients
 * differ from those observed in FIPS document by 32-N...
 */
#define Sigma0(x)	(ROTATE((x),30) ^ ROTATE((x),19) ^ ROTATE((x),10))
#define Sigma1(x)	(ROTATE((x),26) ^ ROTATE((x),21) ^ ROTATE((x),7))
#define sigma0(x)	(ROTATE((x),25) ^ ROTATE((x),14) ^ ((x)>>3))
#define sigma1(x)	(ROTATE((x),15) ^ ROTATE((x),13) ^ ((x)>>10))

#define Ch(x,y,z)	(((x) & (y)) ^ ((~(x)) & (z)))
#define Maj(x,y,z)	(((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

#define	ROUND_00_15(i,a,b,c,d,e,f,g,h)		do {	\
	T1 += h + Sigma1(e) + Ch(e,f,g) + K256[i];	\
	h = Sigma0(a) + Maj(a,b,c);			\
	d += T1;	h += T1;		} while (0)

#define	ROUND_16_63(i,a,b,c,d,e,f,g,h,X)	do {	\
	s0 = X[(i+1)&0x0f];	s0 = sigma0(s0);	\
	s1 = X[(i+14)&0x0f];	s1 = sigma1(s1);	\
	T1 = X[(i)&0x0f] += s0 + s1 + X[(i+9)&0x0f];	\
	ROUND_00_15(i,a,b,c,d,e,f,g,h);		} while (0)

	
static void sha256_block_data_order (SHA256_CTX *ctx, const void *in, size_t num)
{
	unsigned MD32_REG_T a,b,c,d,e,f,g,h,s0,s1,T1;
	SHA_LONG	X[16];
	int i;
	const unsigned char *data=(const unsigned char *)in;
	const union { long one; char little; } is_endian = {1};

	while (num--) {

		a = ctx->h[0];	b = ctx->h[1];	c = ctx->h[2];	d = ctx->h[3];
		e = ctx->h[4];	f = ctx->h[5];	g = ctx->h[6];	h = ctx->h[7];

		if (!is_endian.little && sizeof(SHA_LONG)==4 && ((size_t)in%4)==0)
		{
			const SHA_LONG *W=(const SHA_LONG *)data;

			T1 = X[0] = W[0];	ROUND_00_15(0,a,b,c,d,e,f,g,h);
			T1 = X[1] = W[1];	ROUND_00_15(1,h,a,b,c,d,e,f,g);
			T1 = X[2] = W[2];	ROUND_00_15(2,g,h,a,b,c,d,e,f);
			T1 = X[3] = W[3];	ROUND_00_15(3,f,g,h,a,b,c,d,e);
			T1 = X[4] = W[4];	ROUND_00_15(4,e,f,g,h,a,b,c,d);
			T1 = X[5] = W[5];	ROUND_00_15(5,d,e,f,g,h,a,b,c);
			T1 = X[6] = W[6];	ROUND_00_15(6,c,d,e,f,g,h,a,b);
			T1 = X[7] = W[7];	ROUND_00_15(7,b,c,d,e,f,g,h,a);
			T1 = X[8] = W[8];	ROUND_00_15(8,a,b,c,d,e,f,g,h);
			T1 = X[9] = W[9];	ROUND_00_15(9,h,a,b,c,d,e,f,g);
			T1 = X[10] = W[10];	ROUND_00_15(10,g,h,a,b,c,d,e,f);
			T1 = X[11] = W[11];	ROUND_00_15(11,f,g,h,a,b,c,d,e);
			T1 = X[12] = W[12];	ROUND_00_15(12,e,f,g,h,a,b,c,d);
			T1 = X[13] = W[13];	ROUND_00_15(13,d,e,f,g,h,a,b,c);
			T1 = X[14] = W[14];	ROUND_00_15(14,c,d,e,f,g,h,a,b);
			T1 = X[15] = W[15];	ROUND_00_15(15,b,c,d,e,f,g,h,a);

			data += SHA256_CBLOCK;
		}
		else
		{
			SHA_LONG l;

			HOST_c2l(data,l); T1 = X[0] = l;  ROUND_00_15(0,a,b,c,d,e,f,g,h);
			HOST_c2l(data,l); T1 = X[1] = l;  ROUND_00_15(1,h,a,b,c,d,e,f,g);
			HOST_c2l(data,l); T1 = X[2] = l;  ROUND_00_15(2,g,h,a,b,c,d,e,f);
			HOST_c2l(data,l); T1 = X[3] = l;  ROUND_00_15(3,f,g,h,a,b,c,d,e);
			HOST_c2l(data,l); T1 = X[4] = l;  ROUND_00_15(4,e,f,g,h,a,b,c,d);
			HOST_c2l(data,l); T1 = X[5] = l;  ROUND_00_15(5,d,e,f,g,h,a,b,c);
			HOST_c2l(data,l); T1 = X[6] = l;  ROUND_00_15(6,c,d,e,f,g,h,a,b);
			HOST_c2l(data,l); T1 = X[7] = l;  ROUND_00_15(7,b,c,d,e,f,g,h,a);
			HOST_c2l(data,l); T1 = X[8] = l;  ROUND_00_15(8,a,b,c,d,e,f,g,h);
			HOST_c2l(data,l); T1 = X[9] = l;  ROUND_00_15(9,h,a,b,c,d,e,f,g);
			HOST_c2l(data,l); T1 = X[10] = l; ROUND_00_15(10,g,h,a,b,c,d,e,f);
			HOST_c2l(data,l); T1 = X[11] = l; ROUND_00_15(11,f,g,h,a,b,c,d,e);
			HOST_c2l(data,l); T1 = X[12] = l; ROUND_00_15(12,e,f,g,h,a,b,c,d);
			HOST_c2l(data,l); T1 = X[13] = l; ROUND_00_15(13,d,e,f,g,h,a,b,c);
			HOST_c2l(data,l); T1 = X[14] = l; ROUND_00_15(14,c,d,e,f,g,h,a,b);
			HOST_c2l(data,l); T1 = X[15] = l; ROUND_00_15(15,b,c,d,e,f,g,h,a);
		}

		for (i=16;i<64;i+=8)
		{
			ROUND_16_63(i+0,a,b,c,d,e,f,g,h,X);
			ROUND_16_63(i+1,h,a,b,c,d,e,f,g,X);
			ROUND_16_63(i+2,g,h,a,b,c,d,e,f,X);
			ROUND_16_63(i+3,f,g,h,a,b,c,d,e,X);
			ROUND_16_63(i+4,e,f,g,h,a,b,c,d,X);
			ROUND_16_63(i+5,d,e,f,g,h,a,b,c,X);
			ROUND_16_63(i+6,c,d,e,f,g,h,a,b,X);
			ROUND_16_63(i+7,b,c,d,e,f,g,h,a,X);
		}

		ctx->h[0] += a;	ctx->h[1] += b;	ctx->h[2] += c;	ctx->h[3] += d;
		ctx->h[4] += e;	ctx->h[5] += f;	ctx->h[6] += g;	ctx->h[7] += h;

	}
}

S32 Common_Sha256_Create(Common_Sha256_T *phSha256)
{
	HASH_CTX *c = NULL;
	if (phSha256 == NULL)
	{
		return -1;
	}
	c = (HASH_CTX *)Common_Malloc(sizeof(HASH_CTX),0,__FUNCTION__,__LINE__);
	if (c == NULL)
	{
		return -1;
	}
	
	
	memset (c,0,sizeof(*c));
	c->h[0]=0x6a09e667UL;	c->h[1]=0xbb67ae85UL;
	c->h[2]=0x3c6ef372UL;	c->h[3]=0xa54ff53aUL;
	c->h[4]=0x510e527fUL;	c->h[5]=0x9b05688cUL;
	c->h[6]=0x1f83d9abUL;	c->h[7]=0x5be0cd19UL;
	c->Nl=0;	c->Nh=0;
	c->num=0;	c->md_len=SHA256_DIGEST_LENGTH;
	*phSha256 = (Common_Sha1_T)c;
	return 0;

}
S32 Common_Sha256_Append(Common_Sha256_T hSha256,U8 *pData,S32 nDataLen)
{
	const unsigned char *data=pData;
	unsigned char *p;
	HASH_LONG l;
	size_t n;
	size_t len = nDataLen;
	HASH_CTX *c = (HASH_CTX *)hSha256;
	if (c == NULL)
	{
		return -1;
	}
	if (pData == NULL || nDataLen <= 0)
	{
		return 0;
	}
	
	


	l=(c->Nl+(((HASH_LONG)len)<<3))&0xffffffffUL;
	/* 95-05-24 eay Fixed a bug with the overflow handling, thanks to
	 * Wei Dai <weidai@eskimo.com> for pointing it out. */
	if (l < c->Nl) /* overflow */
		c->Nh++;
	c->Nh+=(HASH_LONG)(len>>29);	/* might cause compiler warning on 16-bit */
	c->Nl=l;

	n = c->num;
	if (n != 0)
		{
		p=(unsigned char *)c->data;

		if (len >= HASH_CBLOCK || len+n >= HASH_CBLOCK)
			{
			memcpy (p+n,data,HASH_CBLOCK-n);
			HASH_BLOCK_DATA_ORDER (c,p,1);
			n      = HASH_CBLOCK-n;
			data  += n;
			len   -= n;
			c->num = 0;
			memset (p,0,HASH_CBLOCK);	/* keep it zeroed */
			}
		else
			{
			memcpy (p+n,data,len);
			c->num += (unsigned int)len;
			return 0;
			}
		}

	n = len/HASH_CBLOCK;
	if (n > 0)
		{
		HASH_BLOCK_DATA_ORDER (c,data,n);
		n    *= HASH_CBLOCK;
		data += n;
		len  -= n;
		}

	if (len != 0)
		{
		p = (unsigned char *)c->data;
		c->num = (unsigned int)len;
		memcpy (p,data,len);
		}
	return 0;
}
S32 Common_Sha256_Finish(Common_Sha256_T hSha256,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/)
{
	HASH_CTX *c = (HASH_CTX *)hSha256;
	unsigned char *md= NULL,md_mm[HASH_LENGTH];
	if (c == NULL)
	{
		return -1;
	}
	
	unsigned char *p = (unsigned char *)c->data;
	size_t n = c->num;

	md = md_mm;

	p[n] = 0x80; /* there is always room for one */
	n++;

	if (n > (HASH_CBLOCK-8))
	{
		memset (p+n,0,HASH_CBLOCK-n);
		n=0;
		HASH_BLOCK_DATA_ORDER (c,p,1);
	}
	memset (p+n,0,HASH_CBLOCK-8-n);

	p += HASH_CBLOCK-8;
#if   defined(DATA_ORDER_IS_BIG_ENDIAN)
	(void)HOST_l2c(c->Nh,p);
	(void)HOST_l2c(c->Nl,p);
#elif defined(DATA_ORDER_IS_LITTLE_ENDIAN)
	(void)HOST_l2c(c->Nl,p);
	(void)HOST_l2c(c->Nh,p);
#endif
	p -= HASH_CBLOCK;
	HASH_BLOCK_DATA_ORDER (c,p,1);
	c->num=0;
	memset (p,0,HASH_CBLOCK);

#ifndef HASH_MAKE_STRING
#error "HASH_MAKE_STRING must be defined!"
#else
	HASH_MAKE_STRING(c,md);
#endif
	if (pHexOut != NULL)
	{
		memcpy(pHexOut,md_mm,HASH_LENGTH);
	}
	if (pStringOut != NULL)
	{
		for(S32 i = 0;i<HASH_LENGTH;i++)
		{
			sprintf((S8 *)pStringOut + i * 2,"%02x",(U8)md_mm[i]);
		}
		pStringOut[HASH_LENGTH * 2] = 0;
	}
	
	

	return 0;
}
S32 Common_Sha256_Destroy(Common_Sha256_T *pSha256)
{
	if (pSha256 == NULL || *pSha256 == NULL)
	{
		return -1;
	}
	Common_Free(*pSha256,__FUNCTION__,__LINE__);
	*pSha256 = NULL;
	return 0;
}
S32 Common_Sha256_Simply(U8 *pData,S32 nDataLen,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/)
{
	Common_Sha256_T hSha256 = NULL;
	if(Common_Sha256_Create(&hSha256))
	{
		return -1;
	}
	if(Common_Sha256_Append(hSha256,pData,nDataLen))
	{
		Common_Sha256_Destroy(&hSha256);
		return -1;
	}
	if(Common_Sha256_Finish(hSha256,pHexOut,pStringOut))
	{
		Common_Sha256_Destroy(&hSha256);
		return -1;
	}
	Common_Sha256_Destroy(&hSha256);
	return 0;
}


// HMACSHA1
typedef struct _tagHMacSha1
{
	Common_Sha1_T hSha1;
	U8 byKey[64];

}HMacSha1_T;
S32 Common_HMacSha1_Create(Common_HMacSha1_T *phHMacSha1,U8 *pKey,S32 nKeyLen)
{
	HMacSha1_T *pMacSha = NULL;
	S32 nShaBlockSize = 64;
	S32 i;
	U8 k0xorIpad[64];
	U8 iPad = 0x36;
	// U8 oPad = 0x5C;
	if (phHMacSha1 == NULL)
	{
		return -1;
	}
	if (pKey == NULL || nKeyLen < 0)
	{
		nKeyLen = 0;
	}
	pMacSha = (HMacSha1_T *)Common_Malloc(sizeof(HMacSha1_T),0,__FUNCTION__,__LINE__);
	if (pMacSha == NULL)
	{
		return -1;
	}
	memset(pMacSha,0,sizeof(HMacSha1_T));
	if(Common_Sha1_Create(&pMacSha->hSha1))
	{
		Common_Free(pMacSha,__FUNCTION__,__LINE__);
		return -1;
	}
	//step 1
	if (nKeyLen > nShaBlockSize)
	{
		// step 2
		Common_Sha1_Simply(pKey,nKeyLen,pMacSha->byKey,NULL);
	}
	else
	{
		// step 3
		for (i = 0; i < nKeyLen;i++)
		{
			pMacSha->byKey[i] = pKey[i];
		}
	}
	// step 4
	for (i = 0; i < nShaBlockSize;i++)
	{
		k0xorIpad[i] = pMacSha->byKey[i] ^ iPad;
	}
	// step 5
	Common_Sha1_Append(pMacSha->hSha1,k0xorIpad,64);
	*phHMacSha1 = (Common_HMacSha1_T)pMacSha;
	return 0;
}

S32 Common_HMacSha1_Append(Common_HMacSha1_T hHMacSha1,U8 *pData,S32 nDataLen)
{
	HMacSha1_T *pMacSha = (HMacSha1_T *)hHMacSha1;
	if (pMacSha == NULL)
	{
		return -1;
	}
	if (pData == NULL || nDataLen <= 0)
	{
		return -1;
	}
	// step 6-1
	return Common_Sha1_Append(pMacSha->hSha1,pData,nDataLen);
}

S32 Common_HMacSha1_Finish(Common_HMacSha1_T hHMacSha1,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/)
{
	HMacSha1_T *pMacSha = (HMacSha1_T *)hHMacSha1;
	S32 i,nShaBlockSize = 64;
	U8 bykoXorOpad[64 + 20];
	U8 oPad = 0x5C;
	if (pMacSha == NULL)
	{
		return -1;
	}
	// step 6-2
	Common_Sha1_Finish(pMacSha->hSha1,bykoXorOpad + nShaBlockSize,NULL);
	// step 7
	for (i = 0; i < nShaBlockSize;i++)
	{
		bykoXorOpad[i] = pMacSha->byKey[i] ^ oPad;
	}
	Common_Sha1_Simply(bykoXorOpad,nShaBlockSize + 20,pHexOut,pStringOut);
	return 0;
}
S32 Common_HMacSha1_Destroy(Common_HMacSha1_T *phHMacSha1)
{
	HMacSha1_T *pMacSha = NULL;
	if (phHMacSha1 == NULL || *phHMacSha1 == NULL)
	{
		return -1;
	}
	pMacSha = (HMacSha1_T *)(*phHMacSha1);
	Common_Sha1_Destroy(&pMacSha->hSha1);
	Common_Free(pMacSha,__FUNCTION__,__LINE__);
	*phHMacSha1 = NULL;
	return 0;
}
S32 Common_HMacSha1_Simply(U8 *pKey,S32 nKeyLen,U8 *pData,S32 nDataLen,U8 *pHexOut/*[20]*/,U8 *pStringOut/*[41]*/)
{
	Common_HMacSha1_T hMacSha1 = NULL;
	if(Common_HMacSha1_Create(&hMacSha1,pKey,nKeyLen))
	{
		return -1;
	}
	if(Common_HMacSha1_Append(hMacSha1,pData,nDataLen))
	{
		Common_HMacSha1_Destroy(&hMacSha1);
		return -1;
	}
	if(Common_HMacSha1_Finish(hMacSha1,pHexOut,pStringOut))
	{
		Common_HMacSha1_Destroy(&hMacSha1);
		return -1;
	}
	Common_HMacSha1_Destroy(&hMacSha1);
	return 0;
}

// HMAC SHA256
typedef struct _tagHMacSha256
{
	Common_Sha256_T hSha256;
	U8 byKey[64];

}HMacSha256_T;
S32 Common_HMacSha256_Create(Common_HMacSha256_T *phHMacSha256,U8 *pKey,S32 nKeyLen)
{
	HMacSha256_T *pMacSha = NULL;
	S32 nShaBlockSize = 64;
	S32 i;
	U8 k0xorIpad[64];
	U8 iPad = 0x36;
	// U8 oPad = 0x5C;
	if (phHMacSha256 == NULL)
	{
		return -1;
	}
	if (pKey == NULL || nKeyLen < 0)
	{
		nKeyLen = 0;
	}
	pMacSha = (HMacSha256_T *)Common_Malloc(sizeof(HMacSha256_T),0,__FUNCTION__,__LINE__);
	if (pMacSha == NULL)
	{
		return -1;
	}
	memset(pMacSha,0,sizeof(HMacSha256_T));
	if(Common_Sha256_Create(&pMacSha->hSha256))
	{
		Common_Free(pMacSha,__FUNCTION__,__LINE__);
		return -1;
	}
	//step 1
	if (nKeyLen > nShaBlockSize)
	{
		// step 2
		Common_Sha256_Simply(pKey,nKeyLen,pMacSha->byKey,NULL);
	}
	else
	{
		// step 3
		for (i = 0; i < nKeyLen;i++)
		{
			pMacSha->byKey[i] = pKey[i];
		}
	}
	// step 4
	for (i = 0; i < nShaBlockSize;i++)
	{
		k0xorIpad[i] = pMacSha->byKey[i] ^ iPad;
	}
	// step 5
	Common_Sha256_Append(pMacSha->hSha256,k0xorIpad,64);
	*phHMacSha256 = (Common_HMacSha1_T)pMacSha;
	return 0;
}
S32 Common_HMacSha256_Append(Common_HMacSha256_T hHMacSha256,U8 *pData,S32 nDataLen)
{
	HMacSha256_T *pMacSha = (HMacSha256_T *)hHMacSha256;
	if (pMacSha == NULL)
	{
		return -1;
	}
	if (pData == NULL || nDataLen <= 0)
	{
		return -1;
	}
	// step 6-1
	return Common_Sha256_Append(pMacSha->hSha256,pData,nDataLen);
}
S32 Common_HMacSha256_Finish(Common_HMacSha256_T hHMacSha256,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/)
{
	HMacSha256_T *pMacSha = (HMacSha256_T *)hHMacSha256;
	S32 i,nShaBlockSize = 64;
	U8 bykoXorOpad[64 + 32];
	U8 oPad = 0x5C;
	if (pMacSha == NULL)
	{
		return -1;
	}
	// step 6-2
	Common_Sha256_Finish(pMacSha->hSha256,bykoXorOpad + nShaBlockSize,NULL);
	// step 7
	for (i = 0; i < nShaBlockSize;i++)
	{
		bykoXorOpad[i] = pMacSha->byKey[i] ^ oPad;
	}
	Common_Sha256_Simply(bykoXorOpad,nShaBlockSize + 32,pHexOut,pStringOut);
	return 0;
}
S32 Common_HMacSha256_Destroy(Common_HMacSha256_T *phHMacSha256)
{
	HMacSha256_T *pMacSha = NULL;
	if (phHMacSha256 == NULL || *phHMacSha256 == NULL)
	{
		return -1;
	}
	pMacSha = (HMacSha256_T *)(*phHMacSha256);
	Common_Sha256_Destroy(&pMacSha->hSha256);
	Common_Free(pMacSha,__FUNCTION__,__LINE__);
	*phHMacSha256 = NULL;
	return 0;
}
S32 Common_HMacSha256_Simply(U8 *pKey,S32 nKeyLen,U8 *pData,S32 nDataLen,U8 *pHexOut/*[32]*/,U8 *pStringOut/*[65]*/)
{
	Common_HMacSha256_T hMacSha256 = NULL;
	if(Common_HMacSha256_Create(&hMacSha256,pKey,nKeyLen))
	{
		return -1;
	}
	if(Common_HMacSha256_Append(hMacSha256,pData,nDataLen))
	{
		Common_HMacSha256_Destroy(&hMacSha256);
		return -1;
	}
	if(Common_HMacSha256_Finish(hMacSha256,pHexOut,pStringOut))
	{
		Common_HMacSha256_Destroy(&hMacSha256);
		return -1;
	}
	Common_HMacSha256_Destroy(&hMacSha256);
	return 0;
}
