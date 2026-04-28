#ifndef __ACCESS_USERCFG_H__
#define __ACCESS_USERCFG_H__
#include "libcommon_api.h"
#define ACCESS_USERCFG_PWD_MAX_NUM 8

#define RESET_PWD_MAX_NUM	16

#define USER_CONFIG_PATH		"/usr/etc/cfgfiles/Access.json"

#define ERR_NONE				0
#define ERR_NORMAL				-1001
#define ERR_USER_NOTEXIST		-1002
#define ERR_PWD_INVALID		-1003
#define ERR_SERIALNO_INVALID	-1004
#define ERR_TOKEN_INVALID		-1005
#define ERR_TIME_INVALID		-1006
#define ERR_RETRY_INVALID		-1007
#define ERR_INFO_INVALID		-1008

#define BIT_PREVIEW			(0x1<<0)
#define BIT_PLAYBACK 		(0x1<<1)
#define BIT_SETTING 			(0x1<<2)
#define BIT_VIEWSETTING 	(0x1<<3)
#define BIT_RECORD 			(0x1<<4)
#define BIT_PTZ			 	(0x1<<5)
#define BIT_BACKUP		 	(0x1<<6)
#define BIT_LOG			 	(0x1<<7)
#define BIT_VIEWINFO		(0x1<<8)
#define BIT_UPGRADE		 	(0x1<<9)
#define BIT_POWER			(0x1<<10)
#define BIT_FORMAT		 	(0x1<<11)
#define BIT_IPCHANNEL		(0x1<<12)
#define BIT_CORRECTTIME		(0x1<<13)
#define BIT_DELETEUSER		(0x1<<14)
#define BIT_MODIFYUSER 		(0x1<<15)
#define BIT_RESETPWD 		(0x1<<16)
#define BIT_RESETPWDSELF	(0x1<<17)
#define BIT_ONLINEAUTH 		(0x1<<18)
#define BIT_REMOTETALK 		(0x1<<19)

#define BIT_CHAN_PREVIEW			(0x1<<0)
#define BIT_CHAN_PLAYBACK 			(0x1<<1)
#define BIT_CHAN_SETTING 			(0x1<<2)
#define BIT_CHAN_VIEWSETTING 		(0x1<<3)
#define BIT_CHAN_RECORD 			(0x1<<4)
#define BIT_CHAN_PTZ			 	(0x1<<5)
#define BIT_CHAN_BACKUP		 	(0x1<<6)

#define CLEAR_TEXT		0
#define CIPHER_TEXT		1
#define SYSTEM_TEXT		CIPHER_TEXT

#define MAX_MODULE_COUNT   16
#define MAX_LOGIN_USER   256

#define MAX_IPTABLE_COUNT   100

#define  ovfs_print_json(JDATA) do{\
	char *ptr = NULL;\
	ptr = Common_Json_Print(JDATA,NULL);\
	if(ptr)\
	{\
		LOGI("%s\n",ptr);\
		Common_Free(ptr, __FUNCTION__,__LINE__);\
	}\
}while(0)

enum{
	LEVEL_GUEST = 0x00,
	LEVEL_DEFAULT = 0x01,
	LEVEL_OPERATOR = 0x02,
	LEVEL_ADMIN = 0x10,
	LEVEL_ROOT = 0xff
};

typedef struct _tagAccessChanRight
{
	U16 wDeviceNo; // 设备号 0,1,2...
	U16 wChannelNo; // 通道号 0,1,2...
	unsigned int bPreview:1; // 预览
	unsigned int bPlayback:1; // 回放
	unsigned int bSetting:1;	// 设置
	unsigned int bViewSetting:1; //查看
	unsigned int bRecord:1; // 录像
	unsigned int bPTZ:1; //PTZ
	unsigned int bBackup:1; // 备份
	// 8
	unsigned int bRes:25; // 保留
}AccessChanRight_T;

