/**
 *   \file ovfs_smart_proto.cpp
 *   \brief OVFS HTTP 人脸信息应用协议
 *   \date 2019-07-24
 *   \author eric
 *   Detailed description
 *
 */
#if (defined WITH_CURL)
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <resolv.h>

#define CURL_DISABLE_TYPECHECK
#include "curl/curl.h"
#include "ovfs_media.h"


#define SMART_PROTO_URL_PUT_HEARTBEAT  "DeviceEndianHeartbeat"
#define SMART_PROTO_URL_CONFIG_COMMAND   "DeviceEndianCommand"
#define SMART_PROTO_POST_LIST_MAX_NODE 8

enum
{
    SMART_PROTO_STATE_STOP = 0,
    SMART_PROTO_STATE_START,
};

typedef struct
{
	MQ_HANDLE_H mqHandle;
    int  state;
    char serialNum[128];
    char devType[32];
    HTTP_PUSH_CFG_T cfg;
    SMART_PROTO_NOTDISTURB_CFG_T notdisturbcfg;
    int  eventPostEnable;
    int  eventPicEnable;
	int  startCommand;
    unsigned long long int postCnt;
    pthread_t heartBeatPth;
	pthread_t checkCommandPth;
    pthread_t checkTimePth;
} SMART_PROTO_CONTEXT_T;

typedef struct
{
	int bCheckHeader;
	int bGetData;
	char *szMatch;
	char *szMatchResult;
	char *pBuff;
	int nBuffSize;
	int nDataSize;
}HttpProtoBuffer_T;

SMART_PROTO_CONTEXT_T s_smart_proto_ct;

int closeAlarm = 0;
int filterAlarm = 0;
int closeSmartResult = 0;
int closeConfig = 0;

static void GetCurTimeStr(char timeStr[32])
{
    struct tm tmS = {};
    time_t a = time(NULL);
    localtime_r(&a, &tmS);
    char zoneStr[8] = {};
    strftime(zoneStr, sizeof(zoneStr), "%z", &tmS);
    /*string +0800 to +08:00*/
    if (strlen(zoneStr) >= 5)
    {
        zoneStr[5] = zoneStr[4];
        zoneStr[4] = zoneStr[3];
        zoneStr[3] = ':';
    }

    strftime(timeStr, 32, "%Y-%m-%dT%H:%M:%S", &tmS);
    strcat(timeStr, zoneStr);
}

static size_t HttpWriteCallBack(void *buffer, size_t size, size_t count, void *response)
{

    char *ptr = NULL;
    ptr = (char *) MEDIA_MALLOC(count * size + 4);
    memset(ptr, 0, count * size + 4);
    memcpy(ptr, buffer, count * size);
    /* Safe cast: response is actually char** passed from curl_easy_setopt CURLOPT_WRITEDATA */
    void **p = (void **) response;
    *p = ptr;

    return count;
}

