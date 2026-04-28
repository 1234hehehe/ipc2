#ifdef WIN32
#include <Windows.h>
#endif
#include "libupdate_api.h"
static S32 static_Update_Broadcast_Callback(S8 *szFromIP,S32 nFromPort,cJSON_Struct *pResult,void *pUserData)
{
	printf("\n........\n");
	printf("[Discovery][%s:%d]:\n",szFromIP,nFromPort);
	Common_Json_StandardPrint(pResult,"<",">\n",NULL);
	printf("\n............\n"); 
	return 0;
}

int GetMd5(char * strFileName,unsigned char *pResult)
{
	const char *pName;
	FILE *pf;
	int len,FileLen = 0;
	Common_Md5_T md5 = NULL;
	
	char buffer[1024];
	pName = strFileName;

	pf = fopen(pName,"rb");
	if (pf == NULL)
	{
		return -1;
	}
	Common_Md5_Create(&md5);
	do 
	{
		len = fread(buffer,1,1024,pf);
		if (len <= 0)
		{
			break;
		}
		FileLen += len;
		Common_Md5_Append(md5,(U8 *)buffer,len);
	} while (1);
	Common_Md5_Finish(md5,pResult,NULL);
	Common_Md5_Destroy(&md5);

	fclose(pf);
	return FileLen;

}
S32 main(S32 argc,char *argv[])
{
	// ËÑË÷
	cJSON_Struct *pInParam = NULL,*pOutParam = NULL;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	wVersionRequested = MAKEWORD( 2, 2 );

	S32 err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return 1;
	}


#endif
	unsigned int m_uMD5_Kernel[4];
	GetMd5("j://custom.bin",(U8 *)m_uMD5_Kernel);
	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Get",0,0);
	Update_Tcp_Require("10.3.31.102",10009,pInParam,&pOutParam,3000);
	//Update_Broadcast_Start(10009,pInParam,NULL,0,2,3,static_Update_Broadcast_Callback,NULL);
#if 0
	printf("\n===================\n");
	Update_Tcp_Require("192.168.0.39",10008,pInParam,&pOutParam,3000);
	Common_Json_StandardPrint(pOutParam,"\ntcp Out <",">\n",NULL);
	Common_Json_Delete(pInParam);
	Common_Json_Delete(pOutParam);
	pOutParam = NULL;
	printf("\n===================\n");
	pInParam = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Update",0,0);
	Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"Get",0,0);
	Update_Udp_Require("192.168.0.35",10008,pInParam,&pOutParam,3000);
	Common_Json_StandardPrint(pOutParam,"\nudp Out <",">\n",NULL);
	Common_Json_Delete(pOutParam);
	pOutParam = NULL;

	printf("\n===================\n");
	Update_Tcp_Require("192.168.0.39",10008,pInParam,&pOutParam,3000);
	Common_Json_StandardPrint(pOutParam,"\ntcp Out <",">\n",NULL);
	Common_Json_Delete(pOutParam);
	pOutParam = NULL;

	Common_Json_Delete(pInParam);
	printf("\n===================\n");
#endif
	Common_Sleep(10,0);
	//Update_Broadcast_Stop(10008);
	Common_Sleep(100,0);
	return 0;
}