typedef struct _tagAccessUserRight
{
	U32 bPreview:1; // 预览权限
	U32 bPlayback:1; // 回放权限
	U32 bSetting:1; //设置
	U32 bViewSetting:1;// 查看配置
	U32 bRecord:1;//录像权限
	U32 bPtz:1;// PTZ 
	U32 bBackup:1;// 备份
	U32 bLog:1; // 日志

	U32 bViewInfo:1; // 查看设备信息
	U32 bUpgrade:1; // 升级
	U32 bPower:1; // 电源管理 关机，重启
	U32 bFormat:1; // 格式化 
	U32 bIPChannel:1; // IP 通道
	U32 bCorrectionTime:1;//校正时间/修改时间, 修改时间可能会影响用户的失效时间 
	U32 bDeleteUser:1; // 删除用户 ,只能操作比自己等级低的
	U32 bModityUser:1;// 修改用户 ,只能操作比自己等级低的
	
	U32 bResetPassword:1;// 重置密码权限 ,只能操作比自己等级低的
	U32 bResetPasswordSelf:1;// 重置自己密码权限
	U32 bOnlineAuthority:1;// 有在线授权的权限
	U32 bRemoteTalk:1;// 远程对讲
	U32 bRes:12; //

	AccessChanRight_T *pChanRight; // 默认是有全权限的，即pChanRight == NULL 时有全权限,!=NULL 时按实际权限走.
	S32 nChanRightCount;

}AccessUserRight_T;

typedef struct _tagIpConnectInfo
{
	S32 hSessionId;
	S8 *szBindIpv4;
	S8 *szBindIpv6;
	S8 *szBindMac; // 绑定MAC，形如 "00:3d:66:ee:02:56"
	S32 tCreateTime;
}IpConnectInfo_T;	

typedef struct _tagBindInfo
{
	S8 *szBindIpv4;			//start ipv4
	S8 *szBindEndIpv4;		//end ipv4
	S8 *szBindIpv6;			//start ipv6
	S8 *szBindEndIpv6;		//end ipv6
	S8 *szBindMac; 
	S8 iDirection;			//0:all 1:in 2:out
	S8 iProtocol;				//0:all 1:tcp 2:udp
}BindInfo_T;

#define ACCESS_USERCFG_AUTH_MAX_NUM 8
#define OVFS_IPCONNECT_MAX_NUM 256
typedef struct _tagAccessUserCfg
{
	S8 *szUserName;
	S8 byPwdCount;
	S8 *szPassword[ACCESS_USERCFG_PWD_MAX_NUM]; // 一用户，多密码联合验证方式,一般用于异地在一个时间范围内登陆
	S8 *szDefaultPassword; // 缺省密码
	S8 *szTempPassword; 	// 找回密码时申请的临时密码
	S8 *szSerialNumber; 		// 找回密码时设备序列号
	S32 tTempCreateTime;	//临时密码创建时间
	S32 tTempValidTime;		//临时密码有效期
	U8 tFailTime;			//临时密码失败次数
	U8 byEncryptMethod; // 0- 明文 1-加密
	U8 byPriority; //用户等级 0-禁止用户,1-访客，2-操作员,10-管理员,0xFF-顶级管理员（类似root）
	U8 bForbidden; // 0-不禁用 1-禁用
	U8 bNeedOnlineAuth;// 需要在线授权才能登陆
	S8 byOnlineAuthListCount;
	S8 *szOnlineAuthManList[ACCESS_USERCFG_AUTH_MAX_NUM];// 指定授权人列表，不指定，则有相关权限的都可以授权;在用户登陆时，授权人都应会收到通知，询问授权请求
	U8 bRemote;
	S8 *szBindIpv4;
	S8 *szBindIpv6;
	S8 *szBindMac; // 绑定MAC，形如 "00:3d:66:ee:02:56"
	AccessUserRight_T *pLocalRight; // 不存在时，表示 默认的权限 ,权限修改只能在等级范围内修改，不能越等级修改
	AccessUserRight_T *pRemoteRight;
	Common_Time_T *pCreateTime; // 用户创建的时间
	Common_Time_T *pStopTime;// 用户失效的时间,失效后 nForbidden将自动默为1,通过改时间都不会再恢复有效

	S32 nSessionCnt;
	S32 hSessionId[MAX_LOGIN_USER];

	S32 nIpConnCnt;				//不同的IP数
	IpConnectInfo_T *pIpConnList[OVFS_IPCONNECT_MAX_NUM];

   //  一般只有同等级的用户才需要绑定登陆,当不同等级绑定时，可能会有让低级用户获取到高级用户权限的风险。
	struct _tagAccessUserCfg *pBindFromUser; // 哪些用户绑在此用户上;绑定的用户必须规定时间内一起登陆才有效,不能独立登陆使用,权限以此用户为准
	struct _tagAccessUserCfg *pBindToUser; // 绑在哪个用户上
	struct _tagAccessUserCfg *pBindUserPrev;
	struct _tagAccessUserCfg *pBindUserNext;

	struct _tagAccessUserCfg *pPrev;
	struct _tagAccessUserCfg *pNext;

}AccessUserCfg;

