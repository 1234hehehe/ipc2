#include <stdlib.h>
#include "des.h"
#include "update_encry.h"

#define ENCRY_GETLOT	        0xE05
#define ENCRY_GETLOT_ALPU       0xF05
#define ENCRY_GETUZINF		    0xE03
#define ENCRY_GETUZINF_ALPU     0xF03

static U8 deskey[24] = 
{
	0x79, 0x24, 0xd1, 0x5f, 0x3a, 0x9d, 0x3c, 0x6d, 
	0xc6, 0x91, 0xad, 0xd7, 0x83, 0x95, 0x17, 0xf6,
	0x81, 0xc5, 0x79, 0xd7, 0x71, 0x6c, 0x51, 0x7f,
};

static S32 decryption(U8* src, U8* dst, U8 DataCount)
{
	S32 i, j, k ,m, n;
	U8 buf1[8], buf2[8], ucDst[64], ucData[64], ucDataNum;
	if(NULL == src || NULL == dst)
		return -1;
	if(DataCount > 32)
		return -1;
	if(0 == DataCount)
		return 0;

	//计算解密DataCount个数据需要多少加密后的数据
	k = DataCount / 5;
	n = DataCount % 5;
	i = ((n) ? 1:0) + k;
	ucDataNum = i << 3;
	m = ((ucDataNum-8)%6);
	m = m ? (ucDataNum + 6 - m): ucDataNum;
	
	for(i = 0; i < m; i++)
		ucData[i] = src[i];
	
	for(i = m - 8; i >= 0; i -= 8)
	{	
		//printk("i %d \n", i);
		for(j = 0; j < 8; j++)
			buf1[j] = ucData[i + j];
		DEThreeDES(deskey, buf1, buf2);
		for(j = 0; j < 8; j++)
			ucDst[i + j] = buf2[j];
		for(j = 0; j < 2; j++)
			ucData[i + j] = buf2[j];
		i+=2;
	}
	
	for(i = 0; i <= k;  i++)
	{
		j = i * 5;
		m = i << 3;
		dst[j] = ucDst[m];
		dst[j + 1] = ucDst[m + 2];
		dst[j + 2] = ucDst[m + 3];
		dst[j + 3] = ucDst[m + 5];
		dst[j + 4] = ucDst[m + 6];
	}
	return 0;
}

static S32 encryption(U8* src, U8* dst, U8 DataCount)
{
	S32 i, j, k ,m, n;
	U8 buf1[8], buf2[8], ucRandData[32],  ucSrc[35], ucData[64], ucDataNum;
	if(NULL == src || NULL == dst)
		return -1;
	if(DataCount > 32)
		return -1;
	if(0 == DataCount)
		return 0;

	for( i = 0; i < DataCount; i++)
		ucSrc[i] = src[i];

	for(i = 0; i < 32; i++) ucRandData[i] = (U8)rand();

	/*printf("rand data ");
	for(i = 0; i < 32; i++) printf("0x%02x ", ucRandData[i]);
	printf("\n");
	*/
	i = DataCount % 5;
	i = i ? (5 - i) : i;
	for(j = i; j > 0; j --)
		ucSrc[DataCount + j - 1] = ucRandData[32 - j];
	i = ((i) ? 1:0) + DataCount / 5;
	for(j = 0; j < i; j++)
	{
		k = j << 3;
		m = j * 5;
		n = j * 3;
		ucData[k] = ucSrc[m];
		ucData[k + 1] = ucRandData[n];
		ucData[k + 2] = ucSrc[m + 1];
		ucData[k + 3] = ucSrc[m + 2];
		ucData[k + 4] = ucRandData[n + 1];
		ucData[k + 5] = ucSrc[m + 3];
		ucData[k + 6] = ucSrc[m + 4];
		ucData[k + 7] = ucRandData[n + 2];
	}
	
	ucDataNum = i << 3;
	m = ((ucDataNum-8)%6);
	m = m ? (ucDataNum + 6 - m): ucDataNum;
	for(j = ucDataNum; j < m; j++)
		ucData[j] = ucRandData[27 + ucDataNum -j];
	
	//printk("i %d ucnum %d \n", i, ucDataNum);
	for(i = 0; i < m - 2; i += 8)
	{	
		//printk("i %d \n", i);
		for(j = 0; j < 8; j++)
			buf1[j] = ucData[i + j];
		ENThreeDES(deskey, buf1, buf2);
		for(j = 0; j < 8; j++)
			dst[i + j] = buf2[j];
		for(j = 6; j < 8; j++)
			ucData[i + j] = buf2[j];
		i -= 2;
	}
	/*for(i = 0; i < m; i++) printf("0x%02x ", dst[i]);
	printf("\n");*/
	return m;
}

