#include <stdlib.h>

#include "libcommon_api.h"
#include "libupdate_api.h"
#include "update_broadcast.h"
#include "update_version.h"


static U32 g_nAuthHelloCount  = 0;
static S8  g_acHelloUUID[64 + 1]  = {0};
static Common_Lock_T  g_phAuthHelloLock = NULL;

static S32 updateBroadcastCallback(UPDATE_BROADCAST_HANDLE handle,
                                                 S8 *szFromIP,
                                                 S32 nFromPort,
                                                 cJSON_Struct *pResult,
                                                 void *pUserData)
{
    Common_Sem_T hSem = (Common_Sem_T)pUserData;

    handle   = handle;
    pResult  = pResult;
    szFromIP = szFromIP;

    Common_Sem_Post(hSem);
    return 0;
}

S32 update_broadCast_Init()
{
    static S32 initFlag = 0;
    S32 ret = -1;

    if (1 == initFlag)
    {
        return 0;
    }

    if (NULL != g_phAuthHelloLock)
    {
        (void)Common_Lock_Destroy(&g_phAuthHelloLock);
    }

    ret = Common_Lock_Create(&g_phAuthHelloLock, "Auth_Hello_Lock");
    if (0 != ret)
    {
        LOGE("Create auth hello lock failed.\n");
        return -1;
    }

    g_nAuthHelloCount = 0;
    memset((void *)g_acHelloUUID, 0x00, sizeof(g_acHelloUUID));
    initFlag = 1;
    return 0;
}

S32 update_broadCast_GetHelloUUID(S8 *pcUUID, S32 lUUIDLen)
{
    if (NULL == g_phAuthHelloLock)
    {
        return -1;
    }

    if (lUUIDLen < sizeof(g_acHelloUUID))
    {
        LOGE("Enter UUID len(%d) < %d.\n", lUUIDLen, sizeof(g_acHelloUUID));
        return -1;
    }

    (void)Common_Lock(g_phAuthHelloLock);
    snprintf(pcUUID, lUUIDLen - 1, "%s", g_acHelloUUID);
    (void)Common_UnLock(g_phAuthHelloLock);

    return 0;
}

void update_broadCast_AuthSayHello(DeviceVersion_S *pstVersion)
{
    S8 *szSerialNumber = NULL;
    cJSON_Struct *pInParam = NULL;
    Common_Sem_T hSem = NULL;
    static int nLastTime = 0;
    int nPos = 0,Sec,MSec = 0,LSec = 0;
    UPDATE_BROADCAST_HANDLE hBroadcast = NULL;

    if (NULL == g_phAuthHelloLock)
    {
        Common_Lock_Create(&g_phAuthHelloLock, NULL);
    }

    Common_GetSystemCount(&Sec, &MSec);
    if (Sec < nLastTime + 60)
    {
        return;
    }

    szSerialNumber = update_version_GetSNByDevVer(pstVersion);
    if (szSerialNumber != NULL)
    {
        nLastTime = Sec;
        pInParam  = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

        Common_Lock(g_phAuthHelloLock);
        if (0 == g_acHelloUUID[0])
        {
            // 如果不存在，则生成一个，一直保留，除非有跟其他设备有冲突，则再生成。
            srand(MSec);
            g_nAuthHelloCount++;
            Common_GetCurrentTime(&LSec, NULL);

            //nPos += sprintf(g_acHelloUUID + nPos,"%s-%x-%x%x-%x-%x",szSerialNumber,LSec,Sec,MSec,g_nAuthHelloCount,rand());
            nPos += snprintf(g_acHelloUUID + nPos, sizeof(g_acHelloUUID) - nPos - 1, 
                             "%s-%x-%x%x-%x-%x", szSerialNumber, LSec, Sec, MSec, 
                             g_nAuthHelloCount, rand());
            //UPDATE_INFO("Auth crypto Hello uuid <%s>!\n",g_szHelloUUID);
        }

        Common_Json_SetAttrValue(pInParam,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Uri",Common_Json_Type_String,"/Update/Crypto/Hello",0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Data/HelloUUID",Common_Json_Type_String,g_acHelloUUID,0,0);
        Common_Json_SetAttrValue(pInParam,-1,"Data/SerialNumber",Common_Json_Type_String,szSerialNumber,0,0);
        Common_UnLock(g_phAuthHelloLock);

        Common_Sem_Create(&hSem,0,1,NULL);
        Update_Broadcast_Start(&hBroadcast, 10008, pInParam, NULL, 0, 1, 1, updateBroadcastCallback, hSem);
        Common_Sem_TryPend(hSem,2000);
        Update_Broadcast_Stop(&hBroadcast);
        Common_Sem_Destroy(&hSem);

        Common_Json_Delete(pInParam);
        pInParam = NULL;

        Common_Free(szSerialNumber,__FUNCTION__,__LINE__);
        szSerialNumber = NULL;
    }

    return;
}


