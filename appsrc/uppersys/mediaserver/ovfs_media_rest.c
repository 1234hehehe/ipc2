/*
 * ovfs_media_rest.c
 *
 *  Created on: 2017年2月28日
 *      Author: eric
 */

#define _GNU_SOURCE
#define _XOPEN_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <stdarg.h>

#include <rtspserver_v2.h>
#include "ovfs_media.h"


ovfs_dst_cap_node s_support_dst[] =
{
    {OVFS_TIME_ZONE_WEST_LINE, -12, 0},                 //日界线西
    {OVFS_TIME_ZONE_SAMOA, -11, 0},                     //中途岛,萨摩亚群岛
    {OVFS_TIME_ZONE_HAWAII, -10, 0},                    //夏威夷
    {OVFS_TIME_ZONE_ALASKA, -9, 0},                     //阿拉斯加
    {OVFS_TIME_ZONE_PACIFIC_OCEAN, -8, 0},              //太平洋时间(美国和加拿大)
    {OVFS_TIME_ZONE_MOUNTAIN, -7, 0},                   //山地时间(美国和加拿大)
    {OVFS_TIME_ZONE_CENTRAL_CANADA, -6, 0},             //中部时间(美国和加拿大)
    {OVFS_TIME_ZONE_EASTERN_TIME_CANADA, -5, 0},        //东部时间(美国和加拿大)
    {OVFS_TIME_ZONE_CARACAS, -4, -30},                  //加拉加斯
    {OVFS_TIME_ZONE_ATLANTIC_CANADA, -4, 0},            //大西洋时间(加拿大)
    {OVFS_TIME_ZONE_NEWFOUNDLAND, -3, -30},             //纽芬兰
    {OVFS_TIME_ZONE_GEORGETOWN, -3, 0},                 //乔治敦, 巴西利亚
    {OVFS_TIME_ZONE_ATLANTIC_OCEAN, -2, 0},             //中大西洋
    {OVFS_TIME_ZONE_ANGLE_ISLANDS, -1, 0},              //福德角群岛
    {OVFS_TIME_ZONE_GREENWICH,0,0},                     //格林威治标准时间：都柏林，爱丁堡，伦敦，里斯本
    {OVFS_TIME_ZONE_AMSTERDAM,1,0},                     //阿姆斯特丹，柏林，伯尔尼，罗马，斯德哥尔摩，维也纳
    {OVFS_TIME_ZONE_ATHENS,2,0},                        //雅典，布加勒斯特
    {OVFS_TIME_ZONE_BAGHDAD,3,0},                       //巴格达 ,科威特
    {OVFS_TIME_ZONE_TEHERAN,3,30},                      //德黑兰
    {OVFS_TIME_ZONE_MOSCOW,4,0},                        //莫斯科，圣彼得堡，伏尔加格勒
    {OVFS_TIME_ZONE_KABUL,4,30},                        //喀布尔
    {OVFS_TIME_ZONE_ISB,5,0},                           //伊斯兰堡, 卡拉奇 ,塔什干
    {OVFS_TIME_ZONE_MADRAS,5,30},                       //马德拉斯，加尔各答，孟买，新德里
    {OVFS_TIME_ZONE_KATHMANDU,5,45},                    //加德满都
    {OVFS_TIME_ZONE_NOVOSIBIRSK,6,0},                   //新西伯利亚
    {OVFS_TIME_ZONE_RANGOON,6,30},                      //仰光
    {OVFS_TIME_ZONE_BANGKOK,7,0},                       //曼谷，河内，雅加达
    {OVFS_TIME_ZONE_BEIJING,8,0},                       //北京，重庆，香港特别行政区，乌鲁木齐
    {OVFS_TIME_ZONE_OSAKA,9,0},                         //首尔,大阪，札幌，东京
    {OVFS_TIME_ZONE_ADELAIDE,9,30},                     //阿德莱德 ,达尔文
    {OVFS_TIME_ZONE_CANBERRA,10,0},                     //堪培拉，墨尔本，悉尼
    {OVFS_TIME_ZONE_SOLOMON_ISLANDS,11,0},              //所罗门群岛，新喀里多尼亚
    {OVFS_TIME_ZONE_OSKLAND,12,0},                      //奥克兰, 惠林顿 ,斐济 ,马加丹
    {OVFS_TIME_ZONE_NUKUALOFA,13,0},                    //努库阿洛法

};

typedef int (*OVFS_MEDIA_REST_FUNC_F)(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);

typedef struct
{
    int ec;
    const char *es;
} MEDIA_REST_ERR_T;

typedef struct
{
    AccessHandle_T accessHandle;
    COMMON_DLIST_T authList;
    MQ_HANDLE_H mqHandle;
    Common_cJSON_T *restTree;
    char *customType;
    int preferh264Cnt[BOARD_VIDEO_DEV_MAX][BOARD_VIDEO_CHAN_MAX][BOARD_VIDEO_STREAM_MAX];
} OVFS_MEDIA_REST_CONTEXT_T;

typedef struct
{
    const char* uri;
    const char* label;
    const char* describtion;

    OVFS_MEDIA_REST_FUNC_F get;
    OVFS_MEDIA_REST_FUNC_F put;
    pthread_mutex_t subscribeLock;
    int subscribeTable[MEDIA_SUBSCRIBE_MAX_NUM];
    int hide;
} OVFS_MEDIA_REST_NODE_ATTR_T;

static MEDIA_REST_ERR_T s_restErrorInfo[] =
{
    {EC_MEDIA_REST_UNKNOWN,         EC_MEDIA_REST_UNKNOWN_STR},
    {EC_MEDIA_REST_PARAM_INVALID,   EC_MEDIA_REST_PARAM_INVALID_STR},
    {EC_MEDIA_REST_NO_METHOD,       EC_MEDIA_REST_NO_METHOD_STR},
    {EC_MEDIA_REST_NO_URI,          EC_MEDIA_REST_NO_URI_STR},
    {EC_MEDIA_REST_OP_FAILED,       EC_MEDIA_REST_OP_FAILED_STR},
};

static OVFS_MEDIA_REST_CONTEXT_T s_media_rest_ct;

static int RestTreeDefaultGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtspAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtspAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtmpAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtmpAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
#ifdef RTMP
static int RestTreeRtmpPushAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtmpPushAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtmpPushNotDisturbGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeRtmpPushNotDisturbPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
#endif
static int RestTreeRestorePut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeDisconnectIpGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeDisconnectIpPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeDisconnectEventGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeDebugPrintGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
static int RestTreeDebugPrintPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData);
#ifdef WITH_CURL
static int RestTreeSmartProtocolAttrGet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData);
static int RestTreeSmartProtocolAttrPut(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData);
static int RestTreeSmartProtocolUriCheck(const char *uri, Common_cJSON_T *curObj,Common_cJSON_T *in, Common_cJSON_T *out, void *userData);
static int RestTreeSmartProtocolNotDisturbGet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData);
static int RestTreeSmartProtocolNotDisturbSet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData);

#endif

static OVFS_MEDIA_REST_NODE_ATTR_T s_restNodeTree[] =
{
    {"/MediaServer","MediaServer","This is MediaServer describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Rtsp","Rtsp","This is Rtsp describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Rtsp/Attribute","Attribute","This is Rtsp Attribute describe",RestTreeRtspAttrGet,RestTreeRtspAttrPut,PTHREAD_MUTEX_INITIALIZER,{0},0},
#ifdef RTMP
    {"/MediaServer/Rtmp","Rtmp","This is Rtmp describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/RtmpPush","RtmpPush","This is Rtmp Push describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Rtmp/Attribute","Attribute","This is Rtmp Attribute describe",RestTreeRtmpAttrGet,RestTreeRtmpAttrPut,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/RtmpPush/Attribute","Attribute","This is RtmpPush Attribute describe",RestTreeRtmpPushAttrGet,RestTreeRtmpPushAttrPut,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/RtmpPush/NotDisturb","NotDisturb","This is RtmpPush NotDisturb describe",RestTreeRtmpPushNotDisturbGet,RestTreeRtmpPushNotDisturbPut,PTHREAD_MUTEX_INITIALIZER,{0},0},
#endif
    {"/MediaServer/Restore","Restore","This is Restore describe",NULL,RestTreeRestorePut,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/MediaDisconnect","MediaDisconnect","This is MediaDisconnect",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/MediaDisconnect/Attribute","Attribute","This is Attribute ",RestTreeDisconnectIpGet,RestTreeDisconnectIpPut,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/MediaDisconnect/Event","Event","This is Event ",RestTreeDisconnectEventGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Subscribe","Subscribe","This is subscribe describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Subscribe/Rtsp","Rtsp","This is subscribe rtsp describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Subscribe/Rtsp/Attribute","Attribute","This is Rtsp Attribute subscribe describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
#ifdef RTMP
    {"/MediaServer/Subscribe/Rtmp","Rtmp","This is subscribe rtmp describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Subscribe/Rtmp/Attribute","Attribute","This is Rtmp Attribute subscribe describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
#endif
    {"/MediaServer/Subscribe/MediaDisconnect","MediaDisconnect","This is MediaDisconnect subscribe describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/Subscribe/MediaDisconnect/Event","Event","This is Event subscribe describe",RestTreeDefaultGet,NULL,PTHREAD_MUTEX_INITIALIZER,{0},0},
    {"/MediaServer/DebugPrint","DebugPrint","This is DebugPrint describe",RestTreeDebugPrintGet,RestTreeDebugPrintPut,PTHREAD_MUTEX_INITIALIZER,{0},1},
#ifdef WITH_CURL
	{"/MediaServer/SmartProtocol", "SmartProtocol", "This is SmartProtocol describe", RestTreeSmartProtocolAttrGet, RestTreeSmartProtocolAttrPut, PTHREAD_MUTEX_INITIALIZER, {0}},
    {"/MediaServer/SmartProtocolUriCheck", "SmartProtocolUriCheck", "This is SmartProtocol UriCheck describe", RestTreeSmartProtocolUriCheck, NULL, PTHREAD_MUTEX_INITIALIZER, {0}},
    {"/MediaServer/SmartProtocolNotDisturb", "SmartProtocolNotDisturb", "This is SmartProtocol NotDisturb describe", RestTreeSmartProtocolNotDisturbGet, RestTreeSmartProtocolNotDisturbSet, PTHREAD_MUTEX_INITIALIZER, {0}},
#endif
};

static void DumpJson(Common_cJSON_T *object)
{
    if (object)
    {
        char * p = Common_cJSON_Print(object,NULL);
        if (object->string)
            printf("@%s=%s@\n",object->string,p);
        else
            printf("@%s@\n",p);
        if (p)
            Common_cJSON_free(p);
    }
}

static Common_cJSON_T *RestTreeGetAttachPosByUri(Common_cJSON_T *object, const char *uri)
{
    char *tppath = Common_StrDup((char *)uri,__func__,__LINE__), *p = NULL;
    char *tmp = tppath;
    Common_cJSON_T *c = object;
    Common_cJSON_T *g = object;
    do
    {
        g = c;
        p = strsep(&tmp, "/");

        if (p && strlen(p) > 0)
            c = Common_cJSON_GetObjectItem(c, p);

    } while (c && p && tmp);


    if (tppath)
        MEDIA_FREE(tppath);

    /*this uri already in json*/
    if (p != NULL && c != NULL && tmp == NULL)
        return NULL;

    return g;
}

static int RestTreeGenerate(Common_cJSON_T **tree, OVFS_MEDIA_REST_NODE_ATTR_T *node, int nodeCnt)
{
    int i = 0;
    OVFS_MEDIA_REST_NODE_ATTR_T *nd = node;

    if (tree == NULL || node == NULL || nodeCnt <= 0)
        return -EINVAL;

    if (*tree == NULL)
        *tree = Common_cJSON_CreateObject();

    for (i = 0; i < nodeCnt; i++)
    {
        nd = node + i;
        Common_cJSON_T * attach = RestTreeGetAttachPosByUri(*tree, nd->uri);
        if (attach == NULL)
        {
            LOGD("already has uri %s\n",nd->uri);
            continue;
        }
        Common_cJSON_T * nj = Common_cJSON_CreateObject();
        Common_cJSON_SetItemExtData(nj,nd,sizeof(OVFS_MEDIA_REST_NODE_ATTR_T));
        Common_cJSON_AddItemToObject(attach,nd->label,nj);
    }

    return 0;
}

static Common_cJSON_T * RestTreeGetNodeByUri(Common_cJSON_T *tree, const char *uri)
{
    if (tree == NULL || uri == NULL)
        return NULL;

    char *tppath = NULL, *p = NULL, *tmp = NULL;
    tppath = Common_StrDup((char *)uri,__func__,__LINE__);

    if ((p = strstr(tppath,"?")) != NULL)
        *p = '\0';

    p = NULL;
    tmp = tppath;
    Common_cJSON_T *c = tree;

    do
    {
        p = strsep(&tmp, "/");

        if (p)
        {
            if (strlen(p) > 0)
                c = Common_cJSON_GetObjectItem(c, p);
        }
        else
        {
            c = Common_cJSON_GetObjectItem(c, tmp);
        }
    } while (c && p && tmp);

    if (tppath)
        MEDIA_FREE(tppath);
    return c;
}

static int RestTreeDefaultGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    if (uri == NULL || curObj == NULL || out == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    Common_cJSON_T* uriList = Common_cJSON_CreateArray();
    Common_cJSON_AddItemToObject(out, "List", uriList);

    Common_cJSON_T* tmpChild = curObj->child;
    while (tmpChild && tmpChild->pExtData)
    {
        OVFS_MEDIA_REST_NODE_ATTR_T *nodeAttr = tmpChild->pExtData;
        if (nodeAttr->hide == 1)
        {
            tmpChild = tmpChild->next;
            continue;
        }
        Common_cJSON_T* tmpObj = Common_cJSON_CreateObject();
        Common_cJSON_AddStringToObject(tmpObj, "Uri", nodeAttr->uri);

        Common_cJSON_AddStringToObject(tmpObj, "Label", nodeAttr->label);
        Common_cJSON_AddStringToObject(tmpObj, "Describe", nodeAttr->describtion);
        Common_cJSON_T* methodArr = Common_cJSON_CreateArray();
        if (nodeAttr->get)
        {
            Common_cJSON_T* tmp = Common_cJSON_CreateString("get");
            Common_cJSON_AddItemToArray(methodArr, tmp);
        }

        if (nodeAttr->put)
        {
            Common_cJSON_T* tmp = Common_cJSON_CreateString("put");
            Common_cJSON_AddItemToArray(methodArr, tmp);
        }

        Common_cJSON_AddItemToObject(tmpObj, "Method", methodArr);

        Common_cJSON_AddItemToArray(uriList, tmpObj);
        tmpChild = tmpChild->next;
    }
    return 0;
}

static int RestTreeRtspAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;
}

static int RestTreeRtspAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;

    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d\n",node->subscribeTable[i]);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}

static int RestTreeRtmpAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMP_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;

}

