#ifdef WIN32
#include <Windows.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdint.h>
#include <sys/types.h>
#include <sys/un.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdarg.h>
#include <sys/stat.h>
#endif

#include "libaccess_api.h"
#include "libmodule_api.h"
#include "libcommon_api.h"
#include "access_auth.h"

#ifdef WIN32

BOOL APIENTRY DllMain( HMODULE hModule,
					  DWORD  ul_reason_for_call,
					  LPVOID lpReserved
					  )
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

#endif

#define MAX_NAMELEN 32
#define DWORD U32
#define BYTE U8
#define USE_GET_USRID	1

typedef struct tag_Ovfs_Log
{
	DWORD						dwLogTime;
	DWORD						dwMajorType;					//!主类型 1-报警; 2-异常; 3-操作; 0-全部
	DWORD						dwMinorType;					//!次类型 0-全部;
	BYTE						sUser[MAX_NAMELEN];				//!用户名
	BYTE						sRemoteHostAddr[MAX_NAMELEN];	//!远程主机地址
	DWORD						dwChannel;						//!通道号
	DWORD						dwDiskNum;						//!硬盘号
	DWORD						dwAlarmInPort;					//!报警输入端口
	DWORD						dwAlarmOutPort;					//!报警输出端口
	DWORD						bStatus;						//!报警状态
}OVFS_LOG, *POVFS_LOG;


 static AccessUserCfg_T *g_UserCfg = NULL;
 static IpTable_T *g_IpTable = NULL;
 static Common_Lock_T g_hSyncLock;
 static Common_Lock_T g_hUserLock;
 static S32 g_iSubscribeId = -1;
 #ifdef USE_GET_USRID
 static S32 g_iModuleId = 0;
 static S32 g_iUsrCount = 0;
 static S32 g_iSync = 0;
 #endif
 #if 0
 static Common_Thread_T g_ThreadHandle;
 #endif
