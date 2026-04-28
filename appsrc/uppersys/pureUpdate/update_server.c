#include <stdio.h>
#include <stdlib.h>
#include "update_server.h"
#include "update_serverMgr.h"

typedef struct tag_stUpdateServer_Mgr{
    UpdateSvrMgr *pstUpdateMgr;
    S32 lUpGradePos;
    S32 lUpdating;
    S32 lBoardTypeSet;
    U32 ulBoardType;
}stUpdateServer_Mgr, stUpdateServer_Mgr_PTR;

static stUpdateServer_Mgr g_stUpdateServer;

static stUpdateServer_Mgr * getUpdateServerHandle()
{
    return &g_stUpdateServer;
}

static S32 setBoardTypeFromFile(S8 *pBuffer, stUpdateServer_Mgr *pstUpdateServer)
{
    S32 bSet = 0;
    S32 nPos = 0;
    S8 sPath[260];

    FILE *pfcfg = NULL;
    S8 *pstr = strrchr(pBuffer,'/');

    if(pstr != NULL)
    {
        nPos = pstr - pBuffer + 1;
        memcpy(sPath, pBuffer, pstr - pBuffer + 1);
        sprintf(sPath + nPos, "boardtype.txt");
        pfcfg = fopen(sPath, "r");
        if(pfcfg != NULL)
        {
            int nLen;
            memset(sPath, 0, sizeof(sPath));
            nLen = fread(sPath, 1, 250, pfcfg);
            if(nLen > 0)
            {
                nLen = strlen(sPath);
                if(nLen > 2)
                {
                    S32 i;
                    S8 c;
                    S32 iStart = -1,iEnd = -1;
                    for(i = 0; i < nLen;i++)
                    {
                        c = sPath[i];
                        if(c == '#')
                        {
                            if(iStart == -1)
                            {
                                iStart = i + 1;
                            }
                            else
                            {
                                iEnd = i;
                                break;
                            }
                        }
                        else if((c >= '0' && c <= '9') ||
                                (c >= 'A' && c <= 'F') ||
                                (c >= 'a' && c <= 'f'))
                        {
                            if(c >= 'A' && c <= 'F')
                            {
                                sPath[i] = 'a' + c - 'A';
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
    
                    if(iStart != -1 && iEnd != -1)
                    {
                        DWORD dwBoadType = 0;
                        if(iEnd - iStart > 4)
                        {
                            iEnd = iStart + 4;
                        }

                        sPath[iStart + iEnd] = 0;
                        sscanf(sPath+iStart,"%x",&dwBoadType);
                        pstUpdateServer->pstUpdateMgr->SetBoardType(dwBoadType);
                        bSet = 1;
                    }
                }
            }

            fclose(pfcfg);
            pfcfg = NULL;
        }
    }

    return bSet;
}

void UpdateServer_Init()
{
    memset((void *)&g_stUpdateServer, 0x00, sizeof(g_stUpdateServer));

    g_stUpdateServer.pstUpdateMgr = updateServer_getUpdateItem();
}

void UpdateServer_SetBoardType(DWORD dwBoardType)
{
    stUpdateServer_Mgr *pstUpdateServer = getUpdateServerHandle();

    if (NULL == pstUpdateServer->pstUpdateMgr)
	{
		pstUpdateServer->pstUpdateMgr = updateServer_getUpdateItem();
	}

    if(NULL == pstUpdateServer->pstUpdateMgr)
	{
		return;
	}

    pstUpdateServer->ulBoardType = dwBoardType;
	LOGD("[Update]SetBoardType:%x\n", dwBoardType);

	return;
}

LONG UpdateServer_Upgrade(LONG lUserID, S8 *pBuffer, BOOL bFile)
{
	LONG ret     = -1;
	LONG bUpdate =  0;

	if(NULL == pBuffer)
	{
		return -1;
	}

    stUpdateServer_Mgr *pstUpdateServer = getUpdateServerHandle();

    if (NULL == pstUpdateServer->pstUpdateMgr)
	{
		pstUpdateServer->pstUpdateMgr = updateServer_getUpdateItem();
	}

    if(NULL == pstUpdateServer->pstUpdateMgr)
	{
		return -1;
	}

    bUpdate = pstUpdateServer->lUpdating++;
	if(bUpdate)
	{
		pstUpdateServer->lUpdating--;
		if(pstUpdateServer->lUpdating < 0)
		{
			pstUpdateServer->lUpdating = 0;
		}

        LOGE("error:update =%d\n", pstUpdateServer->lUpdating);
		return -1;
	}

	pstUpdateServer->lUpGradePos = 0;

	if(!bFile)
	{
		pstUpdateServer->pstUpdateMgr->SetBoardType(pstUpdateServer->ulBoardType);
		ret = pstUpdateServer->pstUpdateMgr->CreateTask(pBuffer);
	}
	else
	{
	    //判断是否需要从boardtype.txt中获取板型.
        S32 bSet = setBoardTypeFromFile(pBuffer, pstUpdateServer);
        if(!(bSet))
		{
		    //未配置boardtype.txt,则配置从序列号中解析出来的板型.
			pstUpdateServer->pstUpdateMgr->SetBoardType(pstUpdateServer->ulBoardType);
		}

		ret = pstUpdateServer->pstUpdateMgr->CreateTaskFile(pBuffer);
	}

	if (ret < 0)
	{
    	pstUpdateServer->lUpdating--;
		if(	pstUpdateServer->lUpdating < 0)
		{
            pstUpdateServer->lUpdating = 0;
		}
	}

	return ret;
}

LONG UpdateServer_GetUpgradeState(LONG lUpgradeHandle)
{
	BOOL bRet;
	LONG lStatus, lRatio;
	LONG bUpdate = 0;

    stUpdateServer_Mgr *pstUpdateServer = getUpdateServerHandle();

    if (NULL == pstUpdateServer->pstUpdateMgr)
	{
		pstUpdateServer->pstUpdateMgr = updateServer_getUpdateItem();
	}

    if(NULL == pstUpdateServer->pstUpdateMgr)
	{
		return -1;
	}

	bUpdate = pstUpdateServer->lUpdating;
	if(!bUpdate)
	{
		return -1;
	}

	bRet = pstUpdateServer->pstUpdateMgr->GetTaskStatus(lUpgradeHandle, &lStatus, &lRatio);
	if(!bRet)
	{
		LOGE("[UPDATE]Get status err.\n");
		return -1;
	}

    // lStatus : -1升级失败,0-升级完成,1-开始升级,2-升级中
	if(lStatus == -1)
	{
		return UPGRADE_STATE_FAIL; // 升级失败
	}
	else if(lStatus == UPDATE_STATUS_E_IDLE)
	{
		return UPGRADE_STATE_IDLE; // 升级完成
	}
	else if(lStatus == UPDATE_STATUS_E_FINISH)
	{
		return UPGRADE_STATE_FINISH; // 升级完成
	}
	else
	{
		return UPGRADE_STATE_ING; // 升级中
	}

	return -1;
}

LONG UpdateServer_GetUpgradeProgress(LONG lUpgradeHandle)
{
	BOOL bRet;
	LONG lStatus, lRatio, lReturn;
	LONG bUpdate = 0;

    stUpdateServer_Mgr *pstUpdateServer = getUpdateServerHandle();

    if (NULL == pstUpdateServer->pstUpdateMgr)
	{
		pstUpdateServer->pstUpdateMgr = updateServer_getUpdateItem();
	}

    if(NULL == pstUpdateServer->pstUpdateMgr)
	{
		return -1;
	}

	bUpdate = pstUpdateServer->lUpdating;
	if(!bUpdate)
	{
		return -1;
	}

	bRet = pstUpdateServer->pstUpdateMgr->GetTaskStatus(lUpgradeHandle, &lStatus, &lRatio);
	if(!bRet)
	{
		LOGE("[UPDATE]Get status err");
		return -1;
	}

	lReturn = lRatio;
	if(pstUpdateServer->lUpGradePos < lReturn)
	{
		pstUpdateServer->lUpGradePos = lReturn;
	}

	return pstUpdateServer->lUpGradePos;
}

BOOL UpdateServer_CloseUpgradeHandle(LONG lUpgradeHandle)
{
	BOOL bRet;
	LONG bUpdate = 0;
	
    stUpdateServer_Mgr *pstUpdateServer = getUpdateServerHandle();

    if (NULL == pstUpdateServer->pstUpdateMgr)
	{
		pstUpdateServer->pstUpdateMgr = updateServer_getUpdateItem();
	}

    if(NULL == pstUpdateServer->pstUpdateMgr)
	{
		return FALSE;
	}

	bUpdate = pstUpdateServer->lUpdating;
	if(!bUpdate)
	{
		return FALSE;
	}
		
	bRet = pstUpdateServer->pstUpdateMgr->DestroyTask(lUpgradeHandle);
	
	pstUpdateServer->lUpdating--;
	if(pstUpdateServer->lUpdating < 0)
	{
	    pstUpdateServer->lUpdating = 0;
	}	
	
	return bRet;
}