static int RestTreeRtmpAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMP_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}
#ifdef RTMP
static int RestTreeRtmpPushAttrGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMPPUSH_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;

}

static int RestTreeRtmpPushAttrPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMPPUSH_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}

static int RestTreeRtmpPushNotDisturbGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMPPUSH_NOTDISTURB_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    //char *str = Common_Json_Print(out, NULL);
    //LOGD("out:[%s]\n",str);
    //free(str);

    return 0;

}

static int RestTreeRtmpPushNotDisturbPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RTMPPUSH_NOTDISTURB_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}

#endif
static int RestTreeRestorePut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    LOGW("Start Restoring\n");
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }
    if (Mq_Request(ct->mqHandle, MEDIA_REQ_RESTORE_CFG, NULL, 0, &ret, NULL, 0) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    return 0;
}

static int RestTreeDisconnectEventGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_DISCON_GET_EVENT, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;
}

static int RestTreeDisconnectIpGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_DISCON_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;
}

static int RestTreeDisconnectIpPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_DISCON_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}

static int RestTreeDebugPrintGet(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    int debugPrint = 0;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    debugPrint = RtspMgr_GetDebugPrint();
    Common_cJSON_AddItemToObject(out, "Enable", Common_cJSON_CreateNumber(debugPrint));
    return 0;
}

static int RestTreeDebugPrintPut(const char* uri,Common_cJSON_T* curObj,Common_cJSON_T* in,Common_cJSON_T* out, void *userData)
{
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }
    Common_cJSON_T *debugPrintJson = NULL;
    debugPrintJson = Common_cJSON_GetObjectItem(in,"Enable");
    if (debugPrintJson != NULL && debugPrintJson->string != NULL)
        RtspMgr_SetDebugPrint(debugPrintJson->valueint);
    return 0;
}

#ifdef WITH_CURL
static int RestTreeSmartProtocolAttrGet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_SMARTPROTOCOL_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;
}

static int RestTreeSmartProtocolAttrPut(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,Common_cJSON_T *out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;

    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_SMARTPROTOCOL_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d\n",node->subscribeTable[i]);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }
    return 0;
}

static int RestTreeSmartProtocolUriCheck(const char *uri, Common_cJSON_T *curObj,Common_cJSON_T *in, Common_cJSON_T *out, void *userData)
{
    int ret = 0;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj , in or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    char *checkuri = NULL, *response = NULL;
    Common_Json_GetAttrValue((cJSON_Struct *) in, -1, "Uri", NULL, &checkuri, NULL, NULL);

    ret = SmartProto_CheckServer(checkuri, &response);

    if (response != NULL)
    {
        cJSON_Struct *rspJson = Common_Json_Parse(response, NULL, NULL);
        if (rspJson != NULL)
        {
            Common_Json_GetAttrValue(rspJson,-1,"ReturnCode", NULL, NULL, &ret, NULL);
            if (ret != 0)
            {
                Common_Json_SetAttrValue(out, -1 , "ErrorInfo", Common_Json_Type_String,"ReturnCode error", 0, 0);
                ret = -1;
            }
            else
                ret = 0;
        }
        else
        {
            Common_Json_SetAttrValue(out, -1 , "ErrorInfo", Common_Json_Type_String,response, 0, 0);
            ret = -1;
        }
    }
    else
    {
        Common_Json_SetAttrValue(out, -1 , "ErrorInfo", Common_Json_Type_String,response, 0, 0);
        ret = -1;
    }

    if (response != NULL)
        MEDIA_FREE(response);
    return ret;
}

static int RestTreeSmartProtocolNotDisturbGet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,
                                    Common_cJSON_T *out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    int ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_GET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }

    return 0;
}

static int RestTreeSmartProtocolNotDisturbSet(const char *uri, Common_cJSON_T *curObj, Common_cJSON_T *in,
                                    Common_cJSON_T *out, void *userData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = userData;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1;
    if (uri == NULL || curObj == NULL || out == NULL || userData == NULL)
    {
        LOGE("curObj or outObj is NULL!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_SMARTPROTOCOL_NOTDISTURB_SET_CFG_JSON, in, sizeof(Common_cJSON_T *), &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        node = Common_cJSON_GetItemExtData(curObj,NULL);
        if (node)
        {
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],in,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
        }
    }

    return 0;
}

#endif

static const char* RestCommGetErrString(int errorCode)
{
    int i = 0;
    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_restErrorInfo);i++)
    {
        if(s_restErrorInfo[i].ec  == errorCode)
            return s_restErrorInfo[i].es;
    }
    return NULL;
}

static int CheckCfg(Common_cJSON_T *cfg)
{
    if (Common_Json_GetAttrValue((cJSON_Struct *) cfg, -1, "MediaServer/Rtsp/Enable", NULL, NULL, NULL, NULL) == NULL)
        return -1;
    if (Common_Json_GetAttrValue((cJSON_Struct *) cfg, -1, "MediaServer/Rtsp/RtspPort", NULL, NULL, NULL, NULL) == NULL)
        return -1;
    if (Common_Json_GetAttrValue((cJSON_Struct *) cfg, -1, "MediaServer/Rtmp/RtmpPort", NULL, NULL, NULL, NULL) == NULL)
        return -1;
    if (Common_Json_GetAttrValue((cJSON_Struct *) cfg, -1, "MediaServer/Rtmp/Enable", NULL, NULL, NULL, NULL) == NULL)
        return -1;
    if (Common_Json_GetAttrValue((cJSON_Struct *) cfg, -1, "MediaServer/Rtsp/MultiCast/Enable", NULL, NULL, NULL, NULL) == NULL)
        return -1;
    return 0;
}