int Access_MakeHandle(int nLoginHandle,int nSubIndex)
{
	int nUserIdx = nLoginHandle & OVFS_LOGINHANDLE_BITMASK;
	int nSubIdx= nSubIndex & OVFS_SUBHANDLE_BITMASK;
	int nHandle;
	// 生成句柄
	g_iUsrCount++;
	if(g_iUsrCount >= 128)
		g_iUsrCount = 0;
	nHandle = (nUserIdx) | (nSubIdx << OVFS_LOGINHANDLE_BITSIZE) | ((g_iModuleId & OVFS_MODULE_BITMASK) << (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))|((g_iUsrCount& OVFS_RAND_BITMASK) << (OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE+OVFS_MODULE_BITSIZE));
	return nHandle;
}

 S32 Access_IsWhiteList(S8 *pUri,S8 *pMethod)
 {
	if(0 == Common_StrniCmp(pUri, (S8*)"/Access/OnlineUser",strlen("/Access/OnlineUser")))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Onvif/Port",strlen("/Onvif/Port")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServer/Rtsp/Attribute",strlen("/MediaServer/Rtsp/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServer/Rtmp/Attribute",strlen("/MediaServer/Rtmp/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServerDongli/Rtsp/Attribute",strlen("/MediaServerDongli/Rtsp/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServerDongli/Rtmp/Attribute",strlen("/MediaServerDongli/Rtmp/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Webserver/Port",strlen("/Webserver/Port")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Version",strlen("/Core/Version")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/DeviceName",strlen("/Core/DeviceName")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/ImportCfg",strlen("/Core/ImportCfg")))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/ExportCfg",strlen("/Core/ExportCfg")))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Time",strlen("/Core/Time")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/Event",strlen("/Boardsys/Event")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/Event/Ability",strlen("/Boardsys/Event/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/AlarmOut/Ability",strlen("/Boardsys/AlarmOut/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/Audio/Ability",strlen("/Boardsys/Audio/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Audio/Attribute/All",strlen("/BoardSys/Audio/Attribute/All")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Record/DiskManage/Disk0/Attribute",strlen("/Record/DiskManage/Disk0/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/Video/Ability",strlen("/Boardsys/Video/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/VideoInput",strlen("/Boardsys/VideoInput")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Boardsys/Video/Attribute",strlen("/Boardsys/Video/Attribute")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/FishEye/Ability",strlen("/BoardSys/FishEye/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Sys/Custom",strlen("/BoardSys/Sys/Custom")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/EventLog/LogFunction",strlen("/EventLog/LogFunction")) && 0 == Common_StriCmp(pMethod, (S8*)"put"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/CollectData",strlen("/Alarm/CollectData")) && 0 == Common_StriCmp(pMethod, (S8*)"put"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Ability",strlen("/Ptz/Ability")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Conf",strlen("/Ptz/Conf")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Preset",strlen("/Ptz/Preset")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Cruises",strlen("/Ptz/Cruises")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Tracks",strlen("/Ptz/Tracks")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Access/PasswordLost",strlen("/Access/PasswordLost")) && 0 == Common_StriCmp(pMethod, (S8*)"get"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Access/RestorePassword",strlen("/Access/RestorePassword")) && 0 == Common_StriCmp(pMethod, (S8*)"put"))
	{
		return 1;
	}
	else if(0 == Common_StrniCmp(pUri, (S8*)"/Network/NetAttr/Wifi/ConfigSTA",strlen("/Network/NetAttr/Wifi/ConfigSTA")) && 0 == Common_StriCmp(pMethod, (S8*)"put"))
	{
		return 1;
	}
	return 0;
 }

  S32 Access_WriteLog(AccessHandle_T hAccessHandle,S8 *pUri,S8 *pMethod,S8 *pRemoteIP,cJSON_Struct *pInParams)
 {
	S32 nRet = -1;
	S32 iMethod = -1;
	S32 iChannel = -1;
	OVFS_LOG eventLog;
	static time_t lstCmd_t = 0;
	static U32 iLstMajorType = 0;
	static U32 iLstMinorType = 0;
	static S8 szLstLoginIP[64] = {0};
	//Common_Time_T t_happent;
	cJSON_Struct *pConfig = NULL,*pChild = NULL,*pArray = NULL,*pOutParams = NULL;

	memset(&eventLog,0,sizeof(OVFS_LOG));
	if(0 == Common_StriCmp(pMethod, (S8*)"get"))
		iMethod = 0;
	else if(0 == Common_StriCmp(pMethod, (S8*)"put"))
		iMethod = 1;
	else if(0 == Common_StriCmp(pMethod, (S8*)"post"))
		iMethod = 2;
	else if(0 == Common_StriCmp(pMethod, (S8*)"delete"))
		iMethod = 3;
	else
		return -1;
	if(0 == iMethod)
	{
		if(0 == Common_StrniCmp(pUri, (S8*)"/Core",strlen("/Core")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Core/ExportCfg",strlen("/Core/ExportCfg")))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_CFGFILE_OUTPUT;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else if(1 == iMethod)
	{
		if(0 == Common_StrniCmp(pUri, (S8*)"/Core",strlen("/Core")) && (1 == iMethod))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Power/Reboot",strlen("/Core/Power/Reboot")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_REBOOT;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Power/ShutDown",strlen("/Core/Power/ShutDown")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_STOP;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/ImportCfg",strlen("/Core/ImportCfg")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_CFGFILE_INTPUT;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Maintain",strlen("/Core/Maintain")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_AUTOREBOOT;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Restore/Items",strlen("/Core/Restore/Items")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_CONFIGRECOVERY;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Time/SysTime",strlen("/Core/Time/SysTime")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_TIMECFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/Time/NTP",strlen("/Core/Time/NTP")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_NTPCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Core/DeviceName",strlen("/Core/DeviceName")) && (1 == iMethod))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DEVICECFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys",strlen("/BoardSys")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Video/Attribute",strlen("/BoardSys/Video/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_COMPRESSCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Audio/Attribute/all",strlen("/BoardSys/Audio/Attribute/all")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_AUDIOCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Video/Roi",strlen("/BoardSys/Video/Roi")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_ROICFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Osd",strlen("/BoardSys/Osd")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_OSDCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/VideoInput/Attribute/Device",strlen("/BoardSys/VideoInput/Attribute/Device")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_VIDEOFORMAT;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/AlarmOut/Attribute",strlen("/BoardSys/AlarmOut/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_ALARMOUTCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Image/Attribute",strlen("/BoardSys/Image/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SENSORCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Mask/All",strlen("/BoardSys/Mask/All")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SHELTERCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/BoardSys/Mask/Device",strlen("/BoardSys/Mask/Device")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SHELTERCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz",strlen("/Ptz")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Ptz/Attribute",strlen("/Ptz/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DECODERCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/PTZ/Image/Attribute",strlen("/PTZ/Image/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SENSORCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/Network",strlen("/Network")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Network/NetAttr",strlen("/Network/NetAttr")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_NETCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Network/NetApp",strlen("/Network/NetApp")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_NETCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/Record",strlen("/Record")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Record/RecordConfig",strlen("/Record/RecordConfig")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_RECORDCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Record/DiskManage/Disk0/Format",strlen("/Record/DiskManage/Disk0/Format")))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_FORMAT_HDD;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm",strlen("/Alarm")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/Arming",strlen("/Alarm/Arming")))
			{
				eventLog.dwMajorType = MAJOR_OPERATION;
				eventLog.dwMinorType = MINOR_REMOTE_CFG_PARM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/EmailCfg",strlen("/Alarm/EmailCfg")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EMAILCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/AlarmIn",strlen("/Alarm/TriggerCfg/AlarmIn")))
			{
				S32 iSize = 0;
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_ALARMINCFG;
				pArray = Common_Json_GetItem(pInParams, -1, "Data/ResList");
				iSize= Common_Json_Size(pArray);
				for(int i = 0;i < iSize;i++)
					Common_Json_GetAttrValue(pArray, i, "Channel" ,NULL, NULL, &iChannel, NULL);
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/Motion",strlen("/Alarm/TriggerCfg/Motion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_MOTIONEXCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/Vhide",strlen("/Alarm/TriggerCfg/Vhide")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_HIDEALARMCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/Vdiagnose",strlen("/Alarm/TriggerCfg/Vdiagnose")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_VDIAGNOSE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/CounterWire",strlen("/Alarm/TriggerCfg/CounterWire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_CNTWIRT_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectWire",strlen("/Alarm/TriggerCfg/DetectWire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTWIRE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectRegion",strlen("/Alarm/TriggerCfg/DetectRegion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTREG_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/ObjectRegion",strlen("/Alarm/TriggerCfg/ObjectRegion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTOBJ_PARAM;
			}
            else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/EBike",strlen("/Alarm/TriggerCfg/EBike")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EBIKE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/SoundDetect",strlen("/Alarm/TriggerCfg/SoundDetect")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTSOUND_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/SmartMotion",strlen("/Alarm/TriggerCfg/SmartMotion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SMOTION_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectFire",strlen("/Alarm/TriggerCfg/DetectFire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTFIRE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectFace",strlen("/Alarm/TriggerCfg/DetectFace")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTFACE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectPlate",strlen("/Alarm/TriggerCfg/DetectPlate")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTPLATE_PARAM;
			}
      else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/DetectPerson",strlen("/Alarm/TriggerCfg/DetectPerson")))
			{
          eventLog.dwMajorType = MAJOR_CONFIG_SET;
          eventLog.dwMinorType = ANTS_DVR_SET_PERSON_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/RecognitionFace",strlen("/Alarm/TriggerCfg/RecognitionFace")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_REGFACE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/TriggerCfg/Temperature",strlen("/Alarm/TriggerCfg/Temperature")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_TEMPERATURE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/LinkageCfg/NetCableBreak",strlen("/Alarm/LinkageCfg/NetCableBreak")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EXCEPTIONCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/LinkageCfg/IllegallyAcc",strlen("/Alarm/LinkageCfg/IllegallyAcc")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EXCEPTIONCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/Alarm/LinkageCfg/IpConflict",strlen("/Alarm/LinkageCfg/IpConflict")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EXCEPTIONCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/Access",strlen("/Access")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Access/UserCfg",strlen("/Access/UserCfg")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_USERCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServer",strlen("/MediaServer")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServer/Rtsp/Attribute",strlen("/MediaServer/Rtsp/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_NETCFG;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/MediaServer/Rtmp/Attribute",strlen("/MediaServer/Rtmp/Attribute")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_NETCFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/AccessHost",strlen("/AccessHost")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/AccessHost",strlen("/AccessHost")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_NET_SET_MANAGERHOST_CFG;
			}
			else
				return -1;
		}
		else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer",strlen("/SmartServer")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/VideoDiagnose",strlen("/SmartServer/Attribute/VideoDiagnose")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_VDIAGNOSE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/CounterWire",strlen("/SmartServer/Attribute/CounterWire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_CNTWIRT_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/DetectWire",strlen("/SmartServer/Attribute/DetectWire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTWIRE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/DetectRegion",strlen("/SmartServer/Attribute/DetectRegion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTREG_PARAM;
			}
            else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/DetectMotor",strlen("/SmartServer/Attribute/DetectMotor")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_EBIKE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/ObjectRegion",strlen("/SmartServer/Attribute/ObjectRegion")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTOBJ_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/MotionDetect",strlen("/SmartServer/Attribute/MotionDetect")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_SMOTION_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/SoundDetect",strlen("/SoundDetect/Attribute/SoundDetect")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTSOUND_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/Fire",strlen("/SmartServer/Attribute/Fire")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTFIRE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/Face",strlen("/SmartServer/Attribute/Face")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTFACE_PARAM;
			}
			else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/Plate",strlen("/SmartServer/Attribute/Plate")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_DETECTPLATE_PARAM;
			}
      else if(0 == Common_StrniCmp(pUri, (S8*)"/SmartServer/Attribute/Person",strlen("/SmartServer/Attribute/Person")))
			{
          eventLog.dwMajorType = MAJOR_CONFIG_SET;
          eventLog.dwMinorType = ANTS_DVR_SET_PERSON_PARAM;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else if(2 == iMethod)
	{
		if(0 == Common_StrniCmp(pUri, (S8*)"/Access",strlen("/Access")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Access/UserCfg",strlen("/Access/UserCfg")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_USERCFG;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else if(3 == iMethod)
	{
		if(0 == Common_StrniCmp(pUri, (S8*)"/Access",strlen("/Access")))
		{
			if(0 == Common_StrniCmp(pUri, (S8*)"/Access/UserCfg",strlen("/Access/UserCfg")))
			{
				eventLog.dwMajorType = MAJOR_CONFIG_SET;
				eventLog.dwMinorType = ANTS_DVR_SET_USERCFG;
			}
			else
				return -1;
		}
		else
			return -1;
	}
	else
	{
		return -1;
	}
	if(-1 == iChannel)
	eventLog.dwChannel = 1;
	else
		eventLog.dwChannel = iChannel+1;
	//Common_Linux2CommonTime(time(NULL),&t_happent);
	eventLog.dwLogTime = time(NULL);
	if(eventLog.dwLogTime - lstCmd_t < 5)
	{
		if(0 == Common_StrCmp(pRemoteIP, szLstLoginIP) && (eventLog.dwMajorType == iLstMajorType) && (eventLog.dwMinorType == iLstMinorType))
		{
			return -1;
		}
	}
	if(pRemoteIP)
	{
		memcpy(eventLog.sRemoteHostAddr,pRemoteIP,strlen(pRemoteIP));
		snprintf(szLstLoginIP,sizeof(szLstLoginIP),"%s",pRemoteIP);
	}

	LOGW("Cur:%d %d %d,Last:%d %d %d\n",eventLog.dwLogTime,eventLog.dwMajorType,eventLog.dwMinorType,lstCmd_t,iLstMajorType,iLstMinorType);
	iLstMajorType = eventLog.dwMajorType;
	iLstMinorType= eventLog.dwMinorType;
	lstCmd_t = eventLog.dwLogTime;
	//Common_Common2LinuxTime(&t_happent,(time_t*)&eventLog.dwLogTime);
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/EventLog/LogFunction",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
		pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

		Common_Json_SetAttrValue(pArray,0,"LogTime",Common_Json_Type_Number,NULL,eventLog.dwLogTime,0);
		Common_Json_SetAttrValue(pArray,0,"MajorType",Common_Json_Type_Number,NULL,eventLog.dwMajorType,0);
		Common_Json_SetAttrValue(pArray,0,"MinorType",Common_Json_Type_Number,NULL,eventLog.dwMinorType,0);
		Common_Json_SetAttrValue(pArray,0,"RemoteHostAddress",Common_Json_Type_String,(S8*)eventLog.sRemoteHostAddr,0,0);
		Common_Json_SetAttrValue(pArray,0,"Channel",Common_Json_Type_Number,NULL,eventLog.dwChannel,0);
		Common_Json_SetAttrValue(pArray,0,"AlarmInPort",Common_Json_Type_Number,NULL,eventLog.dwAlarmInPort,0);
		Common_Json_SetAttrValue(pArray,0,"AlarmOutPort",Common_Json_Type_Number,NULL,eventLog.dwAlarmOutPort,0);
		Common_Json_SetAttrValue(pArray,0,"Status",Common_Json_Type_Number,NULL,eventLog.bStatus,0);
		nRet = Module_CallFunctions((ModuleHandle_T)hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		Common_Json_Delete(pOutParams);
		pConfig = NULL;
		pOutParams = NULL;
	}
	return nRet;
 }

 S32 Access_FreeUserCfg(AccessUserCfg_T *p,S32 bSelf)
 {
 	AccessUserCfg_T *pUserCfg = NULL;
 	AccessUserCfg_T *pNextCfg = NULL,*pPrevCfg = NULL;
	if(!p)
	{
		LOGE("pUserCfg is null\n");
		return 0;
	}
	pUserCfg = p;
	while(pUserCfg)
	{
		pPrevCfg = pUserCfg->pPrev;
		pNextCfg = pUserCfg->pNext;
		if(pUserCfg->szUserName)
			Common_Free(pUserCfg->szUserName, __FUNCTION__, __LINE__);
		if(pUserCfg->szDefaultPassword)
			Common_Free(pUserCfg->szDefaultPassword, __FUNCTION__, __LINE__);
		if(pUserCfg->szTempPassword)
			Common_Free(pUserCfg->szTempPassword, __FUNCTION__, __LINE__);
		if(pUserCfg->szSerialNumber)
			Common_Free(pUserCfg->szSerialNumber, __FUNCTION__, __LINE__);
		for(int i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++){
			if(pUserCfg->szPassword[i])
				Common_Free(pUserCfg->szPassword[i], __FUNCTION__, __LINE__);
		}
		for(int i = 0;i < ACCESS_USERCFG_AUTH_MAX_NUM;i++){
			if(pUserCfg->szOnlineAuthManList[i])
				Common_Free(pUserCfg->szOnlineAuthManList[i], __FUNCTION__, __LINE__);
		}
		if(pUserCfg->szBindIpv4)
			Common_Free(pUserCfg->szBindIpv4, __FUNCTION__, __LINE__);
		if(pUserCfg->szBindIpv6)
			Common_Free(pUserCfg->szBindIpv6, __FUNCTION__, __LINE__);
		if(pUserCfg->szBindMac)
			Common_Free(pUserCfg->szBindMac, __FUNCTION__, __LINE__);
		if(pUserCfg->pLocalRight)
		{
			Common_Free(pUserCfg->pLocalRight->pChanRight, __FUNCTION__, __LINE__);
			Common_Free(pUserCfg->pLocalRight, __FUNCTION__, __LINE__);
		}
		if(pUserCfg->pRemoteRight)
		{
			Common_Free(pUserCfg->pRemoteRight->pChanRight, __FUNCTION__, __LINE__);
			Common_Free(pUserCfg->pRemoteRight, __FUNCTION__, __LINE__);
		}
		if(pUserCfg->pCreateTime)
			Common_Free(pUserCfg->pCreateTime, __FUNCTION__, __LINE__);
		if(pUserCfg->pStopTime)
			Common_Free(pUserCfg->pStopTime, __FUNCTION__, __LINE__);
		for(int i = 0;i < ACCESS_IPCONNECT_MAX_NUM;i++){
			if(pUserCfg->pIpConnList[i])
			{
				IpConnectInfo_T * pIpConnect = pUserCfg->pIpConnList[i];
				if(pIpConnect->szBindIpv4)
					Common_Free(pIpConnect->szBindIpv4, __FUNCTION__, __LINE__);
				if(pIpConnect->szBindIpv6)
					Common_Free(pIpConnect->szBindIpv6, __FUNCTION__, __LINE__);
				if(pIpConnect->szBindMac)
					Common_Free(pIpConnect->szBindMac, __FUNCTION__, __LINE__);
				Common_Free(pIpConnect, __FUNCTION__, __LINE__);
				pUserCfg->pIpConnList[i] = NULL;
				if(pUserCfg->nIpConnCnt > 0)
					pUserCfg->nIpConnCnt--;
			}
		}
		if(bSelf)
		{
			pNextCfg->pPrev = pPrevCfg;
			if(pPrevCfg)
				pPrevCfg->pNext = pNextCfg;
			Common_Free(pUserCfg, __FUNCTION__, __LINE__);
			break;
		}
		else
		{
			Common_Free(pUserCfg, __FUNCTION__, __LINE__);
		}
		pUserCfg = pNextCfg;
	}
	return 0;
 }

 AccessUserCfg_T *Access_GetUserCfg(AccessHandle_T hAccessHandle,S8 *pUserName,S8 *pUserUri,S8 *pMethod)
 {
 	S8 *pValue = NULL;
	S8 strcmd[128] = {0};
	S32 i = 0,s = 0,nRet = -1,iValue = -1,iCount = 0,iArrayCount = 0;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pItem = NULL,*pItem1 = NULL,*pChild = NULL;
	AccessUserCfg_T *pUsercfg = NULL,*pTempCfg = NULL,*pTempCfg1 = NULL;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pConfig != NULL)
	{
		sprintf(strcmd,"/Access/UserCfg?UserName=%s",pUserName);
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		if(pUserUri)
			Common_Json_SetAttrValue(pConfig,-1,"Data/UserUri",Common_Json_Type_String,pUserUri,0,0);
		if(pMethod)
			Common_Json_SetAttrValue(pConfig,-1,"Data/UserMethod",Common_Json_Type_String,pMethod,0,0);
		nRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(nRet < 0)
		{
			LOGE("nRet:%d\n",nRet);
			return NULL;
		}
		if(!pOutParams)
		{
			LOGE("pOutParams == null!\n");
			return NULL;
		}
		//ovfs_print_json(pOutParams);
		pTempCfg1 = (AccessUserCfg_T*)Common_Malloc(sizeof(AccessUserCfg_T), 0, __FUNCTION__, __LINE__);
		if(!pTempCfg1)
		{
			LOGE("pUsercfg == null!\n");
			Common_Json_Delete(pOutParams);
			return NULL;
		}
		memset(pTempCfg1,0,sizeof(AccessUserCfg_T));
		pUsercfg = pTempCfg1;
		pChild = Common_Json_GetItem(pOutParams, -1, "Data/ResList");
		iArrayCount = Common_Json_Size(pChild);
		if(iArrayCount <=0)
		{
			LOGE("error!!!UserCount:%d\n",iArrayCount);
			Common_Json_Delete(pOutParams);
			Common_Free(pTempCfg1,__FUNCTION__, __LINE__);
			return NULL;
		}
		LOGD("UserList count:%d\n",iArrayCount);
		for(s = 0;s < iArrayCount;s++)
		{
			if(0 == s)
			{
				pUsercfg->pPrev = NULL;
				pUsercfg->pNext = NULL;
			}
			else
			{
				pUsercfg->pNext = (AccessUserCfg_T*)Common_Malloc(sizeof(AccessUserCfg_T), 0, __FUNCTION__, __LINE__);
				if(!pUsercfg->pNext)
				{
					LOGE("pUserCfg->pNext == NULL\n");
					Access_FreeUserCfg(pTempCfg1,0);
					Common_Json_Delete(pOutParams);
					return NULL;
				}
				memset(pUsercfg->pNext,0,sizeof(AccessUserCfg_T));
				pTempCfg = pUsercfg;
				pUsercfg = pUsercfg->pNext;
				if(pUsercfg)
				{
					pUsercfg->pPrev = pTempCfg;
					pUsercfg->pNext = NULL;
				}
			}
			pUsercfg->pLocalRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(!pUsercfg->pLocalRight)
			{
				LOGE("pUsercfg->pLocalRight == null!\n");
				Access_FreeUserCfg(pTempCfg1,0);
				Common_Json_Delete(pOutParams);
				return NULL;
			}
			memset(pUsercfg->pLocalRight,0,sizeof(AccessUserRight_T));
			pItem = Common_Json_GetItem(pChild, s, "LocalRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			if(iCount > 0){
				pUsercfg->pLocalRight->pChanRight = (AccessChanRight_T*)Common_Malloc(iCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
				if(!pUsercfg->pLocalRight->pChanRight)
				{
					LOGE("pUsercfg->pLocalRight->pChanRight == null!\n");
					Access_FreeUserCfg(pTempCfg1,0);
					Common_Json_Delete(pOutParams);
					return NULL;
				}
				memset(pUsercfg->pLocalRight->pChanRight,0,iCount*sizeof(AccessChanRight_T));
			}
			pUsercfg->pRemoteRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(!pUsercfg->pRemoteRight)
			{
				LOGE("pUsercfg->pRemoteRight == null!\n");
				Access_FreeUserCfg(pTempCfg1,0);
				Common_Json_Delete(pOutParams);
				return NULL;
			}
			memset(pUsercfg->pRemoteRight,0,sizeof(AccessUserRight_T));
			pItem = Common_Json_GetItem(pChild, s, "RemoteRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			if(iCount> 0){
				pUsercfg->pRemoteRight->pChanRight = (AccessChanRight_T*)Common_Malloc(iCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
				if(!pUsercfg->pRemoteRight->pChanRight)
				{
					LOGE("pUsercfg->pRemoteRight->pChanRight == null!\n");
					Access_FreeUserCfg(pTempCfg1,0);
					Common_Json_Delete(pOutParams);
					return NULL;
				}
				memset(pUsercfg->pRemoteRight->pChanRight,0,iCount*sizeof(AccessChanRight_T));
			}
			Common_Json_GetAttrValue(pChild, s, "UserName", NULL, &pValue, NULL, NULL);
			pUsercfg->szUserName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pChild, s, "EncryptMethod", NULL, NULL, &iValue, NULL);
			pUsercfg->byEncryptMethod= iValue;

			pItem = Common_Json_GetItem(pChild, s, "Password");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				Common_Json_GetAttrValue(pItem, i, NULL, NULL, &pValue, NULL, NULL);
				pUsercfg->szPassword[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				//LOGW("szPassword[%d]:%s\n",i,pUsercfg->szPassword[i]);
				pValue = NULL;
			}
			pUsercfg->byPwdCount = iCount;
			Common_Json_GetAttrValue(pChild, s, "DefaultPwd", NULL, &pValue, NULL, NULL);
			pUsercfg->szDefaultPassword = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pChild, s, "Forbidden", NULL, NULL, &iValue, NULL);
			pUsercfg->bForbidden = iValue;
			Common_Json_GetAttrValue(pChild, s, "Priority", NULL, NULL, &iValue, NULL);
			pUsercfg->byPriority = iValue;
			Common_Json_GetAttrValue(pChild, s, "Remote", NULL, NULL, &iValue, NULL);
			pUsercfg->bRemote = iValue;
			Common_Json_GetAttrValue(pChild, s, "NeedOnlineAuth", NULL, NULL, &iValue, NULL);
			pUsercfg->bNeedOnlineAuth = iValue;
			pItem = Common_Json_GetItem(pChild, s, "OnlineAuthManList");
			iCount= Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				Common_Json_GetAttrValue(pItem, i, NULL, NULL, &pValue, NULL, NULL);
				pUsercfg->szOnlineAuthManList[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				pValue = NULL;
			}
			pUsercfg->byOnlineAuthListCount = iCount;

			Common_Json_GetAttrValue(pChild, s, "BindIPv4", NULL, &pValue, NULL, NULL);
			pUsercfg->szBindIpv4= Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pChild, s, "BindIPv6", NULL, &pValue, NULL, NULL);
			pUsercfg->szBindIpv6= Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pChild, s, "BindMac", NULL, &pValue, NULL, NULL);
			pUsercfg->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			//LOGW("szUserName:%s,byEncryptMethod:%d,szPassword[%d]:%s,bForbidden:%d,byPriority:%d,bRemote:%d,szBindIpv4:%s,szBindIpv6:%s,szBindMac:%s\n",pUsercfg->szUserName,pUsercfg->byEncryptMethod,pUsercfg->szDefaultPassword,pUsercfg->bForbidden,\pUsercfg->byPriority,pUsercfg->bRemote,pUsercfg->szBindIpv4,pUsercfg->szBindIpv6,pUsercfg->szBindMac);

			pItem = Common_Json_GetItem(pChild, s, "LocalRight");
			Common_Json_GetAttrValue(pItem, -1, "RightMask", NULL, NULL, &iValue, NULL);
			pUsercfg->pLocalRight->u32RightMask = iValue;
			//LOGE("szUserName:%s,u32RightMask:%#x\n",pUsercfg->szUserName,pUsercfg->pLocalRight->u32RightMask);
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				pItem1 = Common_Json_GetItem(pItem, i, NULL);
				Common_Json_GetAttrValue(pItem1, -1, "DeviceNo", NULL, NULL, &iValue, NULL);
				pUsercfg->pLocalRight->pChanRight[i].wDeviceNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "ChannelNo", NULL, NULL, &iValue, NULL);
				pUsercfg->pLocalRight->pChanRight[i].wChannelNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "RightMask", NULL, NULL, &iValue, NULL);
				pUsercfg->pLocalRight->pChanRight[i].u32RightMask = iValue;
				//LOGE("u32RightMask[%d][%d]:%d\n",pUsercfg->pLocalRight->pChanRight[i].wDeviceNo,pUsercfg->pLocalRight->pChanRight[i].wChannelNo,pUsercfg->pLocalRight->pChanRight[i].u32RightMask);
			}
			pItem = Common_Json_GetItem(pChild, s, "RemoteRight");
			Common_Json_GetAttrValue(pItem, -1, "RightMask", NULL, NULL, &iValue, NULL);
			pUsercfg->pRemoteRight->u32RightMask = iValue;
			//LOGE("szUserName:%s,u32RightMask:%#x\n",pUsercfg->szUserName,pUsercfg->pRemoteRight->u32RightMask);
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				pItem1 = Common_Json_GetItem(pItem, i, NULL);
				Common_Json_GetAttrValue(pItem1, -1, "DeviceNo", NULL, NULL, &iValue, NULL);
				pUsercfg->pRemoteRight->pChanRight[i].wDeviceNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "ChannelNo", NULL, NULL, &iValue, NULL);
				pUsercfg->pRemoteRight->pChanRight[i].wChannelNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "RightMask", NULL, NULL, &iValue, NULL);
				pUsercfg->pRemoteRight->pChanRight[i].u32RightMask = iValue;
				//LOGE("u32RightMask[%d][%d]:%d\n",pUsercfg->pRemoteRight->pChanRight[i].wDeviceNo,pUsercfg->pRemoteRight->pChanRight[i].wChannelNo,pUsercfg->pRemoteRight->pChanRight[i].u32RightMask);
			}
		}
		Common_Json_Delete(pOutParams);
	}
	return pTempCfg1;
 }

