#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h> 
#include <sys/ioctl.h> 

#include "lib_encry.h"

typedef unsigned char uchar;
typedef unsigned char* puchar;


#define ENCRY_GETUZINF		0xE03
#define ENCRY_SETUZINF          0xE04
#define ENCRY_GETLOT	        0xE05

#define ENCRY_GETUZINF_ALPU     0xF03
#define ENCRY_GETLOT_ALPU       0xF05

#define SWITCH_GETPORT		0xE06

void ENThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out);
void DEThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out);

uchar deskey[24] = {
	0x79, 0x24, 0xd1, 0x5f, 0x3a, 0x9d, 0x3c, 0x6d, 
	0xc6, 0x91, 0xad, 0xd7, 0x83, 0x95, 0x17, 0xf6,
	0x81, 0xc5, 0x79, 0xd7, 0x71, 0x6c, 0x51, 0x7f,
};

static int decryption(puchar src, puchar dst, uchar DataCount)
{
	int i, j, k ,m, n;
	uchar buf1[8], buf2[8], ucDst[64], ucData[64], ucDataNum;
	if(NULL == src || NULL == dst)
		return -1;
	if(DataCount > 32)
		return -1;
	if(0 == DataCount)
		return 0;

	//¼ÆËã½âÃÜDataCount¸öÊı¾İĞèÒª¶àÉÙ¼ÓÃÜºóµÄÊı¾İ
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

static int encryption(puchar src, puchar dst, uchar DataCount)
{
	int i, j, k ,m, n;
	uchar buf1[8], buf2[8], ucRandData[32],  ucSrc[35], ucData[64], ucDataNum;
	if(NULL == src || NULL == dst)
		return -1;
	if(DataCount > 32)
		return -1;
	if(0 == DataCount)
		return 0;

	for( i = 0; i < DataCount; i++)
		ucSrc[i] = src[i];

	for(i = 0; i < 32; i++) ucRandData[i] = (uchar)rand();

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



int GetUserZoneInf(unsigned char ucUZId, unsigned char *ucpData, unsigned char ucDataCount)
{
	int fd =0, i, ret;
	unsigned char ucPSW[8] = {0xff,0xc2, 0xd4,0xC1,0xFF,0x55,0xaa,0x77}; //¿¿¿¿¿0xff¿¿¿¿¿¿¿¿¿¿8¿¿¿¿¿¿¿
	unsigned char ucSG[8] = {0xfc,0x76, 0xb5,0xaa,0xF8,0xF4,0x75,0x27};
	unsigned char ucParam[64], ucUZData[64];

	if(ucUZId > 3 || ucUZId < 2 || ucDataCount > 32)
		return -1;
	if(NULL == ucpData)
		return -1;

	fd = open("/dev/hi_encry", O_RDWR);
	if(fd < 0)
	{
		fd = open("/dev/alpu_encry", O_RDWR);
		if(fd < 0)
		{
			fd = open("/dev/hi_encry_ex", O_RDWR);
			if(fd < 0)
			{
				printf("open encry file fail \n");
				return -1;
			}
			else
			{
	   		     ret = ioctl(fd, ENCRY_GETUZINF, ucpData);
	        	     if(ret < 0)
	        	     {
	                	printf("get User info ioctl fail \n");
				close(fd);
	                	return ret;
	        	     }
/*
			     for(i=0;i<16;i++)
			     {
				printf("devinfo is %x\n",ucpData[i]);
			     }
*/
			     close(fd);
			     return 0 ;				
			}
		}
		else
		{
   		     ret = ioctl(fd, ENCRY_GETUZINF_ALPU, ucpData);
        	     if(ret < 0)
        	     {
			close(fd);
                	printf("get User info ioctl fail \n");
                	return ret;
        	     }
		     close(fd);
		     return 0 ;
		}
		return 0 ;
	}
	ucParam[0] = ucUZId;
	if(2 == ucUZId)
	{
		ucParam[1] = 3;
		ucParam[10] = 0;
	}
	else
	{
		ucParam[1] = 2;
		ucParam[10] = 1;
	}
	ucParam[19] = ucDataCount;
	for(i = 0; i < 8; i++)
	{
		ucParam[2 + i] = ucPSW[i];
		ucParam[11 + i] = ucSG[i];
	}
	
	encryption(ucParam, ucUZData, 20);
	
	ret = ioctl(fd, ENCRY_GETUZINF, ucUZData);
	if(ret < 0)
	{
		close(fd);
		printf("ioctl fail \n");
		return ret;
	}

	decryption(ucUZData, ucParam, ucDataCount);
	
	for(i = 0; i < ucDataCount; i++)
		ucpData[i] = ucParam[i];

/*	
	for(i=0;i<16;i++)
	{
		printf("devinfo is %x\n",ucpData[i]);
	}
*/
	
	close(fd);
	return 0;
}


int SetUserZoneInf(unsigned char ucUZId, unsigned char *ucpData, unsigned char ucDataCount)
{
	return 0;
}

int GetLot(unsigned char *ucpData)
{
	int i=0,fd =0, ret,devtype=0;
	unsigned char sn_tmp[8];

	fd = open("/dev/hi_encry", O_RDWR);
	if(fd < 0)
	{
		fd = open("/dev/alpu_encry", O_RDWR);
		if(fd<0)
		{
			fd = open("/dev/hi_encry_ex", O_RDWR);
			if(fd<0)
			{	
				printf("open encry file fail \n");
				return 0;
			}
			else
			{
				devtype = 2 ;	
			}
		}
		else
		{
			devtype = 1 ;
		}
	}
	else
	{
		devtype = 0 ;
	}
	if(devtype==0)
		ret = ioctl(fd, ENCRY_GETLOT, (unsigned char *)sn_tmp);
	else if(devtype==1)
		ret = ioctl(fd, ENCRY_GETLOT_ALPU, (unsigned char *)sn_tmp);
	else
		ret = ioctl(fd, ENCRY_GETLOT, (unsigned char *)sn_tmp);

	if(ret < 0)
	{
		close(fd);
		printf("get lot ioctl fail \n");
		return ret;
	}
	if(devtype==0)
	{
           ucpData[0]=sn_tmp[7];
	   ucpData[1]=sn_tmp[6];
	   ucpData[2]=sn_tmp[5];
           ucpData[3]=sn_tmp[4];
	   ucpData[4]=sn_tmp[3];
           ucpData[5]=sn_tmp[2];
           ucpData[6]=sn_tmp[1];
	   ucpData[7]=sn_tmp[0];

	}
	else if(devtype==1)
	{
	   ucpData[0]=sn_tmp[0];
	   ucpData[1]=sn_tmp[1];
           ucpData[2]=0x00;
           ucpData[3]=0x0c;
           ucpData[4]=0x69;
           ucpData[5]=sn_tmp[5];
           ucpData[6]=sn_tmp[6];
           ucpData[7]=sn_tmp[7];

	}
	else
	{
           ucpData[0]=sn_tmp[0];
	   ucpData[1]=sn_tmp[1];
	   ucpData[2]=sn_tmp[7];
           ucpData[3]=sn_tmp[6];
	   ucpData[4]=sn_tmp[5];
           ucpData[5]=sn_tmp[4];
           ucpData[6]=sn_tmp[3];
	   ucpData[7]=sn_tmp[2];		
	}

	close(fd);
	return 0;
}

int GetPortByMac(unsigned char *macaddr, unsigned int *port)
{
	int fd =0 ,i, ret;

	unsigned char ucParam[6];

	fd = open("/dev/hi_switchmac", O_RDWR);
	if(fd < 0)
	{
	    printf("open  hi_switchmac fail \n");
	}
	else
	{
	    for(i=0 ; i<6 ;i++)
	    {
                ucParam[i] = macaddr[i] ;
	    }
	    ret = ioctl(fd, SWITCH_GETPORT, ucParam);
	    if(ret < 0)
	    {
		close(fd);
		printf("ioctl fail \n");
		return ret;
	    }
            memcpy((char *)port,(char *)ucParam,sizeof(int));

	}	
	close(fd);
	return 0;
}