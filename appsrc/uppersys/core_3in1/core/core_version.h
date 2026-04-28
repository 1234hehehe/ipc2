#ifndef __CORE_VERSION_H__
#define __CORE_VERSION_H__
#include "libcommon_api.h"
#include "libmodule_api.h"

typedef struct _tagCore_Version
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
    S8 *szVendor;
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
    S8 *szStatus;
    S32 nLensSupport;
    S8 *szLensDrvType;//
    S8 *szLensType;
    S32 nIrisSupport;
    S8 *szIrisType;
    S32 IsOfDome;
    S32 IsOfIr;
    S8 *szSid;

    S32 bActived; //

} Core_Version_T;

S32 Core_Version_Init(ModuleHandle_T hModuleHandle);
S32 Core_Version_CallFunctions(ModuleHandle_T hModuleHandle,
                               cJSON_Struct *pInParams, cJSON_Struct **pOutParams);
S32 Core_Version_load(ModuleHandle_T hModuleHandle, Module_ConfigType_E nType,
                      cJSON_Struct **pOutParam);
S32 Core_Version_Save(ModuleHandle_T hModuleHandle, cJSON_Struct *pOutParam);
void Version_WriteInfoToUbootEnv();
#endif

