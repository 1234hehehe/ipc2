#ifndef __WEB_INTER_API_H__
#define __WEB_INTER_API_H__

#include "ovfs_web_rest.h"
#include "ants_hostmgr_type.h"

/*New*/
int web_init_global_website_info();
/*semantic start*/
int web_semantic_auth(webs_t wp, WEB_REST_INPARAM_T *inparam);
void web_semantic_auth_free(WEB_REST_INPARAM_T *inparam);
int web_semantic_parse_json(webs_t wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct **p_data);
int web_semantic_func_end(webs_t wp, int retCode, Common_cJSON_T *retData, char *errorString);


/*semantic end*/
/*New End*/

//±¨¾¯»Øµ÷
int web_GetConfig(S32 hLogin,U32 dwCommand, S32 lStartChan,S32 lChanNum,VOID *lpParam,U32 dwParamSize, VOID* lpOutBuffer, U32 dwOutBufferSize, U32* lpBytesReturned);
int web_ReleaseAlarmStatus(AntsHostMgrLibALarmStatusInfo_T *pAlarmStatus);
int web_QueryFile(S32 hLogin,S32 lChannel,AntsHostMgrLibQueryFile_T *pQuery);
int web_CapturePicture(S32 hLogin,S32 lChannel,S32 nStreamIdx, S8 *sPicBuffer, U32 dwBufferSize, U32 *lpReturnSize);
int web_QuickResponseCode(S8 *pString,AntsHostMgrLibQRCode_T *pPixelInfo);

#endif