S32 Access_InvalidFindPwd(AccessHandle_T hAccessHandle,S8 *pUserName)
{
	int iRet = -1,iCode = -1;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL;
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Access/InvalidFindPwd",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Data/UserName",Common_Json_Type_String,pUserName,0,0);
		iRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		if(0 == iRet)
		{
			Common_Json_GetAttrValue(pOutParams, -1, "Header/Code", NULL, NULL, &iCode, NULL);
		}
		else
		{
			ovfs_print_json(pOutParams);
		}
	}
	return iCode;
}
S32 Access_MallocGlobalId(AccessHandle_T hAccessHandle,S8 *pModuleName)
{
	S32 nRet = -1,iGlobalId = 0;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Access/GlobalId",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"Post",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Data/ModuleName",Common_Json_Type_String,pModuleName,0,0);
		nRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(!pOutParams)
		{
			LOGE("pOutParams == null!\n");
			return -1;
		}
		if(nRet < 0)
		{
			LOGE("nRet:%d\n",nRet);
			Common_Json_Delete(pOutParams);
			return -1;
		}
		Common_Json_GetAttrValue(pOutParams, -1, "Data/GlobalId", NULL, NULL, &iGlobalId, NULL);
		if(iGlobalId > 0)
		{
			g_iModuleId = iGlobalId;
			LOGD("g_iModuleId:%d\n",g_iModuleId);
		}
		Common_Json_Delete(pOutParams);
	}
	return 0;
}


S32 Access_FreeGlobalId(AccessHandle_T hAccessHandle)
{
	S32 nRet = -1;
	S8 szTemp[128] = {0};
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		sprintf(szTemp,"/Access/GlobalId?GlobalId=%d",g_iModuleId);
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,szTemp,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"delete",0,0);
		nRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(!pOutParams)
		{
			LOGE("pOutParams == null!\n");
			return -1;
		}
		if(nRet < 0)
		{
			LOGE("nRet:%d\n",nRet);
			Common_Json_Delete(pOutParams);
			return -1;
		}
		Common_Json_Delete(pOutParams);
	}
	return 0;
}

S32 Access_GenSessionId(AccessHandle_T hAccessHandle,S8 *pUserUri,S8 *pRemoteIP,cJSON_Struct *pData,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	S8 szDescribe[128] = {0};
 	S32 bFind = 0,s32Error = 0;
	S32 hSessionId = -1,nCurrIdx = 0,iUsrIdx = 0,iArrayCount = 0,iEncMethod = 0,bOk = 0,iIndex = 0;
	S8 *szResult = NULL,*pUserName = NULL,*pValue = NULL,*pTemp = NULL,*szPassword = NULL,*nonce = NULL;;
	cJSON_Struct *pArray = NULL,*pChild = NULL,*pItem = NULL,*pResult = NULL;
 	AccessUserCfg_T * pUserCfg = NULL;

	pArray = Common_Json_GetItem(pData, -1, "ResList");
	iArrayCount = Common_Json_Size(pArray);
	for(int i = 0;i < iArrayCount;i++)
	{
		nCurrIdx = 0;
		iUsrIdx= 0;
		pUserCfg = g_UserCfg;
		pItem = Common_Json_GetItem(pArray, i, NULL);
		Common_Json_GetAttrValue(pItem, -1, "UserName", NULL, &pUserName, NULL, NULL);
		do{
			if(!pUserCfg)
			{
				LOGE("pUserCfg = null\n");
				break;
			}
			//LOGW("pUserCfg->szUserName:%s pUserName:%s\n",pUserCfg->szUserName,pUserName);
			if(0 == Common_StrCmp(pUserCfg->szUserName,pUserName))
			{
				break;
			}
			pUserCfg = pUserCfg->pNext;
			iUsrIdx++;
		}while(pUserCfg);

		if(!pUserCfg)
		{
			s32Error = -1;
			sprintf(szDescribe,"No find the User[%s]\n",pUserName);
			LOGW("No find the User[%s]\n",pUserName);
		}
		else
		{
			Common_Json_GetAttrValue(pItem, -1, "AuthMethod", NULL, NULL, &iEncMethod, NULL);
			do{
				if(3 == iEncMethod)
				{
					U32 nonceLen = 0;
					S8 szGenPwd[20] = {0};
					S8 *s64Password = NULL,*szCreated = NULL,*szHexNonce = NULL;

					szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
					Common_Json_GetAttrValue(pItem, -1, "PasswordDigest", NULL, &s64Password, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Created", NULL, &szCreated, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Nonce", NULL, &szHexNonce, NULL, NULL);
					for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
					{
						pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
						if(pValue){
							if(iIndex == 0)
								sprintf(szPassword,"%s",pValue);
							else
								sprintf(szPassword,"%s,%s",szPassword,pValue);
							Common_Free(pValue, __FUNCTION__, __LINE__);
							pValue = NULL;
							pTemp = NULL;
						}
					}
					nonce = Common_Base64_Decode(szHexNonce, strlen(szHexNonce), &nonceLen);
					szResult = Common_Base64_Decode(s64Password, strlen(s64Password), NULL);
					Common_UsernameToken_CalcDigest(szCreated, szHexNonce,strlen(szHexNonce),szPassword,szGenPwd);
					if(0 != memcmp(szGenPwd, szResult,sizeof(szGenPwd)))
					{
						s32Error = ACCESS_ERROR_TYPE_AUTH;
						sprintf(szDescribe,"Password error");
						LOGE("Password auth failed!\n");
						break;
					}
					bOk = 1;
				}
				else if(2 == iEncMethod)
				{
					S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
					S8 *szSrcResponse = NULL,*szRealm = NULL,*szOpaque = NULL,*szNonce = NULL,*szNonceCount = NULL,*szQop = NULL,*szCNonce = NULL,*szDigestUri = NULL,*szMethod = NULL;
					szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
					Common_Json_GetAttrValue(pItem, -1, "Realm", NULL, &szRealm, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Qop", NULL, &szQop, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Nonce", NULL, &szNonce, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Opaque", NULL, &szOpaque, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Cnonce", NULL, &szCNonce, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Method", NULL, &szMethod, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Uri", NULL, &szDigestUri, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Response", NULL, (S8**)&szSrcResponse, NULL, NULL);
					Common_Json_GetAttrValue(pItem, -1, "Nc", NULL, &szNonceCount, NULL, NULL);
					for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
					{
						pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
						if(pValue){
							if(iIndex == 0)
								sprintf(szPassword,"%s",pValue);
							else
								sprintf(szPassword,"%s,%s",szPassword,pValue);
							Common_Free(pValue, __FUNCTION__, __LINE__);
							pValue = NULL;
							pTemp = NULL;
						}
					}
					Common_Digest_CalcHA1((S8*)"", pUserName, szRealm, szPassword, szNonce, szCNonce, szHA1);
					Common_Digest_CalcResponse(szHA1, szNonce, szNonceCount, szCNonce, szQop, szMethod, szDigestUri, (S8*)"", szResponse);
					if(0 != Common_StrCmp(szResponse, szSrcResponse))
					{
						s32Error = ACCESS_ERROR_TYPE_AUTH;
						sprintf(szDescribe,"Password error");
						LOGE("Password auth failed!\n");
						break;
					}
					bOk = 1;
				}
				else if(1 == iEncMethod||(0 == iEncMethod))
				{
					S8 *pPassword = NULL;
					szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
					Common_Json_GetAttrValue(pItem, -1, "Password", NULL, &pPassword, NULL, NULL);
					pValue = NULL;

					for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
					{
						pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
						if(pValue){
							if(iIndex == 0)
								sprintf(szPassword,"%s",pValue);
							else
								sprintf(szPassword,"%s,%s",szPassword,pValue);
							Common_Free(pValue, __FUNCTION__, __LINE__);
							pValue = NULL;
							pTemp = NULL;
						}
					}
					if(0 != Common_StrCmp(szPassword, pPassword))
					{
						s32Error = ACCESS_ERROR_TYPE_AUTH;
						sprintf(szDescribe,"Password error");
						LOGE("Password auth failed!\n");
						break;
					}
					bOk = 1;
				}
				else
				{
					s32Error = -1;
					sprintf(szDescribe,"unknow data");
				}
			}while(0);
			if(szPassword)
				Common_Free(szPassword, __FUNCTION__, __LINE__);
			if(szResult)
				Common_Free(szResult, __FUNCTION__, __LINE__);
			if(nonce)
				Common_Free(nonce, __FUNCTION__, __LINE__);
			if(!bOk)
				continue;
			for(nCurrIdx = 0;nCurrIdx < ACCESS_USER_LOGIN_MAX_NUM;nCurrIdx++)
			{
				if(pUserCfg->hSessionId[nCurrIdx] <= 0)
				{
					hSessionId = Access_MakeHandle(iUsrIdx,nCurrIdx);
					pUserCfg->hSessionId[nCurrIdx] = hSessionId;
					pUserCfg->nSessionCnt++;
					if(pRemoteIP)
					{
						int iConnIdx = 0;
						for(iConnIdx = 0;iConnIdx < ACCESS_IPCONNECT_MAX_NUM;iConnIdx++)
						{
							if(NULL == pUserCfg->pIpConnList[iConnIdx])
							{
								pUserCfg->pIpConnList[iConnIdx] = (IpConnectInfo_T *)Common_Malloc(sizeof(IpConnectInfo_T), 0, __FUNCTION__, __LINE__);
								memset(pUserCfg->pIpConnList[iConnIdx],0,sizeof(IpConnectInfo_T));
								if(pUserCfg->pIpConnList[iConnIdx])
								{
									IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[iConnIdx];
									pIpConnect->hSessionId = hSessionId;
									if(pRemoteIP)
										pIpConnect->szBindIpv4 = Common_StrDup(pRemoteIP, __FUNCTION__, __LINE__);
									pIpConnect->tCreateTime = time(NULL);
									pUserCfg->nIpConnCnt++;

								}
								break;
							}
						}
						if(iConnIdx >= ACCESS_IPCONNECT_MAX_NUM)
						{
							LOGE("[%s][%s][%d]:%d %d %d\n",pRemoteIP,pUserName,nCurrIdx,hSessionId,iUsrIdx,nCurrIdx);
							nCurrIdx = ACCESS_USER_LOGIN_MAX_NUM;
							hSessionId = -1;
							break;
						}
					}
					bFind = 1;
					break;
				}
			}
			if(nCurrIdx >= ACCESS_USER_LOGIN_MAX_NUM)
			{
				s32Error = -1;
				sprintf(szDescribe,"No valid hSessionId");
				LOGW("No valid hSessionId[%d]\n",hSessionId);
			}
		}
		pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
		if (pResult)
		{
			Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
			Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,s32Error,0);
			Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,(S8*)szDescribe,0,0);
			if(bFind)
			{
				pChild = Common_Json_SetAttrValue(pResult,-1,"Data",Common_Json_Type_Object,NULL,0,0);
				pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
				Common_Json_SetAttrValue(pArray,0,"SessionId",Common_Json_Type_Number,NULL,hSessionId,0);
				g_iSync = 1;
				#if 0
				if(0 != Common_StrCmp(pUserCfg->szUserName, (S8*)"(null)"))
				{
					time_t tCurtime = time(NULL);
					if(pUserCfg->szSerialNumber && pUserCfg->szTempPassword && pUserCfg->tTempValidTime > 0 && tCurtime > pUserCfg->tTempCreateTime && tCurtime < pUserCfg->tTempCreateTime+pUserCfg->tTempValidTime)
					{
						Access_InvalidFindPwd(hAccessHandle,pUserCfg->szUserName);
					}
				}
				#endif
			}
			*pOutParams = pResult;
		}
	}
	return 0;
}

