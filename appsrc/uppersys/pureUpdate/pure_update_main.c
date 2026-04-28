#define _GNU_SOURCE         /* See feature_test_macros(7) */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#include "libcommon_api.h"
#include "update_struct.h"
#include "update_server.h"
#include "cjson.h"
#include "lib_encry.h"
#include "ovfs_wtdg.h"
#include "libupdate_api.h"
#include "update_version.h"
#include "update_broadcast.h"

#define UPDATE_FILE_ABSOLUTE_PATH  "/dev/updatefile"
#define UPDATE_TRANSMODE_TFTP     (1 << 1)
#define UPDATE_TRANSMODE_FTP      (1 << 3)
#define UPDATE_TRANSMODE_LOCAL    (1 << 7)

#define UPDATE_CUSTOMFLE "/root/res/custom/update.json"

#define NORMAL (0)
#define READY  (1)
#define TRANS  (2)

#define DUMP_UDPATE_STATUS(nStaus) \
    do \
    { \
        if (UPGRADE_STATE_FINISH == nStaus) \
        { \
            LOGD("Upgrade status[%d]: FINISH !!!\n", nStaus); \
        } \
        else if (UPGRADE_STATE_ING == nStaus) \
        { \
            LOGD("Upgrade status[%d]: UPDATING !!!\n", nStaus); \
        } \
        else if (UPGRADE_STATE_FAIL == nStaus) \
        { \
            LOGE("Upgrade status[%d]: FAILED !!!\n", nStaus); \
        } \
        else \
        { \
            LOGE("Upgrade status: UNKNOWN !!!\n", nStaus); \
        } \
    }while(0);

typedef enum
{
    UPGRADEPROCESS_E_NORMAL     = 0, //Normal
    UPGRADEPROCESS_E_READY      = 1, //装备升级
    UPGRADEPROCESS_E_TRANS      = 2, //传输数据
    UPGRADEPROCESS_E_CHECK      = 3, //校验数据
    UPGRADEPROCESS_E_ERASE      = 4, //Flash擦除
    UPGRADEPROCESS_E_WRITE      = 5, //写数据
    UPGRADEPROCESS_E_UPDATE_CFG = 6, //更新配置
    UPGRADEPROCESS_E_FINISH     = 7, //升级完成
}UpgradeProcess;

typedef enum
{
    UPGRADESTATUS_E_NORMAL = 0,
    UPGRADESTATUS_E_FAILE  = -1,
    UPGRADESTATUS_E_CANCLE = -2
}UpgradeStatus;

typedef struct _tagUpdateStatus
{
    S32 nPartial; // 0-Normal,1-准备升级,2-传输数据,3-校验数据,4-擦除flash,5-写数据,6-更新配置,7-升级完成
    S32 nProgress;
    S32 nPartProgress;
    S32 nStatus; // 0-正常,-1-失败，-2-取消、中断
    S32 nDownLoadSize;
    S32 nFailCnt;
}UpdateStatus_T;

typedef struct _tagUpdateInfo
{
    S32 nStatus; // 0:未升级;1:进入升级状态.
    S32 nTransMode;
    S8 *FileName;
    S32 nFileSize;
    S8 *nMd5sum;
    S8 *nServer;
    S32 nPort;
    S8 *nUserName;
    S8 *nPassword;
    S8  nUpdatID[64];
    //S32 nCnt;
    UpdateStatus_T nUpdateStatus;
    S32 lUpgradeHandle;
    //update_common_Lock_T  hLock;
}UpdateInfo_T;

typedef S32 (* Update_PartialProc_func)(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo);
/*extern this fucntion from common_net.c*/
extern int CreateAndListenHttp();

static Common_Lock_T  g_phLock;
static UpdateInfo_T   g_stUpdateInfo;
static UpdateMgr_T    g_stUpdateMgr;
static int		  	  g_iStartTimeCount = 0;

//web获取进度为5s一次
static int g_iupdateSuccessTime = 10; //默认升级成功后等10 S重启
int bstart = 0;

static int g_iSensorNameCustom = 0;
static char SensorName[32] = {0};

static int str2Int(S8* str)
{
	S32 value = 0;

    if ((NULL == str) || (0 == str[0]))
	{
		return 0;
	}

    sscanf(str, "%08x", &value);
	return value;
}

#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || defined PLATFORM_JZT40 || defined(PLATFORM_JZT41)
unsigned int read_reg(unsigned int reg)
{
	unsigned int val = 0;

    int fd = open("/dev/mem", (O_RDWR | O_SYNC));
    int mapped_size = 0x1000;
    int base_addr = reg & 0xFFFFF000;

    int map_base = mmap(NULL, mapped_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base_addr);
    if(map_base == MAP_FAILED)
    {
        printf("mmap failed\n");
		return 0;
    }

	val = *(volatile unsigned int*)(map_base + (reg & 0xFFF));
	printf("read_reg : 0x%x is 0x%x\n", reg, val);

    munmap(map_base, mapped_size);
    close(fd);
    return val;
}
#endif//if defined PLATFORM_JZT30 || defined PLATFORM_JZT40
int getCpuType(char *cpu, char *cpucode)
{
	unsigned cpuid = 0;
	if(cpu == NULL || cpucode == NULL)
	{
		printf("getCpuType param error\n");
		return -1;
	}
#if defined PLATFORM_JZT30
	cpuid = read_reg(0x13540238);
	switch (cpuid)
	{
	case 0x11111111:
		sprintf(cpu, "T31N");
		sprintf(cpucode, "JZC12");
		break;
	case 0x22221111:
		sprintf(cpu, "T31X");
		sprintf(cpucode, "JZC13");
		break;
	case 0x33331111:
		sprintf(cpu, "T31L");
		sprintf(cpucode, "JZC11");
		break;
	case 0x55551111:
		sprintf(cpu, "T31ZL");
		sprintf(cpucode, "JZC14");
		break;
	case 0x66661111:
		sprintf(cpu, "T31ZX");
		sprintf(cpucode, "JZC15");
		break;
	case 0x44442222:
		sprintf(cpu, "T31A");
		sprintf(cpucode, "JZC16");
		break;
	case 0xEEEE1111:
		sprintf(cpu, "T31LC");
		sprintf(cpucode, "JZC18");
		break;
	case 0xDDDD1111:
		sprintf(cpu, "T31ZC");
		sprintf(cpucode, "JZC19");
		break;
	case 0xCCCC2222:
		sprintf(cpu, "T31AL");
		sprintf(cpucode, "JZC17");
		break;
	default:
		sprintf(cpu, "UNKNOW");
		sprintf(cpucode, "UNKNOW");
		break;
	}
#elif defined PLATFORM_JZT32
	cpuid = read_reg(0x13540238);
	int cpuid_1 = read_reg(0x13540216);
	switch (cpuid)
	{
	case 0x55551111:
		sprintf(cpu, "T32ZL");
		sprintf(cpucode, "JZC1A");
		if((cpuid_1 & 0x8) != 0)
		{
			sprintf(cpu, "T32ZL_ULTRA");
			sprintf(cpucode, "JZC1A_U");
		}
		break;
	case 0x99991111:
		sprintf(cpu, "T32LQ");
		sprintf(cpucode, "JZC1A");
		if((cpuid_1 & 0x8) != 0)
		{
			sprintf(cpu, "T32ZLQ_ULTRA");
			sprintf(cpucode, "JZC1A_U");
		}
		break;
	case 0x88881111:
		sprintf(cpu, "T32LQH");
		sprintf(cpucode, "JZC1A");
		if((cpuid_1 & 0x8) != 0)
		{
			sprintf(cpu, "T32ZLQH_ULTRA");
			sprintf(cpucode, "JZC1A_U");
		}
		break;
	case 0x77772222:
		sprintf(cpu, "T32ZN");
		sprintf(cpucode, "JZC1B");
		break;
	case 0xAAAA2222:
		sprintf(cpu, "T32NQ");
		sprintf(cpucode, "JZC1B");
		break;
	default:
		sprintf(cpu, "UNKNOW");
		break;
	}
#elif defined PLATFORM_JZT33
/*		cpuid = read_reg(0x13540238);
		int cpuid_1 = read_reg(0x13540216);
		switch (cpuid)
		{
		case 0x55551111:
			sprintf(cpu, "T32ZL");
			sprintf(cpucode, "JZC1A");
			if((cpuid_1 & 0x8) != 0)
			{
				sprintf(cpu, "T32ZL_ULTRA");
				sprintf(cpucode, "JZC1A_U");
			}
			break;
		case 0x99991111:
			sprintf(cpu, "T32LQ");
			sprintf(cpucode, "JZC1A");
			if((cpuid_1 & 0x8) != 0)
			{
				sprintf(cpu, "T32ZLQ_ULTRA");
				sprintf(cpucode, "JZC1A_U");
			}
			break;
		case 0x88881111:
			sprintf(cpu, "T32LQH");
			sprintf(cpucode, "JZC1A");
			if((cpuid_1 & 0x8) != 0)
			{
				sprintf(cpu, "T32ZLQH_ULTRA");
				sprintf(cpucode, "JZC1A_U");
			}
			break;
		case 0x77772222:
			sprintf(cpu, "T32ZN");
			sprintf(cpucode, "JZC1B");
			break;
		case 0xAAAA2222:
			sprintf(cpu, "T32NQ");
			sprintf(cpucode, "JZC1B");
			break;
		default:
			sprintf(cpu, "UNKNOW");
			break;
		}*/
	sprintf(cpu, "T33L");
	sprintf(cpucode, "JZC1C");
#elif defined PLATFORM_JZT40
	cpuid = read_reg(0x13540250);
	switch (cpuid)
	{
	case 0x11111111:
		sprintf(cpu, "T40N");
		sprintf(cpucode, "JZC21");
		break;
	case 0x88881111:
		sprintf(cpu, "T40NN");
		sprintf(cpucode, "JZC23");
		break;
	case 0x77772222:
		sprintf(cpu, "T40XP");
		sprintf(cpucode, "JZC22");
		break;
	default:
		sprintf(cpu, "UNKNOW");
		break;
	}
#elif defined PLATFORM_JZT41
	cpuid = read_reg(0x13540250);
	switch (cpuid)
	{
	case 0x33331111:
		sprintf(cpu, "T41L");
		sprintf(cpucode, "JZC32");
		break;
	case 0x55551111:
		sprintf(cpu, "T41ZL");
		sprintf(cpucode, "JZC32");
		break;
	case 0x99991111:
		sprintf(cpu, "T41LQ");
		sprintf(cpucode, "JZC32");
		break;
	case 0x11112222:
		sprintf(cpu, "T41N");
		sprintf(cpucode, "JZC33");
		break;
	case 0xAAAA2222:
		sprintf(cpu, "T41NQ");
		sprintf(cpucode, "JZC33");
		break;
	case 0x77772222:
		sprintf(cpu, "T41ZN");
		sprintf(cpucode, "JZC33");
		break;
	case 0x88881111:
		sprintf(cpu, "T41LC");
		sprintf(cpucode, "JZC34");
		break;
	case 0x66662222:
		sprintf(cpu, "T41X");
		sprintf(cpucode, "JZC35");
		break;
	default:
		sprintf(cpu, "UNKNOW");
		break;
	}
#else
	printf("not support\n");
	return -1;
#endif

	return 0;
}

