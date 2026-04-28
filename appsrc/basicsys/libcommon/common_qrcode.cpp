#include "libcommon_api.h"
#include "libcommon_struct.h"
#include "qrencode.h"
#pragma pack(push,1)

typedef struct tagBITMAPFILEHEADER 
{ // bmfh	 
U16	 bfType;	 
U32	 bfSize;	 
U16	 bfReserved1;	 
U16	  bfReserved2;	 
U32   bfOffBits; 
} BITMAPFILEHEADER;

typedef struct tagBITMAPINFOHEADER
{ 
// bmih     
U32  biSize;     
S32   biWidth;     
S32   biHeight;
U16   biPlanes;
U16   biBitCount;
U32  biCompression;
U32  biSizeImage;     
S32   biXPelsPerMeter; 
S32   biYPelsPerMeter; 
U32  biClrUsed;     
U32  biClrImportant; 
} BITMAPINFOHEADER; 

typedef struct tagPALETTEENTRY 
{ // pe    
U8 peRed;    
U8 peGreen;    
U8 peBlue;     
U8 peFlags; 
} PALETTEENTRY; 
#pragma pack(pop)


/**
 * QRcode class.
 * Symbol data is represented as an array contains width*width uchars.
 * Each uchar represents a module (dot). If the less significant bit of
 * the uchar is 1, the corresponding module is black. The other bits are
 * meaningless for usual applications, but here its specification is described.
 *
 * <pre>
 * MSB 76543210 LSB
 *     |||||||`- 1=black/0=white
 *     ||||||`-- data and ecc code area
 *     |||||`--- format information
 *     ||||`---- version information
 *     |||`----- timing pattern
 *     ||`------ alignment pattern
 *     |`------- finder pattern and separator
 *     `-------- non-data modules (format, timing, etc.)
 * </pre>
 */