S32 Access_DelSessionId(AccessHandle_T hAccessHandle,S8 *pUserUri,cJSON_Struct **pOutParams,S32 nTimeOut)
{
	S8 *pValue = NULL;
	S8 szDescribe[128] = {0};
 	S32 s32Error = -1,iModuleId = -1;
	S32 hSessionId = -1,nCurrIdx = 0,iUsrIdx = 0,iConnIdx = 0,iLoginIdx = 0;
	cJSON_Struct *pCondition = NULL,*pResult = NULL;
 	AccessUserCfg_T * pUserCfg = NULL;

	pCondition = get_conditionFromUri(pUserUri);
	if(pCondition)
	{
		Common_Json_GetAttrValue(pCondition, -1, "SessionId", NULL, &pValue, NULL, NULL);
		if(pValue)
			hSessionId = atoi(pValue);
		Common_Json_Delete(pCondition);
		LOGD("delete SessionId:%d\n",hSessionId);
		if(hSessionId > 0)
		{
			nCurrIdx = 0;
			pUserCfg = g_UserCfg;
			iModuleId = (hSessionId>>(OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
			iUsrIdx = hSessionId & OVFS_LOGINHANDLE_BITMASK;
			iLoginIdx = (hSessionId >> OVFS_LOGINHANDLE_BITSIZE)& OVFS_SUBHANDLE_BITMASK;
			if(iUsrIdx < 0 || iUsrIdx >= OVFS_MAX_LOGIN_USER ||iLoginIdx < 0 ||iLoginIdx > OVFS_MAX_LOGIN_USER||iModuleId != g_iModuleId)
			{
				s32Error = -1;
				sprintf(szDescribe,"error[%d,%d,%d,%d]",hSessionId,iModuleId,iUsrIdx,iLoginIdx);
				LOGW("SessionId:%d,iModuleId:%d,iUsrIdx:%d,iLoginIdx:%d\n",hSessionId,iModuleId,iUsrIdx,iLoginIdx);
			}
			else
			{
				while(pUserCfg)
				{
					if(nCurrIdx < iUsrIdx)
						nCurrIdx++;
					else
						break;
					pUserCfg = pUserCfg->pNext;
				}
				if(!pUserCfg)
				{
					s32Error = -1;
					sprintf(szDescribe,"error[%d,%d,%d]",hSessionId,iUsrIdx,iLoginIdx);
					LOGW("SessionId:%d,iUsrIdx:%d,iLoginIdx:%d\n",hSessionId,iUsrIdx,iLoginIdx);
				}
				else
				{
					s32Error = 0;
					for(int iIndex = 0;iIndex< ACCESS_USER_LOGIN_MAX_NUM;iIndex++)
					{
						if(pUserCfg->hSessionId[iIndex] == hSessionId)
						{
							pUserCfg->hSessionId[iIndex] = -1;
							pUserCfg->nSessionCnt--;
							for(iConnIdx = 0;iConnIdx < ACCESS_IPCONNECT_MAX_NUM;iConnIdx++)
							{
								IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[iConnIdx];
								if(pIpConnect)
								{
									if(pIpConnect ->hSessionId == hSessionId)
									{
										if(pIpConnect->szBindIpv4)
											Common_Free(pIpConnect->szBindIpv4, __FUNCTION__, __LINE__);
										if(pIpConnect->szBindIpv6)
											Common_Free(pIpConnect->szBindIpv6, __FUNCTION__, __LINE__);
										if(pIpConnect->szBindMac)
											Common_Free(pIpConnect->szBindMac, __FUNCTION__, __LINE__);
										Common_Free(pIpConnect, __FUNCTION__, __LINE__);
										pUserCfg->pIpConnList[iConnIdx] = NULL;
										if(pUserCfg->nIpConnCnt > 0)
											pUserCfg->nIpConnCnt--;
									}
								}
							}
							LOGD("delete [%d] succ\n",hSessionId);
							g_iSync = 1;
							break;
						}
					}
				}
			}
		}
	}
	pResult = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pResult)
	{
		Common_Json_SetAttrValue(pResult,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Code",Common_Json_Type_Number,NULL,s32Error,0);
		Common_Json_SetAttrValue(pResult,-1,"Header/Describe",Common_Json_Type_String,(S8*)szDescribe,0,0);
		*pOutParams = pResult;
	}
	return 0;
}

S32 Access_SyncSessonId(AccessHandle_T hAccessHandle)
{
	S32 nRet = -1;
	S32 iSessionCount = 0,iIpConnCnt = 0;;
	AccessUserCfg_T *pUserCfg = g_UserCfg;
	cJSON_Struct *pConfig = NULL,*pArray = NULL,*pChild = NULL,*pOutParams = NULL;

	if(!pUserCfg)
	{
		return -1;
	}
	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/Access/SyncOnlineUser",0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"post",0,0);
		pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		//pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		Common_Json_SetAttrValue(pChild,-1,"ModuleId",Common_Json_Type_Number,NULL,g_iModuleId,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"SessionId",Common_Json_Type_Array,NULL,0,0);
		iSessionCount = 0;
		while(pUserCfg)
		{
			if(pUserCfg->nSessionCnt > 0)
			{
				for(int i = 0;i < ACCESS_USER_LOGIN_MAX_NUM;i++)
				{
					if(pUserCfg->hSessionId[i] > 0)
					{
						Common_Json_SetAttrValue(pArray,iSessionCount,NULL,Common_Json_Type_Number,NULL,pUserCfg->hSessionId[i],0);
						iSessionCount++;
					}
				}
			}
			pUserCfg = pUserCfg->pNext;
		}
		pArray = Common_Json_SetAttrValue(pChild,-1,"ConnectInfo",Common_Json_Type_Array,NULL,0,0);
		iIpConnCnt = 0;
		pUserCfg = g_UserCfg;
		while(pUserCfg)
		{
			if(pUserCfg->nIpConnCnt> 0)
			{
				for(int i = 0;i < ACCESS_IPCONNECT_MAX_NUM;i++)
				{
					if(pUserCfg->pIpConnList[i])
					{
						IpConnectInfo_T *pIpConnect = pUserCfg->pIpConnList[i];
						if(pIpConnect->hSessionId > 0)
						{
							pChild = Common_Json_SetAttrValue(pArray,iIpConnCnt,NULL,Common_Json_Type_Object,NULL,0,0);
							Common_Json_SetAttrValue(pChild,-1,"SessionId",Common_Json_Type_Number,NULL,pIpConnect->hSessionId,0);
							if(pIpConnect->szBindIpv4)
								Common_Json_SetAttrValue(pChild,-1,"IPv4",Common_Json_Type_String,pIpConnect->szBindIpv4,0,0);
							Common_Json_SetAttrValue(pChild,-1,"CreateTime",Common_Json_Type_Number,NULL,pIpConnect->tCreateTime,0);
							iIpConnCnt++;
						}
					}
				}
			}
			pUserCfg = pUserCfg->pNext;
		}
		//ovfs_print_json(pConfig);
		nRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(!pOutParams)
		{
			LOGE("pOutParams == null!\n");
			return -1;
		}
		if(nRet < 0)
		{
			LOGE("nRet:%d\n",nRet);
			Common_Json_Delete(pOutParams);
			return -1;
		}
		Common_Json_Delete(pOutParams);
	}
	return 0;
}

S32 Access_GetOnlineUser(AccessHandle_T hAccessHandle,S8 *pUserName)
{
 	S8 *pValue = NULL;
	S8 strcmd[128] = {0};
	S32 i = 0,s = 0,nRet = -1,iCount = 0,iArrayCount = 0;
	S32 iSessionId = -1,iUsrIdx = 0,iLoginIdx = 0,iModuleId = 0;
	cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pItem = NULL,*pArray = NULL,*pChild = NULL;
	AccessUserCfg_T *pUsercfg = g_UserCfg;

	pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if(pConfig)
	{
		sprintf(strcmd,"/Access/OnlineUser?UserName=%s&&ModuleId=%d",pUserName,g_iModuleId);
		Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,strcmd,0,0);
		Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
		nRet = Module_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
		Common_Json_Delete(pConfig);
		if(!pOutParams)
		{
			LOGE("pOutParams == null!\n");
			return -1;
		}
		if(nRet < 0)
		{
			LOGE("nRet:%d\n",nRet);
			Common_Json_Delete(pOutParams);
			return -1;
		}
		pChild = Common_Json_GetItem(pOutParams, -1, "Data/ResList");
		iArrayCount = Common_Json_Size(pChild);

		for(s = 0;s < iArrayCount;s++)
		{
			pItem = Common_Json_GetItem(pChild, s, NULL);
			Common_Json_GetAttrValue(pItem, -1, "UserName", NULL, &pValue, NULL, NULL);
			while(pUsercfg)
			{
				if(0 == Common_StrCmp(pUsercfg->szUserName, pValue))
				{
					pArray = Common_Json_GetItem(pItem, -1, "SessionId");
					iCount= Common_Json_Size(pArray);
					for(i = 0;i < iCount;i++)
					{
						iSessionId = -1;
						Common_Json_GetAttrValue(pArray, i, NULL, NULL, NULL, &iSessionId, NULL);
						if(iSessionId > 0)
						{
							iModuleId = (iSessionId>>(OVFS_LOGINHANDLE_BITSIZE + OVFS_SUBHANDLE_BITSIZE))& OVFS_MODULE_BITMASK;
							if(iModuleId == g_iModuleId)
							{
								iUsrIdx = iSessionId & OVFS_LOGINHANDLE_BITMASK;
								iLoginIdx = (iSessionId >> OVFS_LOGINHANDLE_BITSIZE)& OVFS_SUBHANDLE_BITMASK;
								if(iUsrIdx < 0 || iUsrIdx >= OVFS_MAX_LOGIN_USER||iLoginIdx < 0 || iLoginIdx >= OVFS_MAX_LOGIN_USER)
								{
									LOGE("SessionId:%d,nCurrIdx:%d,iLoginIdx:%d\n",iSessionId,iLoginIdx);
									continue;
								}
								//LOGW("[%s][%d]:%d %d %d\n",pUsercfg->szUserName,i,iSessionId,iUsrIdx,iLoginIdx);
								for(int iIndex = 0;iIndex< ACCESS_USER_LOGIN_MAX_NUM;iIndex++)
								{
									if(pUsercfg->hSessionId[iIndex] <= 0)
									{
										pUsercfg->hSessionId[iIndex] = iSessionId;
										pUsercfg->nSessionCnt++;
										break;
									}
								}
							}
						}
					}
					break;
				}
				pUsercfg = pUsercfg->pNext;
			}
		}
		//OVFS_PRINT_JSON(pOutParams);
		Common_Json_Delete(pOutParams);
	}
	return 0;
}

S32 Compare_IsInIPList(S8 *szIP,S8 *szMac,BindInfo_T *pBindInfo)
{
	unsigned int ip_add[4] = {0};
	unsigned int start_ip_add[4] = {0},end_ip_add[4] = {0};
	unsigned int iStartAddr = 0,iEndAddr = 0,iIpAddr = 0;
	int bMacIn = 0;

	if(szIP)
		sscanf(szIP,"%d.%d.%d.%d",&ip_add[0],&ip_add[1],&ip_add[2],&ip_add[3]);
	iIpAddr = (ip_add[0]<<24)|(ip_add[1]<<16)|(ip_add[2]<<8)|(ip_add[3]);
	if(pBindInfo->szBindIpv4)
	{
		sscanf(pBindInfo->szBindIpv4,"%d.%d.%d.%d",&start_ip_add[0],&start_ip_add[1],&start_ip_add[2],&start_ip_add[3]);
		iStartAddr = (start_ip_add[0]<<24)|(start_ip_add[1]<<16)|(start_ip_add[2]<<8)|(start_ip_add[3]);
	}
	if(pBindInfo->szBindEndIpv4)
	{
		sscanf(pBindInfo->szBindEndIpv4,"%d.%d.%d.%d",&end_ip_add[0],&end_ip_add[1],&end_ip_add[2],&end_ip_add[3]);
		iEndAddr = (end_ip_add[0]<<24)|(end_ip_add[1]<<16)|(end_ip_add[2]<<8)|(end_ip_add[3]);
	}
	if(pBindInfo->szBindMac && szMac)
	{
		if(0 == Common_StriCmp(pBindInfo->szBindMac, szMac))
		{
			bMacIn = 1;
		}
		// arp mac
	}

	if(iEndAddr < iStartAddr || iIpAddr > iEndAddr || iIpAddr < iStartAddr)
	{
		return bMacIn;
	}
	return 1;
}

