#ifndef __LIBCRYPTOMGR_API__
#define __LIBCRYPTOMGR_API__
#ifdef WIN32
#define LIBCRYPTOMGR_API __declspec(dllexport)
#else
#define LIBCRYPTOMGR_API extern "C"
#endif

///////// 1. 通用接口 ///////////////////////////////
LIBCRYPTOMGR_API int CryptoMgr_Init();

LIBCRYPTOMGR_API void CryptoMgr_Free(void *lpMem);
/*
本库内部分配内存，由此接口释放
*/
LIBCRYPTOMGR_API int CryptoMgr_UnInit();

///////////////////////2. 中心服务器 接口 //////////////////////////
LIBCRYPTOMGR_API char * CryptoMgr_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,int *lpOutLen); // return char *szOutString
/*
	szSN:由中心服务器生成的序列号
	szDevInfo: 由设备提供
	lpOutLen:输出结果的长度，包含'\0'
	返回值：成功则为生成的密文结果,不使用时，需要用CryptoMgr_Free释放
*/
LIBCRYPTOMGR_API char * CryptoMgr_BadBoy(int nLevel/* = 0*/,char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,int *lpOutLen); // return char *szOutString
/*
	生成自毁码,对非法激活设备进行非激活处理
	nLevel:维护等级,0-低级处理，可以允许后期授权
*/


///////////////// 3. 烧录接口///////////////////////////////////////////////////////////////////////////////////
// Rest:(UDP/TCP)+HTTP+JSON
// 1. 搜索(广播方式),格式支持HTTP协议
/*
烧录与设备交互资源
Rest（Broadcast/HTTP）:
// 搜索设备
Uri:/Update/Discovery
TransType:Broadcast port :10008
Method:Get
Data:
{
	Status:Activated/Unactivated/Illegal 
	SN:"xxx...xxxx"
	Port:"xx"
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx"
	Mac:"xx:xx:xx:xx:xx:xx"
	UUID:"xxx"
	Hardware:"xxx"
}
// 修改IP
Uri:/Update/NetConfig
TransType:Broadcast port :10008
Method:Put
InData:
{
	Ipv4:"xxx.xxx.xxx.xxx"
	Netmask:"xxx.xxx.xxx.xxx",
	Mac:"xx:xx:xx:xx:xx:xx"
}

// 烧写授权信息（需要验证 digest）
Uri:/Update/Crypto/Burn
TransType:http port:10008
Method:Put
InData:
{
	Crypto:xxx
}


*/


/*
示例
1.搜索
GET /Update/Discovery HTTP/1.1\r\n
Content-Length: 0\r\n
Content-Type: application/json; charset=UTF-8\r\n
\r\n
应答:
HTTP/1.1 200 OK\r\n
Content-Length: xxx\r\n
Content-Type: application/json; charset=UTF-8\r\n
\r\n
{
Status:Activated/Unactivated/Illegal 
SN:"xxx...xxxx"
Port:"xx"
Ipv4:"xxx.xxx.xxx.xxx"
Netmask:"xxx.xxx.xxx.xxx"
Mac:"xx:xx:xx:xx:xx:xx"
}

2.烧写授权信息
PUT /Update/Crypto/Burn HTTP/1.1\r\n
Authorization: Digest xxx
Content-Length: 0\r\n
Content-Type: application/json; charset=UTF-8\r\n
\r\n
{
	Crypto:xxx
}
应答:
HTTP/1.1 200 OK\r\n
Content-Length: 0\r\n
Content-Type: application/json; charset=UTF-8\r\n
\r\n


*/
// Digest 验证接口,获取验证的 Response
// 为了避免直接暴露烧录密码，使用此接口代替验证。
// szUserName:"Admin"
// szPassword: 必须 NULL 
// 示例
// CryptoMgr_BurnAuth("Admin",NULL,"591bfea2ca530000605b","Burn@Update.x","333","1","auth","PUT","/update/Crypto/burn",szResponse);

LIBCRYPTOMGR_API int CryptoMgr_BurnAuth(char *szUserName,char *szPassword,char *szNonce,char *szRealm,char *szCNonce,char *szNc,char *szQop,char *szMethod,char *szUri,char *szOutResponse/* [32 + 1]*/); 
#if 1
// 搜索回调定义
/*
szJsonString:
{
Status:Activated/Unactivated/Illegal ,
MLevel:0
SN:"xxx...xxxx",
Port:"xx",
Ipv4:"xxx.xxx.xxx.xxx",
Netmask:"xxx.xxx.xxx.xxx",
Mac:"xx:xx:xx:xx:xx:xx",
}
*/

typedef int (*CRYPTO_SEARCH_LAN_CALLBACK_DEF)(char *szFromIP,int nFromPort,void *pJsonResult,void *pUserData);

// 搜索设备接口
LIBCRYPTOMGR_API int CryptoMgr_Lan_StartSearch(int nSearchPort/*=10008*/,int nIntervalSec,int nCount,CRYPTO_SEARCH_LAN_CALLBACK_DEF fxn,void *pUserData); // 
LIBCRYPTOMGR_API int CryptoMgr_Lan_StopSearch(); //

// 烧录接口
LIBCRYPTOMGR_API int CryptoMgr_Lan_Broadcast_Burn(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szMac,char *szUUID,char *szCryptoInfo); // return char *szOutString
/*
	szMac:待烧录设备的MAC地址，需要修改IP时使用
	szNewIpv4: 需要修改的IP，子网掩码默认255.255.255.0 当未提供szMac时，则不修改，直接以此IP为设备IP进行操作
	szCryptoInfo:待烧录的密文
	返回值：0-成功, < 0 失败
*/

// 烧录接口
LIBCRYPTOMGR_API int CryptoMgr_Lan_Broadcast_BurnUUIDKey(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szUUIDReadyKey,char *szUUIDKeyInfo); // return char *szOutString

#endif
/*
szRequireString:
{
	"Header":
	{
		"Uri":"GetCertificate"
	}
	"Data":
	{
		"SerialNumber":"",
		"UUID":"",
		"Hardware":"",
		"Time":"",
		"Key":""

	}
}
szResponseJson:
{
	"Header":
	{
		"Code":0
		"Describe":""
	}
	"Data":
	{
		"Certificate":""
	}
}
*/
LIBCRYPTOMGR_API int CryptoMgr_Wan_Register(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szOrderNo,char *szHardware,char *szUUID,char **szNewSerialNumber,char **szNewOrderNo,char **szCryptoInfo); // return char *szOutString
LIBCRYPTOMGR_API int CryptoMgr_Wan_GetUUIDKey(char *szIpv4,int nPort,char *szUserName,char *szPassword,char *szHardware,char **szUUIDKeyInfo); // return char *szOutString


LIBCRYPTOMGR_API int CryptoMgr_Wan_Require(char *szIpv4,int nPort,char *szUserName,char *szPassword,void *pInJson,void **pOutJson); // return char *szOutString

#endif