static UpdateInfo_T* getUpdateInfoHandle()
{
    return &g_stUpdateInfo;
}

static UpdateMgr_T* getUpdateMgrHandle()
{
    return &g_stUpdateMgr;
}

int GetHttpPort(int *port)
{
    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (pstVersionHandle != NULL)
        *port = pstVersionHandle->stUpdateVersion.lHttpPort;
    return 0;
}

int GetUpdateUpdatingStatus(int *partial, int *progress, int *status)
{
    UpdateInfo_T *pstUpdateInfoHandle = getUpdateInfoHandle();
    if (NULL == pstUpdateInfoHandle)
    {
        LOGE("The update info handle is NULL.\n");
        return -1;
    }

    if (partial == NULL || progress == NULL || status == NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }


    S32 nProgress = -1;
    if ((UPGRADEPROCESS_E_WRITE == pstUpdateInfoHandle->nUpdateStatus.nPartial)
        && (pstUpdateInfoHandle->nStatus != 0))
    {
        nProgress = UpdateServer_GetUpgradeProgress(pstUpdateInfoHandle->lUpgradeHandle);
        pstUpdateInfoHandle->nUpdateStatus.nProgress     = nProgress;//50 + nProgress / 2;
        pstUpdateInfoHandle->nUpdateStatus.nPartProgress = nProgress;
    }

    *partial = pstUpdateInfoHandle->nUpdateStatus.nPartial;
    *progress = pstUpdateInfoHandle->nUpdateStatus.nProgress;
    *status = pstUpdateInfoHandle->nUpdateStatus.nStatus;
    return 0;
}

static S32 setNetWorkCfg(S8* ifr_name,S8 *pIpAddress, S8 *pNetmask, S8 *pGateway)
{
    S8 cmd[1024] = {0};
    S8 *pGatewayTmp = NULL;

    if (NULL != pIpAddress && NULL != pNetmask &&
    1 == Common_IpAddr_IsValid(pIpAddress,0) && 1 == Common_IpAddr_IsValid(pNetmask,0))
    {
        snprintf(cmd,sizeof(cmd),"ifconfig %s %s netmask %s",ifr_name,pIpAddress,pNetmask);
        Common_System(cmd);
    }

    if (NULL != pGateway)
    {
        Common_GetLocalNetInfo(ifr_name,0,NULL,NULL,&pGatewayTmp,NULL);
        if (NULL != pGatewayTmp)
        {
            if (0 != Common_StriCmp(pGatewayTmp,pGateway))
            {
                snprintf(cmd,sizeof(cmd),"route del default dev %s",ifr_name);
                Common_System(cmd);

                snprintf(cmd,sizeof(cmd),"route add default gw %s dev %s",pGateway,ifr_name);
                Common_System(cmd);
            }
        }
        else
        {
            snprintf(cmd,sizeof(cmd),"route add default gw %s dev %s",pGateway,ifr_name);
            Common_System(cmd);
        }

        if (NULL != pGatewayTmp)
        {
            Common_Free(pGatewayTmp,__FUNCTION__,__LINE__);
            pGatewayTmp = NULL;
        }
    }

    return 0;
}

static cJSON_Struct * updateGetProc()
{
    cJSON_Struct *pJsonOut   = NULL;
    cJSON_Struct *pJsonArray = NULL;

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (NULL == pJsonOut)
    {
        LOGE("Common json new failed.\n");
        return NULL;
    }

    Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

    pJsonArray = Common_Json_SetAttrValue(pJsonOut,-1,"/Data/ResList",Common_Json_Type_Array,NULL,0,0);
    if (NULL == pJsonArray)
    {
        LOGE("Common json setAttrValue failed.\n");
        Common_Json_Delete(pJsonOut);
        return NULL;
    }

    Common_Json_SetAttrValue(pJsonArray,0,"Uri",Common_Json_Type_String,"/Update/Discovery",0,0);
    Common_Json_SetAttrValue(pJsonArray,0,"Label",Common_Json_Type_String,"Discovery",0,0);
    Common_Json_SetAttrValue(pJsonArray,0,"Describe",Common_Json_Type_String,"Broadcast to discovery devices.",0,0);
    //1
    Common_Json_SetAttrValue(pJsonArray,1,"Uri",Common_Json_Type_String,"/Update/NetConfig",0,0);
    Common_Json_SetAttrValue(pJsonArray,1,"Label",Common_Json_Type_String,"NetConfig",0,0);
    Common_Json_SetAttrValue(pJsonArray,1,"Describe",Common_Json_Type_String,"Temporarily modify network information",0,0);
    //2
    Common_Json_SetAttrValue(pJsonArray,2,"Uri",Common_Json_Type_String,"/Update/TransMode",0,0);
    Common_Json_SetAttrValue(pJsonArray,2,"Label",Common_Json_Type_String,"TransMode",0,0);
    Common_Json_SetAttrValue(pJsonArray,2,"Describe",Common_Json_Type_String,"To get TransMode bit mask:b0-Rest,b1-tftpA[active],b2-tftpP[passive],b3-ftpA,b4-ftpP,b5-privA,bit6-privB,bit7-local file",0,0);
    //3
    Common_Json_SetAttrValue(pJsonArray,3,"Uri",Common_Json_Type_String,"/Update/Start",0,0);
    Common_Json_SetAttrValue(pJsonArray,3,"Label",Common_Json_Type_String,"StartUpdate",0,0);
    Common_Json_SetAttrValue(pJsonArray,3,"Describe",Common_Json_Type_String,"Post:Config and Start update.",0,0);
    //4
    Common_Json_SetAttrValue(pJsonArray,4,"Uri",Common_Json_Type_String,"/Update/Updating/<Id>",0,0);
    Common_Json_SetAttrValue(pJsonArray,4,"Label",Common_Json_Type_String,"Updating",0,0);
    Common_Json_SetAttrValue(pJsonArray,4,"Describe",Common_Json_Type_String,"Delete:Destory update;Get:get updating status.Put: upload data.",0,0);

    //5
    Common_Json_SetAttrValue(pJsonArray,5,"Uri",Common_Json_Type_String,"/Update/SetMac",0,0);
    Common_Json_SetAttrValue(pJsonArray,5,"Label",Common_Json_Type_String,"ModifyMac",0,0);
    Common_Json_SetAttrValue(pJsonArray,5,"Describe",Common_Json_Type_String,"Modify Mac",0,0);
    //6
    Common_Json_SetAttrValue(pJsonArray,6,"Uri",Common_Json_Type_String,"/Update/SetCustomerSN",0,0);
    Common_Json_SetAttrValue(pJsonArray,6,"Label",Common_Json_Type_String,"CustomerSN",0,0);
    Common_Json_SetAttrValue(pJsonArray,6,"Describe",Common_Json_Type_String,"Modify CustomerSN",0,0);

	//7
	Common_Json_SetAttrValue(pJsonArray,7,"Uri",Common_Json_Type_String,"/Update/PrepareUpdate",0,0);
	Common_Json_SetAttrValue(pJsonArray,7,"Label",Common_Json_Type_String,"PrepareUpdate",0,0);
	Common_Json_SetAttrValue(pJsonArray,7,"Describe",Common_Json_Type_String,"Post:Prepare update.",0,0);

    return pJsonOut;
}

