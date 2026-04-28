/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 处理订阅相关
 *	源 文 件 : ovfs_network_HTTP.cpp
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
#include "ovfs_network_http.h"


typedef struct
{
	U32 nPort;
	U32 nType;     // 0 - httpget  1 - httpput  2-httppost
	U32 nAuthMode; // 0 - no auth  1 - username password
	U32 nSSL;      // 0 - no ssl   1 - ssl
	S8  nServer[64];
	S8  nUserName[64];
	S8  nPassWord[64];
    S8  nParam[256];	
} NETWORK_HTTP_T;

static NETWORK_HTTP_T s_network_http_info;


static int NetWork_Run_HTTP(cJSON_Struct *parentItem, cJSON_Struct *pJsonDefault)
{
	S8 *pStringValue;
	S32 nIntValue;	
	
	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_http_info.nPort = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			s_network_http_info.nPort = nIntValue;
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HType",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_http_info.nType = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HType",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			s_network_http_info.nType = nIntValue;
		}
	}	

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HAuthMode",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_http_info.nAuthMode= nIntValue;	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HAuthMode",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    s_network_http_info.nAuthMode = nIntValue;	
		}
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HSSL",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_http_info.nSSL= nIntValue;	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HSSL",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    s_network_http_info.nSSL = nIntValue;	
		}
	}	
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"HServer",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_http_info.nServer,pStringValue,sizeof(s_network_http_info.nServer));	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HServer",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nServer,pStringValue,sizeof(s_network_http_info.nServer));	
		}
	}	

	//if (1 == s_network_http_info.nAuthMode)
	{
		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"HUserName",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nUserName,pStringValue,sizeof(s_network_http_info.nUserName));	
		}
		else
		{
			Common_Json_GetAttrValue(pJsonDefault,-1,"HUserName",NULL,&pStringValue,NULL,NULL);
			if (NULL != pStringValue)
		    {
			    Common_Strncpy(s_network_http_info.nUserName,pStringValue,sizeof(s_network_http_info.nUserName));	
			}
		}	
		
		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"HPassword",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nPassWord,pStringValue,sizeof(s_network_http_info.nPassWord));	
		}
		else
		{
			Common_Json_GetAttrValue(pJsonDefault,-1,"HPassword",NULL,&pStringValue,NULL,NULL);
			if (NULL != pStringValue)
		    {
			    Common_Strncpy(s_network_http_info.nPassWord,pStringValue,sizeof(s_network_http_info.nPassWord));	
			}
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"HParam",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_http_info.nParam,pStringValue,sizeof(s_network_http_info.nParam));	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"HParam",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nParam,pStringValue,sizeof(s_network_http_info.nParam));	
		}
	}
	
    return 0;
}


S32 NetWork_Get_HTTP_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}
	
	Common_Json_SetAttrValue(parentItem,-1,"HPort",Common_Json_Type_Number,NULL,s_network_http_info.nPort,0);
	Common_Json_SetAttrValue(parentItem,-1,"HType",Common_Json_Type_Number,NULL,s_network_http_info.nType,0);
	Common_Json_SetAttrValue(parentItem,-1,"HAuthMode",Common_Json_Type_Number,NULL,s_network_http_info.nAuthMode,0);	
	Common_Json_SetAttrValue(parentItem,-1,"HSSL",Common_Json_Type_Number,NULL,s_network_http_info.nSSL,0);	
	Common_Json_SetAttrValue(parentItem,-1,"HServer",Common_Json_Type_String,s_network_http_info.nServer,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"HUserName",Common_Json_Type_String,s_network_http_info.nUserName,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"HPassword",Common_Json_Type_String,s_network_http_info.nPassWord,0,0);
	Common_Json_SetAttrValue(parentItem,-1,"HParam",Common_Json_Type_String,s_network_http_info.nParam,0,0);
	
	return 0;
}

S32 NetWork_Put_HTTP_Json(cJSON_Struct *parentItem)
{
	S8 *pStringValue;
	S32 nIntValue;
	
	if (NULL == parentItem)
	{
		return -1;
	}	
	
	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_http_info.nPort = nIntValue;
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HType",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_http_info.nType = nIntValue;
	}	

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HAuthMode",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_http_info.nAuthMode= nIntValue;	
	}

	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"HSSL",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    s_network_http_info.nSSL= nIntValue;	
	}	
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"HServer",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_http_info.nServer,pStringValue,sizeof(s_network_http_info.nServer));	
	}

	//if (1 == s_network_http_info.nAuthMode)
	{
		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"HUserName",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nUserName,pStringValue,sizeof(s_network_http_info.nUserName));	
		}	
		
		pStringValue = NULL;
		Common_Json_GetAttrValue(parentItem,-1,"HPassword",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_http_info.nPassWord,pStringValue,sizeof(s_network_http_info.nPassWord));	
		}
	}

	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"HParam",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_http_info.nParam,pStringValue,sizeof(s_network_http_info.nParam));	
	}
	
	return 0;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return (curl_off_t)fread(ptr, size, nmemb, (FILE *)stream);
}


