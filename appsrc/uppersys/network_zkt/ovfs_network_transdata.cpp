#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include<netinet/in.h>

#include "ovfs_comm_def.h"
#include "ovfs_network_transdata.h"
#include "ovfs_network_rest_common.h"


typedef struct
{
    int nEnable;
    char pDestIp[256];
    int nDestPort;
    int nSrcPort;
    int nSrcServerFd;
    int bStart;
} NETWORK_TRANSDATA_T;

static Common_Thread_T m_udp_thread = NULL;
static NETWORK_TRANSDATA_T s_network_transdata;

static int NetWork_TranData_Cfg_Json_To_Struct(cJSON_Struct *parentItem)
{
    int nChanged = 0;
	S8 *pStringValue;
	S32 nIntValue;

    if(Common_Json_GetAttrValueInt(parentItem, "Enable", &nIntValue) &&
       s_network_transdata.nEnable != nIntValue)
	{
		s_network_transdata.nEnable = nIntValue;
        nChanged = 1;
	}

    if(Common_Json_GetAttrValueStr(parentItem, "DestIp", &pStringValue) &&
       Common_StrCmp(s_network_transdata.pDestIp, pStringValue) != 0)
    {
        snprintf(s_network_transdata.pDestIp,
            sizeof(s_network_transdata.pDestIp),
            "%s", pStringValue);
    }

    if(Common_Json_GetAttrValueInt(parentItem, "DestPort", &nIntValue) &&
       s_network_transdata.nDestPort != nIntValue)
	{
		s_network_transdata.nDestPort = nIntValue;
	}

    if(Common_Json_GetAttrValueInt(parentItem, "SrcPort", &nIntValue) &&
       s_network_transdata.nSrcPort != nIntValue)
	{
		s_network_transdata.nSrcPort = nIntValue;
        nChanged = 1;
	}

    return nChanged;
}

S32 NetWork_Get_TransData_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}

    Common_Json_SetAttrValueInt(parentItem, "Enable", s_network_transdata.nEnable);
    Common_Json_SetAttrValueStr(parentItem, "DestIp", s_network_transdata.pDestIp);
    Common_Json_SetAttrValueInt(parentItem, "DestPort", s_network_transdata.nDestPort);
    Common_Json_SetAttrValueInt(parentItem, "SrcPort", s_network_transdata.nSrcPort);

	return 0;
}

S32 NetWork_Put_TransData_Json(cJSON_Struct *parentItem)
{
	if (NULL == parentItem)
	{
		return -1;
	}

	int changed = NetWork_TranData_Cfg_Json_To_Struct(parentItem);
    if(changed)
    {
        NetWork_TransData_CheckStatus();
    }

	return 0;
}

int send_udp_msg(char *pData, int nDataLen)
{
    int ret = 0;
    struct sockaddr_in dest_addr;

    if (0 > s_network_transdata.nSrcServerFd ||
        strlen(s_network_transdata.pDestIp) == 0)
    {
        LOGE("nSrcServerFd not ready![%d] [%s]\n",s_network_transdata.nSrcServerFd,s_network_transdata.pDestIp);
        return -1;
    }

    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(s_network_transdata.nDestPort); // 目标端口号
    inet_pton(AF_INET, s_network_transdata.pDestIp, &dest_addr.sin_addr); // 目标IP地址
    //dest_addr.sin_len = sizeof(dest_addr);

    ret = sendto(s_network_transdata.nSrcServerFd, pData, nDataLen, 0,
                   (struct sockaddr *)&dest_addr, sizeof(struct sockaddr));
    LOGD("ret:[%d]\n",ret);
    return 0;
}

S32 NetWork_Put_DoTransData_Json(cJSON_Struct *parentItem)
{
    int nLen = 0;
    int nBase64Len = 0;
    char *pBuf = NULL;
    char *pBase64Buf = NULL;

    if(Common_Json_GetAttrValueStr(parentItem, "Base64Str", &pBase64Buf))
    {
        nBase64Len = strlen(pBase64Buf);
        if(nBase64Len > 0)
        {
            pBuf = Common_Base64_Decode(pBase64Buf, nBase64Len, (U32 *)&nLen);
            send_udp_msg(pBuf, nLen);

            Common_Free(pBuf, __FUNCTION__, __LINE__);
        }
    }

	return 0;
}

int PtzWrite(unsigned char* pBuf, int nBufLen)
{
    int nRet = 0;
    int nBase64Len = 0;
    char *pBase64Buf = NULL;
    cJSON_Struct *pInData = NULL,*pOutData = NULL;

    pInData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueObj(pInData, "Header");
    Common_Json_SetAttrValueStr(pInData, "Header/Uri", "/Ptz/udpsend");
    Common_Json_SetAttrValueStr(pInData, "Header/Method", "put");
    Common_Json_SetAttrValueObj(pInData, "Data");

    pBase64Buf = Common_Base64_Encode((S8 *)pBuf, (U32)nBufLen, (U32 *)&nBase64Len);
    Common_Json_SetAttrValueStr(pInData, "Data/Base64Str", pBase64Buf);

    ovfs_print_json(pInData);

    nRet = Module_CallFunctions(NetWork_RestComm_GetModuleHdl(),pInData,&pOutData,3000);
    LOGD("nRet:[%d]\n", nRet);

    Common_Free(pBase64Buf, __FUNCTION__, __LINE__);
    Common_Json_Delete(pInData);
    Common_Json_Delete(pOutData);

    return nRet;
}