static cJSON_Struct * updateDiscoveryProc()
{
    cJSON_Struct *pJsonOut   = NULL;

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (NULL == pJsonOut)
    {
        LOGI("Common json new failed.\n");
        return NULL;
    }

    Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

	static int iLastReadTime = 0;
	if(abs(time(NULL) - iLastReadTime) >= 10)
	{
		iLastReadTime = time(NULL);

		Common_Lock(pstVersionHandle->phVersionLock);

		(void)update_version_LoadVer(pstVersionHandle, 1, LOADVER_FROM_CORE);

		updateLoadVer(&pstVersionHandle->stUpdateVersion);
		Common_UnLock(pstVersionHandle->phVersionLock);
	}

    if (NULL == pstVersionHandle->stUpdateVersion.szSerialNumber)
    {
        LOGE("The device serial number is NULL!!!\n");
        (void)update_version_Init(pstVersionHandle);
    }

    if (pstVersionHandle->stUpdateVersion.szSerialNumber != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut, -1,"/Data/Status", Common_Json_Type_String,
                                  (NULL == pstVersionHandle->stUpdateVersion.szStatus) ? "Unactivated" : pstVersionHandle->stUpdateVersion.szStatus, 0, 0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/SerialNumber",Common_Json_Type_String, pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
    }
    else
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Status",Common_Json_Type_String,"Unactivated",0,0);
    }

	S8 *pszMac     = NULL,*pwifiMac		= NULL;
    S8 *pIpAddress = NULL,*pwifiIpAddr 	= NULL;
    S8 *pNetmask   = NULL,*pwifiMask 	= NULL;
    S8 *pGateway   = NULL,*pwifiGateway = NULL;

    if(Common_GetLocalNetInfo("eth0", 0, &pIpAddress, &pNetmask, &pGateway, &pszMac) == 0)
    {
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Ipv4",Common_Json_Type_String,pIpAddress,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Netmask",Common_Json_Type_String,pNetmask,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Gateway",Common_Json_Type_String,pGateway,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Mac",Common_Json_Type_String,pszMac,0,0);
	}

	if(Common_GetLocalNetInfo("wlan0", 0, &pwifiIpAddr, &pwifiMask, &pwifiGateway, &pwifiMac) == 0)
	{
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Wifi",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Wifi/Ipv4",Common_Json_Type_String,pwifiIpAddr,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Wifi/Netmask",Common_Json_Type_String,pwifiMask,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Wifi/Gateway",Common_Json_Type_String,pwifiGateway,0,0);
		Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Wifi/Mac",Common_Json_Type_String,pwifiMac,0,0);
	}

    if (pstVersionHandle->stUpdateVersion.szMac != NULL)
    {
        if (pszMac != NULL)
        {
            if (0 != Common_StrCmp(pstVersionHandle->stUpdateVersion.szMac,pszMac))
            {
                Common_Free(pstVersionHandle->stUpdateVersion.szMac,__FUNCTION__,__LINE__);
                pstVersionHandle->stUpdateVersion.szMac = NULL;
            }
        }
        else
        {
            Common_Free(pstVersionHandle->stUpdateVersion.szMac,__FUNCTION__,__LINE__);
            pstVersionHandle->stUpdateVersion.szMac = NULL;
        }
    }

    if (pstVersionHandle->stUpdateVersion.szMac == NULL)
    {
        pstVersionHandle->stUpdateVersion.szMac = Common_StrDup(pszMac,__FUNCTION__,__LINE__);
    }

    Common_Json_SetAttrValue(pJsonOut,-1,"Data/System",Common_Json_Type_String,"linux",0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data/UpdatePort",Common_Json_Type_Number,NULL,10008,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data/TranMask",Common_Json_Type_Number,NULL,3,0);

    if (pstVersionHandle->lVersionOk)
    {
        Common_Lock(pstVersionHandle->phVersionLock);

        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Sid",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSid,0,0);

        //Common_Json_SetAttrValue(pJsonOut,-1,"/Data/HttpPort",Common_Json_Type_Number,NULL,80,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/HttpPort",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lHttpPort,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/HttpsPort",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lHttpsPort,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/OnvifPort",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lOnvifPort,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/OnvifAdaptiveIp",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lOnvifAdaptiveIp,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/RtspEnable",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lRtspEnable,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/RtspPort",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lRtspPort,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/RtspHttpPort",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.lRtspHttpPort,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/IotQrCode",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szIotQrCode,0,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/DeviceName",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szDeviceName,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/ProductName",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szProductName,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/DeviceType",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szDeviceType,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/DeviceTypeString",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szDeviceTypeString,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/DeviceModel",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szDeviceModel,0,0);
        //Common_Json_SetAttrValue(pJsonOut,-1,"Data/CustomerSN",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szCustomerSN,0,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Country",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szCountry,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/City",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szCity,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Web",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szWeb,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Tel",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szTel,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Copyright",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szCopyRight,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Manufacturer",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szManufacturer,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Brand",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szBrand,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Customer",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szCustomer,0,0);

		if(g_iSensorNameCustom)
			Common_Json_SetAttrValue(pJsonOut,-1,"Data/SensorModel",Common_Json_Type_String,SensorName,0,0);
		else
	        Common_Json_SetAttrValue(pJsonOut,-1,"Data/SensorModel",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSensorModel,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/IsOfDome",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.IsOfDome,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/IsOfIr",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.IsOfIr,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/LensSupport",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.nLensSupport,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/LensDrvType",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szLensDrvType,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/szLensType",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szLensType,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/IrisSupport",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.nIrisSupport,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/IrisType",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szIrisType,0,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Version",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szVersion,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/SvnNumber",Common_Json_Type_Number,NULL,pstVersionHandle->stUpdateVersion.nSvnNumber,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/HardVersion",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szHardVersion,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/BuildDate",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szBuildDate,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/ProductDate",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szProductDate,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Hardware",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szHardware,0,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/MovementVersion",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szMovementVersion,0,0);
		char cpu[32];
		char cpucode[32];
		if(getCpuType(cpu, cpucode) == 0)
	        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Cpu",Common_Json_Type_String,cpucode,0,0);

        Common_Json_SetAttrValue(pJsonOut,-1,"Data/UUID",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szUUID,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/AuthMethod",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szAuthMethod,0,0);

        Common_UnLock(pstVersionHandle->phVersionLock);
    }

    Common_Time_T LTime;
    S8 szDataTime[64];
    Common_GetLocalTime(&LTime);
    snprintf(szDataTime,63,"%04d%02d%02d%02d%02d%02d",LTime.year,LTime.month,LTime.day,LTime.hour,LTime.min,LTime.sec);
    szDataTime[63] = 0;

    Common_Json_SetAttrValue(pJsonOut,-1,"/Data/DateTime",Common_Json_Type_String,szDataTime,0,0);

    S32 runtime = 0;
    Common_GetSystemCount(&runtime,NULL);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data/RunTimeSec",Common_Json_Type_Number,NULL,runtime,0);

    int partial = 0, progress = 0, status = 0;
    GetUpdateUpdatingStatus(&partial, &progress, &status);

    if (progress == 0 && partial == 0)
    {

    }
    else
    {
        char rspStr[32] = {};

        if (status != 0)
        {
            snprintf(rspStr, sizeof(rspStr), "ErrorCode,%d",status);
        }
        else
        {
            if (progress == 0)
            {
                snprintf(rspStr, sizeof(rspStr), "%s","Preparing");
            }
            else
            {
                if (progress != 100)
                    snprintf(rspStr, sizeof(rspStr), "Upgrading,%d%%",progress);
                else
                    snprintf(rspStr, sizeof(rspStr), "%s","Finished");
            }
        }

        Common_Json_SetAttrValue(pJsonOut, -1, "/Data/UpgradeStatus", Common_Json_Type_String,
                                 rspStr, 1, 0);
    }

    if (NULL != pIpAddress)
    {
        Common_Free(pIpAddress,__FUNCTION__,__LINE__);
        pIpAddress = NULL;
    }

    if (NULL != pNetmask)
    {
        Common_Free(pNetmask,__FUNCTION__,__LINE__);
        pNetmask = NULL;
    }

    if (NULL != pGateway)
    {
        Common_Free(pGateway,__FUNCTION__,__LINE__);
        pGateway = NULL;
    }

    if (NULL != pszMac)
    {
        Common_Free(pszMac,__FUNCTION__,__LINE__);
        pszMac = NULL;
    }

	if (NULL != pwifiIpAddr)
	{
		Common_Free(pwifiIpAddr,__FUNCTION__,__LINE__);
		pwifiIpAddr = NULL;
	}

	if (NULL != pwifiMask)
	{
		Common_Free(pwifiMask,__FUNCTION__,__LINE__);
		pwifiMask = NULL;
	}

	if (NULL != pwifiGateway)
	{
		Common_Free(pwifiGateway,__FUNCTION__,__LINE__);
		pwifiGateway = NULL;
	}

	if (NULL != pwifiMac)
	{
		Common_Free(pwifiMac,__FUNCTION__,__LINE__);
		pwifiMac = NULL;
	}

    return pJsonOut;
}

S32 ModifyMac(S8 *szEth,S8 *szMac)
{
	S32 nRet  = -1;
	S32 nCode =  0;
	cJSON_Struct *pInParam  = NULL;
    cJSON_Struct *pOutParam = NULL;
    cJSON_Struct *pArray    = NULL;

	if ((NULL == szEth) || (NULL == szMac))
	{
		return -1;
	}

	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
    if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
    {
        Common_Json_SetAttrValueStr(pInParam, "Header/Uri", "/Webserver/Network/Functions/ModifyMac");
    }
    else
    {
	    Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Network/Functions/ModifyMac",0,0);
	}
    Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

    pArray = Common_Json_SetAttrValue(pInParam,-1,"/Data/MacInfo",Common_Json_Type_Array,NULL,0,0);
	if (pArray == NULL)
	{
		return -1;
	}

    Common_Json_SetAttrValue(pArray,0,"EthName",Common_Json_Type_String,szEth,0,0);
	Common_Json_SetAttrValue(pArray,0,"MacAddr",Common_Json_Type_String,szMac,0,0);

	Update_Tcp_Require("127.0.0.1",10009,pInParam,&pOutParam,6000);
	if (pOutParam != NULL)
	{
		nCode = -1;
		Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0 || nCode == 200)
		{
			nRet = 0;
		}
	}

    Common_Json_Delete(pInParam);
	Common_Json_Delete(pOutParam);
	return nRet;
}

static cJSON_Struct * updateSetOnvifDiscoveryProc(cJSON_Struct *pInParams)
{
    S32 nRet = 0;
    cJSON_Struct *pJsonOut = NULL;

    S8 *szSerialNumber    = NULL;
    S8 *szOrgSerialNumber = NULL;
    Common_Json_GetAttrValue(pInParams,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }
    szOrgSerialNumber = pstVersionHandle->stUpdateVersion.szSerialNumber;

    if ((NULL == szOrgSerialNumber) || (NULL == szSerialNumber)
        || (0 != Common_StrCmp(szOrgSerialNumber, szSerialNumber)))
    {
        nRet = -1;
    }
    else
    {
        int onvif_status = 0;
        if(Common_Json_GetAttrValueInt(pInParams, "/Data/OnvifDiscoveryStatus", &onvif_status))
        {
            if(onvif_status == 0)
            {
                if(Common_File_IsExist("/usr/etc/OnvifDiscovery_No_Response") == 0)
                {
                    Common_System("touch /usr/etc/OnvifDiscovery_No_Response");
                }
            }
            else
            {
                if(Common_File_IsExist("/usr/etc/OnvifDiscovery_No_Response"))
                {
                    Common_System("rm -r /usr/etc/OnvifDiscovery_No_Response");
                }
            }
        }
    }

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pJsonOut != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/SerialNumber",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
    }

    return pJsonOut;
}

