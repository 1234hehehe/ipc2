#ifndef __UPDATE_STRUCT_H__
#define __UPDATE_STRUCT_H__
#include "libcommon_api.h"
#ifdef WIN32
#include <winsock2.h>
#include <Windows.h>
#include <Mstcpip.h>
#include <ws2tcpip.h>
#include <Iphlpapi.h>
#include <direct.h>

#endif

#define UPDATE_MAKEFOURCC(ch0, ch1, ch2, ch3) ((U32)(U8)(ch0) | ((U32)(U8)(ch1) << 8) | ((U32)(U8)(ch2) << 16) | ((U32)(U8)(ch3) << 24 ))
#define UPDATE_PACKET_STARTCODE UPDATE_MAKEFOURCC('o','n','v','s')



typedef struct _tagPacketHeader
{
	U32 uStartCode;// 数据同步码 
	S32 nDataLen;// 数据长度.
	U32 uDataFormat:8;// 0- JSON
	U32 uExternHeaderLen:8;// 扩展头长度
	U32 uSeq:15;
	U32 bResp:1; // 0-请求 ,1-应答
	U32 uMsgId:16;// 整包消息编号，便于识别对应包
	U32 uRes:16;
}PacketHeader_T;

typedef struct _tagUDPPacketExtHeader
{
	U32 uType:8;// 1- udp
	U32 uRes:24;
	U32 uTotal;// 总长度
	U32 uTotalPkt:16;// 总包个数
	U32 uPktIdx:16; // 当前包索引 0- 开始
}UDPPacketExtHeader_T;


typedef struct _tagUpdateClientInfo
{
	S32 bNeedDelete;
	S32 nSocket;
	S32 bTcp;
	S8 *pRecvBuffer; // 未完成消息时的临时缓冲。
	S32 nRecvLen; // 已使用的大小
	S32 nNeedLen; // 总需要接收的大小

	char *pSendBuffer;
	int nSendLen;
	int nNeedSendLen;

	PacketHeader_T tHeader;
	S32 nHeaderLen;
	void *pMgr;
	//char *ifr_name;
	// http
	S32 bHttp;// 1-是HTTP,0-私有协议
	S32 bHeaderChecked; // 0- 需要检查是不是HTTP协议,

	CommonHttpContext_T tHttpContext;

	
	


	//udp
	U32 uClientIpv4;
	U16 uClientPort;
	UDPPacketExtHeader_T tExtHeader;
	U32 uLastTime; // 上一次时间
	
	

	Common_Thread_T hThread;
	S32 bThreadExit;

}UpdateClientInfo_T;

typedef S32 (*Update_CallFunctions_Def)(char *ifr_name,UpdateClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData);

#define MAX_LICENCESERVER_DIGEST_NUM  10
typedef struct _tagUpdateDigest
{
	S8 szNonce[20 + 1];
	S8 szOpaque[8 + 1];
	S32 nLastNonceGenTime;
}UpdateDigest_T;

#define UPDATE_UDP_CLIENT_NUM 8
typedef struct _tagUpdateMgr
{
	//tcp
	S32 nListenPort_Tcp;
	S32 nSocketTcp;
	COMMON_DLIST_T socketTcpList;
	Common_Thread_T hThread_Tcp;
	//udp broadcast
	S32 nListenPort_Udp;// udp,broadcast
	S32 nSocketUdp;
	S32 bBroadcast;
	COMMON_DLIST_T socketUdpList;
	Common_Thread_T hThread_Udp;
	S32 bListenThreadExit;

	Update_CallFunctions_Def fCallback;
	void *pCallbackUser;

	UpdateClientInfo_T *pClientInfo_Udp[UPDATE_UDP_CLIENT_NUM];
	S32 nClientInfoCount;

	//digest
	UpdateDigest_T tDigest[MAX_LICENCESERVER_DIGEST_NUM];
	int nDigestCount;
	Common_Lock_T tDigestLock;
	
}UpdateMgr_T;




S32 UpdateOpenSocket(S32 bTcp,S32 nPort,char *dev,char *ip);
S32 Update_Tcp_Start(UpdateMgr_T *pMgr);
S32 Update_Udp_Start_V2(UpdateMgr_T *pMgr);
S32 Update_Require(char *pDomain,S32 nPort,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,S32 nTimeOut);

S32 update_crypto_CallFunctions(UpdateClientInfo_T *pClientInfo,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData);

#define UPDATE_DEBUG printf
#define UPDATE_INFO printf
#define UPDATE_ERROR printf

#endif

