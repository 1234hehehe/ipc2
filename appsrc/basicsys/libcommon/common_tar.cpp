#include "libcommon_api.h"

typedef struct _tagCommon_TarObject 
{
	S64 nSize;
	S64 nTotalSize;
	S32 nError;
	FILE *fPipe;
	Common_Thread_T hThread;
	S32 bExit;
}Common_TarObject_T;


S64 Common_Tar_GetSize(S8 *szTarPathname)
{
	FILE *fPipe = NULL;
	S8 szBuff[256];
	S64 llSize = 0;
	if (szTarPathname == NULL)
	{
		return -1;
	}
	
	sprintf(szBuff,"tar tvvf %s | awk \'{print $3}\'",szTarPathname);
	fPipe = popen(szBuff, "r");
	if (fPipe == NULL)
	{
		LOGE("popen_ex function Failed\n");
		return -1;
	}

	memset(szBuff,0,sizeof(szBuff));
	while(fgets(szBuff, 255, fPipe))
	{
		llSize += atoi(szBuff);
	}

	llSize += atoi(szBuff);


	pclose(fPipe);
	return llSize;

}
// 解压
S32 Common_Tar_Decompress(S8 *szTarPathname,S8 *szToPath)
{
	S8 szBuff[256];
	FILE *fPipe = NULL;
	if (szTarPathname == NULL)
	{
		return -1;
	}
	if (szToPath != NULL)
	{
		sprintf(szBuff,"tar xvvf %s -C %s",szTarPathname,szToPath);
	}
	else
	{
		 sprintf(szBuff,"tar xvvf %s",szTarPathname);
	}

	fPipe = popen(szBuff, "r");
	if (fPipe == NULL)
	{
		LOGE("popen_ex function Failed\n");
		return -1;
	}


	memset(szBuff,0,sizeof(szBuff));

	while(fgets(szBuff, 255, fPipe))
	{
		LOGI(szBuff);
	}


	pclose(fPipe);
	return 0;

}
static S32 static_Tar_Thread(Common_Thread_T hThreadHandle,void *pUserData)
{
	S8 szBuff[256];
	Common_TarObject_T *pObject = (Common_TarObject_T *)pUserData;
	while(!pObject->bExit)
	{
		if (pObject->fPipe == NULL)
		{
			Common_Sleep(0,10000);
			continue;
		}
		if (NULL == fgets(szBuff, 255, pObject->fPipe))
		{
			break;
		}
		pObject->nSize += atoi(szBuff);
	}
	return 0;
}
// 异步解压
S32 Common_Tar_De_Create(Common_Tar_T *phTar,S8 *szTarPathname,S8 *szToPath)
{
	FILE *fPipe = NULL;
	S8 szBuff[256];
	S64 llTotalSize = 0;
	Common_TarObject_T *pObject = NULL;
	if (phTar == NULL || szTarPathname == NULL || *phTar != NULL)
	{
		return -1;
	}
	llTotalSize = Common_Tar_GetSize(szTarPathname);
	if (llTotalSize <= 0)
	{
		return -1;
	}
	pObject = (Common_TarObject_T *)Common_Malloc(sizeof(Common_TarObject_T),0,__FUNCTION__,__LINE__);
	if (pObject == NULL)
	{
		return -1;
	}
	memset(pObject,0,sizeof(Common_TarObject_T));

	if (szToPath != NULL)
	{
		sprintf(szBuff,"tar xvvf %s -C %s | awk \'{print $3}\'",szTarPathname,szToPath);
	}
	else
	{
		sprintf(szBuff,"tar xvvf %s | awk \'{print $3}\'",szTarPathname);
	}
   
	fPipe = popen(szBuff, "r");
	if (fPipe == NULL)
	{
		LOGE("popen_ex function Failed\n");
		Common_Free(pObject,__FUNCTION__,__LINE__);
		return -1;
	}
	pObject->fPipe = fPipe;
	pObject->nTotalSize = llTotalSize;
	
	Common_Thread_Create(&pObject->hThread,"Tar decompress",0,0,static_Tar_Thread,pObject);
	if (phTar)
	{
		*phTar = (Common_Tar_T)pObject;
	}
	return 0;
}
//获取当前解压大小
S64 Common_Tar_GetProgressSize(Common_Tar_T hTar,S64 *pSize,S64 *pTotalSize)
{
	Common_TarObject_T *pObject = (Common_TarObject_T *)hTar;
	if (pObject == NULL)
	{
		return -1;
	}
	if (pObject->nError)
	{
		return pObject->nError;
	}
	if (pSize)
	{
		*pSize = pObject->nSize;
	}
	if (pTotalSize)
	{
		*pTotalSize = pObject->nTotalSize;
	}
	return pObject->nSize;
	
}
// 销毁解压
S32 Common_Tar_De_Destroy(Common_Tar_T *phTar)
{
	Common_TarObject_T *pObject = (Common_TarObject_T *)(*phTar);
	if (pObject == NULL)
	{
		return -1;
	}
	pObject->bExit = 1;
	Common_Thread_Destroy(&pObject->hThread);
	pclose(pObject->fPipe);
	pObject->fPipe = NULL;
	Common_Free(pObject,__FUNCTION__,__LINE__);
	*phTar = NULL;
	return 0;
}

/*
#!/bin/bash

if [ $# -ne 1 ]; then 
echo "Usage: $0 file"
exit 1  
fi

TSIZE=0
for FSIZE in $(tar tvvf $1 | awk '{print $3}'); do
if [ "$FSIZE" = "${FSIZE//[^0-9]/}" ]; then 
TSIZE=$((TSIZE+FSIZE))
fi
done

[ $TSIZE -eq 0 ] && exit 1

MSG="Extracting..."
PROG_POS=$((${#MSG}+1))
PERC_POS=$((${#MSG}+53))

echo $MSG

PREV=-1
NSIZE=0
for FSIZE in $(tar xvvf $1 | awk '{print $3}'); do
if [ "$FSIZE" = "${FSIZE//[^0-9]/}" ]; then 
NSIZE=$((NSIZE+FSIZE))
PERCENT=$((NSIZE*100/TSIZE))
if [ $PERCENT -ne $PREV ]; then 
PLUS=$((PERCENT/2))
PROGRESS=$(printf "%.${PLUS}d" | tr '0' '+')
echo -e "\e[A\e[${PROG_POS}G${PROGRESS}=>"
echo -e "\e[A\e[${PERC_POS}G${PERCENT}%"
PREV=$PERCENT
fi      
fi
done
*/