static size_t HttpProtoWriteCallBack(void *buffer, size_t size, size_t count, void *response)
{

    char *ptr = NULL;
	int nBuffNum;
	HttpProtoBuffer_T *pBuffer = (HttpProtoBuffer_T *)response;
	if(pBuffer->bCheckHeader)
	{

		if(pBuffer->bGetData == 0)
		{
			//printf("here <%d/%d><%s>\n",size,count,buffer);
			if(size * count == 2 && ((char*)buffer)[0] == '\r' && ((char *)buffer)[1] == '\n')
			{
				pBuffer->bGetData = 1;
			}
			else if(pBuffer->szMatch && strstr((char *)buffer,pBuffer->szMatch))
			{
				pBuffer->szMatchResult = strdup((char *)buffer);
				if(pBuffer->szMatchResult)
				{
					pBuffer->szMatchResult[size * count - 2] = 0;
				}
			}
			return size *count;
		}
		else
		{
			//printf("haha %s\n",buffer);
			if(((char*)buffer)[0] != '{')
			{
				pBuffer->bGetData = 0;
				return size*count;
			}
		}
	}
#define SMART_PAGESIZE  4096
	 if(pBuffer->pBuff == NULL)
	 {
	 	nBuffNum = (count * size + SMART_PAGESIZE - 1 )/SMART_PAGESIZE;
    	ptr = (char *) MEDIA_MALLOC(SMART_PAGESIZE * nBuffNum);
		if(ptr)
		{
		  	pBuffer->pBuff = ptr;
			pBuffer->nBuffSize = SMART_PAGESIZE * nBuffNum;
			pBuffer->nDataSize = 0;
		}
	 }
	 else
	 {
	 	ptr = pBuffer->pBuff;
		nBuffNum = (pBuffer->nDataSize + count * size + SMART_PAGESIZE - 1 )/SMART_PAGESIZE;
		if(nBuffNum * SMART_PAGESIZE > pBuffer->nBuffSize)
		{
			ptr = (char *)Common_Realloc(ptr, nBuffNum * SMART_PAGESIZE, NULL,0);
			if(ptr != NULL)
			{
				pBuffer->pBuff = ptr;
				pBuffer->nBuffSize = nBuffNum * SMART_PAGESIZE;
			}
		}

	 }

    if(pBuffer->nBuffSize >= pBuffer->nDataSize + count * size)
	{
		 	memcpy(pBuffer->pBuff + pBuffer->nDataSize, buffer, count * size);
			pBuffer->nDataSize += count * size;
    }



    return count;
}

static int ResponseLog(CURL *curl, curl_infotype type, char *buffer, int size, void *user)
{
    if (access("/root/smart_debug", F_OK) == 0)
        printf("[HTTP] %s", buffer);
    return size;
}

static int HeartBeatCheck(char *uri, char *contentStr, char **response)
{
    CURL *curl;
    CURLcode res;
    int ret = -1;

    if (uri == NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    curl = curl_easy_init();

    if (curl == NULL)
    {
        LOGE("curl init failed\n");
        return -1;
    }

    struct curl_slist *headers = NULL;
    char tmp[128] = { 0 };

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/4.0");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");

    // snprintf(tmp,sizeof(tmp),"%s:%d",ct->serverAddr,ct->serverPort);
    headers = curl_slist_append(headers, tmp);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    memset(tmp, 0, sizeof(tmp));
    snprintf(tmp, sizeof(tmp), "%s/%s", uri,
             SMART_PROTO_URL_PUT_HEARTBEAT);

    curl_easy_setopt(curl, CURLOPT_URL, tmp);

    if (strstr(tmp, "https:") != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }
    // curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, contentStr);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpWriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response);
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1L);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/dev/null");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,(15 >>16)&0xFFFF);
    // curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, ResponseLog);
    // curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        LOGE("curl_wasy_perform error = %s\n", curl_easy_strerror(res));
        if (res == CURLE_COULDNT_RESOLVE_HOST){
            LOGW("res_init\n");
            res_init();
        }
        if (response != NULL && *response == NULL)
        {
            *response = (char *)MEDIA_MALLOC(64);
            snprintf(*response, 64, "%s", curl_easy_strerror(res));
        }
        ret = -1;
    }
    else
    {
        ret = 0;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return ret;
}