// 系统默认固定有二个用户
// admin - 0xFF 顶级管理员 密码:12345678
// guest - 1    访客       密码:guest

// 用户不能自己改自己权限
// 顶级管理员不能重置自己的密码
// 一般管理员能否重置自己的密码，要看是否被顶级管理员授权
// 一般管理员不能重置其他管理密码的密码
// 平级用户不能互相修改用户相关信息
// 权限对照表:
/*
权限          访客           操作员      管理员   
预览（总开关）    1             1               1            
回放              0             1               1
设置              0             0               1
查看设置          0             1               1    
录像              0             0               1
PTZ               0             1               1
备份              0             0               1
日志              0             1               1
设备信息          0             1               1    
升级              0             0               1
修改时间          0             0               1    
删除用户          0             0               1   
修改用户          0             0               1   
重置密码          0             0               1    
重置自己的密码    0             1               1          
在线授权          0             0               1   
预览 (通道)       1             1               1     
回放              0             1               1
设置              0             0               1
查看设置          0             1               1   
录像              0             0               1
PTZ               0             1               1
备份              0             0               1

*/


typedef struct _tagAccessUserCfgMgr
{
	AccessUserCfg *pUserCfg;
	S32 nUserCfgCount;
	S32 nOnlineUserCount;
	AccessUserRight_T tMaxLocalRight[3]; // 3种缺省权限 0访客，1操作员，2管理员 ,(注:顶级管理员，拥有所有权限)
	AccessUserRight_T tMaxRemoteRight[3];

	S32 iAccessMode;							//0:disable 1:enable whitelist 2:enable blacklist
	S32 starttime;			
	S32 endtime;
	BindInfo_T *pWhiteList[MAX_IPTABLE_COUNT];
	BindInfo_T *pBlackList[MAX_IPTABLE_COUNT];
}AccessUserCfgMgr_T;

typedef struct _tagAccessSubscribeNode
{
	S32 nRecvID;
	S8 *szSubscribeUri;
	S8 *szValidUri;
	void *pCondition;
}Access_SubscribeNode,*PAccess_SubscribeNode;

typedef struct _tagAccessSubscribeMgr
{
	COMMON_DLIST_T listdl;
	ModuleHandle_T hModuleHandle;
	Common_Thread_T hThread;
	Common_Lock_T  hCfgLock;
	AccessUserCfgMgr_T *pAccessUsrCfgMgr;
}Access_SubscribeMgr_T,*PAccess_SubscribeMgr_T;

typedef struct _tagModuleInfo
{
	S8 *szName;
	S32 nModuleId;
}ModuleInfo,*PModuleInfo;

typedef struct _tagOnlineModuleList
{
	S32 nModuleCount;
	ModuleInfo nModuleInfo[MAX_MODULE_COUNT];
}OnlineModuleList_T,*POnlineModuleList_T;

#endif