static cJSON_Struct * updateUbootConfigProc(cJSON_Struct *pInParams)
{
    S32 nRet = 0;
    cJSON_Struct *pJsonOut = NULL;

    /*S8 *szSerialNumber    = NULL;
    S8 *szOrgSerialNumber = NULL;
    Common_Json_GetAttrValue(pInParams,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }
    szOrgSerialNumber = pstVersionHandle->stUpdateVersion.szSerialNumber;

    if ((NULL == szOrgSerialNumber) || (NULL == szSerialNumber)
        || (0 != Common_StrCmp(szOrgSerialNumber, szSerialNumber)))
    {
        nRet = -1;
    }
    else*/
    {
        int buf_size = 4096;
        char *pStr = (char *)malloc(buf_size);
        memset(pStr,0xff,buf_size);
        nRet = udpate_common_Flash_WriteMtd(1,pStr,buf_size,32*1024,0);
        free(pStr);
    }

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pJsonOut != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
        //Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        //Common_Json_SetAttrValue(pJsonOut,-1,"Data/SerialNumber",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
    }

    return pJsonOut;
}

static cJSON_Struct * updateSetSidProc(cJSON_Struct *pInParams)
{
    S32 nRet = 0;

    S8 *szSid       = NULL;
    S8 *szSerialNumber     = NULL;
    S8 *szOrgSerialNumber  = NULL;
    cJSON_Struct *pJsonOut = NULL;

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }

    Common_Json_GetAttrValue(pInParams,-1,"/Data/Sid",NULL,&szSid,NULL,NULL);
    Common_Json_GetAttrValue(pInParams,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);

    szOrgSerialNumber = pstVersionHandle->stUpdateVersion.szSerialNumber;
    LOGW("szSid:[%s] szSerialNumber:[%s] szOrgSerialNumber:[%s]\n",szSid,szSerialNumber,szOrgSerialNumber);
    if ((NULL == szOrgSerialNumber) || (NULL == szSerialNumber)
        || (0 != Common_StrCmp(szOrgSerialNumber, szSerialNumber)))
    {
        LOGE("sn error!\n");
        nRet = -1;
    }

    // TODO: PureUpdate DOES NOT SUPPORT set customer SN funciton.
    #if 1
    if (nRet == 0)
    {
        nRet = ModifySid(szSid);
        if(nRet == 0)
        {
            if(0 != Common_StrCmp(szSid, pstVersionHandle->stUpdateVersion.szSid))
            {
                Common_Free(pstVersionHandle->stUpdateVersion.szSid,NULL,0);
                pstVersionHandle->stUpdateVersion.szSid = NULL;
                pstVersionHandle->stUpdateVersion.szSid = Common_StrDup(szSid, NULL,0);
            }
        }
    }
    else
    {
        nRet = -1;
    }
    #endif

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pJsonOut != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/Sid",Common_Json_Type_String,szSid,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/SerialNumber",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
    }

    return pJsonOut;
}

static cJSON_Struct * updateSetMacProc(cJSON_Struct *pInParams)
{
    S32 nRet = 0;
    cJSON_Struct *pJsonOut = NULL;

    S8 *szEth = NULL;
    S8 *szMac = NULL;
    S8 *szSerialNumber    = NULL;
    S8 *szOrgSerialNumber = NULL;
    S8 *szSid = NULL;
    Common_Json_GetAttrValue(pInParams,-1,"/Data/EthName",NULL,&szEth,NULL,NULL);
    Common_Json_GetAttrValue(pInParams,-1,"/Data/MacAddr",NULL,&szMac,NULL,NULL);
    Common_Json_GetAttrValue(pInParams,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);
    Common_Json_GetAttrValueStr(pInParams, "/Data/Sid", &szSid);

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }
    szOrgSerialNumber = pstVersionHandle->stUpdateVersion.szSerialNumber;

    if ((NULL == szOrgSerialNumber) || (NULL == szSerialNumber)
        || (0 != Common_StrCmp(szOrgSerialNumber, szSerialNumber))
        || (NULL == szEth) || (NULL == szMac && szSid == NULL))
    {
        nRet = -1;
    }
    else
    {
        // TODO:  PureUpdate DOES NOT SUPPORT modify mac addr.
        int later = 0;
        Common_Json_GetAttrValueInt(pInParams, "DealLater", &later);

        LOGW("Mac:[%s] Sid:[%s] later:[%d]\n",szMac,szSid,later);

        if(later)
        {
            if(szSid)
            {
                cJSON_Struct *pJsonOutTmp = updateSetSidProc(pInParams);
                Common_Json_Delete(pJsonOutTmp);
                pJsonOutTmp = NULL;
            }

            if(szMac)
            {
                nRet = ModifyMac(szEth, szMac);
            }
            LOGW("nRet:[%d]\n",nRet);
        }
        else
        {
            Common_Json_SetAttrValueInt(pInParams, "DealLater", 1);
        }
    }

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pJsonOut != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Data/SerialNumber",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
    }

    return pJsonOut;
}

S32 ModifySid(S8 *szSid)
{
	S32 nRet  = -1;
	S32 nCode =  0;
	cJSON_Struct *pInParam  = NULL;
    cJSON_Struct *pOutParam = NULL;

	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Core/Sid",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pInParam,-1,"/Data/Sid",Common_Json_Type_String,szSid,0,0);

	Update_Tcp_Require("127.0.0.1",10009,pInParam,&pOutParam,3000);
	if (pOutParam != NULL)
	{
		nCode = -1;
		Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0 || nCode == 200)
		{
			nRet = 0;
		}
	}

    Common_Json_Delete(pInParam);
	Common_Json_Delete(pOutParam);
	return nRet;
}

S32 ModifyNetAttr(S8 *szIPv4,S8 *szNetmask,S8 *szGateway)
{
	S32 nCode = 0;
	cJSON_Struct *pInParam  = NULL;
    cJSON_Struct *pOutParam = NULL;
	S32 nRet = -1;

	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
    if(Common_File_IsExist("/root/lib/libnetwork_sdk.so"))
    {
        Common_Json_SetAttrValueStr(pInParam, "Header/Uri", "/Webserver/Network/NetAttr/Eth/0");
    }
    else
    {
	    Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Network/NetAttr/Eth/0",0,0);
	}
    Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"/Data/EthName",Common_Json_Type_String,"eth0",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"/Data/EnableDhcp",Common_Json_Type_Number,NULL,0,0);

    if (szIPv4 != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"/Data/IpAddrV4",Common_Json_Type_String,szIPv4,0,0);
	}

    if (szNetmask != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"/Data/IpMaskV4",Common_Json_Type_String,szNetmask,0,0);
	}

    if (szGateway != NULL)
	{
		Common_Json_SetAttrValue(pInParam,-1,"/Data/GatewayV4",Common_Json_Type_String,szGateway,0,0);
	}

    Update_Tcp_Require("127.0.0.1", 10009, pInParam, &pOutParam, 3000);
	if (pOutParam != NULL)
	{
		nCode = -1;
		Common_Json_GetAttrValue(pOutParam,-1,"/Header/Code",NULL,NULL,&nCode,NULL);
		if (nCode == 0 || nCode == 200)
		{
			nRet = 0;
		}
	}

    Common_Json_Delete(pInParam);
	Common_Json_Delete(pOutParam);
	return nRet;
}

static cJSON_Struct * updateNetConfigProc(char *ifr_name,cJSON_Struct *pInParams)
{
    S8 *pszMac      = NULL;
    S8 *pIpAddress  = NULL;
    S8 *pNetmask    = NULL;
    S8 *pGateway    = NULL;
    S8 *pszMacTmp   = NULL;
    S8 *pnPermanent = NULL;
    S32 nPermanent  = 0;
    LOGW("TupdateNetConfigProc\n");

    cJSON_Struct *pJsonOut = NULL;
    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }

    do
    {
        Common_Json_GetAttrValue(pInParams,-1,"/Data/Mac",NULL,&pszMacTmp,NULL,NULL);
        if (NULL == pszMacTmp)
        {
            break;
        }

        Common_GetLocalNetInfo(ifr_name,0,NULL,NULL,NULL,&pszMac);
        if (NULL == pszMac)
        {
            break;
        }
		LOGI("pszMacTmp=%s,pszMac=%s\n",pszMacTmp,pszMac);

        if (0 != Common_StriCmp(pszMacTmp,pszMac))
        {
            if (NULL != pszMac)
            {
                Common_Free(pszMac,__FUNCTION__,__LINE__);
                pszMac = NULL;
            }
            break;
        }

		if (NULL != pszMac)
        {
            Common_Free(pszMac, __FUNCTION__, __LINE__);
            pszMac = NULL;
        }

        Common_Json_GetAttrValue(pInParams,-1,"/Header/Permanent",NULL,&pnPermanent,NULL,NULL);
        Common_Json_GetAttrValue(pInParams,-1,"/Data/Ipv4",NULL,&pIpAddress,NULL,NULL);
        Common_Json_GetAttrValue(pInParams,-1,"/Data/Netmask",NULL,&pNetmask,NULL,NULL);
        Common_Json_GetAttrValue(pInParams,-1,"/Data/Gateway",NULL,&pGateway,NULL,NULL);

        if (pnPermanent != NULL)
        {
            nPermanent = atoi(pnPermanent);
        }

        // TODO: PureUpdate 只支持修改有线网卡eth0
        if ((nPermanent) && (strcmp(ifr_name,"eth0") == 0))
        {
        	LOGI("nPermanent=%d,setNetWorkCfg pIpAddress=%s,pNetmask=%s,pGateway=%s\n",nPermanent,pIpAddress, pNetmask, pGateway);
            if(0 != ModifyNetAttr(pIpAddress,pNetmask,pGateway))
            {
                nPermanent = 0;
            }
        }

        if (!nPermanent)
        {
        	LOGI("setNetWorkCfg[%s] pIpAddress=%s,pNetmask=%s,pGateway=%s\n",ifr_name,pIpAddress, pNetmask, pGateway);
            (void)setNetWorkCfg(ifr_name,pIpAddress, pNetmask, pGateway);
        }

        pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/SendType",Common_Json_Type_String,"Broadcast",0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

        if (pstVersionHandle->stUpdateVersion.szSerialNumber != NULL)
        {
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Status",Common_Json_Type_String,"Activated",0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/SerialNumber",Common_Json_Type_String,pstVersionHandle->stUpdateVersion.szSerialNumber,0,0);
        }
        else
        {
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Status",Common_Json_Type_String,"Unactivated",0,0);
        }

        pIpAddress = NULL;
        pNetmask   = NULL;
        pGateway   = NULL;

        Common_GetLocalNetInfo("eth0", 0, &pIpAddress, &pNetmask, &pGateway, &pszMac);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Ipv4",Common_Json_Type_String,pIpAddress,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Netmask",Common_Json_Type_String,pNetmask,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Gateway",Common_Json_Type_String,pGateway,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Mac",Common_Json_Type_String,pszMac,0,0);

        if (NULL != pIpAddress)
        {
            Common_Free(pIpAddress,__FUNCTION__,__LINE__);
            pIpAddress = NULL;
        }

        if (NULL != pNetmask)
        {
            Common_Free(pNetmask,__FUNCTION__,__LINE__);
            pNetmask = NULL;
        }

        if (NULL != pGateway)
        {
            Common_Free(pGateway,__FUNCTION__,__LINE__);
            pGateway = NULL;
        }

        if (NULL != pszMac)
        {
            Common_Free(pszMac,__FUNCTION__,__LINE__);
            pszMac = NULL;
        }
    }while(0);

    return pJsonOut;
}