static int HeartBeatSend(SMART_PROTO_CONTEXT_T *ct, char *contentStr, char **response)
{
    CURL *curl;
    CURLcode res;
    int ret = -1;

    if (ct == NULL || contentStr == NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    curl = curl_easy_init();

    if (curl == NULL)
    {
        LOGE("curl init failed\n");
        return -1;
    }

    struct curl_slist *headers = NULL;
    char tmp[128] = { 0 };

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/4.0");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");

    // snprintf(tmp,sizeof(tmp),"%s:%d",ct->serverAddr,ct->serverPort);
    headers = curl_slist_append(headers, tmp);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    memset(tmp, 0, sizeof(tmp));
    snprintf(tmp, sizeof(tmp), "%s/%s", ct->cfg.serverAddr,
             SMART_PROTO_URL_PUT_HEARTBEAT);

    curl_easy_setopt(curl, CURLOPT_URL, tmp);

    if (strstr(tmp, "https:") != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }

    // curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, contentStr);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpWriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )response);
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1L);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/dev/null");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,(15 >>16)&0xFFFF);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, ResponseLog);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        LOGE("curl_wasy_perform error = %s\n", curl_easy_strerror(res));
        ret = -1;

        if (res == CURLE_COULDNT_RESOLVE_HOST){
            LOGW("res_init\n");
            res_init();
        }
    }
    else
    {
        ret = 0;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    return ret;
}

static int HeartBeatPack(SMART_PROTO_CONTEXT_T *ct, char **contentStr)
{

    if (contentStr == NULL || *contentStr != NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    cJSON_Struct *contentJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "DevType", Common_Json_Type_String, ct->devType,
                             0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "SerialNum", Common_Json_Type_String, ct->serialNum,
                             0, 0);

    char timeStr[32] = {};
    GetCurTimeStr(timeStr);
    Common_Json_SetAttrValue(contentJson, -1, "LocalTime", Common_Json_Type_String, timeStr,
                             0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "ReportCount", Common_Json_Type_Double, NULL,
                             0, ct->postCnt);

    *contentStr = Common_Json_Print(contentJson, NULL);
    Common_Json_Delete(contentJson);
    return 0;
}

static int HeartBeatRspHandle(SMART_PROTO_CONTEXT_T *ct, char *response)
{
    if (response == NULL || ct == NULL || ct->state != SMART_PROTO_STATE_START)
    {
        LOGE("parameters error\n");
        return -1;
    }

    int retCode = -1, pushEvent = -1, pushPic = -1, startCommand = -1;;
    char *retStr = NULL;


    cJSON_Struct *contentJson = Common_Json_Parse(response, NULL, NULL);
    if (contentJson == NULL)
        return -1;

    Common_Json_GetAttrValue(contentJson, -1, "ReturnCode", NULL, NULL, &retCode, NULL);
    Common_Json_GetAttrValue(contentJson, -1, "ReturnStr", NULL, &retStr, NULL, NULL);

    Common_Json_GetAttrValue(contentJson, -1, "PushEventInfo", &pushEvent, NULL, NULL, NULL);
    Common_Json_GetAttrValue(contentJson, -1, "PushEventPic", &pushPic, NULL, NULL, NULL);
	Common_Json_GetAttrValueBol(contentJson,  "StartCommand", &startCommand);
    LOGD("retcode %d str %s push event %d push pic %d startCommand %d\n", retCode, retStr, pushEvent , pushPic, startCommand);

    if (retCode == 0)
    {
        ct->eventPostEnable = pushEvent;
        if (pushEvent == 0)
            ct->eventPicEnable = 0;
        else
            ct->eventPicEnable = pushPic;
		if(startCommand == Common_Json_Type_False || startCommand == Common_Json_Type_True)
		{
			ct->startCommand = startCommand;
		}
    }

    Common_Json_Delete(contentJson);
    return retCode;
}