S32 updateEncry_GetUserZoneInf(U8 ucUZId, U8 *ucpData, U8 ucDataCount)
{
    S32 i   = 0;
	S32 fd  = 0;
    S32 ret = -1;

	U8 ucParam[64]  = {0};
    U8 ucUZData[64] = {0};

	U8 ucSG[8]  = {0xfc,0x76, 0xb5,0xaa,0xF8,0xF4,0x75,0x27};
	U8 ucPSW[8] = {0xff,0xc2, 0xd4,0xC1,0xFF,0x55,0xaa,0x77};

	if((ucUZId > 3) || (ucUZId < 2) || (ucDataCount > 32) || (NULL == ucpData))
    {
        LOGE("Input parameters is illegal.\n");
		return -1;
    }

    do
    {
        fd = open("/dev/hi_encry", O_RDWR);
        if (fd >= 0)
        {
            break;
        }

        fd = open("/dev/alpu_encry", O_RDWR);
        if (fd >= 0)
        {
            ret = ioctl(fd, ENCRY_GETUZINF_ALPU, ucpData);
            close(fd);

            if(ret < 0)
            {
                LOGE("get User info ioctl fail \n");
                return ret;
            }

            return 0;
        }

        fd = open("/dev/hi_encry_ex", O_RDWR);
        if (fd >= 0)
        {
            ret = ioctl(fd, ENCRY_GETUZINF, ucpData); 
            close(fd);

            if(ret < 0)
            {
                LOGE("get User info ioctl fail \n");
                return ret;
            }

            return 0;
		}
    }while(0);

    if (fd < 0)
    {
        LOGE("get User info ioctl fail \n");
        return -1;
    }

    ucParam[0] = ucUZId;
	if(2 == ucUZId)
	{
		ucParam[1]  = 3;
		ucParam[10] = 0;
	}
	else
	{
		ucParam[1]  = 2;
		ucParam[10] = 1;
	}

    ucParam[19] = ucDataCount;
	for(i = 0; i < 8; i++)
	{
		ucParam[2 + i]  = ucPSW[i];
		ucParam[11 + i] = ucSG[i];
	}

	encryption(ucParam, ucUZData, 20);

	ret = ioctl(fd, ENCRY_GETUZINF, ucUZData);
    close(fd);

	if(ret < 0)
	{		
		LOGE("ioctl fail \n");
		return ret;
	}

	decryption(ucUZData, ucParam, ucDataCount);
	
	for(i = 0; i < ucDataCount; i++)
    {
		ucpData[i] = ucParam[i];
    }

	return 0;
}

S32 updateEncry_GetLot(U8 *ucpData)
{
	//S32 i = 0;
    S32 fd =0;
    S32 ret = -1;
    S32 devtype = 0;
	U8 sn_tmp[8];

    enum {
        DEV_E_HI_ENCRY     = 0,
        DEV_E_ALPU_ENCRY   = 1,
        DEV_E_HI_ENCRY_EXT = 2
    };

    do
    {
        fd = open("/dev/hi_encry", O_RDWR);
        if (fd >= 0)
        {
            devtype = DEV_E_HI_ENCRY;
            ret = ioctl(fd, ENCRY_GETLOT, (U8 *)sn_tmp);
            break;
        }

        fd = open("/dev/alpu_encry", O_RDWR);
        if (fd >= 0)
        {
            devtype = DEV_E_ALPU_ENCRY;
            ret = ioctl(fd, ENCRY_GETLOT_ALPU, (U8 *)sn_tmp);
            break;
        }

        fd = open("/dev/hi_encry_ex", O_RDWR);
        if (fd >= 0)
        {
            devtype = DEV_E_HI_ENCRY_EXT;
            ret = ioctl(fd, ENCRY_GETLOT, (U8 *)sn_tmp);
            break;
        }

        LOGE("open encry file fail.\n");
		return 0;
    }while(0);
    close(fd);

    if (ret < 0)
    {
        LOGE("get lot ioctl fail \n");
		return ret;
    }

	if(DEV_E_HI_ENCRY == devtype)
    {
        ucpData[0] = sn_tmp[7];
        ucpData[1] = sn_tmp[6];
        ucpData[2] = sn_tmp[5];
        ucpData[3] = sn_tmp[4];
        ucpData[4] = sn_tmp[3];
        ucpData[5] = sn_tmp[2];
        ucpData[6] = sn_tmp[1];
        ucpData[7] = sn_tmp[0];
    }
	else if(DEV_E_ALPU_ENCRY == devtype)
    {
        ucpData[0] = sn_tmp[0];
        ucpData[1] = sn_tmp[1];
        ucpData[2] = 0x00;
        ucpData[3] = 0x0c;
        ucpData[4] = 0x69;
        ucpData[5] = sn_tmp[5];
        ucpData[6] = sn_tmp[6];
        ucpData[7] = sn_tmp[7];
    }
	else //(DEV_E_HI_ENCRY_EXT == devtype)
    {
        ucpData[0] = sn_tmp[0];
        ucpData[1] = sn_tmp[1];
        ucpData[2] = sn_tmp[7];
        ucpData[3] = sn_tmp[6];
        ucpData[4] = sn_tmp[5];
        ucpData[5] = sn_tmp[4];
        ucpData[6] = sn_tmp[3];
        ucpData[7] = sn_tmp[2];
    }

	return 0;
}