static int ParserCheckInputJson(Common_cJSON_T* inputData,char** method,char** uri, Common_cJSON_T** inData)
{
    Common_cJSON_T* header = Common_cJSON_GetObjectItem(inputData, "Header");
    if (header == NULL)
    {
        LOGE("Get header fail!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    Common_cJSON_T* tmp = NULL;
    tmp = Common_cJSON_GetObjectItem(header, "Method");
    if (tmp == NULL || tmp->valuestring == NULL)
    {
        LOGE("Can't found method!\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }

    if (strcasecmp(tmp->valuestring,"get") != 0 && strcasecmp(tmp->valuestring,"put") != 0)
    {
        LOGE("Can't found method!\n");
        return EC_MEDIA_REST_NO_METHOD;
    }

    if (method)
        *method = tmp->valuestring;

    tmp = Common_cJSON_GetObjectItem(header, "Uri");
    if (tmp == NULL)
    {
        LOGE("Can't found Uri.\n");
        return EC_MEDIA_REST_PARAM_INVALID;
    }
    if (uri)
        *uri = tmp->valuestring;

    tmp = Common_cJSON_GetObjectItem(inputData, "Data");
    if (inData)
        *inData = tmp;

    return 0;
}

static int RestTreeCallFun(char * uri, char *method, Common_cJSON_T *inData, Common_cJSON_T **outData, void *pUserData)
{
    int ret = 0, rsize = 0;
    OVFS_MEDIA_REST_CONTEXT_T *ct = pUserData;
    Common_cJSON_T * restNodeJson = NULL, *responseJson = NULL;
    OVFS_MEDIA_REST_NODE_ATTR_T * restNode = NULL;

    if (uri == NULL || method == NULL)
    {
        ret = EC_MEDIA_REST_PARAM_INVALID;
    }

    if (ret == 0)
    {
        restNodeJson = RestTreeGetNodeByUri(ct->restTree, uri);
        if (restNodeJson == NULL)
        {
            LOGE("no such uri %s\n",uri);
            ret = EC_MEDIA_REST_NO_URI;
        }
    }

    if (ret == 0 && strcasecmp(method,"get") != 0 && strcasecmp(method,"put") != 0)
    {
        LOGE("Can't found method!\n");
        ret = EC_MEDIA_REST_PARAM_INVALID;
    }

    if (ret == 0)
    {
        restNode = Common_cJSON_GetItemExtData(restNodeJson, &rsize);
        if (restNode == NULL || rsize != sizeof(OVFS_MEDIA_REST_NODE_ATTR_T))
        {
            LOGE("no such node or size error\n");
            ret = EC_MEDIA_REST_OP_FAILED;
        }
        else
        {
            if (strcasecmp(method, "get") == 0 && restNode->get == NULL)
            {
                LOGE("no such method\n");
                ret = EC_MEDIA_REST_NO_METHOD;
            }

            if (strcasecmp(method, "put") == 0 && restNode->put == NULL)
            {
                LOGE("no such method\n");
                ret = EC_MEDIA_REST_NO_METHOD;
            }
        }
    }

    responseJson = Common_cJSON_CreateObject();

    if (ret == 0)
    {
        if (strcasecmp(method, "get") == 0)
            ret = restNode->get(uri, restNodeJson, inData, responseJson,pUserData);
        else if (strcasecmp(method, "put") == 0)
            ret = restNode->put(uri, restNodeJson, inData, responseJson,pUserData);
        else
        {
            LOGE("no such method\n");
            ret = EC_MEDIA_REST_NO_METHOD;
        }
    }

    if (responseJson && (responseJson)->child == NULL)
    {
        Common_cJSON_Delete(responseJson);
        responseJson = NULL;
    }

    if (outData)
    {
        *outData = Common_cJSON_CreateObject();
        Common_cJSON_T* header = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(*outData, "Header", header);
        Common_cJSON_AddNumberToObject(header, "Code", ret);
        if (ret != 0)
        {
            char * p = (char *) RestCommGetErrString(ret);
            if (p != NULL)
            {
                Common_cJSON_AddStringToObject(header, "Describe", p);
            }
            else
            {
                char unknownStr[32] = { 0 };
                snprintf(unknownStr, sizeof(unknownStr) - 1, "Unknown error, %d", ret);
                Common_cJSON_AddStringToObject(header, "Describe", unknownStr);
            }
        }
        if (responseJson)
        {
            Common_cJSON_AddItemToObject(*outData, "Data", responseJson);
        }
    }
    else
    {
        if (responseJson)
        {
            Common_cJSON_Delete(responseJson);
        }
    }

    return ret;
}

static S32 RestModuleCalleeFunctions(AccessHandle_T hAccessHandle,cJSON_Struct *pInParams,cJSON_Struct **pOutParams,void *pUserData)
{
    LOGW("RestModuleCalleeFunctions\n");
    char* method = NULL, *uri = NULL;
    Common_cJSON_T *inData = NULL;
    int ret = 0;

    if (hAccessHandle == NULL || pInParams == NULL)
    {
        ret = EC_MEDIA_REST_PARAM_INVALID;
    }

    if (ret == 0)
    {
        char * p = Common_cJSON_PrintUnformatted((Common_cJSON_T *)pInParams,NULL);
        LOGI("Recv Request %s\n",p);
        if (p)
            MEDIA_FREE(p);
    }

    if (ret == 0)
        ret = ParserCheckInputJson(pInParams, &method, &uri, &inData);

    if (ret != 0)
    {
        LOGE("inparam parse fail!\n");
    }

    if (ret == 0)
        ret = RestTreeCallFun(uri, method, inData, (Common_cJSON_T **)pOutParams, pUserData);

    return ret;
}

static S32 RestModuleEventSubscribe(AccessHandle_T hAccessHandle,S32 nType,S32 nRecvID,S8 *szSubscribeUri,cJSON_Struct **pQueryEventInfo,void *pUserData)
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = (OVFS_MEDIA_REST_CONTEXT_T *)pUserData;
    OVFS_MEDIA_REST_NODE_ATTR_T * restNode = NULL;
    char *realUri = NULL, *strP = NULL;
    int rsize = 0, subIdx = -1;

    if (szSubscribeUri == NULL || hAccessHandle == NULL || pUserData == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    LOGI("recv Sub event %s type %d id %d\n",szSubscribeUri,nType,nRecvID);
    if ((strP = strstr(szSubscribeUri,"/MediaServer/Subscribe/")) == NULL)
    {
        realUri = MEDIA_STRDUP(szSubscribeUri);
    }
    else
    {
        realUri = MEDIA_MALLOC(strlen(szSubscribeUri));
        snprintf(realUri,strlen(szSubscribeUri)-1,"/MediaServer/%s",strP+strlen("/MediaServer/Subscribe/"));
    }
    LOGI("trans uri to %s\n",realUri);
    Common_cJSON_T *restNodeJson = RestTreeGetNodeByUri(ct->restTree, realUri);
    if (restNodeJson == NULL)
    {
        LOGE("no such uri %s\n",realUri);
        MEDIA_FREE(realUri);
        return -1;
    }
    else
    {
        restNode = Common_cJSON_GetItemExtData(restNodeJson, &rsize);
        if (restNode == NULL || rsize != sizeof(OVFS_MEDIA_REST_NODE_ATTR_T))
        {
            LOGE("unsupport subscribe\n");
            MEDIA_FREE(realUri);
            return EC_MEDIA_REST_OP_FAILED;
        }
        else
        {
            int c = 0, i = 0;

            pthread_mutex_lock(&restNode->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(restNode->subscribeTable);
            for (i = 0; i < c; i++)
            {
                if (restNode->subscribeTable[i] == 0 && nType == 0)
                {
                    subIdx = i;
                    break;
                }
                else if (restNode->subscribeTable[i] == nRecvID && nType != 0)
                {
                    subIdx = i;
                    break;
                }

            }
            pthread_mutex_unlock(&restNode->subscribeLock);

            if (subIdx < 0)
            {
                LOGE("subscribe max %s max %d\n", realUri, c);
                MEDIA_FREE(realUri);
                return EC_MEDIA_REST_OP_FAILED;
            }

        }
    }

    switch(nType)
    {
        case 0: /*subscribe*/
        {
            RestTreeCallFun(realUri, "get", NULL, (Common_cJSON_T **)pQueryEventInfo,pUserData);
            pthread_mutex_lock(&restNode->subscribeLock);
            restNode->subscribeTable[subIdx] = nRecvID;
            pthread_mutex_unlock(&restNode->subscribeLock);
            LOGI("subscribe id %d idx %d\n",nRecvID,subIdx);
            break;
        }
        case 1: /*unsubscribe*/
        {
            pthread_mutex_lock(&restNode->subscribeLock);
            restNode->subscribeTable[subIdx] = 0;
            LOGI("unsubscribe id %d idx %d\n",nRecvID,subIdx);
            pthread_mutex_unlock(&restNode->subscribeLock);
            break;
        }
        case 2: /*query*/
        {
            RestTreeCallFun(realUri, "get", NULL, (Common_cJSON_T **)pQueryEventInfo,pUserData);
            break;
        }
    }
    MEDIA_FREE(realUri);

    return 0;
}

static void * RestHandleAlarmReportThread(void *data)
{
    int arrySize = 0, i = 0, ret = -1;
    cJSON_Struct *pArry_root = NULL;
    char *alarmName = NULL, *startTime = NULL, *stopTime = NULL, *devName = NULL;
    MEDIA_ALARM_STATUS_T alarmStatusInfo = { 0 };
    MEDIA_ALARM_STATUS_NODE_T *node = NULL;
    OVFS_MEDIA_REST_CONTEXT_T *ct = &s_media_rest_ct;
    cJSON_Struct *pEventInfo = (cJSON_Struct *)data;
    prctl(PR_SET_NAME, __func__);

    WsMgr_SendAlarm(data);
    pArry_root = Common_Json_GetAttrValue(pEventInfo, -1, "Data.ResList", NULL, NULL, NULL, NULL);
    if(pArry_root)
    {
        arrySize = Common_Json_Size(pArry_root);
        if (arrySize <= 0)
        {
            LOGE("array size error %d\n",arrySize);
            Common_Json_Delete(pEventInfo);
            return 0;
        }

        alarmStatusInfo.alarmStatusCnt = arrySize;
        alarmStatusInfo.alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T)*arrySize);
        memset(alarmStatusInfo.alarmStatus,0,sizeof(MEDIA_ALARM_STATUS_NODE_T)*arrySize);
        for (i = 0; i < arrySize; i ++)
        {
            node = alarmStatusInfo.alarmStatus + i;

            alarmName = NULL;
            startTime = NULL;
            stopTime = NULL;

            Common_Json_GetAttrValue(pArry_root, i, "AlarmSrcType", NULL, NULL, &node->alarmSrcType, NULL);

            Common_Json_GetAttrValue(pArry_root, i, "Channel", NULL, NULL, &node->channel, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "Status", NULL, NULL, &node->status, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "Stream", NULL, NULL, &node->stream, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "Device", NULL, NULL, &node->device, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "AlarmType", NULL, NULL, &node->alarmType, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "RegionId", NULL, NULL, &node->regionId, NULL);

            Common_Json_GetAttrValue(pArry_root, i, "AlarmName", NULL, &alarmName, NULL, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "DevName", NULL, &devName, NULL, NULL);
            Common_Json_GetAttrValue(pArry_root, i, "StartTime", NULL, &startTime,NULL,  NULL);
            Common_Json_GetAttrValue(pArry_root, i, "StopTime", NULL, &stopTime,NULL,  NULL);

            if (alarmName)
                snprintf(node->alarmName,sizeof(node->alarmName) - 1,"%s",alarmName);

            if (devName)
                snprintf(node->devName,sizeof(node->devName) - 1,"%s",devName);

            if (startTime && strcmp(startTime,"00000000000000") != 0)
            {
                sscanf(startTime, "%04d%02d%02d%02d%02d%02d",
                          &node->startTime.tm_year,
                          &node->startTime.tm_mon,
                          &node->startTime.tm_mday,
                          &node->startTime.tm_hour,
                          &node->startTime.tm_min,
                          &node->startTime.tm_sec);
                node->startTime.tm_year -= 1900;
                node->startTime.tm_mon -= 1;
                node->startTime.tm_isdst = -1;

            }

            if (stopTime && strcmp(stopTime,"00000000000000") != 0)
            {
                sscanf(stopTime, "%04d%02d%02d%02d%02d%02d",
                          &node->stopTime.tm_year,
                          &node->stopTime.tm_mon,
                          &node->stopTime.tm_mday,
                          &node->stopTime.tm_hour,
                          &node->stopTime.tm_min,
                          &node->stopTime.tm_sec);
                node->stopTime.tm_year -= 1900;
                node->stopTime.tm_mon -= 1;
                node->stopTime.tm_isdst = -1;
            }
        }

        Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_SET_APP_DATA, (void *)(&alarmStatusInfo),
                        sizeof(MEDIA_ALARM_STATUS_T), &ret, NULL, 0);
        MEDIA_FREE(alarmStatusInfo.alarmStatus);
    }
    else
    {

        alarmStatusInfo.alarmStatusCnt = 1;
        alarmStatusInfo.alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T));
        memset(alarmStatusInfo.alarmStatus,0,sizeof(MEDIA_ALARM_STATUS_NODE_T));

        node = alarmStatusInfo.alarmStatus;

        Common_Json_GetAttrValue(pEventInfo, -1, "AlarmSrcType", NULL, NULL, &node->alarmSrcType, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "Channel", NULL, NULL, &node->channel, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "Status", NULL, NULL, &node->status, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "Stream", NULL, NULL, &node->stream, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "Device", NULL, NULL, &node->device, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "AlarmType", NULL, NULL, &node->alarmType, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "RegionId", NULL, NULL, &node->regionId, NULL);

        Common_Json_GetAttrValue(pEventInfo, -1, "AlarmName", NULL, &alarmName, NULL, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "DevName", NULL, &devName, NULL, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "StartTime", NULL, &startTime, NULL, NULL);
        Common_Json_GetAttrValue(pEventInfo, -1, "StopTime", NULL, &stopTime, NULL, NULL);

        if(node->status)
        {
            node->sendPic = 1;
        }

        if (alarmName)
            snprintf(node->alarmName, sizeof(node->alarmName) - 1, "%s", alarmName);

        if (devName)
            snprintf(node->devName, sizeof(node->devName) - 1, "%s", devName);

        if (startTime && strcmp(startTime, "00000000000000") != 0)
        {
            sscanf(startTime, "%04d%02d%02d%02d%02d%02d", &node->startTime.tm_year, &node->startTime.tm_mon,
                            &node->startTime.tm_mday, &node->startTime.tm_hour, &node->startTime.tm_min,
                            &node->startTime.tm_sec);
            node->startTime.tm_year -= 1900;
            node->startTime.tm_mon -= 1;
            node->startTime.tm_isdst = -1;

        }

        if (stopTime && strcmp(stopTime, "00000000000000") != 0)
        {
            sscanf(stopTime, "%04d%02d%02d%02d%02d%02d", &node->stopTime.tm_year, &node->stopTime.tm_mon,
                            &node->stopTime.tm_mday, &node->stopTime.tm_hour, &node->stopTime.tm_min,
                            &node->stopTime.tm_sec);
            node->stopTime.tm_year -= 1900;
            node->stopTime.tm_mon -= 1;
            node->stopTime.tm_isdst = -1;
        }

        LOGW("match node %s status %d region id %d %s %s cnt %d\n",
                        node->alarmName, node->status, node->regionId, startTime, stopTime,alarmStatusInfo.alarmStatusCnt);

        RtspMgr_SendAppData(&alarmStatusInfo, 1);
        Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_SET_APP_DATA, (void *) (&alarmStatusInfo),
                        sizeof(MEDIA_ALARM_STATUS_T), &ret, NULL,0);
        MEDIA_FREE(alarmStatusInfo.alarmStatus);
    }

    Common_Json_Delete(pEventInfo);
    return NULL;
}

static int RestAccessAlarmReport(AccessHandle_T hAccessHandle, int nSubscribeID, cJSON_Struct *pEventInfo, cJSON_Struct **pOutParams, void *pUserData)
{
    /* int arrySize = 0, i = 0, ret = -1; */
    /* cJSON_Struct *pArry_root = NULL; */
    /* char *alarmName = NULL, *startTime = NULL, *stopTime = NULL, *devName = NULL; */
    OVFS_MEDIA_REST_CONTEXT_T *ct = pUserData;
    /* MEDIA_ALARM_STATUS_T alarmStatusInfo = { 0 }; */
    /* MEDIA_ALARM_STATUS_NODE_T *node = NULL; */

    if (pEventInfo == NULL || ct == NULL)
    {
        LOGE("event info was null or user data was null\n");
        return 0;
    }

    LOGI("recv alarm status \n");
//    DumpJson(pEventInfo);
    pthread_t alarmReportPth = 0;
    cJSON_Struct *eJson = Common_Json_Duplicate(pEventInfo,1);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setstacksize(&attr, PTHREAD_STACK_MIN * 16);
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    pthread_create(&alarmReportPth,&attr,RestHandleAlarmReportThread,eJson);
    pthread_attr_destroy(&attr);
    /* pArry_root = Common_Json_GetAttrValue(pEventInfo, -1, "Data.ResList", NULL, NULL, NULL, NULL); */
    /* if(pArry_root) */
    /* { */
    /*     arrySize = Common_Json_Size(pArry_root); */
    /*     if (arrySize <= 0) */
    /*     { */
    /*         LOGE("array size error %d\n",arrySize); */
    /*         return 0; */
    /*     } */

    /*     alarmStatusInfo.alarmStatusCnt = arrySize; */
    /*     alarmStatusInfo.alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T)*arrySize); */
    /*     memset(alarmStatusInfo.alarmStatus,0,sizeof(MEDIA_ALARM_STATUS_NODE_T)*arrySize); */
    /*     for (i = 0; i < arrySize; i ++) */
    /*     { */
    /*         node = alarmStatusInfo.alarmStatus + i; */

    /*         alarmName = NULL; */
    /*         startTime = NULL; */
    /*         stopTime = NULL; */

    /*         Common_Json_GetAttrValue(pArry_root, i, "AlarmSrcType", NULL, NULL, &node->alarmSrcType, NULL); */

    /*         Common_Json_GetAttrValue(pArry_root, i, "Channel", NULL, NULL, &node->channel, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "Status", NULL, NULL, &node->status, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "Stream", NULL, NULL, &node->stream, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "Device", NULL, NULL, &node->device, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "AlarmType", NULL, NULL, &node->alarmType, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "RegionId", NULL, NULL, &node->regionId, NULL); */

    /*         Common_Json_GetAttrValue(pArry_root, i, "AlarmName", NULL, &alarmName, NULL, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "DevName", NULL, &devName, NULL, NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "StartTime", NULL, &startTime,NULL,  NULL); */
    /*         Common_Json_GetAttrValue(pArry_root, i, "StopTime", NULL, &stopTime,NULL,  NULL); */

    /*         if (alarmName) */
    /*             snprintf(node->alarmName,sizeof(node->alarmName) - 1,"%s",alarmName); */

    /*         if (devName) */
    /*             snprintf(node->devName,sizeof(node->devName) - 1,"%s",devName); */

    /*         if (startTime && strcmp(startTime,"00000000000000") != 0) */
    /*         { */
    /*             sscanf(startTime, "%04d%02d%02d%02d%02d%02d", */
    /*                       &node->startTime.tm_year, */
    /*                       &node->startTime.tm_mon, */
    /*                       &node->startTime.tm_mday, */
    /*                       &node->startTime.tm_hour, */
    /*                       &node->startTime.tm_min, */
    /*                       &node->startTime.tm_sec); */
    /*             node->startTime.tm_year -= 1900; */
    /*             node->startTime.tm_mon -= 1; */
    /*             node->startTime.tm_isdst = -1; */

    /*         } */

    /*         if (stopTime && strcmp(stopTime,"00000000000000") != 0) */
    /*         { */
    /*             sscanf(stopTime, "%04d%02d%02d%02d%02d%02d", */
    /*                       &node->stopTime.tm_year, */
    /*                       &node->stopTime.tm_mon, */
    /*                       &node->stopTime.tm_mday, */
    /*                       &node->stopTime.tm_hour, */
    /*                       &node->stopTime.tm_min, */
    /*                       &node->stopTime.tm_sec); */
    /*             node->stopTime.tm_year -= 1900; */
    /*             node->stopTime.tm_mon -= 1; */
    /*             node->stopTime.tm_isdst = -1; */
    /*         } */
    /*     } */

    /*     Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_SET_APP_DATA, (void *)(&alarmStatusInfo), */
    /*                     sizeof(MEDIA_ALARM_STATUS_T), &ret, NULL, 0); */
    /*     MEDIA_FREE(alarmStatusInfo.alarmStatus); */
    /* } */
    /* else */
    /* { */

    /*     alarmStatusInfo.alarmStatusCnt = 1; */
    /*     alarmStatusInfo.alarmStatus = MEDIA_MALLOC(sizeof(MEDIA_ALARM_STATUS_NODE_T)); */
    /*     memset(alarmStatusInfo.alarmStatus,0,sizeof(MEDIA_ALARM_STATUS_NODE_T)); */

    /*     node = alarmStatusInfo.alarmStatus; */

    /*     Common_Json_GetAttrValue(pEventInfo, -1, "AlarmSrcType", NULL, NULL, &node->alarmSrcType, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "Channel", NULL, NULL, &node->channel, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "Status", NULL, NULL, &node->status, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "Stream", NULL, NULL, &node->stream, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "Device", NULL, NULL, &node->device, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "AlarmType", NULL, NULL, &node->alarmType, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "RegionId", NULL, NULL, &node->regionId, NULL); */

    /*     Common_Json_GetAttrValue(pEventInfo, -1, "AlarmName", NULL, &alarmName, NULL, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "DevName", NULL, &devName, NULL, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "StartTime", NULL, &startTime, NULL, NULL); */
    /*     Common_Json_GetAttrValue(pEventInfo, -1, "StopTime", NULL, &stopTime, NULL, NULL); */

    /*     if (alarmName) */
    /*         snprintf(node->alarmName, sizeof(node->alarmName) - 1, "%s", alarmName); */

    /*     if (devName) */
    /*         snprintf(node->devName, sizeof(node->devName) - 1, "%s", devName); */

    /*     if (startTime && strcmp(startTime, "00000000000000") != 0) */
    /*     { */
    /*         sscanf(startTime, "%04d%02d%02d%02d%02d%02d", &node->startTime.tm_year, &node->startTime.tm_mon, */
    /*                         &node->startTime.tm_mday, &node->startTime.tm_hour, &node->startTime.tm_min, */
    /*                         &node->startTime.tm_sec); */
    /*         node->startTime.tm_year -= 1900; */
    /*         node->startTime.tm_mon -= 1; */
    /*         node->startTime.tm_isdst = -1; */

    /*     } */

    /*     if (stopTime && strcmp(stopTime, "00000000000000") != 0) */
    /*     { */
    /*         sscanf(stopTime, "%04d%02d%02d%02d%02d%02d", &node->stopTime.tm_year, &node->stopTime.tm_mon, */
    /*                         &node->stopTime.tm_mday, &node->stopTime.tm_hour, &node->stopTime.tm_min, */
    /*                         &node->stopTime.tm_sec); */
    /*         node->stopTime.tm_year -= 1900; */
    /*         node->stopTime.tm_mon -= 1; */
    /*         node->stopTime.tm_isdst = -1; */
    /*     } */

    /*     LOGW("match node %s status %d region id %d %s %s cnt %d\n", */
    /*                     node->alarmName, node->status, node->regionId, startTime, stopTime,alarmStatusInfo.alarmStatusCnt); */

    /*     RtspMgr_SendAppData(&alarmStatusInfo, 1); */
    /*     Mq_Request(ct->mqHandle, MEDIA_REQ_RTSP_SET_APP_DATA, (void *) (&alarmStatusInfo), */
    /*                     sizeof(MEDIA_ALARM_STATUS_T), &ret, NULL,0); */
    /*     MEDIA_FREE(alarmStatusInfo.alarmStatus); */
    /* } */

    return 0;
}