static void *HeartBeatThread(void *data)
{
    SMART_PROTO_CONTEXT_T *ct = (SMART_PROTO_CONTEXT_T *)data;
    if (ct == NULL)
    {
        LOGE("data is null\n");
        return NULL;
    }

    unsigned long long int timeA = 0;

    while(ct != NULL && ct->state == SMART_PROTO_STATE_START)
    {
        unsigned long long int timeB = Common_GetSystemCount64();
        if (timeB - timeA < ct->cfg.heartInterval * 1000LLU)
        {
            Common_Sleep(0, 500000);
            continue;
        }

        /*send heart beat */
        timeA = timeB;

        char *responseStr = NULL, *sendStr = NULL;
        int ret = 0;

        ret = HeartBeatPack(ct, &sendStr);
        if (ret == 0)
        {
        	LOGD("HeartBeat Send:\n%s\n", sendStr);
            ret = HeartBeatSend(ct, sendStr, &responseStr);
			LOGW("HeartBeat Response:[%d]\n%s\n", ret, responseStr);
        }
        if (ret == 0)
            ret = HeartBeatRspHandle(ct, responseStr);
        if (sendStr != NULL)
            MEDIA_FREE(sendStr);

        if (responseStr != NULL)
            MEDIA_FREE(responseStr);
    }

    LOGW("thread exit\n");

    return NULL;
}

static int ConfigCommandPack(SMART_PROTO_CONTEXT_T *ct, char **contentStr)
{
    if (contentStr == NULL || *contentStr != NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    cJSON_Struct *contentJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "DevType", Common_Json_Type_String, ct->devType,
                             0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "DevName", Common_Json_Type_String, NULL,
                             0, 0);
    Common_Json_SetAttrValue(contentJson, -1, "SerialNum", Common_Json_Type_String, ct->serialNum,
                             0, 0);
    char timeStr[32] = {};
    GetCurTimeStr(timeStr);
    Common_Json_SetAttrValue(contentJson, -1, "LocalTime", Common_Json_Type_String, timeStr,
                             0, 0);

    *contentStr = Common_Json_Print(contentJson, NULL);
    Common_Json_Delete(contentJson);
    return 0;
}


static int ConfigCommandSend(SMART_PROTO_CONTEXT_T *ct, char *contentStr, char **response)
{
    CURL * curl;
    CURLcode res;
    int ret = -1;
	HttpProtoBuffer_T tBuffer;

    if (ct == NULL || contentStr == NULL)
    {
        LOGE("parameters error\n");
        return -1;
    }

    curl = curl_easy_init();

    if (curl == NULL)
    {
        LOGE("curl init failed\n");
        return -1;
    }
	memset(&tBuffer, 0, sizeof(HttpProtoBuffer_T));
    struct curl_slist *headers = NULL;
    char tmp[128] = { 0 };

    headers = curl_slist_append(headers, "Accept: */*");
    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=UTF-8");
    headers = curl_slist_append(headers, "User-Agent: Mozilla/4.0");
    headers = curl_slist_append(headers, "Cache-Control: no-cache");

    // snprintf(tmp,sizeof(tmp),"%s:%d",ct->serverAddr,ct->serverPort);
    headers = curl_slist_append(headers, tmp);

    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    memset(tmp,0,sizeof(tmp));
    snprintf(tmp,sizeof(tmp),"%s/%s",ct->cfg.serverAddr,
             SMART_PROTO_URL_CONFIG_COMMAND);

    curl_easy_setopt(curl, CURLOPT_URL, tmp);

    if (strstr(tmp, "https:") != NULL)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0);
    }

