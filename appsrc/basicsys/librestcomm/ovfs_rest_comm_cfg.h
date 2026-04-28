#ifndef _REST_CFG_
#define _REST_CFG_

#ifdef __cplusplus
extern "C"{
#endif

#include"libcommon_api.h"
#include"libmodule_api.h"
#include"cjson.h"

#define MAX_SUPPORT_CFG_NODE 16


typedef struct
{
	char* 			    uri;

        int                                reduceCnt; // if uri=/A/B/C  reduceCnt=1 then, match uri=/A/B
        
        Common_cJSON_T*     initParam;
  
}REST_CFG_NODE_T;

typedef struct
{
    //int                             delayCnt;
    //pthread_mutex_t     lock;

    //int                                     time;
    //int                                     updateTime;
    
    Common_Thread_T          threadHdl;
    int                                     exit;
    //Common_InterSleep_T     sleepHdl;
    pthread_mutex_t             lock;
    int                                     cnt;
}REST_CFG_SAVE_INFO_T;

int RestCfg_Init();
int RestCfg_Destroy();

int RestCfg_AddCfgNode(REST_CFG_NODE_T* cfgNode);

REST_CFG_NODE_T* RestCfg_MatchNode(const char* uriString);

int RestCfg_SaveCfgFile(void);

int RestCfg_LoadCfg(void);

int RestCfg_DelaySaveCfg(void);

int RestCfg_Lock(void);
int RestCfg_UnLock(void);

#ifdef __cplusplus
};
#endif
#endif
                                                  