static int RestAccessAudioReport(AccessHandle_T hAccessHandle, int nSubscribeID, cJSON_Struct *pEventInfo, cJSON_Struct **pOutParams, void *pUserData)
{
    MEDIA_BOARD_STATE_T boardState = { 0 };
    int ret = -1;
    if (pEventInfo == NULL)
    {
        LOGE("event info was null or user data was null\n");
        return 0;
    }
    LOGI("recv audio status change\n");
    DumpJson(pEventInfo);
    Common_Json_GetAttrValue(pEventInfo, -1, "AudioEnable", NULL, NULL, &boardState.audioState, NULL);
    Mq_Request(s_media_rest_ct.mqHandle, MEDIA_REQ_SET_BOARD_STATE, &boardState, sizeof(MEDIA_BOARD_STATE_T), &ret,NULL, 0);
    RtspMgr_UpdateBoardState(boardState);
    return 0;
}
static int RestReqStreamAddress(void *auth, char *uri, char *addr, int addrSize)
{
    int ret = 0;

    if (auth == NULL || uri == NULL || addr == NULL || addrSize <= 0)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    Common_cJSON_T* inParam = Common_cJSON_CreateObject();
    Common_cJSON_T* outParam = NULL;
    Common_cJSON_T* header = Common_cJSON_CreateObject();
    Common_cJSON_T *addressJson = NULL;
    Common_cJSON_T *authJson = Common_cJSON_Duplicate(auth ,1);

    Common_cJSON_AddStringToObject(header, "Method", "get");
    Common_cJSON_AddStringToObject(header, "Uri", uri);
    Common_cJSON_AddNumberToObject(header, "IsRemote", 1);
    Common_cJSON_AddItemToObject(header, "Auth", authJson);

    Common_cJSON_AddItemToObject(inParam, "Header", header);

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) inParam, (cJSON_Struct**) (&outParam), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d %s\n", ret,uri);
    }
    else if (outParam == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        DumpJson(outParam);
        addressJson = JsonOper_GetObjectItemByPath(outParam, "Data.AddressString");
        if (addressJson == NULL)
        {
            ret = -1;
            LOGE("not find response data \n");
        }
        else if (addressJson->valuestring == NULL)
        {
            ret = -1;
            LOGE("address is NULL\n");
        }
        else if (strlen(addressJson->valuestring) > addrSize)
        {
            ret = -1;
            LOGE("address size was too short addrSize %d addr %s\n",addrSize,addressJson->valuestring);
        }
        else
        {
            LOGI("get stream open uri %s\n", addressJson->valuestring);
            ret = 0;
        }

    }

    if (ret == 0)
    {
        snprintf(addr,addrSize,"%s",addressJson->valuestring);
    }

    if (outParam)
    {
        Common_cJSON_Delete(outParam);
    }

    if (inParam)
    {
        Common_cJSON_Delete(inParam);
    }
    return ret;
}

static int RestAuthNodeCompare(void *a, void *b)
{
    if (a == NULL || b == NULL)
        return -1;

    MEDIA_AUTH_NODE_T *node = a;

    if (node->sessionId == *(int *)b)
        return 0;

    return -1;
}

static void RestAuthNodeFree(void *ptr)
{
    MEDIA_AUTH_NODE_T *node = ptr;
    if (ptr == NULL)
        return;

    if (node->auth)
    {
        Common_Json_Delete(node->auth);
        node->auth = NULL;
    }

    MEDIA_FREE(ptr);
}

static void AddSessionId(int sessionId)
{
#ifdef RTMP
    char cmd[128] = { 0 };
    snprintf(cmd, sizeof(cmd) -1 ,"echo %d >> " MEDIA_AUTH_TMP_FILE,sessionId);
    Common_System(cmd);
#endif
}

static void RemoveSessionId(int sessionId)
{
#ifdef RTMP
    char cmd[128] = { 0 };
    snprintf(cmd, sizeof(cmd) -1 ,"sed -i '/%d/d' " MEDIA_AUTH_TMP_FILE,sessionId);
    Common_System(cmd);
#endif
}

static void RecycleSessionId()
{
#ifdef RTMP
    FILE * fp = fopen(MEDIA_AUTH_TMP_FILE,"r");
    if (fp == NULL)
        return ;

    char sessionIdStr[64] = { 0 };

    while (fgets(sessionIdStr,sizeof(sessionIdStr) - 1,fp) != NULL)
    {
        int sessionId = 0;
        char uri[128] = { 0 };
        cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        cJSON_Struct *outJson = NULL;
        sessionId = atoi(sessionIdStr);

        if (reqJson == NULL || sessionId < 0)
            break;

        snprintf(uri,sizeof(uri) - 1,MEDIA_REST_REQ_LOGOUT,sessionId);
        Common_Json_SetAttrValue(reqJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValue(reqJson,-1, "Header/Method", Common_Json_Type_String,"delete", 0, 0);
        Common_Json_SetAttrValue(reqJson,-1, "Header/Uri", Common_Json_Type_String, uri, 0, 0);

        Access_CallFunctions(s_media_rest_ct.accessHandle,reqJson,&outJson,3000);

        if (outJson)
            Common_Json_Delete(outJson);

        if (reqJson)
            Common_Json_Delete(reqJson);
    }

    fclose(fp);
    unlink("/tmp/.media");
#endif
}


static S32 ovfs_utility_linux_to_ovfs_time(time_t linux_time, Ants_RtspDayTime *ovfs_time)
{
    if (ovfs_time == NULL)
    {
        LOGE("ovfs_time == NULL\n");
        return -1;
    }

    struct tm p;

    Common_LocalTime_r (&linux_time, &p);
    ovfs_time->wYear = 1900 + p.tm_year;
    ovfs_time->byMon = 1 + p.tm_mon;
    ovfs_time->byDay = p.tm_mday;

    ovfs_time->byHour = p.tm_hour;
    ovfs_time->byMin = p.tm_min;
    ovfs_time->bySec = p.tm_sec;

    return 0;
}

static S32 ovfs_utility_ovfs_to_linux_time(const Ants_RtspDayTime *ovfs_time, time_t *linux_time)
{
    if ((ovfs_time == NULL) || (linux_time == NULL))
    {
        LOGE("para is NULL\n");
        return -1;
    }

    struct tm p;
    Common_LocalTime_r (linux_time, &p);   //有个is_dst成员，不能乱配置

    p.tm_year = ovfs_time->wYear - 1900;
    p.tm_mon = ovfs_time->byMon - 1;
    p.tm_mday = ovfs_time->byDay;

    p.tm_hour = ovfs_time->byHour;
    p.tm_min = ovfs_time->byMin;
    p.tm_sec = ovfs_time->bySec;
    *linux_time = mktime(&p);

    return 0;
}

static int RestReqCustomType()
{
    cJSON_Struct * paramJson = NULL, *pOutParams = NULL;
    char * customStr = NULL;
    paramJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (paramJson)
    {
        Common_Json_SetAttrValue(paramJson,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(paramJson,-1,"Header/Uri",Common_Json_Type_String,"/Core/Version",0,0);
        Common_Json_SetAttrValue(paramJson,-1,"Header/Method",Common_Json_Type_String,"get",0,0);
        Access_CallFunctions(s_media_rest_ct.accessHandle,paramJson,&pOutParams,3000);
        DumpJson((Common_cJSON_T *)pOutParams);

        if (pOutParams)
            Common_Json_GetAttrValue((cJSON_Struct *) pOutParams, -1, "Data/Customer", NULL, &customStr,  NULL, NULL);

        if (customStr != NULL)
        {
            LOGW("customer type %s\n",customStr);
            s_media_rest_ct.customType = Common_StrDup(customStr,NULL,0);
        }

        Common_Json_Delete(paramJson);
        Common_Json_Delete(pOutParams);
        paramJson = NULL;
        pOutParams = NULL;
        return 0;
    }

    return -1;
}

int RestReqAudioState()
{
    int ret = 0;
    int enable = 0;
    MEDIA_BOARD_STATE_T boardState = { 0 };
    cJSON_Struct * paramJson = NULL, *pOutParams = NULL;

    paramJson = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (paramJson)
    {
        Common_Json_SetAttrValueObj(paramJson, "Header");
        Common_Json_SetAttrValueStr(paramJson, "Header/Uri", MEDIA_REST_REQ_AUDIO_ATTR_ADDR);
        Common_Json_SetAttrValueStr(paramJson, "Header/Method", "get");
        Access_CallFunctions(s_media_rest_ct.accessHandle,paramJson,&pOutParams,3000);

        if (pOutParams)
        {
            if(Common_Json_GetAttrValueInt(pOutParams, "Data/AudioEnable", &enable))
            {
                boardState.audioState = enable;
                Mq_Request(s_media_rest_ct.mqHandle, MEDIA_REQ_SET_BOARD_STATE, &boardState, sizeof(MEDIA_BOARD_STATE_T), &ret,NULL, 0);
            }
        }

        Common_Json_Delete(paramJson);
        Common_Json_Delete(pOutParams);
        paramJson = NULL;
        pOutParams = NULL;
        return 0;
    }

    return -1;
}

int RegisterStoreUri(AccessHandle_T hAccessHandle)
{
    int ret = -1;
    cJSON_Struct *pConfig = NULL,*pOutParams = NULL,*pChild = NULL,*pArray = NULL,*pItem = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,MEDIA_REST_REQ_CORE_RESTORE,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);
        pItem = Common_Json_SetAttrValue(pArray,0,NULL,Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pItem,-1,"Uri",Common_Json_Type_String,"/MediaServer/Restore",0,0); //MediaServer/Restore was defined OVFS_MEDIA_REST_NODE_ATTR_T s_restNodeTree[]
        Common_Json_SetAttrValue(pItem,-1,"Label",Common_Json_Type_String,"Restore",0,0);
        Common_Json_SetAttrValue(pItem,-1,"NeedReboot",Common_Json_Type_Number,NULL,0,0);
        ret = Access_CallFunctions(hAccessHandle,pConfig,&pOutParams,3000);
        if(ret != 0)
        {
            DumpJson(pConfig);
            LOGE("ret:%d\n",ret);
        }
        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
    }
    return ret;
}

int InitTimeJson(cJSON_Struct *timeJson)
{
    if(timeJson == NULL)
    {
        return -1;
    }

    int i=0, j=0;
    char pathname[128] = {0};
    for(i=0; i<MAX_DAYS; i++)
    {
        snprintf(pathname, sizeof(pathname), "Weekday%d", i);
        Common_Json_SetAttrValueObj(timeJson, pathname);
        for(j=0; j<MAX_TIMESEGMENT; j++)
        {
            snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d", i, j);
            Common_Json_SetAttrValueObj(timeJson, pathname);

            snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Start", i, j);
            Common_Json_SetAttrValueInt(timeJson, pathname, 0);

            snprintf(pathname, sizeof(pathname), "Weekday%d.Sched%d.Stop", i, j);
            Common_Json_SetAttrValueInt(timeJson, pathname, 0);
        }
    }

    return 0;
}