//	LOGD("tmp url [%s]\n", tmp);
    // curl_easy_setopt(curl, CURLOPT_PUT, 1L);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, contentStr);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, HttpProtoWriteCallBack);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void * )&tBuffer);
    curl_easy_setopt(curl, CURLOPT_COOKIESESSION, 1L);
    curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "/dev/null");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT,(15 >>16)&0xFFFF);
    curl_easy_setopt(curl, CURLOPT_DEBUGFUNCTION, ResponseLog);
    curl_easy_setopt(curl, CURLOPT_DEBUGDATA, NULL);
    res = curl_easy_perform(curl);
    if (res != CURLE_OK)
    {
        LOGE("curl_wasy_perform error = %s\n", curl_easy_strerror(res));
        ret = -1;

        if (res == CURLE_COULDNT_RESOLVE_HOST){
            LOGW("res_init\n");
            res_init();
        }
    }
    else
    {
        ret = 0;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (response != NULL )
    {
    	*response = tBuffer.pBuff;
        //LOGE("RSP @%s@\n",*response);
    }
	else if(tBuffer.pBuff != NULL)
	{
		MEDIA_FREE(tBuffer.pBuff);
	}
	if(tBuffer.szMatchResult != NULL)
	{
		MEDIA_FREE(tBuffer.szMatchResult);
	}

    return ret;
}
void CommandTransfer(char *serverCommand, char *slinkCommand, int sCommandLen)
{
	//NVR 和 IPC I8H 存在差异
	if(!serverCommand || !slinkCommand)
	{
		return ;
	}
	if(strcasecmp(serverCommand, "frmNetFtpPara") == 0)
	{
		snprintf(slinkCommand, sCommandLen, "frmFTPSetting");
	}
	else if(strcasecmp(serverCommand, "frmVideoFaceRecognitionPara") == 0)
	{
		snprintf(slinkCommand, sCommandLen, "frmVideoFaceDetectV2");
	}
	else if(strcasecmp(serverCommand, "frmWiegand") == 0)
	{
		snprintf(slinkCommand, sCommandLen, "frmWiegandCfg");
	}
	else if(strcasecmp(serverCommand, "frmNetHttpPara") == 0)
	{
		snprintf(slinkCommand, sCommandLen, "frmHttpPushCfg");
	}
	else
	{
		snprintf(slinkCommand, sCommandLen, serverCommand);
	}
}