static cJSON_Struct * updateTransModeProc()
{
    S32 bTransModeMask = 0;
    cJSON_Struct *pJsonOut = NULL;

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

    bTransModeMask = (1 << 1) | (1 << 7) | (1 << 3);
    Common_Json_SetAttrValue(pJsonOut,-1,"/Data/TransModeMask",Common_Json_Type_Number,NULL,bTransModeMask,0);

    return pJsonOut;
}

static cJSON_Struct * clearIpTable(cJSON_Struct *pInParams)
{
    S32 nRet = 0;
    cJSON_Struct *pJsonOut = NULL;

    S8 *szSerialNumber    = NULL;
    S8 *szOrgSerialNumber = NULL;
    Common_Json_GetAttrValue(pInParams,-1,"/Data/SerialNumber",NULL,&szSerialNumber,NULL,NULL);

    DeviceVersion_S *pstVersionHandle = update_version_GetHandle();
    if (NULL == pstVersionHandle)
    {
        LOGE("The version handle is NULL.\n");
        return NULL;
    }
    szOrgSerialNumber = pstVersionHandle->stUpdateVersion.szSerialNumber;

    if ((NULL == szOrgSerialNumber) || (NULL == szSerialNumber)
        || (0 != Common_StrCmp(szOrgSerialNumber, szSerialNumber)))
    {
    	LOGE("szOrgSerialNumber=%s szSerialNumber=%s\n", szOrgSerialNumber, szSerialNumber);
        nRet = -1;
    }
    else
    {
        if(g_iStartTimeCount >= 3*60)
		{
			nRet = -1;
    		LOGE("\n");
		}
		else
		{
			Common_System("iptables -F");
			Common_System("iptables -P INPUT ACCEPT");
		}
    }

    pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pJsonOut != NULL)
    {
        Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
    }

    return pJsonOut;
}


static cJSON_Struct * updateStartProc(cJSON_Struct *pInParams)
{
    static S32 nCnt = 0;

    S32 nSec  = 0;
    S32 nMSec = 0;
    S32 nID   = 0;
    S32 nRestore = 0;
    S8 *pStringValue = NULL;

    cJSON_Struct *pJsonOut = NULL;

    UpdateInfo_T *pstUpdateInfoHandle = getUpdateInfoHandle();
    if (NULL == pstUpdateInfoHandle)
    {
        LOGE("The update info handle is NULL.\n");
        return NULL;
    }
    LOGI("update Info:nStatus = %d\n", pstUpdateInfoHandle->nStatus);

    Common_Lock(g_phLock);

    if (pstUpdateInfoHandle->nStatus != 0)
    {
        LOGE("Updating[%d]!!!!\n", pstUpdateInfoHandle->nUpdatID);
    }
    else
    {
        memset((void *)pstUpdateInfoHandle, 0x00, sizeof(UpdateInfo_T));
        pstUpdateInfoHandle->nTransMode     = -1;
        pstUpdateInfoHandle->lUpgradeHandle = -1;

        Common_Json_GetAttrValue(pInParams, -1, "/Data/TransMode", NULL, NULL, &pstUpdateInfoHandle->nTransMode, NULL);

		int iTime = -1;
		Common_Json_GetAttrValue(pInParams, -1, "/Data/RebootTime", NULL, NULL, &iTime, NULL);
		if(iTime != -1)
		{
			if(iTime < 6 || iTime > 20)
			{
				iTime = 20;
			}
			g_iupdateSuccessTime = iTime;
		}

        if(Common_Json_GetAttrValueInt(pInParams, "/Data/NeedRestore", &nRestore))
        {
            if(nRestore)
            {
                Common_System("touch /tmp/updating_restore");
            }
        }

        if ((pstUpdateInfoHandle->nTransMode != (1 << 1))
            && (pstUpdateInfoHandle->nTransMode != (1 << 7))
            && (pstUpdateInfoHandle->nTransMode != (1 << 3)))
        {
            LOGE("nTransMode[%d] not support!!!!\n", pstUpdateInfoHandle->nTransMode);
        }
        else
        {
            pStringValue = NULL;
            Common_Json_GetAttrValue(pInParams, -1, "/Data/Md5sum", NULL, &pStringValue, NULL, NULL);
            if (pStringValue != NULL)
            {
                pstUpdateInfoHandle->nMd5sum = Common_StrDup(pStringValue, __FUNCTION__, __LINE__);
            }

            pStringValue = NULL;
            Common_Json_GetAttrValue(pInParams, -1, "/Data/FilePath", NULL, &pStringValue, NULL, NULL);
            if (pStringValue != NULL)
            {
                pstUpdateInfoHandle->FileName = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
            }

            Common_Json_GetAttrValue(pInParams,-1,"/Data/FileSize", NULL, NULL, &pstUpdateInfoHandle->nFileSize, NULL);

            pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Timeout",Common_Json_Type_Number,NULL,60,0);
            Common_GetCurrentTime(&nSec,&nMSec);
            nCnt++;
            nCnt &= 0xFF;
            nID = ((nSec & 0x7FFF) << 16) | nCnt;
            snprintf(pstUpdateInfoHandle->nUpdatID, sizeof(pstUpdateInfoHandle->nUpdatID), "/Update/Updating/%d", nID);
            Common_Json_SetAttrValue(pJsonOut, -1, "/Data/Uri", Common_Json_Type_String, pstUpdateInfoHandle->nUpdatID, 0, 0);

            if (UPDATE_TRANSMODE_TFTP == pstUpdateInfoHandle->nTransMode)
            {
                pStringValue = NULL;
                Common_Json_GetAttrValue(pInParams,-1,"/Data/TftpServer",NULL,&pStringValue,NULL,NULL);
                if (pStringValue != NULL)
                {
                    pstUpdateInfoHandle->nServer = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
                }

                Common_Json_GetAttrValue(pInParams,-1,"/Data/TftpPort",NULL,NULL,&pstUpdateInfoHandle->nPort,NULL);
            }
            else if (UPDATE_TRANSMODE_FTP == pstUpdateInfoHandle->nTransMode)
            {
                pStringValue = NULL;
                Common_Json_GetAttrValue(pInParams,-1,"/Data/FtpServer",NULL,&pStringValue,NULL,NULL);
                if (pStringValue != NULL)
                {
                    pstUpdateInfoHandle->nServer = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
                }

                Common_Json_GetAttrValue(pInParams,-1,"/Data/FtpPort",NULL,NULL,&pstUpdateInfoHandle->nPort,NULL);

                pStringValue = NULL;
                Common_Json_GetAttrValue(pInParams,-1,"/Data/UserName",NULL,&pStringValue,NULL,NULL);
                if (pStringValue != NULL)
                {
                    pstUpdateInfoHandle->nUserName = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
                }

                pStringValue = NULL;
                Common_Json_GetAttrValue(pInParams,-1,"/Data/Password",NULL,&pStringValue,NULL,NULL);
                if (pStringValue != NULL)
                {
                    pstUpdateInfoHandle->nPassword = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);
                }
            }
            else if (UPDATE_TRANSMODE_LOCAL == pstUpdateInfoHandle->nTransMode)
            {
            }
#if DEBUG
            LOGI("FileName=%s, server=%s, username=%s, password=%s, nPort=%d.\n", pstUpdateInfoHandle->FileName,
                                                                                  pstUpdateInfoHandle->nServer,
                                                                                  pstUpdateInfoHandle->nUserName,
                                                                                  pstUpdateInfoHandle->nPassword,
                                                                                  pstUpdateInfoHandle->nPort);
#endif
            char* out = NULL;
            LOGI("pJsonOut:%s \n", out = Common_cJSON_PrintUnformatted((Common_cJSON_T*)pJsonOut, NULL));
            if(out)
            {
                Common_Free(out,__FUNCTION__,__LINE__);
            }

            // 关闭看门狗
            //Wtdg_Stop();
            Wtdg_Feed();
            pstUpdateInfoHandle->nStatus = 1;
        }
    }

    Common_UnLock(g_phLock);
    return pJsonOut;
}

