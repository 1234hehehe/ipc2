/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 处理订阅相关
 *	源 文 件 : ovfs_network_Ftp.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2017/6/23
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>


#include "curl/curl.h"
#include "ovfs_network_ftp.h"


typedef struct
{
	U32 nPort;
	//U32 nType;     // 0 - ftpput  1 - ftpput
	U32 nLinkMode; // 0 - pasv    1 - active
	S8  nServer[64];
	S8  nUserName[64];
	S8  nPassWord[64];
    S8  nServerDir[256];
    U32 nStatus;    //0-idle, 1-uploading
} NETWORK_FTP_T;

typedef struct
{
    S8  nServerFileName[512];
    S8  nClientFileName[128];
} NETWORK_DO_FTP_T;

//static int m_timeCnt = 0;
static COMMON_DLIST_T     s_ftpTaskHdl = NULL;
static Common_Thread_T m_pDoFtpthread = NULL;
static NETWORK_FTP_T s_network_ftp_info;

static int NetWork_Ftp_Cfg_init(cJSON_Struct *parentItem, cJSON_Struct *pJsonDefault)
{
	S8 *pStringValue;
	S32 nIntValue;

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"FPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_ftp_info.nPort = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			s_network_ftp_info.nPort = nIntValue;
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"FLinkMode",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_ftp_info.nLinkMode = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FLinkMode",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    s_network_ftp_info.nLinkMode = nIntValue;
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FServer",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nServer,pStringValue,sizeof(s_network_ftp_info.nServer));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_ftp_info.nServer,pStringValue,sizeof(s_network_ftp_info.nServer));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FUserName",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nUserName,pStringValue,sizeof(s_network_ftp_info.nUserName));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FUserName",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_ftp_info.nUserName,pStringValue,sizeof(s_network_ftp_info.nUserName));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FPassword",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nPassWord,pStringValue,sizeof(s_network_ftp_info.nPassWord));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FPassword",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_ftp_info.nPassWord,pStringValue,sizeof(s_network_ftp_info.nPassWord));
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FServerDir",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nServerDir,pStringValue,sizeof(s_network_ftp_info.nServerDir));
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"FServerDir",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_ftp_info.nServerDir,pStringValue,sizeof(s_network_ftp_info.nServerDir));
		}
	}

    return 0;
}

static int RemoveNodeMatch(void * a, void *b)
{
    NETWORK_DO_FTP_T* p_ftpInfo = (NETWORK_DO_FTP_T*)(a);
    LOGI("rm  file: %s\n",p_ftpInfo->nServerFileName);

    if(a == b)
        return 0;
    else
        return -1;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return (curl_off_t)fread(ptr, size, nmemb, (FILE *)stream);
}

int FtpUpload(NETWORK_DO_FTP_T *doftpinfo)
{
    //if(NULL == filepath) return -1;
    char url[1024] = {0};
    CURL *curl;
	CURLcode res = CURLE_OK;
	FILE *hd_src;
	struct stat file_info;
	curl_off_t fsize;

	if (s_network_ftp_info.nPort == 0)
	{
	    snprintf(url, sizeof(url), "ftp://%s/%s", s_network_ftp_info.nServer, doftpinfo->nServerFileName);
	}
	else
	{
	    snprintf(url, sizeof(url), "ftp://%s:%d/%s", s_network_ftp_info.nServer, s_network_ftp_info.nPort, doftpinfo->nServerFileName);
	}
	LOGI("url = %s\n", url);

	/* get the file size of the local file */
	if(stat(doftpinfo->nClientFileName, &file_info)) {
	    LOGE("Couldn't open '%s': %s\n", doftpinfo->nClientFileName, strerror(errno));
        s_network_ftp_info.nStatus = 0;
	    return -1;
	}
	fsize = (curl_off_t)file_info.st_size;

	LOGD("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);

    /* get a FILE * of the same file */
    hd_src = fopen(doftpinfo->nClientFileName, "rb");

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_USERNAME,s_network_ftp_info.nUserName);
        curl_easy_setopt(curl, CURLOPT_PASSWORD, s_network_ftp_info.nPassWord);

        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);
        curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
        if (1 == s_network_ftp_info.nLinkMode)
        {
            LOGD("s_network_ftp_info.nLinkMode = %d\n", s_network_ftp_info.nLinkMode);
            curl_easy_setopt(curl, CURLOPT_FTP_SSL_CCC, CURLFTPSSL_CCC_ACTIVE);
        }

        res = curl_easy_perform(curl);
        if(res != CURLE_OK)
        {
            LOGE("curl_easy_perform() failed: %s\n",
                 curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }

    fclose(hd_src); /* close the local file */

    curl_global_cleanup();

    s_network_ftp_info.nStatus = 0;

    return res == CURLE_OK ? 0 : -1;
}