S32 NetWork_Put_DoHTTP_Json(cJSON_Struct *parentItem)
{
	S8 *pStringValue;
	//S32 nIntValue;
	//S32 nType = 0;
	S8 *PFileName = NULL;
	S8 url[512];
	CURL *curl;
	CURLcode res = CURLE_OK;
	FILE *hd_src = NULL;
	struct stat file_info;
	curl_off_t fsize;
	long file_size = 0;
	S8 *postContent = NULL;
	S32 readCount = 0; 

	if (NULL == parentItem)
	{
		return -1;
	}
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"HFileName",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    PFileName = Common_StrDup(pStringValue,__FUNCTION__,__LINE__);;	
	}
	
	/* get the file size of the local file */ 
	if(stat(PFileName, &file_info)) {
	    LOGE("Couldn't open '%s': %s\n", PFileName, strerror(errno));
	}
	fsize = (curl_off_t)file_info.st_size;
	file_size = file_info.st_size;
	 
	LOGD("Local file size: %" CURL_FORMAT_CURL_OFF_T " bytes.\n", fsize);
	 
	  /* get a FILE * of the same file */ 
	hd_src = fopen(PFileName, "rb");
	  
	curl_global_init(CURL_GLOBAL_ALL); 
    curl = curl_easy_init();
    if (curl) 
	{
	    if (1 == s_network_http_info.nAuthMode)
	    {
		    curl_easy_setopt(curl, CURLOPT_USERNAME, s_network_http_info.nUserName);
		    curl_easy_setopt(curl, CURLOPT_PASSWORD, s_network_http_info.nPassWord);
	    }

		if (0 == s_network_http_info.nSSL)
		{
			if (s_network_http_info.nPort == 0)
			{
			    snprintf(url, sizeof(url), "http://%s/%s", s_network_http_info.nServer, s_network_http_info.nParam);
			}
			else
			{
			    snprintf(url, sizeof(url), "http://%s:%d/%s", s_network_http_info.nServer, s_network_http_info.nPort, s_network_http_info.nParam);
			}
		}
        else if (1 == s_network_http_info.nSSL)
        {
			if (s_network_http_info.nPort == 0)
			{
			    snprintf(url, sizeof(url), "https://%s/%s", s_network_http_info.nServer, s_network_http_info.nParam);
			}
			else
			{
			    snprintf(url, sizeof(url), "https://%s:%d/%s", s_network_http_info.nServer, s_network_http_info.nPort, s_network_http_info.nParam);
			}        
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
			curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }
		
		LOGD("url = %s\n", url);
	    curl_easy_setopt(curl, CURLOPT_URL, url);	
		curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
		if (1 == s_network_http_info.nType)
		{
			curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
            curl_easy_setopt(curl, CURLOPT_READDATA, hd_src);
		    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L); 
		    curl_easy_setopt(curl, CURLOPT_PUT, 1L);
            curl_easy_setopt(curl, CURLOPT_INFILESIZE_LARGE, (curl_off_t)fsize);			
		}
		else if (2 == s_network_http_info.nType)
		{
			curl_easy_setopt(curl, CURLOPT_POST, 1L);
		    postContent = (char*)Common_Malloc(file_size,0,__FUNCTION__,__LINE__);;
		    if (postContent != NULL) {
				readCount = fread(postContent, 1, file_size, hd_src);
				if (readCount == file_size) {
					curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, file_size);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postContent);
				}
		    }
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

	Common_Free(PFileName,__FUNCTION__,__LINE__);
	Common_Free(postContent,__FUNCTION__,__LINE__);
	
	return res == CURLE_OK ? 0 : -1;
}


S32 NetWork_HTTP_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{	
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;
	
    COMMON_CLR_ARG(s_network_http_info);
	
	pJsonTmp = Common_Json_GetItem(pJson, -1, "NetApp/HTTP");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get HTTP info fail\n",__FUNCTION__, __LINE__);
	}	

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault, -1, "NetApp/HTTP");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get HTTP info fail\n",__FUNCTION__, __LINE__);
	}	
	
	NetWork_Run_HTTP(pJsonTmp, pJsonTmpDefault);
	
	return 0;
}

S32 NetWork_HTTP_Destroy()
{		
	return 0;
}