static cJSON_Struct * updateUpdatingProc(S8 *szMethod, S8 *pUri, S32 *plRet)
{
    cJSON_Struct *pJsonOut = NULL;

    UpdateInfo_T *pstUpdateInfoHandle = getUpdateInfoHandle();
    if (NULL == pstUpdateInfoHandle)
    {
        LOGE("The update info handle is NULL.\n");
        return NULL;
    }

    Common_Lock(g_phLock);

    if (0 == Common_StrniCmp(pUri, pstUpdateInfoHandle->nUpdatID, strlen(pstUpdateInfoHandle->nUpdatID)))
    {
        if (0 == Common_StriCmp(szMethod, "delete"))
        {
            if ((UPGRADEPROCESS_E_FINISH == pstUpdateInfoHandle->nUpdateStatus.nPartial) && (0 == pstUpdateInfoHandle->nStatus))
            {
                //UpdateInfoFree(0);
                *plRet = 0;
            }
            else
            {
                pstUpdateInfoHandle->nUpdateStatus.nStatus = -2;
                *plRet = -103;
            }

            // 开启看门狗
            Wtdg_Start();
        }
        else if (0 == Common_StriCmp(szMethod, "get"))
        {
            S32 nProgress = -1;

            pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);

            if ((UPGRADEPROCESS_E_WRITE == pstUpdateInfoHandle->nUpdateStatus.nPartial) && (pstUpdateInfoHandle->nStatus != 0))
            {
                nProgress = UpdateServer_GetUpgradeProgress(pstUpdateInfoHandle->lUpgradeHandle);
                pstUpdateInfoHandle->nUpdateStatus.nProgress     = nProgress;//50 + nProgress / 2;
                pstUpdateInfoHandle->nUpdateStatus.nPartProgress = nProgress;
            }

            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Partial",Common_Json_Type_Number,NULL,pstUpdateInfoHandle->nUpdateStatus.nPartial,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Progress",Common_Json_Type_Number,NULL,pstUpdateInfoHandle->nUpdateStatus.nProgress,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/PartProgress",Common_Json_Type_Number,NULL,pstUpdateInfoHandle->nUpdateStatus.nPartProgress,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"/Data/Status",Common_Json_Type_Number,NULL,pstUpdateInfoHandle->nUpdateStatus.nStatus,0);
        }
        else if (0 == Common_StriCmp(szMethod,"put"))
        {
        }
    }

    Common_UnLock(g_phLock);
    return pJsonOut;
}

static int GetMemFree()
{
    FILE *fp = fopen("/proc/meminfo", "rb");
    if (fp == NULL)
        return -1;

    char lineBuf[128] = { 0 };
    char tmp[32] = { 0 };
    int freeMem = 0;
    fgets(lineBuf, sizeof(lineBuf), fp);
    fgets(lineBuf, sizeof(lineBuf), fp);
    sscanf(lineBuf, "%*[^0-9]%30[0-9]", tmp);
    freeMem = atoi(tmp);
    fclose(fp);

    return freeMem;
}


static void killProcess();