static S32 NetWork_Run_Ftp(Common_Thread_T hThreadHandle,void* para)
{
    int taskCnt = 0;
//    int ret = 0;
//    char path[128] = {0};
    NETWORK_DO_FTP_T *p_ftpInfo;
    LOGI("enter pid = %d!\n", getpid());

    while (1)
    {
    	taskCnt = Common_DList_GetCount(s_ftpTaskHdl);
		if(taskCnt > 0)
		{
		     void* taskNode = Common_DList_GetFirst(s_ftpTaskHdl);
		     p_ftpInfo = (NETWORK_DO_FTP_T*)taskNode;
			 FtpUpload(p_ftpInfo);
			 Common_DList_Delete(s_ftpTaskHdl, taskNode, RemoveNodeMatch); // remove first node
		}
		Common_Sleep(1, 0);
    }

    LOGI("exit pid = %d!\n", getpid());
    return 0;
}

S32 NetWork_Get_Ftp_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}

	Common_Json_SetAttrValue(parentItem,-1,"FPort",Common_Json_Type_Number,NULL,s_network_ftp_info.nPort,0);
	Common_Json_SetAttrValue(parentItem,-1,"FLinkMode",Common_Json_Type_Number,NULL,s_network_ftp_info.nLinkMode,0);
	Common_Json_SetAttrValue(parentItem,-1,"FServer",Common_Json_Type_String,s_network_ftp_info.nServer,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"FUserName",Common_Json_Type_String,s_network_ftp_info.nUserName,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"FPassword",Common_Json_Type_String,s_network_ftp_info.nPassWord,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"FServerDir",Common_Json_Type_String,s_network_ftp_info.nServerDir,0,0);

	return 0;
}

S32 NetWork_Put_Ftp_Json(cJSON_Struct *parentItem)
{
	S8 *pStringValue;
	S32 nIntValue;

	if (NULL == parentItem)
	{
		return -1;
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"FPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_ftp_info.nPort = nIntValue;
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"FLinkMode",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_ftp_info.nLinkMode = nIntValue;
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FServer",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nServer,pStringValue,sizeof(s_network_ftp_info.nServer));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FUserName",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nUserName,pStringValue,sizeof(s_network_ftp_info.nUserName));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FPassword",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nPassWord,pStringValue,sizeof(s_network_ftp_info.nPassWord));
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"FServerDir",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_ftp_info.nServerDir,pStringValue,sizeof(s_network_ftp_info.nServerDir));
	}

	return 0;
}