int RestMedia_Init(MQ_HANDLE_H mqHandle)
{
    int ret = 0;
    if (mqHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -EINVAL;
    }

    Common_cJSON_T* param = Common_cJSON_CreateObject();
    Common_cJSON_AddStringToObject(param, "SystemName", "ovfs");
    Common_cJSON_AddStringToObject(param, "ModuleName", "MediaServer");

    if (s_media_rest_ct.accessHandle)
    {
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
    }

    if (s_media_rest_ct.restTree)
    {
        Common_cJSON_Delete(s_media_rest_ct.restTree);
        s_media_rest_ct.restTree = NULL;
    }

    s_media_rest_ct.mqHandle = mqHandle;

    ret = Access_Init(&s_media_rest_ct.accessHandle, param, NULL, RestModuleCalleeFunctions, &s_media_rest_ct);
    Common_cJSON_Delete(param);
    if (ret != 0)
    {
        LOGE("module init fail!\n");
        return -1;
    }

    RecycleSessionId();
    RestReqCustomType();
    RestReqAudioState();

    ret = -1;
    Mq_Request(s_media_rest_ct.mqHandle, MEDIA_REQ_MEDIA_CFG_LOAD, NULL, 0, &ret, NULL,0);
    if (ret < 0)
    {
        LOGE("request load cfg failed\n");
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }

    ret = RestTreeGenerate(&s_media_rest_ct.restTree, s_restNodeTree, COMMON_ARRAY_ELEMENT_COUNT(s_restNodeTree));
    if (ret != 0)
    {
        LOGE("create rest tree failed\n");
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }

    ret = Access_RegisterSubscribe(s_media_rest_ct.accessHandle, "/MediaServer", RestModuleEventSubscribe, &s_media_rest_ct);
    if (ret != 0)
    {
        LOGE("resgister subscribe fail!\n");
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }

    ret = Access_SubscribeEvent(s_media_rest_ct.accessHandle, MEDIA_REST_SUB_ALARM_STATUS_ADDR,NULL,NULL, RestAccessAlarmReport, &s_media_rest_ct);
    if (ret < 0)
    {
        LOGE("subscribe alarm fail! %d\n",ret);
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }
    ret = Access_SubscribeEvent(s_media_rest_ct.accessHandle, MEDIA_REST_SUB_AUDIO_STATUS_ADDR,NULL,NULL, RestAccessAudioReport, &s_media_rest_ct);
    if (ret < 0)
    {
        LOGE("subscribe audio fail! %d\n",ret);
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }

    ret = Common_DList_Init(&s_media_rest_ct.authList,RestAuthNodeFree);
    if (ret < 0)
    {
        LOGE("auth list init fail! %d\n",ret);
        Access_Unint(&s_media_rest_ct.accessHandle);
        s_media_rest_ct.accessHandle = NULL;
        s_media_rest_ct.restTree = NULL;
        return -1;
    }
    return 0;
}

int RestMedia_Uninit()
{
    Access_Unint(&s_media_rest_ct.accessHandle);
    Common_cJSON_Delete(s_media_rest_ct.restTree);
    Common_DList_Uninit(&s_media_rest_ct.authList);
    if (s_media_rest_ct.customType)
        MEDIA_FREE(s_media_rest_ct.customType);
    s_media_rest_ct.customType = NULL;
    s_media_rest_ct.accessHandle = NULL;
    s_media_rest_ct.restTree = NULL;
    return 0;
}

int RestMedia_LoadCfg(Common_cJSON_T **cfg)
{
    cJSON_Struct *defaultCfg = NULL;

    if (s_media_rest_ct.accessHandle == NULL || cfg == NULL)
    {
        LOGE("moduel handle is null or cfg is null\n");
        return -1;
    }

    if(*cfg != NULL)
    {
        Common_cJSON_Delete(*cfg);
        *cfg = NULL;
    }

    Access_LoadConfigByType(s_media_rest_ct.accessHandle,Access_ConfigType_Default,&defaultCfg);

    Common_cJSON_T *mediaCfgDefault = Common_cJSON_Parse(MEDIA_CFG_DEFAULT, NULL, NULL);
    cJSON_Struct *NotDisturbTime = Common_Json_GetAttrValueObj(mediaCfgDefault, "MediaServer.RtmpPushNotDisturb.NotDisturbTime");
    InitTimeJson(NotDisturbTime);

    NotDisturbTime = Common_Json_GetAttrValueObj(mediaCfgDefault, "MediaServer.SmartProtocolNotDisturb.NotDisturbTime");
    InitTimeJson(NotDisturbTime);

    if (Access_LoadConfig(s_media_rest_ct.accessHandle, (cJSON_Struct **) cfg) < 0)
    {
        LOGW("load configure failed, use default configure\n");
        //*cfg = Common_cJSON_Parse(MEDIA_CFG_DEFAULT, NULL, NULL);
        JsonOper_MergeObj(mediaCfgDefault,defaultCfg,0);
        //Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) *cfg);
    }
    else if (CheckCfg(*cfg) < 0)
    {
        LOGW("check configure failed, use default configure\n");
        Common_cJSON_Delete(*cfg);
        //*cfg = Common_cJSON_Parse(MEDIA_CFG_DEFAULT, NULL, NULL);
        JsonOper_MergeObj(mediaCfgDefault,defaultCfg,0);
        //Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) *cfg);
    }
    else // LOAD success
    {
        //Common_cJSON_T *mediaCfgDefault = NULL;
        //mediaCfgDefault = Common_cJSON_Parse(MEDIA_CFG_DEFAULT, NULL, NULL);

        JsonOper_MergeObj(mediaCfgDefault,defaultCfg,0);

        JsonOper_MergeObj(mediaCfgDefault,*cfg,0);

        //Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) mediaCfgDefault);
        Common_cJSON_Delete(*cfg);
        //*cfg = mediaCfgDefault;
    }

    *cfg = mediaCfgDefault;
    Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) *cfg);

    if (defaultCfg)
    {
        Common_Json_Delete(defaultCfg);
    }

    if (*cfg == NULL)
    {
        LOGE("unknown reason\n");
        return -1;
    }


    return 0;
}

int RestMedia_RestoreCfg(Common_cJSON_T **cfg)
{
    cJSON_Struct *defaultCfg = NULL;

    if (s_media_rest_ct.accessHandle == NULL || cfg == NULL)
    {
        LOGE("moduel handle is null or cfg is null\n");
        return -1;
    }

    if (*cfg != NULL)
    {
        Common_cJSON_Delete(*cfg);
        *cfg = NULL;
    }

    Access_LoadConfigByType(s_media_rest_ct.accessHandle,Access_ConfigType_Default,&defaultCfg);

    *cfg = Common_cJSON_Parse(MEDIA_CFG_DEFAULT, NULL, NULL);
    JsonOper_MergeObj(*cfg,defaultCfg,0);

    if (defaultCfg)
    {
        Common_Json_Delete(defaultCfg);
    }

    Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) *cfg);
    return 0;
}

int RestMedia_SaveCfg(Common_cJSON_T *cfg)
{
    int ret = 0;

    if (s_media_rest_ct.accessHandle == NULL || cfg == NULL)
    {
        LOGE("access handle is null\n");
        return -1;
    }

    if (CheckCfg(cfg) == 0)
    {
        ret = Access_SaveConfig(s_media_rest_ct.accessHandle, (cJSON_Struct *) cfg);
    }

    return ret;
}

int RestMedia_RegistAuthNode(MEDIA_AUTH_INFO_T *authNode)
{
    if (authNode == NULL || (authNode->authMethod != MEDIA_AUTH_TYPE_TEXT && authNode->authMethod != MEDIA_AUTH_TYPE_AUTO && authNode->authMethod != MEDIA_AUTH_TYPE_DIGEST))
    {
        LOGE("%s\n",strerror(EINVAL));
        return -EINVAL;
    }

    int ret = 0;
    MEDIA_AUTH_NODE_T *node = NULL;
    cJSON_Struct *outJson = NULL, *dataJson = NULL, *tp = NULL;
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    if (reqJson == NULL)
    {
        LOGE("new json failed\n");
        return -1;
    }

    Common_Json_SetAttrValue(reqJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/IsRemote", Common_Json_Type_Number, NULL, 1, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/ClientInfo", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/ClientInfo/IPv4", Common_Json_Type_String, authNode->ipStr, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Method", Common_Json_Type_String,"post", 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Uri", Common_Json_Type_String, MEDIA_REST_REQ_LOGIN, 0, 0);
    Common_Json_SetAttrValue(reqJson, -1, "Data", Common_Json_Type_Object, NULL, 0, 0);
    tp = Common_Json_SetAttrValue(reqJson, -1, "Data/ResList", Common_Json_Type_Array, NULL, 0, 0);
    Common_Json_SetAttrValue(tp, 0, "AuthMethod", Common_Json_Type_Number, NULL, authNode->authMethod, 0);
    Common_Json_SetAttrValue(tp, 0, "UserName", Common_Json_Type_String, authNode->userName, 0, 0);

    switch (authNode->authMethod)
    {
        case MEDIA_AUTH_TYPE_AUTO: //自动 ， 按照digest处理
        case MEDIA_AUTH_TYPE_DIGEST: //diest
        {
            Common_Json_SetAttrValue(tp, 0, "Realm", Common_Json_Type_String, authNode->digest.realm, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Qop", Common_Json_Type_String, authNode->digest.qop, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Nonce", Common_Json_Type_String, authNode->digest.nonce, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Cnonce", Common_Json_Type_String, authNode->digest.cnonce, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Uri", Common_Json_Type_String, authNode->digest.uri, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Response", Common_Json_Type_String, authNode->digest.response, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Opaque", Common_Json_Type_String, authNode->digest.opaque, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Nc", Common_Json_Type_String, authNode->digest.nc, 0, 0);
            Common_Json_SetAttrValue(tp, 0, "Method", Common_Json_Type_String, authNode->digest.method, 0, 0);
            break;
        }
        case MEDIA_AUTH_TYPE_TEXT: // 明文
        {
            Common_Json_SetAttrValue(tp, 0, "Password", Common_Json_Type_String, authNode->password, 0, 0);
            break;
        }
    }

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle,reqJson,&outJson,1000);
    if (ret < 0)
    {
        LOGE("request failed %d\n",ret);
    }

    if (ret == 0 && outJson == NULL)
    {
        LOGE("no response \n");
        ret = -1;
    }

    if (ret == 0)
    {
        dataJson = Common_Json_GetAttrValue(outJson,-1,"Data/ResList/",NULL,NULL,NULL,NULL);
        tp = Common_Json_GetItem(dataJson, 0, NULL);
        if (dataJson == NULL || tp == NULL)
        {
            int ec = 0;
            char *es = NULL;
            ret = -1;

            Common_Json_GetAttrValue(outJson,-1,"Header/Code",NULL,NULL,&ec,NULL);
            Common_Json_GetAttrValue(outJson,-1,"Header/Describe",NULL,&es,NULL,NULL);

            LOGE("authorized failed \n");

            if (ec != 0 && es != NULL)
                LOGE("authorized failed detail info %d %s\n",ec,es);
        }
        else
        {
            node = MEDIA_MALLOC(sizeof(MEDIA_AUTH_NODE_T));
            node->auth = NULL;
            node->loginHandle = -1;
            node->sessionId = authNode->sessionId;
            Common_Json_GetAttrValue(tp, -1, "SessionId", NULL, NULL, &node->loginHandle, NULL);
            ret = 0;
        }
    }

    if (ret == 0)
    {
        node->auth = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (node->auth)
        {
            Common_Json_SetAttrValue(node->auth, -1, "Method", Common_Json_Type_Number, NULL, MEDIA_AUTH_TYPE_SESSIONID, 0);
            Common_Json_SetAttrValue(node->auth, -1, "UserName", Common_Json_Type_String, authNode->userName, 0, 0);
            Common_Json_SetAttrValue(node->auth, -1, "SessionId", Common_Json_Type_Number, NULL,node->loginHandle, 0);
        }

        Common_DList_InsertTail(s_media_rest_ct.authList,node,sizeof(MEDIA_AUTH_NODE_T));
        ret = node->loginHandle;
        AddSessionId(node->loginHandle);
        LOGW("insert node %d %d\n",node->sessionId,node->loginHandle);
    }

    if (reqJson)
        Common_Json_Delete(reqJson);

    if (outJson)
        Common_Json_Delete(outJson);

    if (ret > 0)
        LOGI("register node rtspSessionId %d authSessionId %d\n",node->sessionId,node->loginHandle);
    return ret;
}

int RestMedia_UnregistAuthNode(int sessionId)
{
    int ret = 0;
    char uri[128] = { 0 };
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    cJSON_Struct *outJson = NULL;

    if (reqJson == NULL)
    {
        LOGE("new json failed\n");
        return -1;
    }

    MEDIA_AUTH_NODE_T *node = NULL;
    node = Common_DList_Search(s_media_rest_ct.authList,&sessionId,RestAuthNodeCompare);
    if (node == NULL)
    {
        LOGD("no such sessionId %d\n",sessionId);
        Common_Json_Delete(reqJson);
        return -1;
    }

    snprintf(uri,sizeof(uri) - 1,MEDIA_REST_REQ_LOGOUT,node->loginHandle);
    Common_Json_SetAttrValue(reqJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Method", Common_Json_Type_String,"delete", 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Uri", Common_Json_Type_String, uri, 0, 0);

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle,reqJson,&outJson,3000);

    if (ret < 0)
        LOGE("remove authorized id failed\n");

    if (ret == 0 && outJson)
    {
        Common_Json_GetAttrValue(outJson,-1, "Header/Code", NULL, NULL, &ret, NULL);
    }

    if (ret == 0)
    {
        RemoveSessionId(node->loginHandle);
    }

    if (outJson)
        Common_Json_Delete(outJson);

    if (reqJson)
        Common_Json_Delete(reqJson);

    LOGW("remove node %d %d\n",node->sessionId,node->loginHandle);
    ret = Common_DList_Delete(s_media_rest_ct.authList,&sessionId,RestAuthNodeCompare);

    return ret;
}

void *RestMedia_SearchAuthNode(int sessionId)
{
    MEDIA_AUTH_NODE_T *node = NULL;
    node = Common_DList_Search(s_media_rest_ct.authList,&sessionId,RestAuthNodeCompare);
    return node;
}