void handle_udp_msg(int fd)
{
    int BUFF_LEN = 1024;
    unsigned char buf[1024];  //接收缓冲区，1024字节
    int len;
    int count;
    struct sockaddr_in client_addr;  //clent_addr用于记录发送方的地址信息
    while(s_network_transdata.bStart && s_network_transdata.nEnable)
    {
        memset(buf, 0, BUFF_LEN);
        len = sizeof(client_addr);
        count = recvfrom(fd, buf, BUFF_LEN, 0, (struct sockaddr*)&client_addr, (socklen_t*)&len);  //recvfrom是拥塞函数，没有数据就一直拥塞
        //LOGD("count:[%d]\n",count);
        if(count == -1)
        {
            if(GetLastError()== EAGAIN)
            {
                //LOGE("recieve data timeout!\n");
                continue;
            }
            else
            {
                LOGE("recieve data fail!\n");
            }
            return;
        }
        else
        {
            if(count>0)//向串口发送数据
            {
            	PtzWrite(buf, count);
            }
        }
    }
 }

int UDPServerThread(Common_Thread_T hThreadHandle,void *pUserData)
{
	//设置一个socket地址结构serverAddr,代表服务器internet地址, 端口
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); //把一段内存区的内容全部设置为0
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl (INADDR_ANY);;

    serverAddr.sin_port = htons(s_network_transdata.nSrcPort);//upd端口

	//创建用于internet的流协议(UDP)socket,用server_socket代表服务器socket
    s_network_transdata.nSrcServerFd = socket(AF_INET,SOCK_DGRAM,0);
    if (-1 == s_network_transdata.nSrcServerFd)
    {
        LOGE("Create UDP Socket Failed!");
        return -1;
    }
    LOGW("UDP socket create successfully.\n");

	//把socket和socket地址结构绑定
    if(bind(s_network_transdata.nSrcServerFd,(struct sockaddr*)&serverAddr,sizeof(struct sockaddr)))
    {
        LOGE("Bind UDP Port[%d]\n", htons(serverAddr.sin_port));
        return -1;
    }
    LOGD("Bind UPD successful.Port[%d]\n",  htons(serverAddr.sin_port));

        /* 设置阻塞超时 */
    struct timeval timeOut;
    timeOut.tv_sec = 1;                 //设置1s超时
    timeOut.tv_usec = 0;
    if (setsockopt(s_network_transdata.nSrcServerFd, SOL_SOCKET, SO_RCVTIMEO, &timeOut, sizeof(timeOut)) < 0)
    {
        LOGE("time out setting failed\n");
    }

	handle_udp_msg(s_network_transdata.nSrcServerFd);   //处理接收到的数据
LOGD("exit\n");
    close(s_network_transdata.nSrcServerFd);
    s_network_transdata.nSrcServerFd = -1;
    return 0;
}

S32 NetWork_TransData_CheckStatus()
{
    LOGD("bStart:[%d] nEnable:[%d]\n",s_network_transdata.bStart,s_network_transdata.nEnable);
    if(s_network_transdata.bStart && s_network_transdata.nEnable)
    {
        if(m_udp_thread == NULL)
        {
            Common_Thread_Create(&m_udp_thread, __FUNCTION__, 0, 0, UDPServerThread, NULL);
        }
    }
    else
    {
        close(s_network_transdata.nSrcServerFd);
        LOGW("Common_Thread_Destroy! [%p]\n", m_udp_thread);
        Common_Thread_Destroy(&m_udp_thread);
        LOGW("Common_Thread_Destroy ok!\n");
    }

    return 0;
}

S32 NetWork_TransData_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	cJSON_Struct *pJsonTmp = NULL;

    COMMON_CLR_ARG(s_network_transdata);

	pJsonTmp = Common_Json_GetItem(pJson, -1, "NetApp/TransData");
	if (NULL == pJsonTmp)
	{
	   LOGE("get TransData fail\n");
       pJsonTmp = Common_Json_GetItem(pJsonDefault, -1, "NetApp/TransData");
	}

	if (NULL == pJsonTmp)
	{
        LOGE("get Default TransData fail\n");
	}

	NetWork_TranData_Cfg_Json_To_Struct(pJsonTmp);

	return 0;
}

S32 NetWork_TransData_Start(int status)
{
    LOGD("status:[%d] bStart:[%d]\n",status,s_network_transdata.bStart);
    if(s_network_transdata.bStart != status)
    {
        s_network_transdata.bStart = status;

        NetWork_TransData_CheckStatus();
    }

	return 0;
}

S32 NetWork_TransData_Destroy()
{
    NetWork_TransData_Start(0);
	return 0;
}