S32 Access_IPFilter(S8 *szIPv4,S8 *szMac)
{
	if(!g_IpTable)
		return 0;
	if(0 == g_IpTable->iAccessMode)
		return 0;
	if(!szIPv4)
		return 0;
	S32 bFind = 0;				//在黑白名单中
	S32 bEmpty = 1;				//黑白名单是否为空
	BindInfo_T *pBindInfo = NULL;
	int iCurtime = 0;
	time_t t_curtime = time(NULL);
	Common_Time_T sCurTime = {0};
	Common_Linux2CommonTime(t_curtime, &sCurTime);
	iCurtime = sCurTime.hour*100+sCurTime.min;
	if(g_IpTable->endtime > g_IpTable->starttime && (iCurtime < g_IpTable->starttime ||iCurtime > g_IpTable->endtime))
		return 0;
	for(int i = 0;i < MAX_IPTABLE_COUNT;i++)
	{
		if(1 == g_IpTable->iAccessMode)
			pBindInfo = g_IpTable->pWhiteList[i];
		else if(2 == g_IpTable->iAccessMode)
			pBindInfo = g_IpTable->pBlackList[i];
		if(pBindInfo)
		{
			bEmpty = 0;
			if(Compare_IsInIPList(szIPv4,szMac, pBindInfo))
			{
				bFind = 1;
				break;
			}
		}
	}
	if(bEmpty)
		return 0;
	if(bFind && 1 == g_IpTable->iAccessMode)	//黑名单列表中,没找到指定IP或不在时间段内允许通过
		return 0;
	else if((0 == bFind ) && 2 == g_IpTable->iAccessMode)	//黑名单列表中,没找到指定IP或不在时间段内允许通过
		return 0;
	return -1;
}
static S32 static_RecvSubscribe_fxn(AccessHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
	if (pEventInfo != NULL)
	{
		S8 *pValue = NULL;
		AccessUserCfg_T *pUsercfg = NULL,*pTempCfg = NULL;
		AccessOnLineUser_T *pOnlineUser = NULL,*pCurOnline = NULL,*pTemp = NULL;
		S32 i = 0,s = 0,iObjType = -1,iArrayCount = 0,iValue = -1,iCount = 0;
		cJSON_Struct *pJData = pEventInfo,*pArray = NULL,*pItem = NULL,*pItem1 = NULL;

		if(NULL == (pArray = Common_Json_GetItem(pJData,-1,"ResList")))
		{
			LOGE("no ResList\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		pJData= pArray;
		if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
		{
			LOGE("no iObjType\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		if(iObjType!=Common_Json_Type_Array)
		{
			LOGE("no array\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		iArrayCount= Common_Json_Size(pJData);

		Common_Lock(g_hUserLock);
		pUsercfg = g_UserCfg;
		while(pUsercfg)
		{
			pCurOnline = (AccessOnLineUser_T*)Common_Malloc(sizeof(AccessOnLineUser_T), 0, __FUNCTION__, __LINE__);
			if(!pCurOnline)
			{
				LOGE("Malloc failed\n");
				break;
			}
			memset(pCurOnline,0,sizeof(AccessOnLineUser_T));
			if(!pOnlineUser)
				pOnlineUser = pCurOnline;
			if(pTemp)
				pTemp->pNext = pCurOnline;
			pCurOnline->pPrev = pTemp;
			pCurOnline->pNext = NULL;
			if(pUsercfg->szUserName)
				pCurOnline->szUserName = Common_StrDup(pUsercfg->szUserName, __FUNCTION__, __LINE__);
			pCurOnline->byPwdCount = pUsercfg->byPwdCount;
			for(i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
			{
				if(pUsercfg->szPassword[i])
					pCurOnline->szPassword[i] = Common_StrDup(pUsercfg->szPassword[i], __FUNCTION__, __LINE__);
			}
			pCurOnline->nSessionCnt = pUsercfg->nSessionCnt;
			memcpy(pCurOnline->hSessionId,pUsercfg->hSessionId,sizeof(pUsercfg->hSessionId));
			pCurOnline->nIpConnCnt = pUsercfg->nIpConnCnt;
			for(i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
			{
				if(pUsercfg->pIpConnList[i])
				{
					IpConnectInfo_T *pTempConn = pUsercfg->pIpConnList[i];
					pCurOnline->pIpConnList[i] = (IpConnectInfo_T *)Common_Malloc(sizeof(IpConnectInfo_T),0,__FUNCTION__, __LINE__);
					memset(pCurOnline->pIpConnList[i],0,sizeof(IpConnectInfo_T));
					if(pCurOnline->pIpConnList[i])
					{
						IpConnectInfo_T *pIpConnect = pCurOnline->pIpConnList[i];
						pIpConnect->hSessionId = pTempConn->hSessionId;
						pIpConnect->tCreateTime = pTempConn->tCreateTime;
						if(pTempConn->szBindIpv4)
							pIpConnect->szBindIpv4 = Common_StrDup(pTempConn->szBindIpv4, __FUNCTION__, __LINE__);
						if(pTempConn->szBindIpv6)
							pIpConnect->szBindIpv6 = Common_StrDup(pTempConn->szBindIpv6, __FUNCTION__, __LINE__);
						if(pTempConn->szBindMac)
							pIpConnect->szBindMac = Common_StrDup(pTempConn->szBindMac, __FUNCTION__, __LINE__);
					}
				}
			}
			pTemp = pCurOnline;
			pCurOnline = pCurOnline->pNext;
			pUsercfg= pUsercfg->pNext;
		}
		pUsercfg = NULL;
		Access_FreeUserCfg(g_UserCfg, 0);
		g_UserCfg = NULL;
		for(s = 0;s < iArrayCount;s++)
		{
			pUsercfg = (AccessUserCfg_T*)Common_Malloc(sizeof(AccessUserCfg_T), 0, __FUNCTION__, __LINE__);
			if(!pUsercfg)
			{
				LOGE("pUserCfg == NULL\n");
				break;
			}
			memset(pUsercfg,0,sizeof(AccessUserCfg_T));
			if(pTempCfg)
				pTempCfg->pNext = pUsercfg;
			if(!g_UserCfg)
				g_UserCfg= pUsercfg;
			pUsercfg->pPrev = pTempCfg;
			pUsercfg->pNext = NULL;
			pTempCfg = pUsercfg;

			pUsercfg->pLocalRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(!pUsercfg->pLocalRight)
			{
				LOGE("pUsercfg->pLocalRight == null!\n");
				Access_FreeUserCfg(pUsercfg,1);
				break;
			}
			memset(pUsercfg->pLocalRight,0,sizeof(AccessUserRight_T));
			pItem = Common_Json_GetItem(pJData, s, "LocalRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			if(iCount > 0){
				if(!pUsercfg->pLocalRight->pChanRight)
				{
					pUsercfg->pLocalRight->pChanRight = (AccessChanRight_T*)Common_Malloc(iCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
					if(!pUsercfg->pLocalRight->pChanRight)
					{
						LOGE("pUsercfg->pLocalRight->pChanRight == null!\n");
						Access_FreeUserCfg(pUsercfg,1);
						break;
					}
					memset(pUsercfg->pLocalRight->pChanRight,0,iCount*sizeof(AccessChanRight_T));
				}
			}

			pUsercfg->pRemoteRight = (AccessUserRight_T*)Common_Malloc(sizeof(AccessUserRight_T), 0, __FUNCTION__, __LINE__);
			if(!pUsercfg->pRemoteRight)
			{
				LOGE("pUsercfg->pRemoteRight == null!\n");
				Access_FreeUserCfg(pUsercfg,1);
				break;
			}
			memset(pUsercfg->pRemoteRight,0,sizeof(AccessUserRight_T));
			pItem = Common_Json_GetItem(pJData, s, "RemoteRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			if(iCount> 0){

				if(!pUsercfg->pRemoteRight->pChanRight)
				{
					pUsercfg->pRemoteRight->pChanRight = (AccessChanRight_T*)Common_Malloc(iCount*sizeof(AccessChanRight_T), 0, __FUNCTION__, __LINE__);
					if(!pUsercfg->pRemoteRight->pChanRight)
					{
						LOGE("pUsercfg->pRemoteRight->pChanRight == null!\n");
						Access_FreeUserCfg(pUsercfg,1);
						break;
					}
					memset(pUsercfg->pRemoteRight->pChanRight,0,iCount*sizeof(AccessChanRight_T));
				}
			}
			Common_Json_GetAttrValue(pJData, s, "UserName", NULL, &pValue, NULL, NULL);
			if(pValue)
				pUsercfg->szUserName = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;

			Common_Json_GetAttrValue(pJData, s, "EncryptMethod", NULL, NULL, &iValue, NULL);
			if(pUsercfg->byEncryptMethod != iValue)
				pUsercfg->byEncryptMethod = iValue;

			pItem = Common_Json_GetItem(pJData, s, "Password");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				Common_Json_GetAttrValue(pItem, i, NULL, NULL, &pValue, NULL, NULL);
				if(pValue)
					pUsercfg->szPassword[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				//LOGW("szPassword[%d]:%s\n",i,pUsercfg->szPassword[i]);
				pValue = NULL;
			}
			pUsercfg->byPwdCount = iCount;
			pCurOnline = pOnlineUser;
			while(pCurOnline)
			{
				int bDiff = 0;
				if(0 != Common_StrCmp(pCurOnline->szUserName , pUsercfg->szUserName))
				{
					pCurOnline = pCurOnline->pNext;
					continue;
				}
				if(pCurOnline->byPwdCount != pUsercfg->byPwdCount)
					break;
				for(i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
				{
					if(0 != Common_StrCmp(pCurOnline->szPassword[i] , pUsercfg->szPassword[i]))
					{
						bDiff = 1;
						break;
					}
				}
				if(bDiff)
					break;
				pUsercfg->nSessionCnt = pCurOnline->nSessionCnt;
				memcpy(pUsercfg->hSessionId,pCurOnline->hSessionId,sizeof(pCurOnline->hSessionId));

				pUsercfg->nIpConnCnt = pCurOnline->nIpConnCnt;
				for(i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
				{
					if(pCurOnline->pIpConnList[i])
					{
						IpConnectInfo_T *pTempConn = pCurOnline->pIpConnList[i];
						pUsercfg->pIpConnList[i] = (IpConnectInfo_T *)Common_Malloc(sizeof(IpConnectInfo_T),0,__FUNCTION__, __LINE__);
						memset(pUsercfg->pIpConnList[i],0,sizeof(IpConnectInfo_T));
						if(pUsercfg->pIpConnList[i])
						{
							IpConnectInfo_T *pIpConnect = pUsercfg->pIpConnList[i];
							pIpConnect->hSessionId = pTempConn->hSessionId;
							pIpConnect->tCreateTime = pTempConn->tCreateTime;
							if(pTempConn->szBindIpv4)
								pIpConnect->szBindIpv4 = Common_StrDup(pTempConn->szBindIpv4, __FUNCTION__, __LINE__);
							if(pTempConn->szBindIpv6)
								pIpConnect->szBindIpv6 = Common_StrDup(pTempConn->szBindIpv6, __FUNCTION__, __LINE__);
							if(pTempConn->szBindMac)
								pIpConnect->szBindMac = Common_StrDup(pTempConn->szBindMac, __FUNCTION__, __LINE__);
						}
					}
				}
				#if 0
				LOGE("[%s]hSessionId:",pUsercfg->szUserName);
				for(i = 0;i < ACCESS_USER_LOGIN_MAX_NUM;i++)
				{
					if(pUsercfg->hSessionId[i] > 0)
					{
						printf("%d ",pUsercfg->hSessionId[i]);
					}
				}
				printf("\n");
				#endif
				break;
			}
			pItem1 = Common_Json_GetAttrValue(pJData, s, "DefaultPwd", NULL, &pValue, NULL, NULL);
			if(pItem1 && pValue)
				pUsercfg->szDefaultPassword = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "SerialNumber", NULL, &pValue, NULL, NULL);
			if(pItem1 && pValue)
				pUsercfg->szSerialNumber = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "TempPassword", NULL, &pValue, NULL, NULL);
			if(pItem1 && pValue)
				pUsercfg->szTempPassword = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "TempCreateTime", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->tTempCreateTime != iValue)
				pUsercfg->tTempCreateTime = iValue;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "TempValidTime", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->tTempValidTime != iValue)
				pUsercfg->tTempValidTime = iValue;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "Forbidden", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->bForbidden != iValue)
				pUsercfg->bForbidden = iValue;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "Priority", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->byPriority != iValue)
				pUsercfg->byPriority = iValue;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "Remote", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->bRemote != iValue)
				pUsercfg->bRemote = iValue;
			pItem1 = Common_Json_GetAttrValue(pJData, s, "NeedOnlineAuth", NULL, NULL, &iValue, NULL);
			if(pItem1 && pUsercfg->bNeedOnlineAuth != iValue)
				pUsercfg->bNeedOnlineAuth = iValue;
			pItem = Common_Json_GetItem(pJData, s, "OnlineAuthManList");
			iCount= Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				Common_Json_GetAttrValue(pItem, i, NULL, NULL, &pValue, NULL, NULL);
				if(pValue)
					pUsercfg->szOnlineAuthManList[i] = Common_StrDup(pValue, __FUNCTION__, __LINE__);
				pValue = NULL;
			}
			pUsercfg->byOnlineAuthListCount = iCount;

			Common_Json_GetAttrValue(pJData, s, "BindIPv4", NULL, &pValue, NULL, NULL);
			if(pValue)
				pUsercfg->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pJData, s, "BindIPv6", NULL, &pValue, NULL, NULL);
			if(pValue)
				pUsercfg->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			Common_Json_GetAttrValue(pJData, s, "BindMac", NULL, &pValue, NULL, NULL);
			if(pValue)
				pUsercfg->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
			pValue = NULL;
			//LOGW("szUserName:%s,byEncryptMethod:%d,szPassword[%d]:%s,bForbidden:%d,byPriority:%d,bRemote:%d,szBindIpv4:%s,szBindIpv6:%s,szBindMac:%s\n",pUsercfg->szUserName,pUsercfg->byEncryptMethod,pUsercfg->szDefaultPassword,pUsercfg->bForbidden,\pUsercfg->byPriority,pUsercfg->bRemote,pUsercfg->szBindIpv4,pUsercfg->szBindIpv6,pUsercfg->szBindMac);

			pItem = Common_Json_GetItem(pJData, s, "LocalRight");
			Common_Json_GetAttrValue(pItem, -1, "RightMask", NULL, NULL, &iValue, NULL);
			if(pUsercfg->pLocalRight->u32RightMask != (U32)iValue)
				pUsercfg->pLocalRight->u32RightMask = iValue;
			pItem = Common_Json_GetItem(pJData, s, "LocalRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				pItem1 = Common_Json_GetItem(pItem, i, NULL);
				Common_Json_GetAttrValue(pItem1, -1, "DeviceNo", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].wDeviceNo != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].wDeviceNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "ChannelNo", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].wChannelNo != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].wChannelNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "RightMask", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].u32RightMask != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].u32RightMask = iValue;
				//LOGE("u32RightMask[%d][%d]:%d\n",pUsercfg->pLocalRight->pChanRight[i].wDeviceNo,pUsercfg->pLocalRight->pChanRight[i].wChannelNo,pUsercfg->pLocalRight->pChanRight[i].u32RightMask);
			}
			pItem = Common_Json_GetItem(pJData, s, "RemoteRight");
			Common_Json_GetAttrValue(pItem, -1, "RightMask", NULL, NULL, &iValue, NULL);
			if(pUsercfg->pRemoteRight->u32RightMask != (U32)iValue)
				pUsercfg->pRemoteRight->u32RightMask = iValue;
			pItem = Common_Json_GetItem(pJData, s, "RemoteRight");
			pItem = Common_Json_GetItem(pItem, -1, "ChanRight");
			iCount = Common_Json_Size(pItem);
			for(i = 0;i < iCount;i++)
			{
				pItem1 = Common_Json_GetItem(pItem, i, NULL);
				Common_Json_GetAttrValue(pItem1, -1, "DeviceNo", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].wDeviceNo != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].wDeviceNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "ChannelNo", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].wChannelNo != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].wChannelNo = iValue;
				Common_Json_GetAttrValue(pItem1, -1, "RightMask", NULL, NULL, &iValue, NULL);
				if(pUsercfg->pLocalRight->pChanRight[i].u32RightMask != (U16)iValue)
					pUsercfg->pLocalRight->pChanRight[i].u32RightMask = iValue;
				//LOGE("u32RightMask[%d][%d]:%d\n",pUsercfg->pRemoteRight->pChanRight[i].wDeviceNo,pUsercfg->pRemoteRight->pChanRight[i].wChannelNo,pUsercfg->pRemoteRight->pChanRight[i].u32RightMask);
			}
			pUsercfg = pUsercfg->pNext;
		}
		while(pOnlineUser)
		{
			pCurOnline = pOnlineUser;
			pOnlineUser = pOnlineUser->pNext;
			if(pOnlineUser)
				pOnlineUser->pPrev = NULL;
			if(pCurOnline->szUserName)
				Common_Free(pCurOnline->szUserName, __FUNCTION__, __LINE__);
			for(i = 0;i < ACCESS_USERCFG_PWD_MAX_NUM;i++)
			{
				if(pCurOnline->szPassword[i])
					Common_Free(pCurOnline->szPassword[i], __FUNCTION__, __LINE__);
			}
			for(i = 0;i < ACCESS_IPCONNECT_MAX_NUM;i++)
			{
				if(pCurOnline->pIpConnList[i])
				{
					IpConnectInfo_T *pIpConnect = pCurOnline->pIpConnList[i];
					if(pIpConnect->szBindIpv4)
						Common_Free(pIpConnect->szBindIpv4, __FUNCTION__, __LINE__);
					if(pIpConnect->szBindIpv6)
						Common_Free(pIpConnect->szBindIpv6, __FUNCTION__, __LINE__);
					if(pIpConnect->szBindMac)
						Common_Free(pIpConnect->szBindMac, __FUNCTION__, __LINE__);
					Common_Free(pIpConnect, __FUNCTION__, __LINE__);
				}
			}
			Common_Free(pCurOnline, __FUNCTION__, __LINE__);
			pCurOnline = NULL;
		}
		Common_UnLock(g_hUserLock);
		g_iSubscribeId = nSubscribeID;
	}
	else
	{
		LOGW("pEventInfo is null\n");
	}
	return 0;
}