int RestMedia_RequestVencType(MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };
    Common_cJSON_T* inParam = NULL;
    Common_cJSON_T* outParam = NULL;
    Common_cJSON_T* header = NULL;
    Common_cJSON_T *addressJson = NULL;
    Common_cJSON_T *auth = NULL;

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    inParam = Common_cJSON_CreateObject();
    header = Common_cJSON_CreateObject();

    if (inParam == NULL || header == NULL)
    {
        if (inParam)
            Common_cJSON_Delete(inParam);

        if (outParam)
            Common_cJSON_Delete(outParam);

        LOGE("create json failed\n");
        return -1;
    }

    snprintf(uri, MEDIA_URI_MAX_LEN - 1,MEDIA_REST_REQ_VIDEO_ENC_TYPE_ADDR,reqInfo->dev,reqInfo->chan,reqInfo->streamid);
    LOGW("request enc type uri %s\n",uri);
    Common_cJSON_AddStringToObject(header, "Method", "get");
    Common_cJSON_AddStringToObject(header, "Uri", uri);
    Common_cJSON_AddNumberToObject(header, "IsRemote", 1);
    auth = Common_cJSON_Duplicate(reqInfo->auth,1);

    Common_cJSON_AddItemToObject(header,"Auth",auth);

    Common_cJSON_AddItemToObject(inParam, "Header", header);

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) inParam, (cJSON_Struct**) (&outParam), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outParam == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        addressJson = JsonOper_GetObjectItemByPath(outParam, "Data.EncodeFormat");
        if (addressJson == NULL)
        {
            ret = -1;
            LOGE("not find response data\n");
        }
        else if (addressJson->valueint != MEDIA_STREAM_VIDEO_ENC_TYPE_H264
                 && addressJson->valueint != MEDIA_STREAM_VIDEO_ENC_TYPE_H265
                 && addressJson->valueint != MEDIA_STREAM_VIDEO_ENC_TYPE_JPEG
                 && addressJson->valueint != MEDIA_STREAM_VIDEO_ENC_TYPE_H264_PLUS
                 && addressJson->valueint != MEDIA_STREAM_VIDEO_ENC_TYPE_H265_PLUS)
        {
            ret = -1;
            LOGE("encoder format error\n");
        }
        else
        {
            LOGI("get stream encoder %d\n", addressJson->valueint);
            ret = addressJson->valueint;
        }
    }

    if (inParam)
    {
        Common_cJSON_Delete(inParam);
    }

    if (outParam)
    {
        Common_cJSON_Delete(outParam);
    }
    return ret;
}

int RestMedia_RequestAudioType(MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = 0;
    Common_cJSON_T* inParam = NULL;
    Common_cJSON_T* outParam = NULL;
    Common_cJSON_T* header = NULL;
    Common_cJSON_T *addressJson = NULL;
    Common_cJSON_T *auth = NULL;

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    inParam = Common_cJSON_CreateObject();
    header = Common_cJSON_CreateObject();

    if (inParam == NULL || header == NULL)
    {
        if (inParam)
            Common_cJSON_Delete(inParam);

        if (outParam)
            Common_cJSON_Delete(outParam);

        LOGE("create json failed\n");
        return -1;
    }

    Common_cJSON_AddStringToObject(header, "Method", "get");
    Common_cJSON_AddStringToObject(header, "Uri", MEDIA_REST_REQ_AUDIO_ATTR_ADDR);
    Common_cJSON_AddNumberToObject(header, "IsRemote", 1);
    auth = Common_cJSON_Duplicate(reqInfo->auth,1);

    Common_cJSON_AddItemToObject(header,"Auth",auth);

    Common_cJSON_AddItemToObject(inParam, "Header", header);

    DumpJson(inParam);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) inParam, (cJSON_Struct**) (&outParam), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outParam == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        addressJson = JsonOper_GetObjectItemByPath(outParam, "Data.EncFormat");
        if (addressJson == NULL)
        {
            ret = -1;
            LOGE("not find response data\n");
        }
        else if (addressJson->valueint != 1
                 && addressJson->valueint != 2
                 && addressJson->valueint != 5)
        {
            ret = -1;
            LOGE("encoder format error\n");
        }
        else
        {
            LOGI("get stream encoder %d\n", addressJson->valueint);

            if(addressJson->valueint == 1)
            {
                ret = ANTS_RTSPSERVER_PAYLOADTYPE_G711A;
            }
            else if(addressJson->valueint == 2)
            {
                ret = ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
            }
            else if(addressJson->valueint == 5)
            {
                ret = ANTS_RTSPSERVER_PAYLOADTYPE_AAC;
            }

        }
    }

    if (inParam)
    {
        Common_cJSON_Delete(inParam);
    }

    if (outParam)
    {
        Common_cJSON_Delete(outParam);
    }
    LOGI("get audio enable %d\n",ret);
    return ret;
}

int RestMedia_RequestStreamOpen(MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = -1, i = 0;
    char *uri = NULL;
    char *addr0 = NULL, *addr1 = NULL, *addr2 = NULL;

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("moduel handle is null or reqInfo is null\n");
        return -1;
    }

    int chanOff = reqInfo->chan * MEDIA_STREAM_QUEUE_ID_MAX;
    CoOpen_Param_T *param = MEDIA_MALLOC(sizeof(CoOpen_Param_T) * MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX);
    memset(param,0,sizeof(CoOpen_Param_T) * MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX);
    for (i = 0; i < MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX; i++)
    {
        param[i].nIndex = -1;
    }

    uri = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);
    addr0 = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);
    addr1 = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);
    addr2 = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);

    if (param == NULL || uri == NULL || addr0 == NULL || addr1 == NULL || addr2 == NULL)
    {
        LOGE("calloc failed\n");
        if (uri)
            MEDIA_FREE(uri);

        if (addr0)
            MEDIA_FREE(addr0);

        if (addr1)
            MEDIA_FREE(addr1);

        if (addr2)
            MEDIA_FREE(addr2);
        if (param)
            MEDIA_FREE(param);
        return -1;
    }


    memset(uri,0,MEDIA_URI_MAX_LEN);
    memset(addr0,0,MEDIA_URI_MAX_LEN);
    memset(addr1,0,MEDIA_URI_MAX_LEN);
    memset(addr2,0,MEDIA_URI_MAX_LEN);

    /*real stream*/
    if (reqInfo->type == MEDIA_STREAM_TYPE_REAL)
    {
        LOGI("%d %d %d\n", reqInfo->dev, reqInfo->chan, reqInfo->streamid);
        snprintf(uri, MEDIA_URI_MAX_LEN - 1, MEDIA_REST_REQ_REAL_VIDEO_ADDR, reqInfo->dev, reqInfo->chan, reqInfo->streamid);
        ret = RestReqStreamAddress(reqInfo->auth, uri, addr0, MEDIA_URI_MAX_LEN - 1);
        if (ret == 0)
        {
            LOGI("recv video address %s audio address %s chan num %d\n", addr0, MEDIA_REST_REQ_REAL_AUDIO_ADDR, reqInfo->chan);
            param[chanOff+reqInfo->streamid].nIndex = chanOff+reqInfo->streamid;
            param[chanOff+reqInfo->streamid].szUri = addr0;
            param[chanOff+reqInfo->streamid].nMode = 0;

            snprintf(addr1,MEDIA_URI_MAX_LEN-1,"%s",MEDIA_REST_REQ_REAL_AUDIO_ADDR);
            param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].nIndex = chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].szUri = addr1;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].nMode = 0;

//                LOGI("recv smart address %s idx %d \n", addr2, MEDIA_STREAM_QUEUE_ID_MAX);
            snprintf(addr2,MEDIA_URI_MAX_LEN-1,"%s",MEDIA_REST_REQ_SMART_ADDR);
            param[chanOff+MEDIA_STREAM_QUEUE_ID_SMART].nIndex = chanOff+MEDIA_STREAM_QUEUE_ID_SMART;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_SMART].szUri = addr2;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_SMART].nMode = 0;
        }
        else
        {
            LOGE("request video address failed %d\n", ret);
        }
    }
    /*talking stream*/
    else if (reqInfo->type == MEDIA_STREAM_TYPE_TALKING)
    {
        ret = 0;
        snprintf(addr0,MEDIA_URI_MAX_LEN-1,"%s",MEDIA_REST_REQ_TALK_ENC_AUDIO_ADDR);
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].nIndex = chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL;
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].szUri = addr0;
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_REAL].nMode = 0;
        snprintf(addr1,MEDIA_URI_MAX_LEN-1,"%s",MEDIA_REST_REQ_TALK_DEC_AUDIO_ADDR);
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_TALK].nIndex = chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_TALK;
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_TALK].szUri = addr1;
        param[chanOff+MEDIA_STREAM_QUEUE_ID_AUDIO_TALK].nMode = 1;
        if (addr2 != NULL)
        {
            MEDIA_FREE(addr2);
            addr2 = NULL;
        }
    }
    else if (reqInfo->type == MEDIA_STREAM_TYPE_RECORD)
    {
        snprintf(uri, MEDIA_URI_MAX_LEN - 1, MEDIA_REST_REQ_RECORD_ADDR);
        ret = RestReqStreamAddress(reqInfo->auth, uri, addr0, MEDIA_URI_MAX_LEN - 1);
        if (ret == 0)
        {
            param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].nIndex = chanOff+MEDIA_STREAM_QUEUE_ID_RECORD;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].szUri = addr0;
            param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].nMode = 0;

            param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            char ptime[16] = { 0 };
            Common_Json_SetAttrValue(param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams, -1, "Mode", Common_Json_Type_Number, NULL, 0, 0);
            cJSON_Struct *pNode = Common_Json_SetAttrValue(param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams, -1, "ReplayCh", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pNode, 0, "Device", Common_Json_Type_Number, NULL, reqInfo->dev, 0);
            Common_Json_SetAttrValue(pNode, 0, "Channel", Common_Json_Type_Number, NULL, reqInfo->chan, 0);
            Common_Json_SetAttrValue(param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams, -1, "StreamType", Common_Json_Type_Number, NULL, 0, 0);
            snprintf(ptime,sizeof(ptime) - 1,"%04u%02u%02u%02u%02u%02u",
                            reqInfo->recordPlayStartYear,reqInfo->recordPlayStartMon,reqInfo->recordPlayStartDay,
                            reqInfo->recordPlayStartHour,reqInfo->recordPlayStartMinute,reqInfo->recordPlayStartSecond);

            Common_Json_SetAttrValue(param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams, -1, "StartTime", Common_Json_Type_String, ptime, 0, 0);

            snprintf(ptime,sizeof(ptime) - 1,"%04u%02u%02u%02u%02u%02u",
                            reqInfo->recordPlayStopYear,reqInfo->recordPlayStopMon,reqInfo->recordPlayStopDay,
                            reqInfo->recordPlayStopHour,reqInfo->recordPlayStopMinute,reqInfo->recordPlayStopSecond);
            Common_Json_SetAttrValue(param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams, -1, "EndTime", Common_Json_Type_String, ptime, 0, 0);
            if (addr1 != NULL)
            {
                MEDIA_FREE(addr1);
                addr1 = NULL;
            }

            if (addr2 != NULL)
            {
                MEDIA_FREE(addr2);
                addr2 = NULL;
            }
        }
        else
        {
            LOGE("request record address failed %d\n", ret);
        }
    }

    if (ret == 0)
    {
        if (reqInfo->type == MEDIA_STREAM_TYPE_RECORD)
            ret = Access_StreamQueue_CoOpen(s_media_rest_ct.accessHandle, -1, param, MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX, 1000);
        else
            ret = Access_StreamQueue_CoOpen(s_media_rest_ct.accessHandle, -1, param, MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX, 1000);

        LOGW("open stream handle %d\n",ret);
        if (ret > 0)
        {
            if (ret > 0 && reqInfo->type == MEDIA_STREAM_TYPE_RECORD)
            {
                cJSON_Struct *pControl = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                if(pControl != NULL)
                {
                    Common_Json_SetAttrValue(pControl, -1, "operate", Common_Json_Type_String, "start", 0, 0);
                }
                S32 iRET = Access_StreamQueue_Control(s_media_rest_ct.accessHandle, ret, chanOff+MEDIA_STREAM_QUEUE_ID_RECORD, pControl, NULL,0);
                LOGW("********************** iRet = %d*********************\n", iRET);
                Common_Json_Delete(pControl);
            }
        }
        else
        {
            LOGE("stream queue open failed %d\n", ret);
        }
    }

    if (uri)
        MEDIA_FREE(uri);

    if (ret < 0 && addr0)
        MEDIA_FREE(addr0);

    if (ret < 0 && addr1)
        MEDIA_FREE(addr1);

    if (ret < 0 && addr2)
        MEDIA_FREE(addr2);

    if (ret < 0 && param)
    {
        if (reqInfo->type == MEDIA_STREAM_TYPE_RECORD)
    	{
			if(reqInfo->param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams != NULL)
	        {
	        	Common_Json_Delete(reqInfo->param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams);
	        	reqInfo->param[chanOff+MEDIA_STREAM_QUEUE_ID_RECORD].pOpenParams = NULL;
	        }
    	}
        MEDIA_FREE(param);
        param = NULL;
    }
    reqInfo->param = param;
    return ret;
}
int RestMedia_RequestStreamAdd(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo,int channel,int streamId)
{
    int ret = -1;
    char *uri = NULL, *addr = NULL;
    int chanOff = channel * MEDIA_STREAM_QUEUE_ID_MAX;
    if (streamId < MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN || streamId > MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD)
    {
        LOGE("stream id out of range %d\n",chanOff + streamId);
        return -1;
    }

    if (reqInfo->param[chanOff + streamId].szUri != NULL)
    {
        LOGW("already open channel %d stream %d \n",channel , streamId);
        return 0;
    }

    uri = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);
    addr = MEDIA_MALLOC(MEDIA_URI_MAX_LEN);
    LOGW("ADD CH %d STREAM %d\n",channel,streamId);
    switch(streamId)
    {
        case MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN:
        case MEDIA_STREAM_QUEUE_ID_VIDEO_SUB:
        case MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD:
        {
            snprintf(uri, MEDIA_URI_MAX_LEN - 1, MEDIA_REST_REQ_REAL_VIDEO_ADDR,reqInfo->dev,channel,streamId);
            LOGW("URI %s\n",uri);
            ret = RestReqStreamAddress(reqInfo->auth, uri, addr, MEDIA_URI_MAX_LEN - 1);
            if (ret == 0)
            {
                reqInfo->param[chanOff + streamId].szUri = addr;
                reqInfo->param[chanOff + streamId].nIndex = chanOff + streamId;
                reqInfo->param[chanOff + streamId].nMode = 0;
                LOGW("coopen  streamid %d streamQueueId %d\n",streamId,*streamQueueId);
//                ret = Access_StreamQueue_CoOpen(s_media_rest_ct.accessHandle, *streamQueueId, reqInfo->param, MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX, 1000);
                LOGW("coopen DONE RET %d  channel %d streamid %d streamQueueId %d\n",ret,channel, streamId,*streamQueueId);
            }
            break;
        }
        default:
            break;
    }
    if (ret < 0 && addr)
        MEDIA_FREE(addr);
    if (uri)
        MEDIA_FREE(uri);
    return ret;
}