S32 NetWork_Put_DoFtp_Json(cJSON_Struct *parentItem)
{
//	S8 *pStringValue;
	S32 nType = -1;
	S8 *PServerFileName = NULL;
	S8 *PClientFileName = NULL;
	//S8 url[128];
	//CURL *curl;
	//CURLcode res = CURLE_OK;
	//FILE *hd_src;
	//struct stat file_info;
	//curl_off_t fsize;
	//S8 nUploadFileName[256];


    if(s_network_ftp_info.nStatus == 1)
    {
        return 1;
    }

	NETWORK_DO_FTP_T *p_ftpInfo = (NETWORK_DO_FTP_T *)Common_Malloc(sizeof(NETWORK_DO_FTP_T),0,__func__,__LINE__);

	if (NULL == parentItem)
	{
		return -1;
	}

	Common_Json_GetAttrValue(parentItem,-1,"FType",NULL,NULL,&nType,NULL);
	LOGW("nType=%d\n",nType);

	Common_Json_GetAttrValue(parentItem,-1,"ServerFileName",NULL,&PServerFileName,NULL,NULL);

	Common_Json_GetAttrValue(parentItem,-1,"ClientFileName",NULL,&PClientFileName,NULL,NULL);


	if (NULL == PServerFileName || NULL == PClientFileName)
	{
        return -1;
	}

	snprintf(p_ftpInfo->nServerFileName,
            sizeof(p_ftpInfo->nServerFileName),
            "%s/%s",
            PServerFileName[0] != '/'?s_network_ftp_info.nServerDir:"",
            PServerFileName);

	snprintf(p_ftpInfo->nClientFileName,sizeof(p_ftpInfo->nClientFileName),"%s",PClientFileName);
	LOGW("nClientFileName=%s\n",p_ftpInfo->nClientFileName);

    s_network_ftp_info.nStatus = 1;

	Common_DList_InsertTail(s_ftpTaskHdl, p_ftpInfo, sizeof(NETWORK_DO_FTP_T));
#if 0
	/* get the file size of the local file */
	if(stat(PClientFileName, &file_info)) {
	    LOGE("Couldn't open '%s': %s\n", PClientFileName, strerror(errno));
	    return 1;
	}
	fsize = (curl_off_t)file_info.st_size;

	LOGD("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);

	  /* get a FILE * of the same file */
	hd_src = fopen(PClientFileName, "rb");

	curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if (curl)
	{
	    curl_easy_setopt(curl, CURLOPT_USERNAME, s_network_ftp_info.nUserName);
	    curl_easy_setopt(curl, CURLOPT_PASSWORD, s_network_ftp_info.nPassWord);
		if (s_network_ftp_info.nPort == 0)
		{
		    snprintf(url, sizeof(url), "ftp://%s/%s", s_network_ftp_info.nServer, nUploadFileName);
		}
		else
		{
		    snprintf(url, sizeof(url), "ftp://%s:%d/%s", s_network_ftp_info.nServer, s_network_ftp_info.nPort, nUploadFileName);
		}
		LOGD("url = %s\n", url);
	    curl_easy_setopt(curl, CURLOPT_URL, url);
	    curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(curl, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);
        curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);
		if (1 == s_network_ftp_info.nLinkMode)
		{
		    LOGD("s_network_ftp_info.nLinkMode = %d\n", s_network_ftp_info.nLinkMode);
		    curl_easy_setopt(curl, CURLOPT_FTP_SSL_CCC, CURLFTPSSL_CCC_ACTIVE);
		}

		res = curl_easy_perform(curl);
	    if(res != CURLE_OK)
	    {
	        LOGE("curl_easy_perform() failed: %s\n",
	              curl_easy_strerror(res));
	    }
		curl_easy_cleanup(curl);
	}

	fclose(hd_src); /* close the local file */

	curl_global_cleanup();

	Common_Free(PServerFileName,__FUNCTION__,__LINE__);
	Common_Free(PClientFileName,__FUNCTION__,__LINE__);

	return res == CURLE_OK ? 0 : -1;
#endif
	return 0;
}

S32 NetWork_Get_Ftp_Status_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}

    //0-idle, 1-uploading
	Common_Json_SetAttrValue(parentItem,-1,"Status",Common_Json_Type_Number,NULL,s_network_ftp_info.nStatus,0);

	return 0;
}

static void FreeTaskNode(void * data)
{
    Common_Free(data,__FUNCTION__,__LINE__);
    return ;
}

S32 NetWork_Ftp_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;

    COMMON_CLR_ARG(s_network_ftp_info);

	pJsonTmp = Common_Json_GetItem(pJson, -1, "NetApp/Ftp");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get Ftp info fail\n",__FUNCTION__, __LINE__);
	}

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault, -1, "NetApp/Ftp");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get Ftp info fail\n",__FUNCTION__, __LINE__);
	}

	NetWork_Ftp_Cfg_init(pJsonTmp, pJsonTmpDefault);

	Common_DList_Init(&s_ftpTaskHdl,FreeTaskNode);
	Common_Thread_Create(&m_pDoFtpthread,"DoFtpThread",1024*128,COMMON_THREAD_CREATEFLAG_NORMAL,NetWork_Run_Ftp,(void *)NULL);

	return 0;
}

S32 NetWork_Ftp_Destroy()
{
	Common_DList_Uninit(&s_ftpTaskHdl);
	return 0;
}

