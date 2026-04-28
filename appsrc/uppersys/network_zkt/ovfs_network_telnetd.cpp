/******************************************************************************
 *	Copyright(c) OVFS, 2015. All rights reserved.
 *  部    门 : NVR2.0
 *	描    述 : 处理订阅相关
 *	源 文 件 : ovfs_network_telnetd.cpp
 *	作    者 : 舒适
 *  环    境 ：ubuntu12.04/gcc 4.xx/uedit18.xx/sourceinsight v.xx....
 *	版    本 : 1.0
 *  时    间 : 2017/6/23
 *****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "ovfs_network_telnetd.h"


typedef struct
{
    S32 bEnable;
	S32 nPort;
	S8  nPassWord[64];
} NETWORK_TELNETD_T;

static NETWORK_TELNETD_T s_network_telnetd_info;

#if 0
static int NetWork_Run_Telnetd(cJSON_Struct *parentItem, cJSON_Struct *pJsonDefault)
{
	S8 *pStringValue;
	S32 nIntValue;
	S8 cmd[128];	


	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"TEnable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
      /*force telent disable when start up */
	    s_network_telnetd_info.bEnable = 0;//nIntValue;	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"TEnable",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
		    s_network_telnetd_info.bEnable = nIntValue;	
		}
	}
	
	nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"TPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		s_network_telnetd_info.nPort = nIntValue;
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"TPort",NULL,NULL,&nIntValue,NULL);
		if (-1 != nIntValue)
		{
			s_network_telnetd_info.nPort = nIntValue;
		}
	}
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"TPassword",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
    {
	    Common_Strncpy(s_network_telnetd_info.nPassWord,pStringValue,sizeof(s_network_telnetd_info.nPassWord));	
	}
	else
	{
		Common_Json_GetAttrValue(pJsonDefault,-1,"TPassword",NULL,&pStringValue,NULL,NULL);
		if (NULL != pStringValue)
	    {
		    Common_Strncpy(s_network_telnetd_info.nPassWord,pStringValue,sizeof(s_network_telnetd_info.nPassWord));	
		}
	}


	if (0 == s_network_telnetd_info.nPort)
	{
        s_network_telnetd_info.nPort = 23;
	}
		
	if (0 == s_network_telnetd_info.bEnable)
	{
		snprintf(cmd, sizeof(cmd), "killall -9 telnetd");
		
		// LOGW("cmd = %s \n", cmd);
		Common_System(cmd);		
	}
    else
    {
		if (23 == s_network_telnetd_info.nPort)
		{	
		    //snprintf(cmd, sizeof(cmd), "killall -9 telnetd;telnetd");
			snprintf(cmd, sizeof(cmd), "telnetd");
		}
		else
		{
            //snprintf(cmd, sizeof(cmd), "killall -9 telnetd;telnetd -p %d", s_network_telnetd_info.nPort);
			snprintf(cmd, sizeof(cmd), "telnetd -p %d", s_network_telnetd_info.nPort);			
		}	
		
		// LOGW("cmd = %s \n", cmd);
		Common_System(cmd);			
	}

	if (0 == strlen(s_network_telnetd_info.nPassWord))
	{	
		snprintf(cmd, sizeof(cmd), "passwd -d root");
	}
	else
	{
		snprintf(cmd, sizeof(cmd), "echo -e \'%s\\n%s\\n\' | passwd root > /dev/null", s_network_telnetd_info.nPassWord, s_network_telnetd_info.nPassWord);
	}	
	
	// LOGW("cmd = %s \n", cmd);
  /*eric comment this line to disable changed passwd*/
	// Common_System(cmd);		
	
    return 0;
}
#endif

S32 NetWork_Get_Telnetd_Json(cJSON_Struct *parentItem)
{
    if (NULL == parentItem)
    {
        return -1;
	}
	
	Common_Json_SetAttrValue(parentItem,-1,"TEnable",Common_Json_Type_Number,NULL,s_network_telnetd_info.bEnable,0);
	Common_Json_SetAttrValue(parentItem,-1,"TPort",Common_Json_Type_Number,NULL,s_network_telnetd_info.nPort,0);
	Common_Json_SetAttrValue(parentItem,-1,"TPassword",Common_Json_Type_String,s_network_telnetd_info.nPassWord,0,0);
	
	return 0;
}