static int ConfigCommandRspHandle(SMART_PROTO_CONTEXT_T *ct, char *response, char **moreRequest)
{
    if (response == NULL || ct == NULL || ct->state != SMART_PROTO_STATE_START)
    {
        LOGE("parameters error\n");
        return -1;
    }

    int retCode = -1, commandSeq = -1;
    char *command = NULL;
	char timeStr[32] = {};
	char configCommand[32] = {0};
	int configCommandSeq = 0;
	char transferCommand[32] = {0};
    cJSON_Struct * contentJson = Common_Json_Parse(response, NULL, NULL);
    if (contentJson == NULL)
    {
    	LOGE("invalid json !\n");
        return -1;
	}
	int debugInt = -1;
	if(Common_Json_GetAttrValueInt(contentJson, "Ch", &debugInt) && debugInt != -1)
	{
		LOGD("Ch = [%d]\n", debugInt);
	}
	debugInt = -1;
	if(Common_Json_GetAttrValueInt(contentJson, "Dev", &debugInt) && debugInt != -1)
	{
		LOGD("Dev = [%d]\n", debugInt);
	}
	debugInt = -1;
	if(Common_Json_GetAttrValueInt(contentJson, "Type", &debugInt) && debugInt != -1)
	{
		LOGD("Type = [%d]\n", debugInt);
	}


    Common_Json_GetAttrValue(contentJson, -1, "Command", NULL, &command, NULL, NULL);
    Common_Json_GetAttrValue(contentJson, -1, "CommandSeq", NULL, NULL, &commandSeq, NULL);

	LOGD("command [%s] seq[%d]\n", command, commandSeq);
	if(command != NULL && strlen(command)>0 && commandSeq != -1)
	{
		snprintf(configCommand, sizeof(configCommand), "%s", command);
		configCommandSeq = commandSeq;

		CommandTransfer(command, transferCommand, sizeof(transferCommand));
		cJSON_Struct *inJson = NULL, *outJson = NULL;
		inJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
		if(inJson)
		{
			Common_Json_SetAttrValueObj(inJson, "Header");
			Common_Json_SetAttrValueStr(inJson, "Header/Method", "put");
			Common_Json_SetAttrValueStr(inJson, "Header/Uri", "/Webserver/CustomAction");
			Common_Json_SetAttrValueObj(inJson, "Header/Auth");
			Common_Json_SetAttrValueInt(inJson, "Header/Auth/Method", 1);
			Common_Json_SetAttrValueStr(inJson, "Header/Auth/UserName", "(null)");
			Common_Json_SetAttrValueStr(inJson, "Header/Auth/Password", "ovfsZSJQZLHL");
			Common_Json_SetAttrValueObj(inJson, "Data");
			Common_Json_SetAttrValueObj(inJson, "Data/Header");
			Common_Json_SetAttrValueStr(inJson, "Data/Header/Uri", transferCommand);
			Common_Json_SetAttrValueObj(inJson, "Data/Header/Auth");
			Common_Json_SetAttrValueInt(inJson, "Data/Header/Auth/Method", 1);
			Common_Json_SetAttrValueStr(inJson, "Data/Header/Auth/UserName", "(null)");
			Common_Json_SetAttrValueStr(inJson, "Data/Header/Auth/Password", "ovfsZSJQZLHL");
			Common_Json_AddItem(inJson, -1, "Data/Data", contentJson);
//			Utils_GetLocalIP(ip);
//			Common_Json_SetAttrValueStr(inJson, "Data/Data/Data/Ip", ip);
			contentJson = NULL;

			retCode = RestMedia_Request(inJson, &outJson);
			Common_Json_Delete(inJson);
			inJson = NULL;
			if(retCode != 0)
			{
				LOGE("fail to request! retCode=[%d]\n", retCode);
			}
			if(outJson != NULL)
			{
				Common_Json_SetAttrName(outJson, "Header", "Result");
				Common_Json_SetAttrValueStr(outJson, "DevType", ct->devType);
				Common_Json_SetAttrValueStr(outJson, "DevName", NULL);
				Common_Json_SetAttrValueStr(outJson, "SerialNum",  ct->serialNum);
				GetCurTimeStr(timeStr);
				Common_Json_SetAttrValueStr(outJson,  "LocalTime", timeStr);

				Common_Json_SetAttrValueStr(outJson, "Command", configCommand);
				Common_Json_SetAttrValueInt(outJson, "CommandSeq", configCommandSeq);

				if(Common_StrCmp("frmNetHttpPara", configCommand) == 0)
				{
					Common_Json_SetAttrName(outJson, "Data/ServerAddr", "HTTPServer");
					Common_Json_RemoveItem(outJson, -1, "Data/ServerPort");
				}

			}
			else
			{
				outJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
				Common_Json_SetAttrValueStr(outJson, "DevType", ct->devType);
				Common_Json_SetAttrValueStr(outJson, "DevName", NULL);
				Common_Json_SetAttrValueStr(outJson, "SerialNum",  ct->serialNum);
				GetCurTimeStr(timeStr);
				Common_Json_SetAttrValueStr(outJson,  "LocalTime", timeStr);

				Common_Json_SetAttrValueStr(outJson, "Command", configCommand);
				Common_Json_SetAttrValueInt(outJson, "CommandSeq", configCommandSeq);
				Common_Json_SetAttrValueObj(outJson, "Result");
				Common_Json_SetAttrValueInt(outJson, "Result/Code", retCode);

			}
			char *outBuffer = Common_Json_Print(outJson, NULL);
			if(outBuffer != NULL)
			{
				//fprintf(stderr, "%s\n", outBuffer);

				ConfigCommandSend(ct, outBuffer, moreRequest);
				free(outBuffer);
				outBuffer = NULL;
			}
			Common_Json_Delete(outJson);
			outJson = NULL;
		}
		else
		{

		}
	}
	else
	{
		Common_Sleep(5, 0);
	}
	if(contentJson)
	{
    	Common_Json_Delete(contentJson);
		contentJson = NULL;
	}
    return retCode;
}