S32 Common_QuickResponseCode(const S8 *szString,Common_QRCode_T *pPixelInfo)
{
	QRcode *pQRcode = NULL;
	U8 *pData = NULL,*pBaseData = NULL;
	int nCol = 0,nRow = 0,nOrgRow = 0;
	int nWidth,nMargin,nStride,nHeight;
	int nIdx = 0,nPos = 0,nOrder = 0,nOrgPos = 0;
	int nPixelWide,QRWidth;//QRStride;
	int nPicType = 0;//,nBitsPerPixel = 1;
	
	if(szString == NULL || pPixelInfo == NULL)
	{
		return -1;
	}
	nPicType = pPixelInfo->byPicType;
	if(nPicType  != 0 && nPicType != 1 && nPicType != 2)
	{
		return -1;
	}
	pQRcode = QRcode_encodeString(szString, 4, QR_ECLEVEL_L, QR_MODE_8, 1);
	if(pQRcode == NULL)
	{
		return -1;
	}
	nPicType = pPixelInfo->byPicType;
	nPixelWide = pPixelInfo->byPixelWide;
	nPixelWide &= 0xFF;
	if(nPixelWide == 0)
	{
		nPixelWide = 1;
	}
	
	nOrder = pPixelInfo->byBitsOrder;
	QRWidth = pQRcode->width;
	nMargin = pPixelInfo->byMargin;
	nMargin = nMargin & 0xFF;
	if((0x0000FFFF - 2 * nMargin)/ nPixelWide < QRWidth)
	{
		QRcode_free(pQRcode);
		return -1;
	}
	// QRStride = QRWidth + 2 * nMargin;
	nWidth = QRWidth * nPixelWide;
	
	nHeight = nWidth + 2 * nMargin;
	
	//printf("nHeight= %d nStride = %d QRWidth = %d nMargin= %d nPixelWide = %d\n",nHeight,nStride,QRWidth,nMargin,nPixelWide);
	if(nPicType == 0)
	{
		// nBitsPerPixel = 1;
		nStride = ((nHeight + 7) >> 3) << 3;
    	pBaseData = (U8 *)malloc(((nStride * nHeight) >> 3));
		pData = pBaseData;
		pPixelInfo->pPixelData = pBaseData;
		pPixelInfo->dwDataSize = (nStride * nHeight) >> 3;
	}
	else if(nPicType == 1)
	{
		BITMAPFILEHEADER *pHeader;
		BITMAPINFOHEADER *pInfoHeader;
		PALETTEENTRY *pPaletteentry;
		// nBitsPerPixel = 1;
		nStride = ((nHeight + 31) >> 5) << 5;
		pBaseData = (U8 *)malloc(((nStride * nHeight) >> 3) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2*sizeof(PALETTEENTRY));
		pData = pBaseData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2*sizeof(PALETTEENTRY) ;
		pHeader = (BITMAPFILEHEADER *)pBaseData;
		pInfoHeader = (BITMAPINFOHEADER *)(pHeader + 1);
		pPaletteentry = (PALETTEENTRY *)(pInfoHeader + 1);
		memset(pHeader,0,sizeof(BITMAPFILEHEADER));
		pHeader->bfType = 0x4D42;
		pPixelInfo->dwDataSize = ((nStride * nHeight) >> 3) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2*sizeof(PALETTEENTRY);
		//pHeader->bfSize = htonl(pPixelInfo->dwDataSize);
		pHeader->bfSize = pPixelInfo->dwDataSize;
		pHeader->bfOffBits =  sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 2*sizeof(PALETTEENTRY);
		memset(pInfoHeader,0,sizeof(BITMAPINFOHEADER));
		pInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
		pInfoHeader->biWidth = nHeight;
		pInfoHeader->biHeight = nHeight;
		pInfoHeader->biPlanes = 1;
		pInfoHeader->biBitCount = 1;
		pInfoHeader->biCompression = 0;
		pPaletteentry[0].peRed =0xa0;
		pPaletteentry[0].peGreen =0xa0;
		pPaletteentry[0].peBlue =0xa0;
		pPaletteentry[0].peFlags =0xFF;
		pPaletteentry[1].peRed =0x0;
		pPaletteentry[1].peGreen =0x0;
		pPaletteentry[1].peBlue =0x0;
		pPaletteentry[1].peFlags =0xFF;
		pPixelInfo->pPixelData = pBaseData;
		
		
	}
	else if(nPicType == 2)
	{// bmp bit24
		BITMAPFILEHEADER *pHeader;
		BITMAPINFOHEADER *pInfoHeader;
		// nBitsPerPixel = 24;
		nStride = ((nHeight * 24 + 31) >> 5) << 5;
		pBaseData = (U8 *)malloc(((nStride * nHeight) >> 3) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
		pData = pBaseData + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) ;
		pHeader = (BITMAPFILEHEADER *)pBaseData;
		pInfoHeader = (BITMAPINFOHEADER *)(pHeader + 1);
		memset(pHeader,0,sizeof(BITMAPFILEHEADER));
		pHeader->bfType = 0x4D42;
		pPixelInfo->dwDataSize = ((nStride * nHeight) >> 3) + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		//pHeader->bfSize = htonl(pPixelInfo->dwDataSize);
		pHeader->bfSize = pPixelInfo->dwDataSize;
		pHeader->bfOffBits =  sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
		memset(pInfoHeader,0,sizeof(BITMAPINFOHEADER));
		pInfoHeader->biSize = sizeof(BITMAPINFOHEADER);
		pInfoHeader->biWidth = nHeight;
		pInfoHeader->biHeight = nHeight;
		pInfoHeader->biPlanes = 1;
		pInfoHeader->biBitCount = 24;
		pInfoHeader->biCompression = 0;
		
		pPixelInfo->pPixelData = pBaseData;
		
		
	}
	if(pData == NULL)
	{
		QRcode_free(pQRcode);
		return -1;
	}
	memset(pData,0,((nStride * nHeight ) >> 3));
	if(nPicType == 0)
	{
		nRow = -1;
	}
	else
	{
		nRow = nHeight;
	}
	nOrgRow = -1;
	while(1)
	{
		if(nPicType == 0)
		{
			nRow++;
			if(nRow >= nHeight)
			{
				break;
			}
		}
		else
		{
			nRow--;
			if(nRow < 0)
			{
				break;
			}
		}
		nOrgRow++;
		for(nCol = 0; nCol < nHeight;nCol++)
		{
			if(nPicType == 2)
			{
				nIdx = ((nRow * nStride) >> 3 )  + nCol * 3;
			}
			else
			{
				nIdx = (nRow * nStride + nCol) >> 3;
				nPos = (nRow * nStride + nCol) & 7;
			}
				if(nRow < nMargin || 
				   nRow >= nHeight - nMargin ||
				   nCol < nMargin ||
				   nCol >= nHeight - nMargin)
				{
					// ±ß¿ò
					if(nPicType == 2)
					{
						pData[nIdx] = 0xa0; 
						pData[nIdx + 1] = 0xa0; 
						pData[nIdx + 2] = 0xa0; 
					}
				
#if 0				
					if(nOrder == 1)
					{
						pData[nIdx] &= ~(1 << nPos);
					}
					else
					{
						pData[nIdx] &= ~(1 << (7 - nPos));
					}
					//pData[nIdx] &= ~(1 << ((nOrder == 1)?nPos:(7 - nPos)));
#endif				
					
				}
				else 
				{

					nOrgPos = ((nOrgRow - nMargin)/nPixelWide) * QRWidth  + (nCol - nMargin)/nPixelWide;
					  
				
					if(pQRcode->data[nOrgPos] & 1)
					{
						//pData[nIdx] |= (1 << (nOrder == 1?nPos:(7 - nPos)));
						if(nPicType == 2)
						{
							pData[nIdx] = 0; 
							pData[nIdx + 1] = 0; 
							pData[nIdx + 2] = 0; 
							
						}
						else if(nOrder == 1)
						{
							pData[nIdx] |= (1 << nPos);
						}
						else
						{
							pData[nIdx] |= (1 << (7 - nPos));
						}
					}
					else
					{

						if(nPicType == 2)
						{
							pData[nIdx] = 0xa0; 
							pData[nIdx + 1] = 0xa0; 
							pData[nIdx + 2] = 0xa0; 
						}
						
						//pData[nIdx] &= ~(1 << (nOrder == 1?nPos:(7 - nPos)));
#if 0					
						if(nOrder == 1)
						{
							pData[nIdx] &= ~(1 << nPos);
						}
						else
						{
							pData[nIdx] &= ~(1 << (7 - nPos));
						}
#endif					
					}

					
				}
		}
	}
	
	pPixelInfo->wWidth = nWidth;
	pPixelInfo->wStride = nStride;
	pPixelInfo->byPixelWide = nPixelWide;
	QRcode_free(pQRcode);
	
	return 0;
}
