#ifndef __UPDATE_VERSION_H__
#define __UPDATE_VERSION_H__
#include "update_common.h"

#define LOADVER_FROM_MTD    (0)
#define LOADVER_FROM_CORE   (1)

typedef struct _tag_Update_Version_S
{
	S8 *szDeviceName; // 设备名
	S8 *szProductName; // 产品名
	S8 *szDeviceType; // 设备类型
	S8 *szDeviceTypeString; // 设备类型String
	S8 *szDeviceModel; // 设备型号
	S8 *szWeb;
	S8 *szTel;
	S8 *szCopyRight;
	S8 *szManufacturer;
	S8 *szBrand;
	S8 *szCountry;
	S8 *szCity;
	S8 *szCustomer;
	S8 *szSensorModel;
	S8 *szVersion;// 版本号
	S32 nSvnNumber;// svn 号
	S8 *szHardVersion;
	S8 *szBuildDate;
	S8 *szProductDate;
	S8 *szSerialNumber;
	S8 *szHardware;
	S8 *szUUID;
	S8 *szAuthMethod;
	S8 *szMac;
	S8 *szStatus;
	S32 nLensSupport;
	S8 *szLensDrvType;//
	S8 *szLensType;
	S32 nIrisSupport;
	S8 *szIrisType;
	S32 IsOfDome;
	S32 IsOfIr;
    S8 *szSid;
	S32 bActived;

    //Port
    S32 lRtspEnable;
    S32 lRtspPort;
    S32 lRtspHttpPort;
    S32 lHttpPort;
    S32 lHttpsPort;
    S32 lOnvifPort;
    S32 lOnvifAdaptiveIp;
    S8 *szIotQrCode;
    S8 *szMovementVersion;
}Update_Version_S;

typedef struct tag_DeviceVersion_S
{
    S32 lVersionOk;
    Common_Lock_T phVersionLock;
    Update_Version_S stUpdateVersion;
}DeviceVersion_S, DeviceVersion_S_PTR;


DeviceVersion_S *update_version_GetHandle();
S32 update_version_Init(DeviceVersion_S *pstDeviceVersion);
S32 update_version_LoadVer(DeviceVersion_S *pstVersion, S32 lforce, S32 lSource);
S32 udpate_version_GetSN(S8 *pSerialNum, S32 lLen);
S32 update_version_GetUUID(S8 *pcUUID, S32 lLen);
S32 update_version_GetMac(S8 *pcMac, S32 lLen);
S8 *update_version_GetSNByDevVer(DeviceVersion_S *pstVersion);
S32 setSidConfig(cJSON_Struct *pConfig);


#endif