static void * SmartCheckConfigCommandThread(void *data)
{
	//LOGD("\n");
	SMART_PROTO_CONTEXT_T *ct = (SMART_PROTO_CONTEXT_T *)data;
	if(ct == NULL)
	{
		LOGE("data is null!\n");
		return NULL;
	}
	int ret = 0;
	char *contentStr = NULL, *responseStr = NULL;
	while(ct != NULL && ct->state == SMART_PROTO_STATE_START)
	{
		if(ct->startCommand != Common_Json_Type_True || closeConfig)
		{
			Common_Sleep(0, 500000);
			continue;
		}
		int startSendTime = 0;
		int currentTime = 0;
		do
		{
            if(ct->state != SMART_PROTO_STATE_START)
            {
                break;
            }
			Common_GetSystemCount(&currentTime, NULL);
			if(startSendTime == 0 || ct->startCommand == Common_Json_Type_True)
			{
				startSendTime = currentTime;
			}

			ret = 0;
			contentStr = NULL;
			responseStr = NULL;
			//LOGD("Check Config Command Here!\n");
			ret = ConfigCommandPack(ct, &contentStr);
			if(ret == 0 && contentStr != NULL)
			{
				ret = ConfigCommandSend(ct, contentStr, &responseStr);
				free(contentStr);
				contentStr = NULL;

				char *moreRequest = NULL;
				if(ret == 0)
				{
					while(responseStr != NULL)
					{
                        if(ct->state != SMART_PROTO_STATE_START)
                        {
                            break;
                        }
						moreRequest = NULL;
						ret = ConfigCommandRspHandle(ct, responseStr, &moreRequest);
						free(responseStr);
						responseStr = NULL;
						if(moreRequest != NULL)
						{
							LOGD("More Request!\n");
							responseStr = moreRequest;
						}
					}
				}
			}
			else
			{
				LOGE("fail to config pack!\n");
			}
			Common_Sleep(0, 200000);
		}while(currentTime < startSendTime + 60);
		//LOGD("BREAK!\n");

	}

	LOGW("thread exit\n");
	return NULL;
}

int SmartProto_Init(MQ_HANDLE_H mqHandle)
{
	LOGD("begin\n");
	HTTP_PUSH_CORE_VERSION_T coreVersion;

	if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(errno));
        return -EINVAL;
    }

	curl_global_init(CURL_GLOBAL_DEFAULT);

	memset(&s_smart_proto_ct,0,sizeof(SMART_PROTO_CONTEXT_T));
    s_smart_proto_ct.mqHandle = mqHandle;

	memset(&coreVersion, 0, sizeof(HTTP_PUSH_CORE_VERSION_T));
	RestMeida_RequestCoreVersion(&coreVersion);
	snprintf(s_smart_proto_ct.serialNum, sizeof(s_smart_proto_ct.serialNum), "%s", coreVersion.SerialNumber);
	snprintf(s_smart_proto_ct.devType, sizeof(s_smart_proto_ct.devType), "%s", (char *)"IPC");
	LOGD("end\n");
	return 0;
}

static void * CheckTimeThread(void *data)
{
    SMART_PROTO_CONTEXT_T *ct = (SMART_PROTO_CONTEXT_T *)data;
    closeAlarm = 0;
    filterAlarm = 0;
    closeConfig = 0;
    closeSmartResult = 0;

    while(ct != NULL && ct->state == SMART_PROTO_STATE_START)
    {
        int isHappened = 0;
        time_t timeA = time(NULL);
        Common_Time_T t_happent;

        Common_Linux2CommonTime(timeA,&t_happent);
        int t_start = t_happent.hour*100+t_happent.min;
    	int i = t_happent.wday;
        int j = 0;
    	for(j = 0;j < MAX_TIMESEGMENT;j++)
    	{
            if((0 == ct->notdisturbcfg.NotDisturbTime[i][j].startTime) && (0 == ct->notdisturbcfg.NotDisturbTime[i][j].stopTime))
            {
                continue;
            }
    		if((t_start >= ct->notdisturbcfg.NotDisturbTime[i][j].startTime) && (t_start <= ct->notdisturbcfg.NotDisturbTime[i][j].stopTime))
    		{
                isHappened = 1;
    			break;
    		}
    	}

        if(isHappened)
        {
            if(ct->notdisturbcfg.enableAlarm == 0)
            {
                closeAlarm = 1;
            }
            else
            {
                filterAlarm = 1;
            }

            if(ct->notdisturbcfg.enableConfig == 0)
            {
                closeConfig = 1;
            }

            if(ct->notdisturbcfg.enableSmartResult == 0)
            {
                closeSmartResult = 1;
            }
        }
        else
        {
            closeAlarm = 0;
            filterAlarm = 0;
            closeConfig = 0;
            closeSmartResult = 0;
        }

        Common_Sleep(1, 0);
    }

    return NULL;
}

