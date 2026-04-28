#include "libcommon_struct.h"
#include "libcommon_api.h"
#if 0
#include "common_font_table.h"
S32 Common_Unicode_to_Utf8(unsigned short unicode,unsigned char *pUtf8)
{
	//·µ»Ø×Ö½ÚÊý

	int nBytes = 0;
	//0x00000000 - 0x0000007F
	//0x00000080 - 0x000007FF
	//0x00000800 - 0x0000FFFF
	//0x00010000 - 0x001FFFFF
	//0x00200000 - 0x03FFFFFF
	//0x04000000 - 0x7FFFFFFF
	if (pUtf8 == NULL)
	{
		return 0;
	}

	if (unicode < 0x80)
	{
		nBytes = 1;
		pUtf8[0] = unicode & 0xFF;

	}
	else if (unicode < 0x800)
	{
		nBytes = 2;
		pUtf8[0] = (0x6 << 5) | (unicode >> 6);
		pUtf8[1] = (0x2 << 6) | (unicode & 0x3F);
	}
	else
	{
		nBytes = 3;
		pUtf8[0] = (0xE << 4) | (unicode >> 12);
		pUtf8[1] = (0x2 << 6) | ((unicode >> 6) & 0x3F);
		pUtf8[2] = (0x2 << 6) | (unicode & 0x3F);
	}
	return nBytes;
}

unsigned short Common_Utf8_to_Unicode(unsigned char *pUtf8,int *pnRetBytes)
{
	if (pUtf8 == NULL || pnRetBytes == NULL)
	{
		return 0;
	}

	int nBytes = 0, i;
	unsigned short Unicode = 0, tmpshort;

	for (i = 0; i < 8; i++)
	{
		if (pUtf8[0] & (1 << (7 - i)))
		{
			nBytes++;
		}
		else
		{
			break;
		}
	}
	if (nBytes == 1 || nBytes > 3)
	{
		*pnRetBytes = 0;
		return 0;
	}
	if (nBytes == 0)
	{
		*pnRetBytes = 1;
		return pUtf8[0];
	}
	Unicode = 0;
	for (i = 0; i < nBytes; i++)
	{
		tmpshort = pUtf8[i];
		if (i != 0 && (tmpshort & 0xC0) != 0x80)
		{
			*pnRetBytes = 0;
			return 0;
		}
		if (i == 0)
		{

			Unicode |= (tmpshort << ((nBytes - 1) * 6));
		}
		else
		{
			Unicode |= (tmpshort & 0x3F) << ((nBytes - i - 1) * 6);
		}
	}
	*pnRetBytes = nBytes;
	return Unicode;
}

unsigned short Common_Unicode_to_Gb2312(unsigned short uniCode)
{
	int idx;
	for (idx = 0; idx < MAX_UNI_INDEX; idx++)
	{
		if (COMMON_TOOL_GB_TO_UNI[idx][0] == uniCode)
		{
			return COMMON_TOOL_GB_TO_UNI[idx][1];
		}
	}
	return 0;
}

unsigned short Common_Gb2312_to_Unicode(unsigned short gb2312)
{
	int idx;
	for (idx = 0; idx < MAX_UNI_INDEX; idx++)
	{
		if (COMMON_TOOL_GB_TO_UNI[idx][1] == gb2312)
		{
			return COMMON_TOOL_GB_TO_UNI[idx][0];
		}
	}
	return 0;
}

S32 Common_Utf8_to_Gb2312_string(unsigned char *pUTF8,unsigned char *pGB2312,int nBufsize)
{
	unsigned short unicode, gb2312;
	int i, dstPos;
	int nBytes;
	if (pUTF8 == NULL || pGB2312 == NULL || nBufsize == 0)
	{
		return 0;
	}
	
	//   unsigned char strUtf8[8];
	dstPos = 0;
	i = 0;
	while (1)
	{
		if (pUTF8[i] == 0)
		{
			break;
		}
		if (dstPos >= nBufsize - 1)
		{
			break;
		}
		if (pUTF8[i] < 0x80)
		{
			if (dstPos + 1 > nBufsize - 1)
			{
				break;
			}
			pGB2312[dstPos++] = pUTF8[i++];
			continue;
		}
		unicode = Common_Utf8_to_Unicode(pUTF8 + i, &nBytes);
		if (nBytes == 0)
		{    //ÓÐ´í

			i++;
			continue;
		}
		gb2312 = Common_Unicode_to_Gb2312(unicode);
		if (gb2312 == 0)
		{

			i += nBytes;
			continue;

		}
		if (dstPos + 2 > nBufsize - 1)
		{
			break;
		}
		pGB2312[dstPos++] = gb2312 >> 8;
		pGB2312[dstPos++] = gb2312 & 0xFF;
		i += nBytes;
	}
	pGB2312[dstPos] = 0;
	return dstPos;
}

S32 Common_Gb2312_to_Utf8_String(unsigned char *pGB2312,unsigned char *pUTF8,int nBufsize)
{
	unsigned short unicode, gb2312;
	int i, dstPos, j;
	int nBytes;
	unsigned char strUtf8[8];
	if (pGB2312 == NULL || pUTF8 == NULL || nBufsize == 0)
	{
		return 0;
	}

	
	//  int nRetBytes = 0;
	dstPos = 0;
	i = 0;
	while (1)
	{
		if (pGB2312[i] == 0)
		{
			break;
		}
		if (dstPos >= nBufsize - 1)
		{
			break;
		}
		if (pGB2312[i] < 0x80)
		{
			if (dstPos + 1 > nBufsize - 1)
			{
				break;
			}
			pUTF8[dstPos++] = pGB2312[i];

			i++;
			continue;
		}

		if (pGB2312[i + 1] < 0x80)
		{    //´íÎó,¶ªµô
			i++;
			continue;
		}

		gb2312 = (pGB2312[i] << 8) | pGB2312[i + 1];
		unicode = Common_Gb2312_to_Unicode(gb2312);
		if (unicode == 0)
		{
			//´íÎó,¶ªµô
			i++;
			continue;
		}

		nBytes = Common_Unicode_to_Utf8(unicode, strUtf8);
		if (nBytes == 0)
		{
			i += 2;
			continue;
		}
		if (dstPos + nBytes > nBufsize - 1)
		{
			break;
		}
		for (j = 0; j < nBytes; j++)
		{
			pUTF8[dstPos++] = strUtf8[j];
		}

		i += 2;
	}
	pUTF8[dstPos] = 0;
	return dstPos;
}
#endif