int RestMedia_RequestStreamAddOrDecHandle(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    return Access_StreamQueue_CoOpen(s_media_rest_ct.accessHandle, *streamQueueId, reqInfo->param, MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX, 1000);
}

int RestMedia_RequestStreamDec(int *streamQueueId,MEDIA_REQ_STREAM_INFO_T *reqInfo,int channel,int streamId)
{
    int ret = 0;

    int chanOff = channel * MEDIA_STREAM_QUEUE_ID_MAX;
    if (streamId < MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN || streamId > MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD)
    {
        LOGE("stream id out of range %d\n",streamId);
        return -1;
    }
    if (reqInfo->param[chanOff + streamId].szUri == NULL)
    {
        LOGE("this channel %d stream %d was not open\n",channel, streamId);
        return -1;
    }
    LOGW("DEC CH %d STREAM %d\n",channel,streamId);
    switch(streamId)
    {
        case MEDIA_STREAM_QUEUE_ID_VIDEO_MAIN:
        case MEDIA_STREAM_QUEUE_ID_VIDEO_SUB:
        case MEDIA_STREAM_QUEUE_ID_VIDEO_THIRD:
        {
            MEDIA_FREE(reqInfo->param[chanOff + streamId].szUri);
            reqInfo->param[chanOff + streamId].szUri = NULL;
//            ret = Access_StreamQueue_CoOpen(s_media_rest_ct.accessHandle, *streamQueueId, reqInfo->param, MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX, 1000);
            break;
        }
        default:
            break;
    }

    return ret;

}

int RestMedia_RequestRecordControl(int streamHandle, int cmd, void *cmdData)
{
    int ret = 0;
    cJSON_Struct *pControl = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    if(pControl == NULL)
    {
        LOGE("new json failed\n");
        return -1;
    }

    switch(cmd)
    {
        case MEDIA_STREAM_CONTROL_PLAY:
            Common_Json_SetAttrValue(pControl, -1, "operate", Common_Json_Type_String, "start", 0, 0);
            break;
        case MEDIA_STREAM_CONTROL_STOP:
            Common_Json_SetAttrValue(pControl, -1, "operate", Common_Json_Type_String, "stop", 0, 0);
            break;
        case MEDIA_STREAM_CONTROL_FORCE_IFRAME:
        {
            Common_Json_SetAttrValue(pControl, -1, "operate", Common_Json_Type_String, "forcekey", 0, 0);
            Common_Json_SetAttrValue(pControl, -1, "param", Common_Json_Type_Number, NULL, *(int *)cmdData, 0);
            LOGW("FORCE I FRAME %d\n",*(int *)cmdData);
            break;
        }
        default:
            LOGE("unknown control type %d\n",cmd);
            Common_Json_Delete(pControl);
            return -1;
    }
    ret = Access_StreamQueue_Control(s_media_rest_ct.accessHandle, streamHandle, MEDIA_STREAM_QUEUE_ID_RECORD, pControl, NULL, 0);
    LOGW("control ret %d %d %d\n",ret,cmd,streamHandle);
    Common_Json_Delete(pControl);
    return ret;
}

int RestMedia_RequestStreamClose(int streamHandle,MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int i = 0;
    LOGI("close streamHandle %d\n",streamHandle);
    if (reqInfo->param)
    {
        for (i = 0;i < MEDIA_STREAM_QUEUE_ID_MAX * MEDIA_CHANNEL_ID_MAX;i++)
        {
            if (reqInfo->param[i].szUri)
            {
                LOGW("close uri %s\n",reqInfo->param[i].szUri);
                MEDIA_FREE(reqInfo->param[i].szUri);
                reqInfo->param[i].szUri = NULL;
            }
			if(reqInfo->param[i].pOpenParams != NULL)
	        {
	        	Common_Json_Delete(reqInfo->param[i].pOpenParams);
	        	reqInfo->param[i].pOpenParams = NULL;
	        }
			if(reqInfo->param[i].pOutStreamInfo != NULL)
	        {
	        	Common_Json_Delete(reqInfo->param[i].pOutStreamInfo);
	        	reqInfo->param[i].pOutStreamInfo = NULL;
	        }
        }
        MEDIA_FREE(reqInfo->param);
        reqInfo->param = NULL;
    }
    return Access_StreamQueue_Close(s_media_rest_ct.accessHandle,streamHandle);
}

int RestMedia_RequestStreamRead(int streamHandle, int index, int *rindex, void **data, int *dataSize, int *restCnt, int timeout)
{
    cJSON_Struct *restCntJson = NULL;
    int ret = 0;
    ret = Access_StreamQueue_ReadData(s_media_rest_ct.accessHandle, streamHandle, index, rindex, &restCntJson, data, dataSize,
                    timeout);
    if (ret == 0 && restCntJson != NULL && restCnt != NULL)
    {
        Common_Json_GetAttrValue(restCntJson, -1, "RestCnt", NULL, NULL, restCnt, 0);
//        LOGW("restCnt %d\n",*restCnt);
    }

    if (restCntJson)
        Common_Json_Delete(restCntJson);
    return ret;
}

int RestMedia_RequestStreamRelease(int streamHandle)
{
    return Access_StreamQueue_ReleaseData(s_media_rest_ct.accessHandle,streamHandle);
}

int RestMedia_RequestStreamWrite(int streamHandle,int index, void *data, int size)
{
    /*only for audio data writing , boardsys will read and decoder it*/
    return Access_StreamQueue_WriteData(s_media_rest_ct.accessHandle, streamHandle, index, NULL, NULL, 0, data, size);
}

int RestMedia_WriteEventLog(int majorType, int minorType, int channel, char *ipAddr)
{
    cJSON_Struct * pConfig = NULL, *pChild = NULL, *pArray = NULL, *pOutParams = NULL;
    pConfig = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if (pConfig)
    {
        Common_Json_SetAttrValue(pConfig,-1,"Header",Common_Json_Type_Object,NULL,0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Uri",Common_Json_Type_String,"/EventLog/LogFunction",0,0);
        Common_Json_SetAttrValue(pConfig,-1,"Header/Method",Common_Json_Type_String,"put",0,0);
        pChild = Common_Json_SetAttrValue(pConfig,-1,"Data",Common_Json_Type_Object,NULL,0,0);
        pArray = Common_Json_SetAttrValue(pChild,-1,"ResList",Common_Json_Type_Array,NULL,0,0);

        Common_Json_SetAttrValue(pArray,0,"MajorType",Common_Json_Type_Number,NULL,majorType,0);
        Common_Json_SetAttrValue(pArray,0,"MinorType",Common_Json_Type_Number,NULL,minorType,0);
        Common_Json_SetAttrValue(pArray,0,"Channel",Common_Json_Type_Number,NULL,channel,0);
        Common_Json_SetAttrValue(pArray,0,"RemoteHostAddress",Common_Json_Type_String,ipAddr,0,0);
        Access_CallFunctions(s_media_rest_ct.accessHandle,pConfig,&pOutParams,3000);
        Common_Json_Delete(pConfig);
        Common_Json_Delete(pOutParams);
        pConfig = NULL;
        pOutParams = NULL;
    }
    return 0;
}

int RestMedia_CheckPlayBackTime(Ants_RtspDayTime *startTime, Ants_RtspDayTime *stopTime)
{
    int ret = 0;
    time_t startT, stopT;
    int diffTime = 0;
    if (access("/tmp/TZ", F_OK) == 0)
    {
        FILE *fp = fopen("/tmp/TZ", "rb");
        if (fp != NULL)
        {
            char tmpStr[16] = {};
            fgets(tmpStr, sizeof(tmpStr), fp);
            LOGD("time zone %s\n", tmpStr);
            memset(tmpStr, 0 , sizeof(tmpStr));
            fgets(tmpStr, sizeof(tmpStr), fp);
            diffTime = atoi(tmpStr);
            LOGD("time diff %s %d\n", tmpStr, diffTime);
            fclose(fp);
        }
        else
        {
            ret = -1;
        }
    }
    else
    {
        ret = -1;
    }

    LOGD("hour %d %d\n", startTime->byHour, stopTime->byHour);
    ovfs_utility_ovfs_to_linux_time(startTime, &startT);
    ovfs_utility_ovfs_to_linux_time(stopTime, &stopT);

    startT += diffTime;
    stopT += diffTime;
    ovfs_utility_linux_to_ovfs_time(startT, startTime);
    ovfs_utility_linux_to_ovfs_time(stopT, stopTime);
    LOGD("hour %d %d\n", startTime->byHour, stopTime->byHour);

    return ret;
}

int RestMedia_RequestNaluType(MEDIA_REQ_STREAM_INFO_T *reqInfo, void **sps, int *spsSize, void **pps, int *ppsSize)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    cJSON_Struct *outJson = NULL, *authJson = NULL;
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    snprintf(uri, MEDIA_URI_MAX_LEN - 1,MEDIA_REST_REQ_REAL_VIDEO_NALU,reqInfo->dev,reqInfo->chan,reqInfo->streamid);
    Common_Json_SetAttrValue(reqJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Method", Common_Json_Type_String,"get", 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Uri", Common_Json_Type_String, uri, 0, 0);
    authJson = Common_Json_Duplicate(reqInfo->auth,1);
    Common_Json_AddItem(reqJson,-1,"Header/Auth",authJson);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) reqJson, (cJSON_Struct**) (&outJson), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outJson == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        char *spsStr64 = NULL, *ppsStr64 = NULL;
        int spsSize64 = 0, ppsSize64 = 0;
        Common_Json_GetAttrValue(outJson, -1, "Data/SpsStr", NULL, &spsStr64, NULL, NULL);
        Common_Json_GetAttrValue(outJson, -1, "Data/PpsStr", NULL, &ppsStr64, NULL, NULL);
        Common_Json_GetAttrValue(outJson, -1, "Data/SpsSize", NULL, NULL, &spsSize64, NULL);
        Common_Json_GetAttrValue(outJson, -1, "Data/PpsSize", NULL, NULL, &ppsSize64, NULL);
        LOGD("sps %s pps %s spslen %d ppslen %d\n",spsStr64,ppsStr64,spsSize64,ppsSize64);
        if(spsStr64)
        {
            *sps = Common_Base64_Decode(spsStr64,(unsigned int)spsSize64,(unsigned int *)spsSize);
        }
        if(ppsStr64)
        {
            *pps = Common_Base64_Decode(ppsStr64,(unsigned int)ppsSize64,(unsigned int *)ppsSize);
        }

        LOGD("sps[%s] sizeof[%d] len[%d] pps[%s] sizeof[%d] len[%d]\n",
            *sps,sizeof(*sps),*spsSize,*pps,sizeof(*pps),*ppsSize);
    }

    if (reqJson)
    {
        Common_Json_Delete(reqJson);
    }

    if (outJson)
    {
        Common_Json_Delete(outJson);
    }
    return ret;
}


int RestMedia_RequestVPSType(MEDIA_REQ_STREAM_INFO_T *reqInfo, void **vps, int *vpsSize)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    cJSON_Struct *outJson = NULL, *authJson = NULL;
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    snprintf(uri, MEDIA_URI_MAX_LEN - 1,MEDIA_REST_REQ_REAL_VIDEO_NALU,reqInfo->dev,reqInfo->chan,reqInfo->streamid);
    Common_Json_SetAttrValue(reqJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Method", Common_Json_Type_String,"get", 0, 0);
    Common_Json_SetAttrValue(reqJson,-1, "Header/Uri", Common_Json_Type_String, uri, 0, 0);
    authJson = Common_Json_Duplicate(reqInfo->auth,1);
    Common_Json_AddItem(reqJson,-1,"Header/Auth",authJson);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) reqJson, (cJSON_Struct**) (&outJson), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outJson == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        char *vpsStr64 = NULL;
        int vpsSize64 = 0;
        Common_Json_GetAttrValue(outJson, -1, "Data/VpsStr", NULL, &vpsStr64, NULL, NULL);
        Common_Json_GetAttrValue(outJson, -1, "Data/VpsSize", NULL, NULL, &vpsSize64, NULL);
        LOGD("vps %s vpslen %d\n",vpsStr64,vpsSize64);
        if(vpsStr64)
        {
            *vps = Common_Base64_Decode(vpsStr64,(unsigned int)vpsSize64,(unsigned int *)vpsSize);
        }
        LOGD("vps len %d\n",*vpsSize);
    }

    if (reqJson)
    {
        Common_Json_Delete(reqJson);
    }

    if (outJson)
    {
        Common_Json_Delete(outJson);
    }
    return ret;
}

char *RestMedia_GetCustomType()
{
    return s_media_rest_ct.customType;
}