S32 NetWork_Put_Telnetd_Json(cJSON_Struct *parentItem)
{
	S8 *pStringValue;
	S32 nIntValue;
	S8 cmd[256];
	S32 bFlag = 0;
	
	if (NULL == parentItem)
	{
		return -1;
	}	
	
    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"TEnable",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
		if (s_network_telnetd_info.bEnable != nIntValue)
		{	
		    s_network_telnetd_info.bEnable = nIntValue;
			
			if (0 == nIntValue)
			{
				snprintf(cmd, sizeof(cmd), "killall -9 telnetd");
				
				// LOGW("cmd = %s \n", cmd);
				Common_System(cmd);
			}
            else
            {
			    bFlag = 1;	
			}				
		}
	}
	
    nIntValue = -1;
	Common_Json_GetAttrValue(parentItem,-1,"TPort",NULL,NULL,&nIntValue,NULL);
	if (-1 != nIntValue)
	{
	    if (0 == nIntValue)
	    {
            nIntValue = 23;
		}
		
		if (s_network_telnetd_info.nPort != nIntValue)
		{	
		    s_network_telnetd_info.nPort = nIntValue;
			bFlag = 1;
		}
	}	
	
	pStringValue = NULL;
	Common_Json_GetAttrValue(parentItem,-1,"TPassword",NULL,&pStringValue,NULL,NULL);
	if (NULL != pStringValue)
	{
		if (0 != Common_StrnCmp(s_network_telnetd_info.nPassWord, pStringValue, sizeof(s_network_telnetd_info.nPassWord)))
		{	
		    Common_Strncpy(s_network_telnetd_info.nPassWord,pStringValue,sizeof(s_network_telnetd_info.nPassWord));
			
			if (0 == strlen(s_network_telnetd_info.nPassWord))
			{	
				snprintf(cmd, sizeof(cmd), "passwd -d root");
			}
			else
			{
				snprintf(cmd, sizeof(cmd), "echo -e \'%s\\n%s\\n\' | passwd root > /dev/null", s_network_telnetd_info.nPassWord, s_network_telnetd_info.nPassWord);
			}	
			
			// LOGW("cmd = %s \n", cmd);
      /*eric comment this line to disable changed passwd*/
            // Common_System(cmd);				
		}
	}

    if (1 == bFlag && 1 == s_network_telnetd_info.bEnable)
    {
		if (23 == s_network_telnetd_info.nPort)
		{	
		    snprintf(cmd, sizeof(cmd), "killall -9 telnetd;telnetd");
		}
		else
		{
            snprintf(cmd, sizeof(cmd), "killall -9 telnetd;telnetd -p %d", s_network_telnetd_info.nPort);			
		}	
		
		// LOGW("cmd = %s \n", cmd);
		Common_System(cmd);	
	}		
	
	return 0;
}

S32 NetWork_Telnetd_Init(cJSON_Struct *pJson, cJSON_Struct *pJsonDefault)
{	
	cJSON_Struct *pJsonTmp = NULL;
	cJSON_Struct *pJsonTmpDefault = NULL;
	
    COMMON_CLR_ARG(s_network_telnetd_info);
	s_network_telnetd_info.bEnable = 0;
	s_network_telnetd_info.nPort = 23;
	snprintf(s_network_telnetd_info.nPassWord, sizeof(s_network_telnetd_info.nPassWord), "antslq");
	
	pJsonTmp = Common_Json_GetItem(pJson, -1, "NetApp/Telnetd");
	if (NULL == pJsonTmp)
	{
	   LOGE("[%s:%d]:get telnetd info fail\n",__FUNCTION__, __LINE__);
	}	

	pJsonTmpDefault = Common_Json_GetItem(pJsonDefault, -1, "NetApp/Telnetd");
	if (NULL == pJsonTmpDefault)
	{
	   LOGE("[%s:%d]:Default get telnetd info fail\n",__FUNCTION__, __LINE__);
	}	
	
  //NetWork_Run_Telnetd(pJsonTmp, pJsonTmpDefault);
	
	return 0;
}

S32 NetWork_Telnetd_Destroy()
{		
	return 0;
}