static S32 updateCallFunctions(char *ifr_name,UpdateClientInfo_T *pClientInfo,
                                         cJSON_Struct *pInParams,
                                         cJSON_Struct **pOutParams,
                                         void *pUserData)
{
    S32 nRet = -1;
    S8 *pUri = NULL;
    S8 *szMethod = NULL;
    cJSON_Struct *pJsonOut = NULL;

    if ((pInParams == NULL) || (NULL == pOutParams))
    {
        LOGE("Enter params is NULL.\n");
        return -1;
    }
    else
    {
        #if 0
        char* out = NULL;
        LOGD("recv call input:%s \n",out = Common_cJSON_PrintUnformatted((Common_cJSON_T*)pInParams,NULL));
        if(out)
        {
            Common_Free(out,__FUNCTION__,__LINE__);
        }
        #endif
    }

    // TODO:  Update_crypto_CallFunctions优化
    if (0 == update_crypto_CallFunctions(pClientInfo,pInParams,pOutParams,pUserData))
    {
        return 0;
    }

    Common_Json_GetAttrValue(pInParams,-1,"/Header/Uri",NULL,&pUri,NULL,NULL);
    if (NULL == pUri)
    {
        return -1;
    }
    Common_Json_GetAttrValue(pInParams,-1,"/Header/Method",NULL,&szMethod,NULL,NULL);
	//if(bstart)
    //Common_Json_StandardPrint(pInParams, "JsonIn <","> \n",NULL);

    if ((0 == Common_StriCmp(pUri,"/Update") || 0 == Common_StriCmp(pUri,"/Update/"))
        && 0 == Common_StriCmp(szMethod,"get"))
    {
        pJsonOut = updateGetProc();
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/Discovery")) && (0 == Common_StriCmp(szMethod,"get")))
    {
		if(!bstart)
        pJsonOut = updateDiscoveryProc();
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/SetOnvifDiscovery")) && (0 == Common_StriCmp(szMethod,"put")))
    {
		if(!bstart)
        pJsonOut = updateSetOnvifDiscoveryProc(pInParams);
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/SetMac")) && (0 == Common_StriCmp(szMethod,"put")))
    {
		if(!bstart)
        pJsonOut = updateSetMacProc(pInParams);
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/SetSid")) && (0 == Common_StriCmp(szMethod,"put")))
    {
		if(!bstart)
        pJsonOut = updateSetSidProc(pInParams);
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/NetConfig")) && (0 == Common_StriCmp(szMethod,"put")))
    {
		if(!bstart)
        pJsonOut = updateNetConfigProc(ifr_name,pInParams);
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/TransMode")) && (0 == Common_StriCmp(szMethod,"get")))
    {
        pJsonOut = updateTransModeProc();
    }
	else if ( 0== Common_StriCmp(pUri,"/Update/PrepareUpdate"))
	{
		//Wtdg_Stop();
		Wtdg_Feed();
		Common_System("touch /tmp/updating");
		killProcess();
		LOGW("/Update/PrepareUpdate\n");
		nRet = 0;

		pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Header/Code",Common_Json_Type_Number,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pJsonOut,-1,"/Data/FreeSize",Common_Json_Type_Number,NULL,GetMemFree(),0);
	}
    else if ((0 == Common_StriCmp(pUri,"/Update/Start")) && (0 == Common_StriCmp(szMethod,"post")))
    {
    	Common_System("touch /tmp/updating");
    	Common_System("echo 0 > /sys/devices/platform/jz-sfc/sfc_set_global;");
        pJsonOut = updateStartProc(pInParams);
        LOGW("/Update/Start\n");
    }
    else if (0 == Common_StrniCmp(pUri,"/Update/Updating",strlen("/Update/Updating")))
    {
        pJsonOut = updateUpdatingProc(szMethod, pUri, &nRet);
        LOGD("%s\n",Common_Json_Print(pJsonOut,NULL));
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/ClearIpTable")) && (0 == Common_StriCmp(szMethod,"post")))
    {
        pJsonOut = clearIpTable(pInParams);
        LOGW("clearIpTable\n");
    }
    else if ((0 == Common_StriCmp(pUri,"/Update/UbootConfig")) && (0 == Common_StriCmp(szMethod,"put")))
    {
		if(!bstart)
        pJsonOut = updateUbootConfigProc(pInParams);
    }
    else
    {
        LOGE("Input uri error.\n");
        Common_Json_StandardPrint(pInParams,"Update <","> \n",NULL);
    }

    if (NULL == pJsonOut && !bstart)
    {
        pJsonOut = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
        if (pJsonOut != NULL)
        {
            Common_Json_SetAttrValue(pJsonOut,-1,"Header",Common_Json_Type_Object,NULL,0,0);
            Common_Json_SetAttrValue(pJsonOut,-1,"Header/Code",Common_Json_Type_Number,NULL,nRet,0);
        }
    }

    if (NULL == pJsonOut)
    {
        LOGE("Output json is NULL.\n");
        return -1;
    }

	//if(bstart)
    //Common_Json_StandardPrint(pJsonOut, "JsonOut <","> \n",NULL);

    *pOutParams = pJsonOut;
    return 0;
}

static void initUpdateInfo(UpdateInfo_T *pstUpdateInfo)
{
    memset((void *)pstUpdateInfo, 0x00, sizeof(UpdateInfo_T));

    pstUpdateInfo->nTransMode     = -1;
    pstUpdateInfo->lUpgradeHandle = -1;
}

static void initUpdateMgr(UpdateMgr_T *pstUpdateMgr, Update_CallFunctions_Def pFnx)
{
    memset((void *)pstUpdateMgr, 0, sizeof(UpdateMgr_T));
    pstUpdateMgr->nListenPort_Tcp = 10008;
    pstUpdateMgr->nListenPort_Udp = 10008;
    pstUpdateMgr->fCallback = pFnx;
    Common_Lock_Create(&pstUpdateMgr->tDigestLock, "Digest lock");
}

static S32 getFileSize(S8 *strFileName)
{
    struct stat temp;

    if (0 == stat(strFileName, &temp))
    {
        return temp.st_size;
    }

    return 0;
}

static S32 updateInfoFree(UpdateInfo_T *pstUpdateInfo, S32 nStatus)
{
   if (pstUpdateInfo->FileName != NULL)
   {
        Common_Free(pstUpdateInfo->FileName,__FUNCTION__,__LINE__);
        pstUpdateInfo->FileName = NULL;
   }

   if (pstUpdateInfo->nMd5sum != NULL)
   {
        Common_Free(pstUpdateInfo->nMd5sum,__FUNCTION__,__LINE__);
        pstUpdateInfo->nMd5sum = NULL;
   }

   if (pstUpdateInfo->nServer != NULL)
   {
        Common_Free(pstUpdateInfo->nServer,__FUNCTION__,__LINE__);
        pstUpdateInfo->nServer = NULL;
   }

   if (pstUpdateInfo->nUserName != NULL)
   {
        Common_Free(pstUpdateInfo->nUserName,__FUNCTION__,__LINE__);
        pstUpdateInfo->nUserName = NULL;
   }

   if (pstUpdateInfo->nPassword != NULL)
   {
        Common_Free(pstUpdateInfo->nPassword,__FUNCTION__,__LINE__);
        pstUpdateInfo->nPassword = NULL;
   }

   if (pstUpdateInfo->lUpgradeHandle != -1)
   {
       UpdateServer_CloseUpgradeHandle(pstUpdateInfo->lUpgradeHandle);
       pstUpdateInfo->lUpgradeHandle = -1;
   }

   pstUpdateInfo->nStatus = 0;
   pstUpdateInfo->nUpdateStatus.nStatus = nStatus;
   Wtdg_Start();

   return 0;
}

static void killProcess()
{
    LOGD("[Update] kill some modules, ready to update\n");
    if (access("/root/killone.sh", F_OK) == 0)
    {
        Common_System("touch /tmp/core_debug;/root/killone.sh");
    }
    else
    {
        Common_System("touch /tmp/core_debug;pkill -9 udevd;\
                       pkill -9 syslogd;\
                       killall -9 appinstall;\
                       pkill -9 ovfs_ptz;\
                       pkill -9 ovfs_network;\
                       pkill -9 ovfs_onvif;\
                       pkill -9 ovfs_record;\
                       pkill -9 ovfs_event;\
                       pkill -9 ovfs_filemanage;\
                       pkill -9 ovfs_smartserver;\
                       pkill -9 ovfs_alarm;\
                       pkill -9 ovfs_access_host;\
                       pkill -9 ovfs_mediaserver;\
                       pkill -9 ovfs_websocket;\
                       pkill -9 ovfs_person_record;\
                       pkill -9 ovfs_zkdc;\
                       pkill -9 ovfs_gyro;\
                       pkill -9 ovfs_board;\
                       killall udhcpc;\
                       killall -9 snmptrap.sh;");

        Common_System("killall auto.sh;pkill -9 Alpr");
        Common_System("for i in $(ipcs -m | grep 0xbeb | awk -F ' ' '{print $2}');do ipcrm -m $i;done");
        /* system("pkill -9 webserver ;/root/nginx/sbin/nginx -p /root/nginx -s quit;"); */
        /* CreateAndListenHttp(); */
        /* Common_System("pkill -9 telnetd"); */
        /* Common_System("pkill -9 ovfs"); */
        /* Common_System("pkill -9 auto"); */
        /* Common_System("pkill -9 nginx"); */
        /* Common_System("pkill -9 sysinit"); */
        /* Common_System("killall -9 appinstall"); */
    }
#if defined PLATFORM_HI3516CV500
    Common_System("rm /root/bin/*fs*;rm /root/bin/wpa*;rm /root/bin/host*;rm /root/lib/*;rm /root/*;rm -fr /root/ko;rm -fr /root/nginx;rm -fr /tmp/smartsnap/;rm -fr /var/ebike_snap_tmp;");
#endif
    Common_Sleep(2, 0); //sleep(1)
    Common_System("sysctl -w vm.drop_caches=3;sync;free;");
	bstart = 1;
}

static S32 updateNormalProc(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    S8 cmd[128] = {0};
    memset((void *)cmd, 0x00, sizeof(cmd));

    pstVersion = pstVersion;

    killProcess();
    pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_READY;

    if (pstUpdateInfo->nTransMode == (1 << 1))
    {
        if (pstUpdateInfo->FileName != NULL && pstUpdateInfo->nServer != NULL)
        {
            if (pstUpdateInfo->nPort > 0)
            {
                snprintf(cmd, sizeof(cmd), "tftp -g -l %s -r %s %s %d",
                                           UPDATE_FILE_ABSOLUTE_PATH,
                                           pstUpdateInfo->FileName,
                                           pstUpdateInfo->nServer,
                                           pstUpdateInfo->nPort);
            }
            else
            {
                snprintf(cmd,sizeof(cmd), "tftp -g -l %s -r %s %s",
                                          UPDATE_FILE_ABSOLUTE_PATH,
                                          pstUpdateInfo->FileName,
                                          pstUpdateInfo->nServer);
            }
            Common_System(cmd);

            pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_TRANS;
        }
        else
        {
            updateInfoFree(pstUpdateInfo, -1);
            return 0;
        }
    }
    else if ((1 << 3) == pstUpdateInfo->nTransMode)
    {
#if DEBUG
        LOGI("FileName=%s, server=%s, username=%s, password=%s, nPort=%d.\n", pstUpdateInfo->FileName,
                                                                              pstUpdateInfo->nServer,
                                                                              pstUpdateInfo->nUserName,
                                                                              pstUpdateInfo->nPassword,
                                                                              pstUpdateInfo->nPort);
#endif
        if (pstUpdateInfo->FileName != NULL && pstUpdateInfo->nServer != NULL)
        {
            if (pstUpdateInfo->nUserName != NULL && pstUpdateInfo->nPassword != NULL)
            {
                if (pstUpdateInfo->nPort > 0)
                {
                    snprintf(cmd,sizeof(cmd), "ftpget -u %s -p %s -P %d %s %s %s",
                                               pstUpdateInfo->nUserName,
                                               pstUpdateInfo->nPassword,
                                               pstUpdateInfo->nPort,
                                               pstUpdateInfo->nServer,
                                               UPDATE_FILE_ABSOLUTE_PATH,
                                               pstUpdateInfo->FileName);
                }
                else
                {
                    snprintf(cmd,sizeof(cmd), "ftpget -u %s -p %s %s %s %s",
                                               pstUpdateInfo->nUserName,
                                               pstUpdateInfo->nPassword,
                                               pstUpdateInfo->nServer,
                                               UPDATE_FILE_ABSOLUTE_PATH,
                                               pstUpdateInfo->FileName);
                }
            }
            else
            {
                if (pstUpdateInfo->nPort > 0)
                {
                    snprintf(cmd,sizeof(cmd), "ftpget -P %d %s %s %s",
                                               pstUpdateInfo->nPort,
                                               pstUpdateInfo->nServer,
                                               UPDATE_FILE_ABSOLUTE_PATH,
                                               pstUpdateInfo->FileName);
                }
                else
                {
                    snprintf(cmd,sizeof(cmd), "ftpget %s %s %s",
                                               pstUpdateInfo->nServer,
                                               UPDATE_FILE_ABSOLUTE_PATH,
                                               pstUpdateInfo->FileName);
                }
            }
            Common_System(cmd);

            pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_TRANS;
        }
        else
        {
            LOGE("Input parameters error.\n");
            updateInfoFree(pstUpdateInfo, -1);
            return 0;
        }
    }
    else if ((1 << 7) == pstUpdateInfo->nTransMode)
    {
        pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_CHECK;
    }

    return 0;
}

static S32 updateTransProc(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    S32 nFileSize = 0;
    pstVersion = pstVersion;

    if (((1 << 1) == pstUpdateInfo->nTransMode) || ((1 << 3) == pstUpdateInfo->nTransMode))
    {
        nFileSize = getFileSize(UPDATE_FILE_ABSOLUTE_PATH);

        if (nFileSize == pstUpdateInfo->nUpdateStatus.nDownLoadSize && nFileSize < pstUpdateInfo->nFileSize)
        {
            pstUpdateInfo->nUpdateStatus.nFailCnt++;
            if (pstUpdateInfo->nUpdateStatus.nFailCnt > 30)
            {
                LOGE("pstUpdateInfo->nUpdateStatus.nFailCnt = %d\n", pstUpdateInfo->nUpdateStatus.nFailCnt);
                remove(UPDATE_FILE_ABSOLUTE_PATH);
                updateInfoFree(pstUpdateInfo, -1);
            }
            return 0;
        }

        pstUpdateInfo->nUpdateStatus.nDownLoadSize = nFileSize;
        if (pstUpdateInfo->nUpdateStatus.nDownLoadSize < pstUpdateInfo->nFileSize)
        {
            pstUpdateInfo->nUpdateStatus.nProgress = pstUpdateInfo->nUpdateStatus.nDownLoadSize * 50 / pstUpdateInfo->nFileSize;
            pstUpdateInfo->nUpdateStatus.nPartProgress = pstUpdateInfo->nUpdateStatus.nDownLoadSize * 100 / pstUpdateInfo->nFileSize;
            return 0;
        }
        else if (pstUpdateInfo->nUpdateStatus.nDownLoadSize == pstUpdateInfo->nFileSize)
        {
            pstUpdateInfo->nUpdateStatus.nProgress = 0;//50;
            pstUpdateInfo->nUpdateStatus.nPartProgress = 100;
            pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_CHECK;
        }
    }

    return 0;
}

static S32 updateCheckProc(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    S8 cmd[128];
    S8 buf[128];

    pstVersion = pstVersion;

    if (pstUpdateInfo->nMd5sum == NULL)
    {
        remove(UPDATE_FILE_ABSOLUTE_PATH);
        updateInfoFree(pstUpdateInfo, -1);
        return 0;
    }

    if (pstUpdateInfo->nTransMode == (1 << 1) || pstUpdateInfo->nTransMode == (1 << 3))
    {
        snprintf(cmd,sizeof(cmd),"md5sum %s | awk '{print $1}'",UPDATE_FILE_ABSOLUTE_PATH);
    }
    else if (pstUpdateInfo->nTransMode == (1 << 7))
    {
        if (pstUpdateInfo->FileName != NULL)
        {
            snprintf(cmd,sizeof(cmd),"md5sum %s | awk '{print $1}'",pstUpdateInfo->FileName);
        }
        else
        {
            remove(UPDATE_FILE_ABSOLUTE_PATH);
            updateInfoFree(pstUpdateInfo, -1);
            return 0;
        }
    }

    if (0 == Common_Exe_Cmd(cmd, 5*1000, buf, sizeof(buf)))
    {
        if (0 == Common_StrniCmp(buf,pstUpdateInfo->nMd5sum,strlen(pstUpdateInfo->nMd5sum)))
        {
            pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_ERASE;
        }
        else
        {
            LOGE("buf = %s\npstUpdateInfo->nMd5sum = %s\n",buf,pstUpdateInfo->nMd5sum);
            remove(UPDATE_FILE_ABSOLUTE_PATH);
			if(pstUpdateInfo->FileName != NULL)
			{
            	remove(pstUpdateInfo->FileName);
			}
            updateInfoFree(pstUpdateInfo, -1);
            return 0;
        }
    }
    else
    {
        LOGE("Get Md5 Fail!!!!!!!!!!\n");
        remove(UPDATE_FILE_ABSOLUTE_PATH);
        if(pstUpdateInfo->FileName != NULL)
		{
        	remove(pstUpdateInfo->FileName);
		}
        updateInfoFree(pstUpdateInfo, -1);
        return 0;
    }

    return 0;
}

static S32 updateEraseProc(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    S32 lUpgradeHandle = -1;

    if (NULL == pstVersion->szSerialNumber)
    {
        UpdateServer_SetBoardType(0);
    }
    else
    {
        S8 szsv4[5];
        szsv4[0] = pstVersion->szSerialNumber[0];
        szsv4[1] = pstVersion->szSerialNumber[1];
        szsv4[2] = pstVersion->szSerialNumber[2];
        szsv4[3] = pstVersion->szSerialNumber[3];
        szsv4[4] = 0;

        UpdateServer_SetBoardType(str2Int(szsv4));
    }

    if (pstUpdateInfo->nTransMode == (1 << 1) || pstUpdateInfo->nTransMode == (1 << 3))
    {
        //LOGE("UpdateServer_Upgrade start!!!!!!!!!!\n");
        lUpgradeHandle = UpdateServer_Upgrade(0,UPDATE_FILE_ABSOLUTE_PATH,1);
        //LOGE("UpdateServer_Upgrade stop!!!!!!!!!!\n");
    }
    else if (pstUpdateInfo->nTransMode == (1 << 7))
    {
        lUpgradeHandle = UpdateServer_Upgrade(0,pstUpdateInfo->FileName,1);
        //lUpgradeHandle = UpdateServer_Upgrade(0,"/tmp/ipc_test.update",1);
    }

    if (lUpgradeHandle < 0)
    {
        LOGE("lUpgradeHandle error!!!!!!!!!!\n",lUpgradeHandle);
        remove(UPDATE_FILE_ABSOLUTE_PATH);
        updateInfoFree(pstUpdateInfo, -1);
        return 0;
    }

    pstUpdateInfo->lUpgradeHandle         = lUpgradeHandle;
    pstUpdateInfo->nUpdateStatus.nPartial = UPGRADEPROCESS_E_WRITE;
    return 0;
}

static S32 updateWriteProc(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    S32 nStaus = -1;
    pstVersion = pstVersion;

    nStaus = UpdateServer_GetUpgradeState(pstUpdateInfo->lUpgradeHandle);
    DUMP_UDPATE_STATUS(nStaus);

    if (UPGRADE_STATE_FINISH == nStaus)
    {
		LOGE("update finish!!!!!!!!!!!!!!\n");
        UpdateServer_CloseUpgradeHandle(pstUpdateInfo->lUpgradeHandle);
        pstUpdateInfo->lUpgradeHandle              = -1;
        pstUpdateInfo->nUpdateStatus.nProgress     = 100;
        pstUpdateInfo->nUpdateStatus.nPartProgress = 100;
        pstUpdateInfo->nUpdateStatus.nPartial      = UPGRADEPROCESS_E_FINISH;

        remove(UPDATE_FILE_ABSOLUTE_PATH);
        if(pstUpdateInfo->FileName != NULL)
		{
        	remove(pstUpdateInfo->FileName);
		}
        updateInfoFree(pstUpdateInfo, 0);
        return 0;
    }
    else if ((UPGRADE_STATE_FAIL == nStaus) || (-1 == nStaus))
    {
        updateInfoFree(pstUpdateInfo, -1);
        return 0;
    }
    else if (UPGRADE_STATE_ING == nStaus)
    {
        if (UPGRADESTATUS_E_CANCLE == pstUpdateInfo->nUpdateStatus.nStatus)
        {
            LOGE("Cancel update!!!!!!!!!!!!!!\n");
            UpdateServer_CloseUpgradeHandle(pstUpdateInfo->lUpgradeHandle);
            pstUpdateInfo->lUpgradeHandle = -1;

            remove(UPDATE_FILE_ABSOLUTE_PATH);
            updateInfoFree(pstUpdateInfo, -2);
        }

        return 0;
    }

    return 0;
}

static S32 updateCancle(Update_Version_S *pstVersion, UpdateInfo_T *pstUpdateInfo)
{
    pstVersion = pstVersion;

    LOGE("Cancel update!!!!!!!!!!!!!!\n");
    remove(UPDATE_FILE_ABSOLUTE_PATH);
    if(pstUpdateInfo->FileName != NULL)
	{
    	remove(pstUpdateInfo->FileName);
	}
    updateInfoFree(pstUpdateInfo, -2);
    return 0;
}

static struct _tagPartialProc{
    S32 lPartial;
    Update_PartialProc_func fnx;
} g_staPartial[] =
{
    {UPGRADEPROCESS_E_NORMAL, updateNormalProc},
    {UPGRADEPROCESS_E_TRANS,  updateTransProc},
    {UPGRADEPROCESS_E_CHECK,  updateCheckProc},
    {UPGRADEPROCESS_E_ERASE,  updateEraseProc},
    {UPGRADEPROCESS_E_WRITE,  updateWriteProc}
};

static S32 updateThread(Update_Version_S *pstVersion)
{
    S32 i   = 0;
    S32 ret = 0;

    UpdateInfo_T *pstUpdateInfo = getUpdateInfoHandle();

    Common_Lock(g_phLock);
    if (0 == pstUpdateInfo->nStatus)
    {
        Common_UnLock(g_phLock);
        return 0;
    }

    if (-2 == pstUpdateInfo->nUpdateStatus.nStatus)
    {
        updateCancle(pstVersion, pstUpdateInfo);
        Common_UnLock(g_phLock);
        return 0;
    }

	LOGE("Wtdg_SetTime 120\n");
	//Wtdg_Feed();
	Wtdg_SetTime(120);

    for (i = 0; i < sizeof(g_staPartial)/sizeof(g_staPartial[0]); i++)
    {
        if (pstUpdateInfo->nUpdateStatus.nPartial == g_staPartial[i].lPartial)
        {
            break;
        }
    }

    if (i == sizeof(g_staPartial)/sizeof(g_staPartial[0]))
    {
        LOGE("Can not match partial[%d].", pstUpdateInfo->nUpdateStatus.nPartial);
        Common_UnLock(g_phLock);
        return -1;
    }

    ret = g_staPartial[i].fnx(pstVersion, pstUpdateInfo);
    Common_UnLock(g_phLock);

    if (pstUpdateInfo->nUpdateStatus.nProgress == 100 &&
        pstUpdateInfo->nUpdateStatus.nPartial == UPGRADEPROCESS_E_FINISH)
    {
    	Common_System("rm /tmp/updating");
        if(Common_File_IsExist("/tmp/updating_restore"))
        {
            Common_System("rm /tmp/updating_restore");
        }
        LOGE("try to reboot device\n");
        Common_Sleep(g_iupdateSuccessTime,0);
        Common_System("sync");
        Common_System("reboot");
    }
	else if(pstUpdateInfo->nUpdateStatus.nPartial < UPGRADEPROCESS_E_ERASE && pstUpdateInfo->nUpdateStatus.nStatus != 0)
	{
    	Common_System("rm /tmp/updating");
        if(Common_File_IsExist("/tmp/updating_restore"))
        {
            Common_System("rm /tmp/updating_restore");
        }
        LOGE("update failed, try to reboot device\n");
        Common_Sleep(g_iupdateSuccessTime,0);
        Common_System("sync");
        Common_System("reboot");
	}

    return ret;
}

int update_load_custom()
{
    int ret = -1;
    int nStrLen = 0;
	Common_cJSON_T *pTempJson = NULL;
    char *pConfigString = NULL;

    FILE *fp = NULL;

    fp = fopen(UPDATE_CUSTOMFLE, "rb");
    if (NULL == fp)
    {
        LOGI("fopen %s error.\n", UPDATE_CUSTOMFLE);
        return -1;
    }


    fseek(fp, 0, SEEK_END);
    nStrLen = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (nStrLen > 0)
    {
        pConfigString = (char *)Common_Malloc(nStrLen, 0, __FUNCTION__, __LINE__);
        if (pConfigString != NULL)
        {
            if(nStrLen == fread(pConfigString, 1, (U32)nStrLen, fp))
            {
                pTempJson = Common_cJSON_Parse(pConfigString, NULL, NULL);
            }
        }
    }
    fclose(fp);

    if (pConfigString != NULL)
    {
        Common_Free(pConfigString, __FUNCTION__, __LINE__);
        pConfigString = NULL;
    }

	if(pTempJson)
	{
		Common_cJSON_T* tmp = Common_cJSON_GetObjectItem(pTempJson, "SensorModelCustom");
		if(tmp && tmp->type == Common_cJSON_String)
		{
			g_iSensorNameCustom = 1;
			memcpy(SensorName, tmp->valuestring, strlen(tmp->valuestring));
		}

		Common_cJSON_Delete(pTempJson);
		pTempJson = NULL;
	}
}

S32 main(S32 argc, S8 *argv[])
{
    DeviceVersion_S *pstVersionHandle  = update_version_GetHandle();
    UpdateInfo_T *pstUpdateInfoHandle  = getUpdateInfoHandle();
    UpdateMgr_T *pstUpdateMgrHandle    = getUpdateMgrHandle();

    LOG_INIT("pureUpdate_LOG", COMMON_LOG_LV_HIGH);

    Common_RegistSigHandle(SIGPIPE);

    memset((void *)pstVersionHandle, 0x00, sizeof(DeviceVersion_S));
    initUpdateInfo(pstUpdateInfoHandle);
    initUpdateMgr(pstUpdateMgrHandle, updateCallFunctions);
    (void)update_version_Init(pstVersionHandle);
    (void)update_broadCast_Init();

	update_load_custom();

    (void)Common_Lock_Create(&g_phLock, "Update_Lock");
    Update_Udp_Start_V2(pstUpdateMgrHandle);
    Update_Tcp_Start(pstUpdateMgrHandle);

    while(1)
    {
        if (0 == pstVersionHandle->lVersionOk)
        {
            (void)update_version_LoadVer(pstVersionHandle, 0, LOADVER_FROM_MTD);
        }

        if (1 == pstVersionHandle->lVersionOk)
        {
            update_broadCast_AuthSayHello(pstVersionHandle);
        }

        Common_Sleep(1, 0);
        (void)updateThread(&pstVersionHandle->stUpdateVersion);

		g_iStartTimeCount++;
		if(g_iStartTimeCount > 4*60)
		{
			g_iStartTimeCount = 4*60;
		}
    }

    Common_Lock_Destroy(&g_phLock);
    return 0;
}