int SmartProto_Start()
{
	LOGD("begin\n");
	int ret = 0;
    if (s_smart_proto_ct.state != SMART_PROTO_STATE_STOP)
    {
        LOGE("state error\n");
        return -1;
    }

	ret = -1;
	if (Mq_Request(s_smart_proto_ct.mqHandle, MEDIA_REQ_SMARTPROTOCOL_GET_CFG, NULL, 0, &ret, &s_smart_proto_ct.cfg, sizeof(s_smart_proto_ct.cfg)) < 0 || ret < 0)
    {
        LOGE("request cfg failed\n");
        return ret;
    }

    if (Mq_Request(s_smart_proto_ct.mqHandle, MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG, NULL, 0, &ret, &s_smart_proto_ct.notdisturbcfg, sizeof(s_smart_proto_ct.notdisturbcfg)) < 0 || ret < 0)
    {
        LOGE("request notdisturbcfg failed\n");
        return ret;
    }

    if (s_smart_proto_ct.cfg.enable == 0)
    {
        return 0;
    }

    if (strlen(s_smart_proto_ct.cfg.serverAddr) < 2)
    {
        LOGE("parameters error\n");
        return -1;
    }

    if (strcmp(s_smart_proto_ct.cfg.serverAddr, "http://0.0.0.0") == 0)
    {
    	LOGE("parameters error\n");
        return -1;
    }

    if (s_smart_proto_ct.cfg.eventListMaxLen <= 2)
        s_smart_proto_ct.cfg.eventListMaxLen = SMART_PROTO_POST_LIST_MAX_NODE;

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 16);
    s_smart_proto_ct.state = SMART_PROTO_STATE_START;
    pthread_create(&s_smart_proto_ct.checkTimePth, &attr, CheckTimeThread, &s_smart_proto_ct);
    pthread_create(&s_smart_proto_ct.heartBeatPth, &attr, HeartBeatThread, &s_smart_proto_ct);
	pthread_create(&s_smart_proto_ct.checkCommandPth, &attr, SmartCheckConfigCommandThread, &s_smart_proto_ct);
	LOGD("end\n");
    return 0;
}

int SmartProto_Stop()
{
	LOGD("begin\n");
    if (s_smart_proto_ct.state != SMART_PROTO_STATE_START)
    {
        return -1;
    }
    s_smart_proto_ct.state = SMART_PROTO_STATE_STOP;
    pthread_join(s_smart_proto_ct.checkTimePth, NULL);
    pthread_join(s_smart_proto_ct.heartBeatPth, NULL);
	pthread_join(s_smart_proto_ct.checkCommandPth, NULL);
	LOGD("end\n");
    return 0;
}

int SmartProto_Restart()
{
	SmartProto_Stop();
	SmartProto_Start();

	return 0;
}

int SmartProto_CheckServer(char *uri, char **resonpseStr)
{
    int ret = -1;
    char *sendStr = NULL;
    ret = HeartBeatPack(&s_smart_proto_ct, &sendStr);
    if (ret == 0)
    {
        ret = HeartBeatCheck(uri, sendStr, resonpseStr);
    }

    if (sendStr)
        MEDIA_FREE(sendStr);
    return ret;
}

#endif