int RestMedia_SendMediaDisconnectEvent()
{
    OVFS_MEDIA_REST_CONTEXT_T *ct = &s_media_rest_ct;;
    OVFS_MEDIA_REST_NODE_ATTR_T *node = NULL;
    int i = 0, c = 0, ret = -1, rsize = 0;

    Common_cJSON_T *out = Common_cJSON_CreateObject();

    if (Mq_Request(ct->mqHandle, MEDIA_REQ_DISCON_GET_EVENT, NULL, 0, &ret, out, sizeof(Common_cJSON_T *)) < 0 || ret < 0)
    {
        LOGE("request disconnect event failed\n");
        Common_cJSON_Delete(out);
        out = NULL;
        return EC_MEDIA_REST_OP_FAILED;
    }
    else
    {
        Common_cJSON_T *restNodeJson = RestTreeGetNodeByUri(ct->restTree, "/MediaServer/MediaDisconnect/Event");
        if (restNodeJson == NULL)
        {
            LOGE("no such uri /MediaServer/MediaDisconnect/Event\n");
            Common_cJSON_Delete(out);
            out = NULL;
            return EC_MEDIA_REST_NO_URI;
        }
        else
        {
            node = Common_cJSON_GetItemExtData(restNodeJson, &rsize);
        }

        if (node)
        {
            cJSON_Struct *eventJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(eventJson,-1, "Header", Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(eventJson,-1, "Header/Code", Common_Json_Type_Number, NULL, 0, 0);
            Common_Json_AddItem(eventJson,-1,"Data",(cJSON_Struct *)out);
            pthread_mutex_lock(&node->subscribeLock);
            c = COMMON_ARRAY_ELEMENT_COUNT(node->subscribeTable);
            for (i = 0;i<c;i++)
            {
                if (node->subscribeTable[i] > 0)
                {
                    int ret = Access_SendEvent(ct->accessHandle,node->subscribeTable[i],eventJson,NULL,1000);
                    LOGW("send event to %d RET %d\n",node->subscribeTable[i],ret);
                }
            }
            pthread_mutex_unlock(&node->subscribeLock);
            Common_Json_Delete(eventJson);
        }
        else
        {
            LOGE("could not find this extra node\n");
            Common_cJSON_Delete(out);
            out = NULL;
            return EC_MEDIA_REST_NO_URI;
        }
    }

    return 0;
}

int RestMedia_RequestVideoIFrame(MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };
    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -1;
    }

    cJSON_Struct *outJson = NULL, *authJson = NULL;
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    snprintf(uri, MEDIA_URI_MAX_LEN - 1, MEDIA_REST_REQ_VIDEO_IFRAME, reqInfo->dev, reqInfo->chan, reqInfo->streamid);
    Common_Json_SetAttrValue(reqJson, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson, -1, "Header/Method", Common_Json_Type_String, "Put", 0, 0);
    Common_Json_SetAttrValue(reqJson, -1, "Header/Uri", Common_Json_Type_String, uri, 0, 0);
    authJson = Common_Json_Duplicate(reqInfo->auth, 1);
    Common_Json_AddItem(reqJson, -1, "Header/Auth", authJson);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) reqJson, (cJSON_Struct**) (&outJson), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outJson == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        Common_Json_GetAttrValue(outJson,-1, "Header/Code", NULL, NULL, &ret, NULL);
        LOGD("Request %s ret %d\n",uri,ret);
    }

    if (reqJson)
    {
        Common_Json_Delete(reqJson);
    }

    if (outJson)
    {
        Common_Json_Delete(outJson);
    }
    return ret;
}
static int ParserBoardAbility(cJSON_Struct *json, BOARD_ABILITY_T *ability)
{
    int devNum = 0, chanNum = 0, streamNum = 0, i = 0, j = 0;
    DumpJson((Common_cJSON_T *)json);
    Common_Json_GetAttrValue(json,-1, "Data/DevTotalNum", NULL, NULL, &devNum, NULL);
    if (devNum <= 0)
    {
        LOGE("could not parser DevTotalNum\n");
        return -1;
    }
    ability->videoAbility.devNum = devNum;
    for (i = 0; i < devNum; i++)
    {
        char viStr[32] = { 0 };
        snprintf(viStr,sizeof(viStr),"Data/viDev%d/viChanNum",i);
        Common_Json_GetAttrValue(json,-1, viStr, NULL, NULL, &chanNum, NULL);
        if (chanNum <= 0)
        {
            LOGE("could not parser viDev\n");
            return -1;
        }
        ability->videoAbility.dev[i].chanNum = chanNum;
        for (j = 0; j < chanNum; j++)
        {
            char chanStr[32] = { 0 };
            snprintf(chanStr,sizeof(chanStr),"Data/viDev%d/viChan%d",i,j);
            Common_Json_GetAttrValue(json,-1, chanStr, NULL, NULL, &streamNum, NULL);
            if (streamNum <= 0)
            {
                LOGE("could not parser viDev/viChan\n");
                return -1;
            }
            ability->videoAbility.dev[i].chan[j].streamNum = streamNum;
        }
    }
    return 0;
}
int RestMedia_RequestBoardAbility(BOARD_ABILITY_T *ability, MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };
    Common_cJSON_T* inParam = NULL;
    Common_cJSON_T* outParam = NULL;
    Common_cJSON_T* header = NULL;
    Common_cJSON_T *auth = NULL;
    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }
    inParam = Common_cJSON_CreateObject();
    header = Common_cJSON_CreateObject();
    if (inParam == NULL || header == NULL)
    {
        if (inParam)
            Common_cJSON_Delete(inParam);
        if (outParam)
            Common_cJSON_Delete(outParam);
        LOGE("create json failed\n");
        return -1;
    }
    snprintf(uri, MEDIA_URI_MAX_LEN - 1,"%s",MEDIA_REST_REQ_BOARD_SYSTEM_ABILITY_ADDR);
    Common_cJSON_AddStringToObject(header, "Method", "get");
    Common_cJSON_AddStringToObject(header, "Uri", uri);
    Common_cJSON_AddNumberToObject(header, "IsRemote", 1);
    auth = Common_cJSON_Duplicate(reqInfo->auth,1);
    Common_cJSON_AddItemToObject(header,"Auth",auth);
    Common_cJSON_AddItemToObject(inParam, "Header", header);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) inParam, (cJSON_Struct**) (&outParam), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outParam == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        ret = ParserBoardAbility(outParam,ability);
    }
    if (inParam)
    {
        Common_cJSON_Delete(inParam);
    }
    if (outParam)
    {
        Common_cJSON_Delete(outParam);
    }
    return ret;
}

int RestMedia_RequestBoardCodecTempChange(int changeOrBack, MEDIA_REQ_STREAM_INFO_T *reqInfo)
{
    int ret = 0;
    char uri[MEDIA_URI_MAX_LEN] = { 0 };
    Common_cJSON_T* inParam = NULL, *outJson = NULL;
    Common_cJSON_T* header = NULL;
    Common_cJSON_T *auth = NULL;
    Common_cJSON_T *dataJson = NULL;

    if (s_media_rest_ct.accessHandle == NULL || reqInfo == NULL || reqInfo->auth == NULL)
    {
        LOGE("%s\n",strerror(EINVAL));
        return -1;
    }

    /* if (changeOrBack == 0) */
    /*     s_media_rest_ct.preferh264Cnt[reqInfo->dev][reqInfo->chan][reqInfo->streamid]--; */
    /* else */
    /*     s_media_rest_ct.preferh264Cnt[reqInfo->dev][reqInfo->chan][reqInfo->streamid]++; */

    /* if (changeOrBack == 1  && */
    /*     s_media_rest_ct.preferh264Cnt[reqInfo->dev][reqInfo->chan][reqInfo->streamid] > 1) */
    /*     return 0; */

    /* if (changeOrBack == 0  && */
    /*     s_media_rest_ct.preferh264Cnt[reqInfo->dev][reqInfo->chan][reqInfo->streamid] >= 1) */
    /*     return 0; */

    inParam = Common_cJSON_CreateObject();
    header = Common_cJSON_CreateObject();
    dataJson = Common_cJSON_CreateObject();
    if (inParam == NULL || header == NULL)
    {
        if (inParam)
            Common_cJSON_Delete(inParam);
        LOGE("create json failed\n");
        return -1;
    }

    snprintf(uri, MEDIA_URI_MAX_LEN - 1,
             "/Boardsys/Video/Attribute/Device%d/Channel%d/Stream%d/TmpParam?ChangeH264=%d",
             reqInfo->dev,reqInfo->chan,reqInfo->streamid,changeOrBack);
    LOGE("request tmp change %s\n",uri);
    Common_cJSON_AddStringToObject(header, "Method", "put");
    Common_cJSON_AddStringToObject(header, "Uri", uri);
    Common_cJSON_AddNumberToObject(header, "IsRemote", 1);
    auth = Common_cJSON_Duplicate(reqInfo->auth,1);
    Common_cJSON_AddItemToObject(header,"Auth",auth);
    Common_cJSON_AddItemToObject(inParam, "Header", header);
    Common_cJSON_AddItemToObject(inParam, "Data", dataJson);
    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) inParam, (cJSON_Struct*)&outJson, 10000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else
    {
        ret = 0;
    }


    if (inParam)
    {
        Common_cJSON_Delete(inParam);
    }

    if (outJson != NULL)
    {
        Common_cJSON_Delete(outJson);
    }

    return ret;
}

int RestMeida_RequestCoreVersion(HTTP_PUSH_CORE_VERSION_T *coreVersion)
{
    int ret = 0;

    if (s_media_rest_ct.accessHandle == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -1;
    }

    // LOGD("try to set alarmout[%d] %d \n",idx,enable);
    cJSON_Struct *outJson = NULL;
    cJSON_Struct *reqJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);

    Common_Json_SetAttrValue(reqJson, -1, "Header", Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValue(reqJson, -1, "Header/Method", Common_Json_Type_String, "Get", 0, 0);

    Common_Json_SetAttrValue(reqJson, -1, "Header/Uri", Common_Json_Type_String, "/Core/Version", 0, 0);

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct *) reqJson,
                               (cJSON_Struct **) (&outJson), 1000);
    if (ret != 0)
    {
        LOGE("module call fail! ret=%d\n", ret);
    }
    else if (outJson == NULL)
    {
        ret = -1;
        LOGE("no response data\n");
    }
    else
    {
        Common_Json_GetAttrValue(outJson, -1, "Header/Code", NULL, NULL, &ret, NULL);
    }

    if (reqJson)
    {
        Common_Json_Delete(reqJson);
    }

    cJSON_Struct *dataJson =  Common_Json_GetAttrValue(outJson, -1, "Data", NULL, NULL, &ret, NULL);
    // if(outJson)
    //     Common_Json_Delete(outJson);

    char *vstr = NULL;
    int vint = 0;

    Common_Json_GetAttrValue(dataJson, -1, "SerialNumber", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->SerialNumber, sizeof(coreVersion->SerialNumber), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "DeviceName", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->DeviceName, sizeof(coreVersion->DeviceName), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "ProductName", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->ProductName, sizeof(coreVersion->ProductName), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "DeviceType", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->DeviceType, sizeof(coreVersion->DeviceType), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "DeviceModel", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->DeviceModel, sizeof(coreVersion->DeviceModel), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Country", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Country, sizeof(coreVersion->Country), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "City", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->City, sizeof(coreVersion->City), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Web", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Web, sizeof(coreVersion->Web), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Tel", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Tel, sizeof(coreVersion->Tel), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Copyright", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Copyright, sizeof(coreVersion->Copyright), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Brand", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Brand, sizeof(coreVersion->Brand), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Customer", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Customer, sizeof(coreVersion->Customer), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "SensorModel", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->SensorModel, sizeof(coreVersion->SensorModel), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "LensDrvType", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->LensDrvType, sizeof(coreVersion->LensDrvType), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "LensType", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->LensType, sizeof(coreVersion->LensType), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "IrisType", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->IrisType, sizeof(coreVersion->IrisType), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Version", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Version, sizeof(coreVersion->Version), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "HardVersion", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->HardVersion, sizeof(coreVersion->HardVersion), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "BuildDate", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->BuildDate, sizeof(coreVersion->BuildDate), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "ProductDate", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->ProductDate, sizeof(coreVersion->ProductDate), "%s",
                 vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Hardware", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Hardware, sizeof(coreVersion->Hardware), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "UUID", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->UUID, sizeof(coreVersion->UUID), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "AuthMethod", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->AuthMethod, sizeof(coreVersion->AuthMethod), "%s", vstr);

    vstr = NULL;
    Common_Json_GetAttrValue(dataJson, -1, "Status", NULL, &vstr, NULL, NULL);
    if (vstr)
        snprintf(coreVersion->Status, sizeof(coreVersion->Status), "%s", vstr);

    Common_Json_GetAttrValue(dataJson, -1, "IsOfDome", NULL, NULL, &vint, NULL);
    coreVersion->IsOfDome = vint;

    Common_Json_GetAttrValue(dataJson, -1, "IsOfIr", NULL, NULL, &vint, NULL);
    coreVersion->IsOfIr = vint;

    Common_Json_GetAttrValue(dataJson, -1, "LensSupport", NULL, NULL, &vint, NULL);
    coreVersion->LensSupport = vint;

    Common_Json_GetAttrValue(dataJson, -1, "IrisSupport", NULL, NULL, &vint, NULL);
    coreVersion->IrisSupport = vint;

    Common_Json_GetAttrValue(dataJson, -1, "SvnNumber", NULL, NULL, &vint, NULL);
    coreVersion->SvnNumber = vint;

    if (outJson)
        Common_Json_Delete(outJson);

    return 0;
}

int RestMedia_Request(cJSON_Struct *pRequire, cJSON_Struct **pResponce)
{
    int ret = 0;

    if (s_media_rest_ct.accessHandle == NULL || pRequire == NULL)
    {
        LOGE("%s\n", strerror(EINVAL));
        return -1;
    }

    ret = Access_CallFunctions(s_media_rest_ct.accessHandle, (cJSON_Struct*) pRequire, (cJSON_Struct**) pResponce, 3000);

    return ret;
}
