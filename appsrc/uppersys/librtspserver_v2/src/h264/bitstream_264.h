
#ifndef _BITSTREAM_264_H_
#define _BITSTREAM_264_H_


/*****************************************************************************
 * Constants
 ****************************************************************************/
#if 1
#define BSWAP(a) { \
                unsigned int _temp0,_temp1,_temp2,_temp3,_temp4;\
                _temp1=(a & 0xFF00FF00)>>8;\
               _temp0=(a & 0x00FF00FF)<<8;\
                _temp2=_temp0+_temp1;\
                _temp3=(_temp2 & 0x0000FFFF)<<16;\
                _temp4=(_temp2 & 0xFFFF0000)>>16;\
                     a=_temp3+_temp4;\
                }
#else
#define BSWAP(a) __asm{ mov eax,a};__asm{ bswap eax}; __asm{ mov a,eax}
#endif


 
//#define BSWAP(a) __asm mov eax,a __asm bswap eax __asm mov a, eax

/* comment any #defs we dont use */

#define VIDOBJ_START_CODE		0x00000100	/* ..0x0000011f  */
#define VIDOBJLAY_START_CODE	0x00000120	/* ..0x0000012f */
#define VISOBJSEQ_START_CODE	0x000001b0
#define VISOBJSEQ_STOP_CODE		0x000001b1	/* ??? */
#define USERDATA_START_CODE		0x000001b2
#define GRPOFVOP_START_CODE		0x000001b3
/*#define VIDSESERR_ERROR_CODE  0x000001b4 */
#define VISOBJ_START_CODE		0x000001b5
#define VOP_START_CODE			0x000001b6
/*#define STUFFING_START_CODE	0x000001c3 */


#define VISOBJ_TYPE_VIDEO				1
/*#define VISOBJ_TYPE_STILLTEXTURE      2 */
/*#define VISOBJ_TYPE_MESH              3 */
/*#define VISOBJ_TYPE_FBA               4 */
/*#define VISOBJ_TYPE_3DMESH            5 */


#define VIDOBJLAY_TYPE_SIMPLE			1
/*#define VIDOBJLAY_TYPE_SIMPLE_SCALABLE    2 */
/*#define VIDOBJLAY_TYPE_CORE				3 */
/*#define VIDOBJLAY_TYPE_MAIN				4 */
/*#define VIDOBJLAY_TYPE_NBIT				5 */
/*#define VIDOBJLAY_TYPE_ANIM_TEXT			6 */
/*#define VIDOBJLAY_TYPE_ANIM_MESH			7 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FACE		8 */
/*#define VIDOBJLAY_TYPE_STILL_SCALABLE		9 */
#define VIDOBJLAY_TYPE_ART_SIMPLE		10
/*#define VIDOBJLAY_TYPE_CORE_SCALABLE		11 */
/*#define VIDOBJLAY_TYPE_ACE				12 */
/*#define VIDOBJLAY_TYPE_ADVANCED_SCALABLE_TEXTURE 13 */
/*#define VIDOBJLAY_TYPE_SIMPLE_FBA			14 */
/*#define VIDEOJLAY_TYPE_SIMPLE_STUDIO    15*/
/*#define VIDEOJLAY_TYPE_CORE_STUDIO      16*/
#define VIDOBJLAY_TYPE_ASP              17
/*#define VIDOBJLAY_TYPE_FGS              18*/


/*#define VIDOBJLAY_AR_SQUARE           1 */
/*#define VIDOBJLAY_AR_625TYPE_43       2 */
/*#define VIDOBJLAY_AR_525TYPE_43       3 */
/*#define VIDOBJLAY_AR_625TYPE_169      8 */
/*#define VIDOBJLAY_AR_525TYPE_169      9 */
#define VIDOBJLAY_AR_EXTPAR				15


#define VIDOBJLAY_SHAPE_RECTANGULAR		0
#define VIDOBJLAY_SHAPE_BINARY			1
#define VIDOBJLAY_SHAPE_BINARY_ONLY		2
#define VIDOBJLAY_SHAPE_GRAYSCALE		3


#define SPRITE_NONE		0
#define SPRITE_STATIC	1
#define SPRITE_GMC		2



#define READ_MARKER()	BitstreamSkip(bs, 1)
#define WRITE_MARKER()	BitstreamPutBit(bs, 1)

/* vop coding types  */
/* intra, prediction, backward, sprite, not_coded */
#define I_VOP	0
#define P_VOP	1
#define B_VOP	2
#define S_VOP	3
#define N_VOP	4

/* resync-specific */
#define NUMBITS_VP_RESYNC_MARKER  17
#define RESYNC_MARKER 1


#define eg_write_ue eg_write
#define eg_read_ue eg_read
#define eg_write_direct BitstreamPutBits
#define eg_write_direct1 BitstreamPutBit
#define eg_read_direct BitstreamGetBits
#define eg_read_direct1 BitstreamGetBit1
#define eg_read_skip BitstreamSkip
#define eg_show BitstreamShowBits
#define eg_init BitstreamInit
#define eg_align BitstreamPadZero
#define bs_t Bitstream
#define eg_len BitstreamLength
#define eg_flush BitstreamFlush

typedef struct
{
    unsigned int bufa;
    unsigned int bufb;
    unsigned int buf;
    unsigned int pos;
    unsigned int *tail;
    unsigned int *start;
    unsigned int *end;
    unsigned int length;
    unsigned int initpos;
}
Bitstream;
void  eg_write(Bitstream* const bs, int code_num);
/* initialise bitstream structure */
void BitstreamInit(Bitstream * const bs,
                   void *const bitstream,
                   unsigned int length);
void BitstreamSkip(Bitstream * const bs,
                   const unsigned int bits);
unsigned int BitstreamGetBits(Bitstream * const bs,
                              const unsigned int n);
unsigned int BitstreamGetBit1(Bitstream * const bs);
 int eg_read(Bitstream* const bs);
 int eg_read_se(Bitstream* const bs);





#endif /* _BITSTREAM_H_ */