static S32 static_RecvSubscribe_Iptable_fxn(AccessHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
	if (pEventInfo != NULL)
	{
		S8 *pValue = NULL;
		S32 i = 0,iValue = -1,nCount = 0;
		cJSON_Struct *pJData = pEventInfo,*pArray = NULL;
		Common_Lock(g_hUserLock);
		if(g_IpTable)
		{
			for(i = 0;i < MAX_IPTABLE_COUNT;i++)
			{
				BindInfo_T *pBindInfo = g_IpTable->pWhiteList[i];
				if(pBindInfo)
				{
					if(pBindInfo->szBindIpv4)
						Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv4)
						Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindIpv6)
						Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv6)
						Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindMac)
						Common_Free(pBindInfo->szBindMac, __FUNCTION__, __LINE__);
					g_IpTable->pWhiteList[i] = NULL;
				}
				pBindInfo = g_IpTable->pBlackList[i];
				if(pBindInfo)
				{
					if(pBindInfo->szBindIpv4)
						Common_Free(pBindInfo->szBindIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv4)
						Common_Free(pBindInfo->szBindEndIpv4, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindIpv6)
						Common_Free(pBindInfo->szBindIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindEndIpv6)
						Common_Free(pBindInfo->szBindEndIpv6, __FUNCTION__, __LINE__);
					if(pBindInfo->szBindMac)
						Common_Free(pBindInfo->szBindMac, __FUNCTION__, __LINE__);
					g_IpTable->pBlackList[i] = NULL;
				}
			}
			Common_Free(g_IpTable, __FUNCTION__, __LINE__);
			g_IpTable = NULL;
		}
		if(!g_IpTable)
		{
			g_IpTable = (IpTable_T*)Common_Malloc(sizeof(IpTable_T), 0, __FUNCTION__, __LINE__);
			if(!g_IpTable)
			{
				LOGE("No memmory!\n");
				return -1;
			}
			memset(g_IpTable,0,sizeof(IpTable_T));
		}
		Common_Json_GetAttrValue(pJData, -1, "Mode" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			g_IpTable->iAccessMode = iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, -1, "StartTime" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			g_IpTable->starttime= iValue;
		iValue = -1;
		Common_Json_GetAttrValue(pJData, -1, "EndTime" ,NULL, NULL, &iValue, NULL);
		if(iValue != -1)
			g_IpTable->endtime= iValue;
		pArray = Common_Json_GetItem(pJData,-1,"WhiteList");
		if(pArray)
		{
			nCount = Common_Json_Size(pArray);
			for(i = 0;i < nCount && nCount < MAX_IPTABLE_COUNT;i++)
			{
				BindInfo_T *pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
				if(!pBindInfo)		//no memmory
				{
					LOGE("No memmory\n");
					break;
				}
				if(pBindInfo)
				{
					memset(pBindInfo,0,sizeof(BindInfo_T));
					Common_Json_GetAttrValue(pArray, i, "Ipv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Ipv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Mac" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					iValue = -1;
					Common_Json_GetAttrValue(pArray, i, "Direction" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iDirection = iValue;
					Common_Json_GetAttrValue(pArray, i, "Protocol" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iProtocol= iValue;
					g_IpTable->pWhiteList[i] = pBindInfo;
				}
			}
		}
		pArray = Common_Json_GetItem(pJData,-1,"BlackList");
		if(pArray)
		{
			nCount = Common_Json_Size(pArray);
			for(i = 0;i < nCount && nCount < MAX_IPTABLE_COUNT;i++)
			{
				BindInfo_T *pBindInfo = (BindInfo_T*)Common_Malloc(sizeof(BindInfo_T), 0,  __FUNCTION__, __LINE__);
				if(!pBindInfo)		//no memmory
				{
					LOGE("No memmory\n");
					break;
				}
				if(pBindInfo)
				{
					memset(pBindInfo,0,sizeof(BindInfo_T));
					Common_Json_GetAttrValue(pArray, i, "Ipv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv4" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv4 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Ipv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "EndIpv6" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindEndIpv6 = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					pValue = NULL;
					Common_Json_GetAttrValue(pArray, i, "Mac" ,NULL, &pValue, NULL, NULL);
					if(pValue)
						pBindInfo->szBindMac = Common_StrDup(pValue, __FUNCTION__, __LINE__);
					iValue = -1;
					Common_Json_GetAttrValue(pArray, i, "Direction" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iDirection = iValue;
					Common_Json_GetAttrValue(pArray, i, "Protocol" ,NULL, NULL, &iValue, NULL);
					if(iValue != -1)
						pBindInfo->iProtocol= iValue;
					g_IpTable->pBlackList[i] = pBindInfo;
				}
			}
		}
		Common_UnLock(g_hUserLock);
	}
	else
	{
		LOGW("pEventInfo is null\n");
	}
	return 0;
}
#if 0
static S32 static_RecvOnlineUser_fxn(AccessHandle_T hModuleHandle,S32 nSubscribeID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,void *pUserData)
{
	//LOGW("=========nSubscribeID = %d=========\n",nSubscribeID);
	if (pEventInfo != NULL)
	{
		S8 *pValue = NULL;
		AccessUserCfg_T *pUsercfg = NULL;
		S32 s32SessionId = -1,iUsrIdx = 0,iLoginIdx = 0;
		S32 i = 0,s = 0,iObjType = -1,iArrayCount = 0,iCount = 0;
		cJSON_Struct *pJData = pEventInfo,*pArray = NULL,*pArray1 = NULL,*pItem = NULL;

		if(NULL == (pArray = Common_Json_GetItem(pJData,-1,"ResList")))
		{
			LOGE("no ResList\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		pJData= pArray;
		if(-1 == Common_Json_GetAttr(pJData,NULL,NULL,&iObjType,NULL,NULL,NULL))
		{
			LOGE("no iObjType\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		if(iObjType!=Common_Json_Type_Array)
		{
			LOGE("no array\n");
			ovfs_print_json(pEventInfo);
			return -1;
		}
		iArrayCount= Common_Json_Size(pJData);
		pUsercfg = g_UserCfg;
		if(!pUsercfg)
		{
			LOGE("pUsercfg is null!\n");
			return -1;
		}
		//LOGD("iArrayCount:%d\n",iArrayCount);
		OVFS_PRINT_JSON(pJData);
		for(s = 0;s < iArrayCount;s++)
		{
			pItem = Common_Json_GetItem(pJData, s, NULL);
			Common_Json_GetAttrValue(pItem, -1, "UserName", NULL, &pValue, NULL, NULL);
			while(pUsercfg)
			{
				if(0 == Common_StrCmp(pUsercfg->szUserName, pValue))
				{
					pArray1 = Common_Json_GetItem(pItem, -1, "SessionId");
					iCount= Common_Json_Size(pArray1);
					pUsercfg->nSessionCnt = iCount;
					for(i = 0;i < iCount;i++)
					{
						s32SessionId = -1;
						Common_Json_GetAttrValue(pArray1, i, NULL, NULL, NULL, &s32SessionId, NULL);
						if(s32SessionId > 0)
						{
							iUsrIdx = s32SessionId & OVFS_LOGINHANDLE_BITMASK;
							iLoginIdx = (s32SessionId >> OVFS_LOGINHANDLE_BITSIZE)& OVFS_SUBHANDLE_BITMASK;
							if(iUsrIdx < 0 || iUsrIdx >= OVFS_MAX_LOGIN_USER||iLoginIdx < 0 || iLoginIdx >= OVFS_MAX_LOGIN_USER)
							{
								LOGE("SessionId:%d,nCurrIdx:%d,iLoginIdx:%d\n",s32SessionId,iLoginIdx);
								continue;
							}
							pUsercfg->hSessionId[iLoginIdx] = s32SessionId;
						}
					}
					break;
				}
				pUsercfg = pUsercfg->pNext;
			}
			pUsercfg = g_UserCfg;
		}
	}
	else
	{
		LOGW("pEventInfo is null\n");
	}
	return 0;
}
#endif

S32 Thread_SyncSessionId(Common_Thread_T hThreadHandle,void *pUserData)
{
	AccessHandle_T pAccessHandle = *(AccessHandle_T*)pUserData;
	while(1)
	{
		if(g_iSync)
		{
			Common_Lock(g_hUserLock);
			Common_Lock(g_hSyncLock);
			Access_SyncSessonId(pAccessHandle);
			g_iSync = 0;
			Common_UnLock(g_hSyncLock);
			Common_UnLock(g_hUserLock);
			Common_Sleep(0,10000);
		}
		else
		{
			Common_Sleep(1,0);
		}
	}
	return 0;
}

 S32 Access_Init(AccessHandle_T *pAccessHandle,cJSON_Struct *pInitConfig,cJSON_Struct **pOutConfig,Access_CallFunctions_Def fxn,void *pUserData)
 {
 	S32 iRet  = -1;
	S8 *pModuleName = NULL;
	Common_Thread_T hThread = NULL;
	ModuleHandle_T hModuleHandle;
	AccessUserCfg_T *pUserCfg = NULL;

	Common_Lock_Create(&g_hSyncLock,NULL);
	Common_Lock_Create(&g_hUserLock,NULL);
	iRet = Module_Init((ModuleHandle_T *)pAccessHandle,pInitConfig,pOutConfig,(Module_CallFunctions_Def)fxn,pUserData);
	if(0 != iRet)
	{
		Common_Lock_Destroy(&g_hSyncLock);
		LOGE("Module_Init failed,iRet:%d\n",iRet);
		return iRet;
	}

	Common_Json_GetAttrValue(pInitConfig,-1,"ModuleName",NULL,&pModuleName,NULL,NULL);
	hModuleHandle = *(ModuleHandle_T *)pAccessHandle;
	LoadRightModel(hModuleHandle);
	do{
		LOGD("=========get user cfg=========\n");
		pUserCfg = Access_GetUserCfg(hModuleHandle, (S8*)"all",NULL,NULL);
		if(pUserCfg)
		{
			//Access_FreeUserCfg(g_UserCfg,0);
			g_UserCfg = pUserCfg;
			Access_MallocGlobalId(hModuleHandle,pModuleName);
			Access_GetOnlineUser(hModuleHandle, (S8*)"all");
			break;
		}
		else
		{
			Common_Sleep(1, 0);
			LOGE("get user cfg failed!!!\n");
			continue;
		}
	}while(!pUserCfg);
	LOGI("init succ!\n");

	Module_SubscribeEvent(hModuleHandle,(S8*)"/Access/Subscribe/UserCfg?UserName=all",static_RecvSubscribe_fxn,NULL);
	Module_SubscribeEvent(hModuleHandle,(S8*)"/Access/Subscribe/Iptable",static_RecvSubscribe_Iptable_fxn,NULL);
	//Module_SubscribeEvent(*(ModuleHandle_T *)pAccessHandle,(S8*)"/Access/Subscribe/OnlineUser?UserName=all",static_RecvOnlineUser_fxn,NULL);
	Common_Thread_Create(&hThread,__FUNCTION__,0,0,Thread_SyncSessionId,(void *)pAccessHandle);
	return iRet;
 }
 S32 Access_Unint(AccessHandle_T *pAccessHandle)
 {
 	ModuleHandle_T hModuleHandle = *(ModuleHandle_T *)pAccessHandle;
 	UnLoadRightModel(hModuleHandle);
	Access_FreeGlobalId(pAccessHandle);
	Common_Lock(g_hUserLock);
	if(g_UserCfg)
		Access_FreeUserCfg(g_UserCfg,0);
	g_UserCfg = NULL;
	Common_UnLock(g_hUserLock);
	Common_Lock_Destroy(&g_hSyncLock);
	Common_Lock_Destroy(&g_hUserLock);
	if(g_iSubscribeId > 0)
		Module_UnSubscribeEvent(hModuleHandle,g_iSubscribeId);
	g_iSubscribeId = -1;
	return Module_Unint((ModuleHandle_T *)pAccessHandle);
 }
S8 *Access_GetMacByIpFromARP(char *szIP,char *szMacBuff/*[18]*/)
 {
 	// /proc/net/arp
 	FILE *pf = NULL;
	char *szMac = NULL;
	char szBuffer[128],*szRead = NULL;
	pf = fopen("/proc/net/arp","r");
	if(pf != NULL)
	{
		do{
			szRead = fgets(szBuffer, 128, pf);
			if(szRead != NULL)
			{
				if(NULL != strstr(szRead,szIP))
				{
					char *p=  strstr(szRead,":");
					if(p != NULL)
					{
						memcpy(szMacBuff,p - 2,17);
						szMacBuff[17] = 0;
						szMac = szMacBuff;
					}
					break;
				}
			}
		}while(szRead != NULL);
		fclose(pf);
		pf = NULL;
	}
	return szMac;
 }
 S32 Access_UserAuth(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams)
 {
	S32 iIndex = 0,iUsrIdx = 0,iLoginIdx = 0;
	S8 *szPassword = NULL,*szResult = NULL,*pValue = NULL,*pTemp = NULL,*szFrom = NULL;
	S8 *nonce = NULL;
	S32 bOK = 0,iEncMethod = -1,bRemote = 0;
	S8 *pUserName = NULL,*pIPv4 = NULL,*pIPv6 = NULL,*pMac = NULL,*pFromMac=NULL,szMacBuff[18],*pUserMethod = NULL,*pUserUri = NULL;
	AccessUserCfg_T * pUserCfg = NULL;
	AccessUserRight_T *pRight = NULL;

	if(!pInParams)
	{
		LOGE("pInParams is null\n");
		return bOK;
	}
	Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &pUserUri, NULL, NULL);
	Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &pUserMethod, NULL, NULL);
	Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/UserName", NULL, &pUserName, NULL, NULL);
	Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Method", NULL, NULL, &iEncMethod, NULL);
	Common_Json_GetAttrValue(pInParams, -1, "Header/From/Uri", NULL, &szFrom, NULL, NULL);
	Common_Json_GetAttrValue(pInParams, -1, "Header/IsRemote", NULL, NULL, &bRemote, NULL);
	if(bRemote)
	{
		Common_Json_GetAttrValue(pInParams, -1, "Header/ClientInfo/IPv4", NULL, &pIPv4, NULL, NULL);

		Common_Json_GetAttrValue(pInParams, -1, "Header/ClientInfo/IPv6", NULL, &pIPv6, NULL, NULL);

		Common_Json_GetAttrValue(pInParams, -1, "Header/ClientInfo/MAC", NULL, &pFromMac, NULL, NULL);

	}
	if(Access_IsWhiteList(pUserUri, pUserMethod))
	{
		return 1;
	}
	if(bRemote && pIPv4 != NULL && iEncMethod != 4)
	{
		// arp mac
		pMac = Access_GetMacByIpFromARP(pIPv4,szMacBuff);
		if(pMac == NULL)
		{
			pMac = pFromMac;
		}

		LOGI("Access : %s -> %s \n",pIPv4,pMac);

	}
	if(bRemote && 0 != Access_IPFilter(pIPv4,pMac))
	{
		ovfs_print_json(pInParams);
		LOGE("IPv4[%s] is filter\n",pIPv4);
		return bOK;
	}

	if(pUserUri&&pUserMethod)
	{
		pUserCfg = g_UserCfg;
		if(4 == iEncMethod)
		{
			S32 s32SessionId = -1,nCurrIdx = -1;
			cJSON_Struct *pItem = NULL;
			pItem = Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/SessionId", NULL, NULL, &s32SessionId, NULL);
			if(pItem)
			{
				if(s32SessionId >= 0)
				{
					nCurrIdx = 0;
					iUsrIdx = s32SessionId & OVFS_LOGINHANDLE_BITMASK;
					iLoginIdx = (s32SessionId >> OVFS_LOGINHANDLE_BITSIZE)& OVFS_SUBHANDLE_BITMASK;
					if(iUsrIdx < 0 || iUsrIdx >= OVFS_MAX_LOGIN_USER||iLoginIdx < 0 || iLoginIdx >= OVFS_MAX_LOGIN_USER)
					{
						LOGE("SessionId:%d,nCurrIdx:%d,iLoginIdx:%d\n",s32SessionId,nCurrIdx,iLoginIdx);
						return bOK;
					}

					while(pUserCfg)
					{
						if(nCurrIdx < iUsrIdx)
							nCurrIdx++;
						else
							break;
						pUserCfg = pUserCfg->pNext;
					}
					if(!pUserCfg)
					{
						LOGE("pUserCfg = null[%d,%d,%d]\n",s32SessionId,nCurrIdx,iLoginIdx);
						return bOK;
					}
					//ovfs_print_json(pInParams);
					for(iIndex = 0;iIndex < ACCESS_USER_LOGIN_MAX_NUM;iIndex++)
					{
						if(pUserCfg->hSessionId[iIndex] == s32SessionId)
						{
							break;
						}
					}
					if(iIndex >= ACCESS_USER_LOGIN_MAX_NUM)
					{
						LOGE("[%s]Not find SessionId[%d]\n",pUserCfg->szUserName,s32SessionId);
						return bOK;
					}
				}
				else
				{
					LOGE("SessionId[%d] error\n",s32SessionId);
					return 0;
				}
			}
			else
			{
				OVFS_PRINT_JSON(pInParams);
				LOGE("param error\n");
				return 0;
			}
		}
		else
		{
			while(pUserCfg)
			{
				if(!pUserName||(0 == strlen(pUserName))||(0 == Common_StriCmp((S8*)"(null)",pUserName)))
				{
					if(0 == Common_StrCmp(pUserCfg->szUserName,(S8*)"(null)"))
					{
						break;
					}
				}
				else
				{
					if(0 == Common_StrCmp(pUserCfg->szUserName,pUserName))
					{
						break;
					}
				}
				pUserCfg = pUserCfg->pNext;
			}

		}
		if(!pUserCfg)
		{
			LOGE("pUserCfg = null,pUserName:%s\n",pUserName);
			return -1;
		}

		//LOGE("szUserName:%s,LocalRightMask:%#x\n",pUserCfg->szUserName,pUserCfg->pLocalRight->u32RightMask);
		//LOGE("szUserName:%s,RemoteRightMask:%#x\n",pUserCfg->szUserName,pUserCfg->pRemoteRight->u32RightMask);
		do{
			if(pUserCfg->bForbidden)
			{
				LOGE("bForbidden:%d\n",pUserCfg->bForbidden);
				break;
			}
			if((pUserCfg->byPriority<LEVEL_GUEST||pUserCfg->byPriority>LEVEL_ADMIN)&&(pUserCfg->byPriority!=LEVEL_ROOT))
			{
				LOGE("byPriority:%d\n",pUserCfg->byPriority);
				break;
			}
			if(4 == iEncMethod)
			{

			}
			else if(3 == iEncMethod)
			{
				S8 szGenPwd[20] = {0};
				U32 nonceLen = 0;
				S8 *s64Password = NULL,*szCreated = NULL,*szHexNonce = NULL;

				szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/UsernameToken/PasswordDigest", NULL, &s64Password, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/UsernameToken/Created", NULL, &szCreated, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/UsernameToken/Nonce", NULL, &szHexNonce, NULL, NULL);
				for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
				{
					pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
					if(pValue){
						if(iIndex == 0)
							sprintf(szPassword,"%s",pValue);
						else
							sprintf(szPassword,"%s,%s",szPassword,pValue);
						Common_Free(pValue, __FUNCTION__, __LINE__);
						pValue = NULL;
						pTemp = NULL;
					}
				}
				nonce = Common_Base64_Decode(szHexNonce, strlen(szHexNonce), &nonceLen);
				szResult = Common_Base64_Decode(s64Password, strlen(s64Password), NULL);
				Common_UsernameToken_CalcDigest(szCreated, nonce,nonceLen,szPassword,szGenPwd);
				if(0 != memcmp(szGenPwd, szResult,sizeof(szGenPwd)))
				{
					bOK = -1;
					LOGE("Password auth failed!\n");
					break;
				}
			}
			else if(2 == iEncMethod)
			{
				S8 szResponse[32+1] = {0},szHA1[32+1] = {0};
				S8 *szSrcResponse = NULL,*szRealm = NULL,*szOpaque = NULL,*szNonce = NULL,*szNonceCount = NULL,*szQop = NULL,*szCNonce = NULL,*szDigestUri = NULL,*szMethod = NULL;
				szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Realm", NULL, &szRealm, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Qop", NULL, &szQop, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Nonce", NULL, &szNonce, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Opaque", NULL, &szOpaque, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Cnonce", NULL, &szCNonce, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Method", NULL, &szMethod, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Uri", NULL, &szDigestUri, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Response", NULL, (S8**)&szSrcResponse, NULL, NULL);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Digest/Nc", NULL, &szNonceCount, NULL, NULL);
				for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
				{
					pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
					if(pValue){
						if(iIndex == 0)
							sprintf(szPassword,"%s",pValue);
						else
							sprintf(szPassword,"%s,%s",szPassword,pValue);
						Common_Free(pValue, __FUNCTION__, __LINE__);
						pValue = NULL;
						pTemp = NULL;
					}
				}
				Common_Digest_CalcHA1((S8*)"", pUserName, szRealm, szPassword, szNonce, szCNonce, szHA1);
				Common_Digest_CalcResponse(szHA1, szNonce, szNonceCount, szCNonce, szQop, szMethod, szDigestUri, (S8*)"", szResponse);
				if(0 != Common_StrCmp(szResponse, szSrcResponse))
				{
					bOK = -1;
					LOGE("Password auth failed!\n");
					break;
				}
			}
			else if(1 == iEncMethod||(0 == iEncMethod))
			{
				S8 *pPassword = NULL;
				szPassword = (S8*)Common_Malloc(128, 0, __FUNCTION__, __LINE__);
				Common_Json_GetAttrValue(pInParams, -1, "Header/Auth/Password", NULL, &pPassword, NULL, NULL);
				pValue = NULL;

				for(iIndex = 0;iIndex < pUserCfg->byPwdCount;iIndex++)
				{
					pValue = auth_DecryptString(pUserCfg->szPassword[iIndex], NULL, pTemp, 128);
					if(pValue){
						if(iIndex == 0)
							sprintf(szPassword,"%s",pValue);
						else
							sprintf(szPassword,"%s,%s",szPassword,pValue);
						Common_Free(pValue, __FUNCTION__, __LINE__);
						pValue = NULL;
						pTemp = NULL;
					}
				}
				if(0 != Common_StrCmp(szPassword, pPassword))
				{
					bOK = -1;
					LOGE("Password auth failed!\n");
					break;
				}
			}
			else
			{
				LOGE("Password auth failed[iEncMethod:%d]!\n",iEncMethod);
				return 0;
			}
			if(bRemote  && iEncMethod != 4)
			{
				if(pUserCfg->szBindIpv4&&strlen(pUserCfg->szBindIpv4)&&(0 != Common_StrCmp(pUserCfg->szBindIpv4, pIPv4)))
				{
					LOGE("IPv4 auth failed!\n");
					break;
				}
				if(pUserCfg->szBindIpv6&&strlen(pUserCfg->szBindIpv6)&&(0 != Common_StrCmp(pUserCfg->szBindIpv6, pIPv6)))
				{
					LOGE("IPv6 auth failed!\n");
					break;
				}
				if(pUserCfg->szBindMac&&strlen(pUserCfg->szBindMac)&&(0 != Common_StriCmp(pUserCfg->szBindMac, pMac)))
				{
					LOGE("MAC auth failed! <%s> <%s>\n",pUserCfg->szBindMac,pMac?pMac:"nul");
					break;
				}
			}
			if(bRemote)
				pRight = pUserCfg->pRemoteRight;
			else
				pRight = pUserCfg->pLocalRight;
			if(!pRight){
				LOGE("pRight is null!\n");
				break;
			}
			bOK= AnalyzeUriAndAuthRight(pUserMethod,pUserUri,pRight->u32RightMask);
			#if 0		//去掉，slink也是从webserver出去的
			if(bOK && (0 != Common_StrCmp(pUserCfg->szUserName, (S8*)"(null)")) && (0 == Common_StriCmp(szFrom,(S8*)"/Webserver")))
			{
				time_t tCurtime = time(NULL);
				if(pUserCfg->szSerialNumber && pUserCfg->szTempPassword && pUserCfg->tTempValidTime > 0 && tCurtime > pUserCfg->tTempCreateTime && tCurtime < pUserCfg->tTempCreateTime+pUserCfg->tTempValidTime)
				{
					Access_InvalidFindPwd(hAccessHandle,pUserCfg->szUserName);
				}
			}
			#endif
		}while(0);
		if(szPassword)
			Common_Free(szPassword, __FUNCTION__, __LINE__);
		if(szResult)
			Common_Free(szResult, __FUNCTION__, __LINE__);
		if(nonce)
			Common_Free(nonce, __FUNCTION__, __LINE__);
 	}
	else
	{
		ovfs_print_json(pInParams);
	}
	return bOK;
 }

#if 0
cJSON_Struct *GenHeartPacket(S32 iSessionId)
{
	cJSON_Struct *pOutParams = NULL,*pChild = NULL,*pArray = NULL;
	if(iSessionId < 0)
	{
		LOGE("Generate HeartPacket failed![iSessionId:%d]\n",iSessionId);
		return NULL;
	}
	pOutParams = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	if (pOutParams != NULL)
	{
		Common_Json_SetAttrValue(pOutParams,-1,"Header",Common_Json_Type_Object,NULL,0,0);
		Common_Json_SetAttrValue(pOutParams,-1,"Header/Uri",Common_Json_Type_String,"/Access/OnlineUser",0,0);
		Common_Json_SetAttrValue(pOutParams,-1,"Header/Method",Common_Json_Type_String,"Put",0,0);
		pChild = Common_Json_SetAttrValue(pOutParams,-1,"Data",Common_Json_Type_Object,NULL,0,0);
		pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
		Common_Json_SetAttrValue(pArray,0,"SessionId",Common_Json_Type_Number,NULL,iSessionId,0);
	}
	return pOutParams;
}

S32 Thread_OnlineUserHeart(Common_Thread_T hThreadHandle,void *pUserData)
{
	S32 nRet = -1;
	cJSON_Struct *pHeartPacket = NULL,*pOutParams = NULL;
	ModuleHandle_T hModuleHandle = *(ModuleHandle_T*)pUserData;

	pHeartPacket = GenHeartPacket(g_hSessionId);
	if(!pHeartPacket)
	{
		LOGE("Generate HeartPacket failed![pHeartPacket:null]\n");
		return -1;
	}
	while(1)
	{
		if(g_hSessionId > 0){
			nRet = Module_CallFunctions(hModuleHandle,pHeartPacket,&pOutParams,3000);
			if(nRet != 0)
			{
				LOGE("Send HeartPacket failed!nRet:%d\n",nRet);
			}
		}
		Common_Sleep(60, 0);
	}
	Common_Json_Delete(pHeartPacket);
	Common_Json_Delete(pOutParams);
	return 0;
}
#endif

 S32 Access_CallFunctions(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut)
 {
 	 S32 bReboot = 0;
 	 S32 bOk = 0,iRet = -1, iServerPort = 0;
	 cJSON_Struct *pInData = NULL;
	 S8 *pUserUri = NULL,*pUserMethod = NULL,*pRemoteIP = NULL,*pServerIP = NULL;

	 //ovfs_print_json(pInParams);
	 Common_Lock(g_hUserLock);
	 bOk= Access_UserAuth(hAccessHandle,pInParams);
	 if(0 == bOk)
	 {
 	 	LOGE("Access_UserAuth failed!\n");
		//ovfs_print_json(pInParams);
		Common_UnLock(g_hUserLock);
		return ACCESS_ERROR_TYPE_NORIGHT;
	 }
	 else if(-1 == bOk)
	 {
 	 	LOGE("Access_UserAuth failed!\n");
		//ovfs_print_json(pInParams);
		Common_UnLock(g_hUserLock);
		return ACCESS_ERROR_TYPE_AUTH;
	 }
	 Common_Json_GetAttrValue(pInParams, -1, "Header/Uri", NULL, &pUserUri, NULL, NULL);
	 Common_Json_GetAttrValue(pInParams, -1, "Header/Method", NULL, &pUserMethod, NULL, NULL);
	 Common_Json_GetAttrValue(pInParams, -1, "Header/ClientInfo/IPv4", NULL, &pRemoteIP, NULL, NULL);
	 Common_Json_GetAttrValue(pInParams, -1, "Header/RemoteServerInfo/IP", NULL, &pServerIP, NULL, NULL);
     Common_Json_GetAttrValue(pInParams, -1, "Header/RemoteServerInfo/Port", NULL, NULL, &iServerPort, NULL);
     if(0 == Common_StrniCmp(pUserUri, (S8*)"/Access/OnlineUser",18))		//是否是登录
	 {
	 	if((0 == Common_StriCmp(pUserMethod, (S8*)"post")) ||(0 == Common_StriCmp(pUserMethod, (S8*)"delete")))
	 	{
			pInData = Common_Json_GetItem(pInParams, -1, "Data");
			Common_Lock(g_hSyncLock);
		 	if(0 == Common_StriCmp(pUserMethod, (S8*)"post"))
				Access_GenSessionId(hAccessHandle,pUserUri,pRemoteIP,pInData,pOutParams,nTimeOut);
			else if(0 == Common_StriCmp(pUserMethod, (S8*)"delete"))
				Access_DelSessionId(hAccessHandle,pUserUri,pOutParams,nTimeOut);
			Common_UnLock(g_hSyncLock);
			Common_UnLock(g_hUserLock);
			return 0;
		}
	 }
	 Common_UnLock(g_hUserLock);
	 if(0 == Common_StrniCmp(pUserUri, (S8*)"/Core/Power/Reboot",strlen("/Core/Power/Reboot")) && (0 == Common_StriCmp(pUserMethod, (S8*)"put")))
	 {
	 	bReboot = 1;
	 }
	 else if(0 == Common_StrniCmp(pUserUri, (S8*)"/Core/Power/ShutDown",strlen("/Core/Power/ShutDown")) && (0 == Common_StriCmp(pUserMethod, (S8*)"put")))
	 {
	 	bReboot = 1;
	 }
	 /*if(bReboot)
	 	Access_WriteLog(hAccessHandle,pUserUri,pUserMethod,pRemoteIP,pInParams);
    */
     if(pServerIP)
     {
         iRet = Module_CallFunctions_Remote(pServerIP,iServerPort,pInParams,pOutParams,nTimeOut);
     }
     else
     {
	    iRet = Module_CallFunctions((ModuleHandle_T)hAccessHandle,pInParams,pOutParams,nTimeOut);
     }
     /*if(*pOutParams)
	 {
	 	 int iCode = -1;
		 Common_Json_GetAttrValue(*pOutParams, -1, "Header/Code", NULL, NULL, &iCode, NULL);
		 if(0 == iCode && (0 == bReboot))
		 {
			Access_WriteLog(hAccessHandle,pUserUri,pUserMethod,pRemoteIP,pInParams);
		 }
	 }*/
	 return iRet;
 }

 S32 Access_LoadConfig(AccessHandle_T hAccessHandle,cJSON_Struct **pConfig)
 {
	 return Module_LoadConfig((ModuleHandle_T)hAccessHandle,pConfig);
 }
 S32 Access_SaveConfig(AccessHandle_T hAccessHandle,cJSON_Struct *pConfig)
 {
	 return Module_SaveConfig((ModuleHandle_T)hAccessHandle,pConfig);
 }

 S32 Access_LoadTempData(AccessHandle_T hAccessHandle,cJSON_Struct **pTempJson)
 {
	 return Module_LoadTempData((ModuleHandle_T)hAccessHandle,pTempJson);
 }
 S32 Access_SaveTempData(AccessHandle_T hAccessHandle,cJSON_Struct *pTempJson)
 {
	 return Module_SaveTempData((ModuleHandle_T)hAccessHandle,pTempJson);
 }

 S32 Access_LoadConfigByType(AccessHandle_T hAccessHandle,Access_ConfigType_E nType,cJSON_Struct **pConfigJson)
 {
	 return Module_LoadConfigByType((ModuleHandle_T)hAccessHandle,(Module_ConfigType_E)nType,pConfigJson);
 }
 S32 Access_SaveConfigByType(AccessHandle_T hAccessHandle,Access_ConfigType_E nType,cJSON_Struct *pConfigJson)
 {
	 return Module_SaveConfigByType((ModuleHandle_T)hAccessHandle,(Module_ConfigType_E)nType,pConfigJson);
 }


 // 订阅操作接口
 S32 Access_SubscribeEvent(AccessHandle_T hAccessHandle,S8 *szSubscribeUri,cJSON_Struct *pInParams,cJSON_Struct *pOutParams,Access_Events_Def fxn,void *pUserData)
 {
	 return Module_SubscribeEvent((ModuleHandle_T)hAccessHandle,szSubscribeUri,(Module_Events_Def)fxn,pUserData);
 }
 S32 Access_UnSubscribeEvent(AccessHandle_T hAccessHandle,S32 nSubscribeID)
 {
	 return Module_UnSubscribeEvent((ModuleHandle_T)hAccessHandle,nSubscribeID);
 }
 S32 Access_QueryEvent(AccessHandle_T hAccessHandle,S32 nSubscribeID,cJSON_Struct **pOutEventInfo,int nMSecTimeOut)
 {
	 return Module_QueryEvent((ModuleHandle_T)hAccessHandle,nSubscribeID,pOutEventInfo,nMSecTimeOut);
 }
 // 被订阅操作接口
 S32 Access_RegisterSubscribe(AccessHandle_T hAccessHandle,S8 *szSubscribeUri,Access_Subscribe_Def fxn,void *pUserData)
 {
	 return Module_RegisterSubscribe((ModuleHandle_T)hAccessHandle,szSubscribeUri,(Module_Subscribe_Def)fxn,pUserData);
 }
 S32 Access_UnRegisterSubscribe(AccessHandle_T hAccessHandle,S8 *szSubscribeUri)
 {
	 return Module_UnRegisterSubscribe((ModuleHandle_T)hAccessHandle,szSubscribeUri);
 }
 S32 Access_SendEvent(AccessHandle_T hAccessHandle,S32 nRecvID,cJSON_Struct *pEventInfo,cJSON_Struct **pOutParams,S32 nMSecTimeOut)
 {
	 return Module_SendEvent((ModuleHandle_T)hAccessHandle,nRecvID,pEventInfo,pOutParams,nMSecTimeOut);
 }
 //订阅绑定私有数据,用于保存一些私有数据，避免遍历
 S32 Access_Subscribe_SetPrivateInfo(AccessHandle_T hAccessHandle,S32 nRecvID/*or nSubscribeID*/,void *pBuff,S32 nSize)
 {
	 return Module_Subscribe_SetPrivateInfo((ModuleHandle_T)hAccessHandle,nRecvID,pBuff,nSize);
 }
 S32 Access_Subscribe_GetPrivateInfo(AccessHandle_T hAccessHandle,S32 nRecvID,void **pBuff,S32 *lpSize)
 {
	 return Module_Subscribe_GetPrivateInfo((ModuleHandle_T)hAccessHandle,nRecvID,pBuff,lpSize);
 }

S32 Access_StreamQueue_Open(AccessHandle_T hModuleHandle,char *szUri,cJSON_Struct *pOpenParams,cJSON_Struct **pStreamInfo,S32 nMSecTimeout)
{
	return Module_StreamQueue_Open((ModuleHandle_T)hModuleHandle,szUri,pOpenParams,pStreamInfo,nMSecTimeout);
}

 S32 Access_StreamQueue_CoOpen(AccessHandle_T hModuleHandle,S32 fd,CoOpen_Param_T *pSets,S32 nSetsNum,S32 nMSecTimeout)
 {
	 return Module_StreamQueue_CoOpen((ModuleHandle_T)hModuleHandle,fd,pSets,nSetsNum,nMSecTimeout);
 }

S32 Access_StreamQueue_Close(AccessHandle_T hModuleHandle,S32 fd)
{
	return Module_StreamQueue_Close((ModuleHandle_T)hModuleHandle,fd);
}
S32 Access_StreamQueue_WriteData(AccessHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pPrivInfo, void *pData1, S32 nSize1, void *pData2, S32 nSize2)
{
	return Module_StreamQueue_WriteData((ModuleHandle_T)hModuleHandle,fd,nIndex,pPrivInfo,pData1,nSize1,pData2,nSize2);
}
S32 Access_StreamQueue_ReadData(AccessHandle_T hModuleHandle,S32 fd,S32 nIndex,S32 *lpIndex,cJSON_Struct **pPrivInfo,void **pData, S32 *lpSize, S32 nTimeout)
{
	return Module_StreamQueue_ReadData((ModuleHandle_T)hModuleHandle,fd,nIndex,lpIndex,pPrivInfo,pData,lpSize,nTimeout);
}
S32 Access_StreamQueue_ReleaseData(AccessHandle_T hModuleHandle,S32 fd)
{
	return Module_StreamQueue_ReleaseData((ModuleHandle_T)hModuleHandle,fd);
}
S32 Access_StreamQueue_EPoll_Create(StreamQueue_EPollHandle_T *pHandle,S32 nSize)
{
	return Module_StreamQueue_EPoll_Create(pHandle,nSize);
}
S32 Access_StreamQueue_EPoll_Ctl(StreamQueue_EPollHandle_T hHandle,S32 nOp,S32 nFd,StreamQueue_EPoll_Event_T *pEvent)
{
	return Module_StreamQueue_EPoll_Ctl(hHandle,nOp,nFd,pEvent);
}
S32 Access_StreamQueue_EPoll_Wait(StreamQueue_EPollHandle_T hHandle,StreamQueue_EPoll_Event_T *pResults,S32 nMaxResultsNum,S32 nTimeout)
{
	return Module_StreamQueue_EPoll_Wait(hHandle,pResults,nMaxResultsNum,nTimeout);
}
S32 Access_StreamQueue_EPoll_Destroy(StreamQueue_EPollHandle_T *hHandle)
{
	return Module_StreamQueue_EPoll_Destroy(hHandle);
}

S32 Access_StreamQueue_Control(AccessHandle_T hModuleHandle,S32 fd,S32 nIndex,cJSON_Struct *pControlInfo,cJSON_Struct **pResults,S32 nMSecTimeOut)
{
	return Module_StreamQueue_Control((ModuleHandle_T)hModuleHandle,fd,nIndex,pControlInfo,pResults,nMSecTimeOut);
}


