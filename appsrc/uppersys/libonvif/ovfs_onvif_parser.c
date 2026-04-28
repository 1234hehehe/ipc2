#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/prctl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <limits.h>
#include <errno.h>
#include <arpa/inet.h>
#include <libcommon_api.h>
#include <math.h>
#include <fcntl.h>
#include <net/route.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include <unistd.h>

#include <netdb.h>
#include <net/ethernet.h>
#include <linux/if_arp.h>

#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <linux/if_ether.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <mxml.h>

#include "http_parser.h"
#include "ovfs_onvif.h"
#include "ovfs_onvif_parser.h"
#include "ovfs_onvif_http.h"
#include "ovfs_onvif_rest.h"
#include "ovfs_com_utils.h"

//时区_编号，后面实现的时候再去添加
typedef enum
{
    //后面添加的时区在最后面添加 已经废弃的宏 不要删除
    //枚举值不要随意修改 IE会按枚举值索引语言
    OVFS_TIME_ZONE_WEST_LINE = 0,                  //国际日期变更线西
    OVFS_TIME_ZONE_SAMOA = 1,                       //中途岛,萨摩亚群岛
    OVFS_TIME_ZONE_HAWAII = 2,                      //夏威夷
    OVFS_TIME_ZONE_ALASKA = 3,                      //阿拉斯加
    OVFS_TIME_ZONE_PACIFIC_OCEAN = 4,
    //太平洋时间(美国和加拿大)
    OVFS_TIME_ZONE_MOUNTAIN = 5,                    //山地时间(美国和加拿大)
    OVFS_TIME_ZONE_CENTRAL_CANADA = 6,              //中部时间(美国和加拿大)
    OVFS_TIME_ZONE_EASTERN_TIME_CANADA = 7,           //东部时间(美国和加拿大)
    OVFS_TIME_ZONE_CARACAS = 8,                       //加拉加斯
    OVFS_TIME_ZONE_ATLANTIC_CANADA = 9,               //大西洋时间(加拿大)
    OVFS_TIME_ZONE_NEWFOUNDLAND = 10,                 //纽芬兰
    OVFS_TIME_ZONE_GEORGETOWN = 11,                   //乔治敦, 巴西利亚
    OVFS_TIME_ZONE_ATLANTIC_OCEAN = 12,               //中大西洋
    OVFS_TIME_ZONE_ANGLE_ISLANDS = 13,                //佛得角群岛,亚速尔群岛
    OVFS_TIME_ZONE_GREENWICH = 14,                    //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
    OVFS_TIME_ZONE_AMSTERDAM = 15,                    //阿姆斯特丹, 柏林, 罗马, 巴黎
    OVFS_TIME_ZONE_ATHENS = 16,                       //雅典, 耶路撒冷 ,伊斯坦布尔
    OVFS_TIME_ZONE_BAGHDAD = 17,                      //巴格达 ,科威特
    OVFS_TIME_ZONE_TEHERAN = 18,                      //德黑兰
    OVFS_TIME_ZONE_MOSCOW = 19,                       //莫斯科, 圣彼得堡, 伏尔加格勒
    OVFS_TIME_ZONE_KABUL = 20,                        //喀布尔
    OVFS_TIME_ZONE_ISB = 21,                          //伊斯兰堡, 卡拉奇 ,塔什干
    OVFS_TIME_ZONE_MADRAS = 22,                       //马德拉斯,钦奈, 加尔各答, 孟买, 新德里
    OVFS_TIME_ZONE_KATHMANDU = 23,                    //加德满都
    OVFS_TIME_ZONE_NOVOSIBIRSK = 24,                  //阿拉木图,达卡,新西伯利亚
    OVFS_TIME_ZONE_RANGOON = 25,                      //仰光
    OVFS_TIME_ZONE_BANGKOK = 26,                      //曼谷, 河内, 雅加达
    OVFS_TIME_ZONE_BEIJING = 27,                      //北京,香港特别行政区,乌鲁木齐,新加坡
    OVFS_TIME_ZONE_OSAKA = 28,                        //首尔,大阪, 札幌, 东京
    OVFS_TIME_ZONE_ADELAIDE = 29,                   //阿德莱德 ,达尔文
    OVFS_TIME_ZONE_CANBERRA = 30,                     //堪培拉, 墨尔本, 悉尼
    OVFS_TIME_ZONE_SOLOMON_ISLANDS = 31,              //所罗门群岛, 新喀里多尼亚
    OVFS_TIME_ZONE_OSKLAND = 32,                      //奥克兰, 惠林顿 ,斐济 ,马加丹
    OVFS_TIME_ZONE_NUKUALOFA = 33,                    //努库阿洛法

#if 0

    OVFS_TIME_ZONE_SANTIAGO = 23,                     //圣地亚哥
    OVFS_TIME_ZONE_ASUNCION = 24,                     //亚松森

    OVFS_TIME_ZONE_BRASILIA = 26,                     //巴西利亚
    OVFS_TIME_ZONE_BUENOS_AIRES = 27,                 //布宜诺斯艾利斯
    OVFS_TIME_ZONE_GREENLAND = 28,                    //格陵兰
    OVFS_TIME_ZONE_CAYENNE = 29,                      //卡宴, 福塔雷萨
    OVFS_TIME_ZONE_MONTEVIDEO = 30,                   //蒙得维的亚
    OVFS_TIME_ZONE_SALVADOR = 31,                     //萨尔瓦多
    OVFS_TIME_ZONE_COORDINATED_2 = 32,                //协调世界时-02

    OVFS_TIME_ZONE_ANGLE_ISLANDS = 34,                //佛得角群岛
    OVFS_TIME_ZONE_AZORES_ISLANDS = 35,               //亚速尔群岛
    OVFS_TIME_ZONE_GREENWICH = 36,                    //格林威治标准时间:都柏林, 爱丁堡, 伦敦, 里斯本
    OVFS_TIME_ZONE_CASABLANK = 37,                    //卡莎布兰卡
    OVFS_TIME_ZONE_MONROVIA = 38,                     //蒙罗维亚, 雷克雅未克
    OVFS_TIME_ZONE_CORRDINATED = 39,                  //协调世界时
    OVFS_TIME_ZONE_AMSTERDAM = 40,                    //阿姆斯特丹, 柏林, 伯尔尼, 罗马, 斯德哥尔摩, 维也纳
    OVFS_TIME_ZONE_BELGRADE = 41,                     //贝尔格莱德, 布拉迪斯拉发, 布达佩斯, 卢布尔雅那
    OVFS_TIME_ZONE_BRUSSELS = 42,                     //布鲁塞尔, 哥本哈根, 马德里, 巴黎
    OVFS_TIME_ZONE_SARAJEVO = 43,                     //萨拉热窝, 斯科普里, 华沙, 萨格勒布
    OVFS_TIME_ZONE_WESTERN_CENTRAL_AFRICA = 44,       //中非西部
    OVFS_TIME_ZONE_WINDHOEK = 45,                     //温得和克
    OVFS_TIME_ZONE_AMMAN = 46,                        //安曼
    OVFS_TIME_ZONE_BEIRUT = 47,                       //贝鲁特
    OVFS_TIME_ZONE_DAMASCUS = 48,                     //大马士革
    OVFS_TIME_ZONE_HRE = 49,                          //哈拉雷, 比勒陀利亚
    OVFS_TIME_ZONE_HELSINKI = 50,                     //赫尔辛基, 基辅, 里加, 索菲亚, 塔林, 维尔纽斯
    OVFS_TIME_ZONE_CAIRO = 51,                        //开罗
    OVFS_TIME_ZONE_NICOSIA = 52,                      //尼科西亚
    OVFS_TIME_ZONE_ATHENS = 53,                       //雅典, 布加勒斯特
    OVFS_TIME_ZONE_JERUSALEM = 54,                    //耶路撒冷
    OVFS_TIME_ZONE_ISTANBUL = 55,                     //伊斯坦布尔
    OVFS_TIME_ZONE_KALININGRAD = 56,                  //加里宁格勒, 明斯克
    OVFS_TIME_ZONE_BAGHDAD = 57,                      //巴格达
    OVFS_TIME_ZONE_KUWAIT = 58,                       //科威特, 利雅得
    OVFS_TIME_ZONE_NAIROBI = 59,                      //内罗毕
    OVFS_TIME_ZONE_MOSCOW = 60,                       //莫斯科, 圣彼得堡, 伏尔加格勒
    OVFS_TIME_ZONE_TEHERAN = 61,                      //德黑兰
    OVFS_TIME_ZONE_ABU_DHABI = 62,                    //阿布扎比, 马斯喀特
    OVFS_TIME_ZONE_YEREVAN = 63,                      //埃里温
    OVFS_TIME_ZONE_BAKU = 64,                         //巴库
    OVFS_TIME_ZONE_TBILISI = 65,                      //第比利斯
    OVFS_TIME_ZONE_CAUCASUS = 66,                     //高加索标准时间
    OVFS_TIME_ZONE_PORT_LOUIS = 67,                   //路易港
    OVFS_TIME_ZONE_KABUL = 68,                        //喀布尔
    OVFS_TIME_ZONE_TASHKENT = 69,                     //阿什哈巴德, 塔什干
    OVFS_TIME_ZONE_ISB = 70,                          //伊斯兰堡, 卡拉奇
    OVFS_TIME_ZONE_EKATERINBURG = 71,                 //叶卡捷琳堡
    OVFS_TIME_ZONE_MADRAS = 72,                       //钦奈, 加尔各答, 孟买, 新德里
    OVFS_TIME_ZONE_SRIWALDEN = 73,                    //斯里加亚渥登普拉
    OVFS_TIME_ZONE_KATHMANDU = 74,                    //加德满都
    OVFS_TIME_ZONE_ASTANA = 75,                       //阿斯塔纳
    OVFS_TIME_ZONE_DACCA = 76,                        //达卡
    OVFS_TIME_ZONE_NOVOSIBIRSK = 77,                  //新西伯利亚
    OVFS_TIME_ZONE_RANGOON = 78,                      //仰光
    OVFS_TIME_ZONE_BANGKOK = 79,                      //曼谷, 河内, 雅加达
    OVFS_TIME_ZONE_KRASNOYARSK = 80,                  //克拉斯诺亚尔斯克
    OVFS_TIME_ZONE_BEIJING = 81,                      //北京, 重庆, 香港特别行政区, 乌鲁木齐
    OVFS_TIME_ZONE_KUALA_LUMPUR = 82,                 //吉隆坡, 新加坡
    OVFS_TIME_ZONE_PERTH = 83,                        //珀斯
    OVFS_TIME_ZONE_TAIPEI = 84,                       //台北
    OVFS_TIME_ZONE_ULAN_BATOR = 85,                   //乌兰巴托
    OVFS_TIME_ZONE_IKT = 86,                          //伊尔库茨克
    OVFS_TIME_ZONE_OSAKA = 87,                        //大阪, 札幌, 东京
    OVFS_TIME_ZONE_SEOUL = 88,                        //首尔
    OVFS_TIME_ZONE_YAKUTSK = 89,                      //雅库茨克
    OVFS_TIME_ZONE_ADELAIDE = 90,                     //阿德莱德
    OVFS_TIME_ZONE_DARWIN = 91,                       //达尔文
    OVFS_TIME_ZONE_BNE = 92,                          //布里斯班
    OVFS_TIME_ZONE_GUAM = 93,                         //关岛, 莫尔兹比港
    OVFS_TIME_ZONE_HOBART = 94,                       //霍巴特
    OVFS_TIME_ZONE_CANBERRA = 95,                     //堪培拉, 墨尔本, 悉尼
    OVFS_TIME_ZONE_VVO = 96,                          //符拉迪沃斯托克
    OVFS_TIME_ZONE_SOLOMON_ISLANDS = 97,              //所罗门群岛, 新喀里多尼亚
    OVFS_TIME_ZONE_MAGADAN = 98,                      //马加丹
    OVFS_TIME_ZONE_OSKLAND = 99,                      //奥克兰, 惠林顿
    OVFS_TIME_ZONE_SKIN_DROGBA = 100,                 //皮德罗巴甫洛夫斯基-勘察加 - 旧用
    OVFS_TIME_ZONE_FIJI = 101,                        //斐济
    OVFS_TIME_ZONE_COORDINATED_12 = 102,              //协调世界时+12
    OVFS_TIME_ZONE_NUKUALOFA = 103,                   //努库阿洛法
//新添加的宏
    OVFS_TIME_ZONE_CHRISTMAS_ISLANDS = 104,             //圣诞岛
    OVFS_TIME_ZONE_TRIPOLI = 105,                         //的黎波里
    OVFS_TIME_ZONE_EASTEUROPE = 106,                      //东欧

    OVFS_TIME_ZONE_MINSK = 107,             //明斯克
    OVFS_TIME_ZONE_CHOKURDAKH = 108,        //乔库尔达赫
#endif
    OVFS_TIME_ZONE_UNKNOW = 0x13149527,     //未定义值

} ovfs_time_zone;

typedef struct
{
    ovfs_time_zone zone_type;
    S32 zone_hour;
    S32 zone_min;
} ovfs_dst_cap_node;

static ONVIF_XML_ACTION_T s_xml_action[] =
{
    {ONVIF_XML_ACTION_GET_DEVICE_INFO_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_GET_DEVICE_INFO_STR},
    {ONVIF_XML_ACTION_GET_DNS_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_GET_DNS_STR},
    {ONVIF_XML_ACTION_GET_CAPBILITY_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_CAPBILITY_STR},
    {ONVIF_XML_ACTION_GET_SCOPE_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_GET_SCOPE_STR},
    {ONVIF_XML_ACTION_SET_SCOPE_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_SET_SCOPE_STR},
    {ONVIF_XML_ACTION_GET_DATE_TIME_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_GET_DATE_TIME_STR},
    {ONVIF_XML_ACTION_GET_NETWORK_IF_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_NETWORK_IF_STR},
    /*media1*/
    {ONVIF_XML_ACTION_GET_PROFILES_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_PROFILES_STR},
    {ONVIF_XML_ACTION_GET_SERVICE_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_SERVICE_STR},
    {ONVIF_XML_ACTION_GET_VIDEO_SOURCE_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_VIDEO_SOURCE_STR},
    {ONVIF_XML_ACTION_GET_PROFILE_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_PROFILE_STR},
    {ONVIF_XML_ACTION_GET_STREAM_URI_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_STREAM_URI_STR},
    {ONVIF_XML_ACTION_GET_VIDEO_SOURCE_CFG_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_VIDEO_SOURCE_CFG_STR},
    {ONVIF_XML_ACTION_GET_OSDS_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_OSDS_STR},
    {ONVIF_XML_ACTION_SET_OSD_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SET_OSD_STR},

    {ONVIF_XML_ACTION_GET_IMAGE_SET_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_IMAGE_SET_STR},
    {ONVIF_XML_ACTION_GET_OPTIONS_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GET_OPTIONS_STR},
    {ONVIF_XML_ACTION_SET_IMAGING_SETTING_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SET_IMAGING_SETTING_STR},
    {ONVIF_XML_ACTION_GetVideoEncoderConfigurationOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetVideoEncoderConfigurationOptions_STR},
    {ONVIF_XML_ACTION_SetVideoEncoderConfiguration_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetVideoEncoderConfiguration_STR},
    {ONVIF_XML_ACTION_SetNetworkInterfaces_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetNetworkInterfaces_STR},
    {ONVIF_XML_ACTION_SetNetworkDefaultGateway_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetNetworkDefaultGateway_STR},
    {ONVIF_XML_ACTION_GetOSDOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetOSDOptions_STR},
    {ONVIF_XML_ACTION_GetMoveOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetMoveOptions_STR},
    {ONVIF_XML_ACTION_SetSystemDateAndTime_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetSystemDateAndTime_STR},
    {ONVIF_XML_ACTION_GetVideoAnalyticsConfigurations_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetVideoAnalyticsConfigurations_STR},
    {ONVIF_XML_ACTION_GetAnalyticsModules_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAnalyticsModules_STR},
    {ONVIF_XML_ACTION_ModifyAnalyticsModules_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_ModifyAnalyticsModules_STR},

    {ONVIF_XML_ACTION_GetVideoEncoderConfiguration_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetVideoEncoderConfiguration_STR},
    {ONVIF_XML_ACTION_GetAudioEncoderConfiguration_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioEncoderConfiguration_STR},
    {ONVIF_XML_ACTION_GetAudioEncoderConfigurationOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioEncoderConfigurationOptions_STR},
    {ONVIF_XML_ACTION_GetRules_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetRules_STR},
    {ONVIF_XML_ACTION_ModifyRules_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_ModifyRules_STR},
    {ONVIF_XML_ACTION_SetVideoAnalyticsConfiguration_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetVideoAnalyticsConfiguration_STR},
    {ONVIF_XML_ACTION_Subscribe_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_Subscribe_STR},
    {ONVIF_XML_ACTION_Renew_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_Renew_STR},
    {ONVIF_XML_ACTION_Unsubscribe_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_Unsubscribe_STR},
    {ONVIF_XML_ACTION_GetServiceCapabilities_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetServiceCapabilities_STR},
    {ONVIF_XML_ACTION_CreateOSD_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_CreateOSD_STR},
    {ONVIF_XML_ACTION_DeleteOSD_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_DeleteOSD_STR},
    {ONVIF_XML_ACTION_GetEventProperties_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetEventProperties_STR},
    {ONVIF_XML_ACTION_CreatePullPointSubscription_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_CreatePullPointSubscription_STR},
    {ONVIF_XML_ACTION_PullMessages_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_PullMessages_STR},
    {ONVIF_XML_ACTION_GetAudioSources_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioSources_STR},
    {ONVIF_XML_ACTION_GetAudioSourceConfigurations_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioSourceConfigurations_STR},
    {ONVIF_XML_ACTION_GetSnapshotUri_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetSnapshotUri_STR},
    {ONVIF_XML_ACTION_GetOSD_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetOSD_STR},
#if (defined ONVIF_EXT_GPS)
    {ONVIF_XML_ACTION_PushAnalogGpsInfo_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_PushAnalogGpsInfo_STR},
    {ONVIF_XML_ACTION_PushStationInfo_ID, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_PushStationInfo_STR},
#endif
    {ONVIF_XML_ACTION_GetNetworkProtocols_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetNetworkProtocols_STR},
    {ONVIF_XML_ACTION_GetVideoEncoderConfigurations_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetVideoEncoderConfigurations_STR},
    {ONVIF_XML_ACTION_GetAudioEncoderConfigurations_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioEncoderConfigurations_STR},

    {ONVIF_XML_ACTION_GetConfigurations_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetConfigurations_STR},
	{ONVIF_XML_ACTION_GetConfiguration_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetConfiguration_STR},
	{ONVIF_XML_ACTION_GetConfigurationOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetConfigurationOptions_STR},
	{ONVIF_XML_ACTION_GetPresetTours_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetPresetTours_STR},
	{ONVIF_XML_ACTION_GetPresetTour_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetPresetTour_STR},
	{ONVIF_XML_ACTION_GetNode_ID,ONVIF_XML_ACTION_AUTH_REQ,ONVIF_XML_ACTION_GetNode_STR},
	{ONVIF_XML_ACTION_GetNodes_ID,ONVIF_XML_ACTION_AUTH_REQ,ONVIF_XML_ACTION_GetNodes_STR},
	{ONVIF_XML_ACTION_GetPresets_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetPresets_STR},
	{ONVIF_XML_ACTION_SetPreset_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SetPreset_STR},
	{ONVIF_XML_ACTION_GotoPreset_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GotoPreset_STR},
	{ONVIF_XML_ACTION_DelPreset_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_DelPreset_STR},
    {ONVIF_XML_ACTION_GetStatus_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetStatus_STR},
    {ONVIF_XML_ACTION_AbsoluteMove_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_AbsoluteMove_STR},
	{ONVIF_XML_ACTION_ContinuousMove_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_ContinuousMove_STR},
	{ONVIF_XML_ACTION_Move_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_Move_STR},
	{ONVIF_XML_ACTION_Stop_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_Stop_STR},
	{ONVIF_XML_ACTION_HK_MaskOptions_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_HK_MaskOptions_STR},
	{ONVIF_XML_ACTION_HK_PrivacyMask_ID, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_HK_PrivacyMask_STR},


    {ONVIF_XML_ACTION_SetSynchronizationPoint, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_SetSynchronizationPoint_STR},
    {ONVIF_XML_ACTION_GetVideoSourceConfigurations, ONVIF_XML_ACTION_NOAUTH_REQ, ONVIF_XML_ACTION_GetVideoSourceConfigurations_STR},
    {ONVIF_XML_ACTION_GetNTP, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetNTP_STR},
    {ONVIF_XML_ACTION_GetDiscoveryMode, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetDiscoveryMode_STR},
    {ONVIF_XML_ACTION_GetNetworkDefaultGateway, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetNetworkDefaultGateway_STR},
    {ONVIF_XML_ACTION_GetHostname, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetHostname_STR},
    {ONVIF_XML_ACTION_SystemReboot, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_SystemReboot_STR},
    {ONVIF_XML_ACTION_GetSupportedAnalyticsModules, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetSupportedAnalyticsModules_STR},
    {ONVIF_XML_ACTION_GetSupportedRules, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetSupportedRules_STR},
    {ONVIF_XML_ACTION_AddVideoEncoderConfiguration, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_AddVideoEncoderConfiguration_STR},
    {ONVIF_XML_ACTION_GetGuaranteedNumberOfVideoEncoderInstances, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetGuaranteedNumberOfVideoEncoderInstances_STR},
    {ONVIF_XML_ACTION_GetVideoEncoderInstances, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetVideoEncoderInstances_STR},
    {ONVIF_XML_ACTION_GetAudioSourceConfigurationOptions, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioSourceConfigurationOptions_STR},
    {ONVIF_XML_ACTION_GetMetadataConfigurations, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetMetadataConfigurations_STR},
    {ONVIF_XML_ACTION_GetAudioOutputConfigurationOptions, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioOutputConfigurationOptions_STR},
    {ONVIF_XML_ACTION_GetAudioOutputConfigurations, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioOutputConfigurations_STR},
    {ONVIF_XML_ACTION_GetAudioDecoderConfigurationOptions, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioDecoderConfigurationOptions_STR},
    {ONVIF_XML_ACTION_GetAudioDecoderConfigurations, ONVIF_XML_ACTION_AUTH_REQ, ONVIF_XML_ACTION_GetAudioDecoderConfigurations_STR},


};

mxml_node_t *XmlParserOne_vague(mxml_node_t *param, mxml_node_t *top, const char *keyStr, char **value);

static int message_begin_cb(http_parser *p)
{
    return 0;
}

/* static size_t strnlen_parser(const char *s, size_t maxlen) */
/* { */
/*     const char *p = (const char *)memchr(s, '\0', maxlen); */
/*     if (p == NULL) */
/*         return maxlen; */

/*     return p - s; */
/* } */

/* static size_t strlncat(char *dst, size_t len, const char *src, size_t n) */
/* { */
/*     size_t slen = strnlen_parser(src, n); */
/*     size_t dlen = strnlen_parser(dst, len); */

/*     if (dlen < len) */
/*     { */
/*         size_t rlen = len - dlen; */
/*         size_t ncpy = slen < rlen ? slen : (rlen - 1); */
/*         memcpy(dst + dlen, src, ncpy); */
/*         dst[dlen + ncpy] = '\0'; */
/*     } */

/*     return slen + dlen; */
/* } */

static int header_field_cb(http_parser *p, const char *buf, size_t len)
{
    if (strncmp(buf, "Authorization", strlen("Authorization")) == 0)
    {
        HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
        result->authLen = len;
    }

    if (strncmp(buf, "RemoteAddr", strlen("RemoteAddr")) == 0)
    {
        HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
        result->remoteAddrLen = len;
    }

    if (strncmp(buf, "HostAddr", strlen("HostAddr")) == 0)
    {
        HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
        result->hostAddrLen = len;
    }

    if (strncmp(buf, "Scheme", strlen("Scheme")) == 0)
    {
        HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
        result->schemeLen = len;
    }
    return 0;
}

static int header_value_cb(http_parser *p, const char *buf, size_t len)
{
    HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
    if (result->authStr == NULL && result->authLen > 0)
    {
        result->authStr = (char *)buf;
        result->authLen = len;
    }

    if (result->remoteAddrStr == NULL && result->remoteAddrLen > 0)
    {
        result->remoteAddrStr = (char *)buf;
        result->remoteAddrLen = len + 1;
    }

    if (result->hostAddrStr == NULL && result->hostAddrLen > 0)
    {
        result->hostAddrStr = (char *)buf;
        result->hostAddrLen = len + 1;
    }

    if (result->scheme == NULL && result->schemeLen > 0)
    {
        result->scheme = (char *)buf;
        result->schemeLen = len + 1;
    }
    return 0;
}

static int request_url_cb(http_parser *p, const char *buf, size_t len)
{
    HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
    result->uriStr = (char *)buf;
    result->uriLen = len;
    return 0;
}

static int response_status_cb(http_parser *p, const char *buf, size_t len)
{
    LOGD("status @%s@\n", buf);
    return 0;
}

static int body_cb(http_parser *p, const char *buf, size_t len)
{

    HTTP_PARSER_RESULT_T *result = (HTTP_PARSER_RESULT_T *)(p->data);
    result->contentStr = (char *)buf;
    result->contentLen = len;
    return 0;
}

static int headers_complete_cb(http_parser *p)
{
    return 0;
}

static int message_complete_cb(http_parser *p)
{
    return 0;
}

static int chunk_header_cb(http_parser *p)
{
    return 0;
}

static int chunk_complete_cb(http_parser *p)
{
    return 0;
}

/*parser filed str max len is 1024*/
static inline int ParserFiledStr(char *buf, const char *key,
                                 char *result) __attribute__((always_inline));
static inline int ParserFiledStr(char *buf, const char *key, char *result)
{
    char *p = strstr(buf, key);
    if (p != NULL)
        return sscanf(p, "%*[^\"]\"%1024[^\"]", result);

    return 0;
}

/*parser filed str max len is 1024*/
static inline int ParserFiledNum(char *buf, const char *key,
                                 char *result) __attribute__((always_inline));
static inline int ParserFiledNum(char *buf, const char *key, char *result)
{
    char *p = strcasestr(buf, key);
    if (p != NULL)
        return sscanf(p, "%*[^=]=%1024[^,]", result);

    return 0;
}

static inline void TimeT2StrISO8601(time_t *a, char b[32])
{
    struct tm tmS = {};
    Common_LocalTime_r(a, &tmS);
    char zoneStr[8] = {};
    strftime(zoneStr, sizeof(zoneStr), "%z", &tmS);
    /*string +0800 to +08:00*/
    if (strlen(zoneStr) >= 5)
    {
        zoneStr[5] = zoneStr[4];
        zoneStr[4] = zoneStr[3];
        zoneStr[3] = ':';
    }

    strftime(b, 32, "%Y-%m-%dT%H:%M:%S", &tmS);
    strcat(b, zoneStr);
}

int HttpParser_Post(char *buf, int len, HTTP_PARSER_RESULT_T *result,
                    ONVIF_AUTH_INFO_T *authResult)
{
    http_parser hp;
    http_parser_settings settings = { message_begin_cb,
                                      request_url_cb,
                                      response_status_cb,
                                      header_field_cb,
                                      header_value_cb,
                                      headers_complete_cb,
                                      body_cb,
                                      message_complete_cb,
                                      chunk_header_cb,
                                      chunk_complete_cb
                                    };
    memset(&hp, 0, sizeof(hp));
    memset(result, 0, sizeof(HTTP_PARSER_RESULT_T));
    http_parser_init(&hp, HTTP_POST);
    hp.data = result;
    size_t parsed = http_parser_execute(&hp, &settings, buf, len);
    if (parsed != len)
    {
        LOGD("parser failed\n");
    }

    if (result->authStr != NULL)
        HttpParser_GetAuthResult(result->authStr, result->authLen, authResult);

    return 0;
}

void HttpParser_GetAuthResult(char *authStr, int authLen,
                              ONVIF_AUTH_INFO_T *authResult)
{
    /* Digest username="admin", realm="abc@test.com", qop="auth", algorithm="MD5", uri="/onvif/device_service", nonce="5c76ae4cca542ae8944a", nc=00000001, cnonce="4257A406785CA25246B3B083CA1F0B85", opaque="625558ec", response="a4942beace89ca8cbc5436f1ccd88a7e" */
    char *tmp  = (char *)ONVIF_MALLOC(1024);
    if (ParserFiledStr(authStr, "username", tmp) == 1)
        authResult->userName = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "realm", tmp) == 1)
        authResult->digest.realm = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "cnonce", tmp) == 1)
        authResult->digest.cnonce = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "opaque", tmp) == 1)
        authResult->digest.opaque = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledNum(authStr, "nc=", tmp) == 1)
        authResult->digest.nc = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "nonce", tmp) == 1)
        authResult->digest.nonce = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "uri", tmp) == 1)
        authResult->digest.uri = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "response", tmp) == 1)
        authResult->digest.response = (char *)ONVIF_STRDUP(tmp);

    if (ParserFiledStr(authStr, "qop", tmp) == 1)
        authResult->digest.qop = (char *)ONVIF_STRDUP(tmp);

    authResult->digest.method = (char *)ONVIF_STRDUP((char *)"POST");

    authResult->hasAuth = 1;
    authResult->authMethod = ONVIF_AUTH_TYPE_DIGEST;
    ONVIF_FREE(tmp);
}

void HttpRsp_Unauth401(char *buf, int len)
{

    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char nonce[24] = {0};
    char opaque[16] = {0};

    Common_Digest_CalcOpaque(opaque);
    Common_Digest_CalcNonce(nonce);

    snprintf(httpbody, 4096, ONVIF_HTTP_401_BODY, ONVIF_HTTP_XMLNS);
    snprintf(httpheader, 1024, ONVIF_HTTP_401_HEAD, nonce, opaque,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_BadReq400(char *buf, int len)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_400_BODY, ONVIF_HTTP_XMLNS);
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_DeviceInfo(char *buf, int len, char *manufacturer, char *model, char *firmwareVersion,
                        char *serialNumber, char *hardwareId
                        , ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_DEVICE_INFO_BODY,
             ONVIF_HTTP_XMLNS,
             manufacturer, model,firmwareVersion, serialNumber, hardwareId);
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_DateTime(char *buf, int len, ONVIF_TIME_ALL_T *timeAll,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *tzName, int tzOffset)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

//    char timezone[64] = { 0 };
    char datetimetype[64] = { 0 };
    char daylightsave[64] = {0 };
    if (timeAll->ntp.Enable == 1)
        snprintf(datetimetype, sizeof(datetimetype), "%s", "NTP");
    else
        snprintf(datetimetype, sizeof(datetimetype), "%s", "Manual");


    if (timeAll->dst.Enable == 1)
    {
        snprintf(daylightsave, sizeof(daylightsave), "%s", "true");
    }
    else
    {
        snprintf(daylightsave, sizeof(daylightsave), "%s", "false");
    }

    char tz[16] = {};
    struct tm tmS = {};
    time_t a = time(NULL);
    Common_LocalTime_r(&a, &tmS);
    char zoneStr[8] = {};
    strftime(zoneStr, sizeof(zoneStr), "%z", &tmS);
    LOGE("zoneStr=%s\n", zoneStr);
    if(zoneStr[0] == '+')
    {
        zoneStr[0] = '-';
    }
    else
    {
        zoneStr[0] = '+';
    }
    zoneStr[5] = zoneStr[4];
    zoneStr[4] = zoneStr[3];
    zoneStr[3] = ':';

    snprintf(tz, sizeof(tz), "CST%s", zoneStr);
/*
    if (tzOffset > 0)
        snprintf(timezone, sizeof(timezone), "%s-%d", tzName, abs(tzOffset) / 100);
    else if (tzOffset < 0)
        snprintf(timezone, sizeof(timezone), "%s+%d", tzName, abs(tzOffset) / 100);
    else
        snprintf(timezone, sizeof(timezone), "%s", tzName);
*/
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_DATE_TIME_BODY,
             ONVIF_HTTP_XMLNS,
             datetimetype, daylightsave, tz, timeAll->utc.Hour, timeAll->utc.Min,
             timeAll->utc.Sec,
             timeAll->utc.Year, timeAll->utc.Month, timeAll->utc.Day,
             timeAll->systime.Hour, timeAll->systime.Min, timeAll->systime.Sec,
             timeAll->systime.Year, timeAll->systime.Month, timeAll->systime.Day
            );
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_NetworkIf(char *buf, int len, ONVIF_NET_INFO_T *networkAttr,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);
    int prelen = Utils_GetNetMaskBit(networkAttr->ipMaskV4);
    if (prelen <= 0)
        prelen = 24;
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NETWORK_IF_BODY,
             ONVIF_HTTP_XMLNS,
             networkAttr->ethName, networkAttr->ethName, networkAttr->mac,
             networkAttr->ipV4, prelen,/*(int)inet_addr(networkAttr->IpMaskV4)*/
             (networkAttr->bDhcp== 1) ? "true" : "false");

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Capabilities(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                          ONVIF_WEB_ATTR_T *webAttr,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpextAbility  = NULL;
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char ipaddr[32] = { 0 };
    snprintf(ipaddr, xmlDetail->httpResult->hostAddrLen, "%s",
             xmlDetail->httpResult->hostAddrStr);
#if 0
#if (defined ONVIF_EXT_GPS)
    httpextAbility = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    snprintf(httpextAbility, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_CAPABILITIES_EXT_GPS, ipaddr,webAttr->httpPort);
#endif
	LOGD("IsOfDome=%d,LensSupport=%d\n",ptzCapability->ptzInfo.IsOfDome,ptzCapability->ptzInfo.LensSupport);
	if((ptzCapability->ptzInfo.IsOfDome > 0) || (ptzCapability->ptzInfo.LensSupport== 1))
	{
		char *ptzAbility = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
		snprintf(ptzAbility, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_CAPABILITIES_EXT, ipaddr,webAttr->httpPort);
		if(NULL == httpextAbility)
		{
			httpextAbility = ptzAbility;
		}
		else
		{
			char *temp = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN*2);
			snprintf(temp,ONVIF_MSG_HEAD_LEN,"%s%s",httpextAbility,ptzAbility);
			ONVIF_FREE(httpextAbility);
			httpextAbility = temp;
		}
	}
#endif
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_CAPABILITIES_BODY,
             ONVIF_HTTP_XMLNS,
             ipaddr, webAttr->httpPort,
             ipaddr, webAttr->httpPort,
             ipaddr, webAttr->httpPort,
             ipaddr, webAttr->httpPort,
             ipaddr, webAttr->httpPort,
             ipaddr, webAttr->httpPort,
			 ipaddr, webAttr->httpPort,
             (httpextAbility != NULL) ? httpextAbility : "",
             ipaddr, webAttr->httpPort);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    if (httpextAbility != NULL)
        ONVIF_FREE(httpextAbility);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

int ovfs_ovf_analytics_motion_sensitivity_net2local(int num)
{
    int iRet = 0;

    if (num <= 10)
    {
        iRet = 0;
    }
    else if ((num > 10) && (num <= 20))
    {
        iRet = 1;
    }
    else if ((num > 20) && (num <= 40))
    {
        iRet = 2;
    }
    else if ((num > 40) && (num <= 60))
    {
        iRet = 3;
    }
    else if ((num > 60) && (num <= 80))
    {
        iRet = 4;
    }
    else if (num > 80)
    {
        iRet = 5;
    }

    return iRet;
}

int ovfs_ovf_analytics_motion_sensitivity_local2net(int num)
{
    int iRet = 0;

    switch (num)
    {
    case 0:
    {
        iRet = 10;
        break;
    }
    case 1:
    {
        iRet = 20;
        break;
    }
    case 2:
    {
        iRet = 40;
        break;
    }
    case 3:
    {
        iRet = 60;
        break;
    }
    case 4:
    {
        iRet = 80;
        break;
    }
    case 5:
    {
        iRet = 100;
        break;
    }
    default :
    {
        iRet = 0;
        break;
    }
    }

    return iRet;
}

static void OsdString2HttpString(char *osdString, int len, int maxOutLen, char *outHttpString)
{
    int i = 0, j = 0;

    for (i = 0; i < len; i++)
    {
        if (osdString[i] == 0x26)
        {
            outHttpString[j] = '&';
            outHttpString[j + 1] = 'a';
            outHttpString[j + 2] = 'm';
            outHttpString[j + 3] = 'p';
            outHttpString[j + 4] = ';';
            j += 4;
        }
        else if (osdString[i] == 0x3C)
        {
            outHttpString[j] = '&';
            outHttpString[j + 1] = 'l';
            outHttpString[j + 2] = 't';
            outHttpString[j + 3] = ';';
            j += 3;
        }
        else if (osdString[i] == 0x3E)
        {
            outHttpString[j] = '&';
            outHttpString[j + 1] = 'g';
            outHttpString[j + 2] = 't';
            outHttpString[j + 3] = ';';
            j += 3;
        }
        else
        {
            outHttpString[j] = osdString[i];
        }

        j++;
        if (j >= maxOutLen)
        {
            LOGE("length out of range\n");
            break;
        }
    }
}

static void HttpString2OsdString(char *httpString, int len, int maxOutLen, char *outOsdString)
{
    int cj = 0, ci = 0, chLen = strlen(httpString);

    for (cj = 0; cj < chLen; cj++)
    {
        if (strncmp(httpString + cj, "&amp;", 5) == 0)
        {
            outOsdString[ci] = '&';
            cj += 4;
        }
        else if (strncmp(httpString + cj, "&gt;", 4) == 0)
        {
            outOsdString[ci] = '>';
            cj += 3;
        }
        else if (strncmp(httpString + cj, "&lt;", 4) == 0)
        {
            outOsdString[ci] = '<';
            cj += 3;
        }
        else
            outOsdString[ci] = httpString[cj];

        ci++;
        if (ci >= maxOutLen)
        {
            LOGE("length out of range\n");
            break;
        }
    }
}


void HttpRsp_Profiles(char *buf, int len,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_PROFILES_T *profiles,
                      char *chProfile)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    char *profilesbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);

    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MAX_MSG_BUF_LEN);
    memset(profilesbody, 0, ONVIF_MAX_MSG_BUF_LEN);

    int sizeReturn = 0;
    int blockW = profiles->motion.blockW;
    int blockH = profiles->motion.blockH;
    int i, j, iByteNum, iBitNum;
    char *pBase64 = NULL;
    char *strValue = profiles->motion.rect;
    char str[512] = {0};
    char cell[512] = {0};
    for(i = 0; i < 18; i++)
    {
        for(j = 0; j < 22; j++)
        {
            iByteNum = (i * 22 + j) / 8;
            iBitNum = 7 - ((i * 22 + j) % 8);
            int x = (j * blockW + 11) / 22;
            int y = (i * blockH + 9) / 18;

            if(*(strValue + y * blockW + x) != '0')
                cell[iByteNum] |= (1 << iBitNum);
        }
    }
    /*for (i = 0; i < iByteNum + 1; i ++)
    {
        printf("%x ", cell[i]);
    }
    printf("\n");*/
    sizeReturn = ovfs_onvif_Packbits((unsigned char *)cell, (unsigned char *)str,
                                     (unsigned int)(iByteNum + 1));
    pBase64 = ovfs_onvif_BASE64Encode((const char *)str, sizeReturn);
    //LOGW("Base64:%s\n", pBase64);

    int zoom_max = 0;
    float zoom_x = 0;

    RestOnvif_RequestGetZoomCfg(&zoom_x,&zoom_max);

    int isMedia20 = 0;
    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        LOGW("IS Media20\n");
        isMedia20 = 1;
    }
    LOGW("name space %s\n", xmlDetail->actionNs);

    char tokenName[32] = { 0 };
    snprintf(tokenName, sizeof(tokenName), "ProfileToken");

    mxml_node_t *token = mxmlFindElement((mxml_node_t *)xmlDetail->xmlParam,
                                         (mxml_node_t *)xmlDetail->xmlParam,
                                         tokenName, NULL, NULL, MXML_DESCEND_ALL);

    char *pStr = (char *)mxmlGetText(token, NULL);
    if(pStr)
    {
        LOGD("ProfileToken:[%s]\n",pStr);
        if(strcasecmp(pStr, "CH01_sub") == 0)
        {
            profiles->stream[0].isValid = 0;
            profiles->stream[2].isValid = 0;
        }
        else if(strcasecmp(pStr, "CH01") == 0)
        {
            profiles->stream[1].isValid = 0;
            profiles->stream[2].isValid = 0;
        }
        else if(strcasecmp(pStr, "CH01_third") == 0)
        {
            profiles->stream[0].isValid = 0;
            profiles->stream[1].isValid = 0;
        }
    }

    if (isMedia20)//strstr(xmlDetail->actionNs , "ns1") != NULL)
    {
        int i = 0;
        int iLen = 0,iFill = 0;
        for(i = 0; i < 3; i++)
        {
            if(profiles->stream[i].isValid == 0)
            {
                continue;
            }

            //int encodeFormat = profiles->stream[i].encodeFormat;
            //int encodeprofiles = profiles->stream[i].profiles;

            iFill = snprintf(profilesbody + iLen, ONVIF_MAX_MSG_BUF_LEN - iLen,
                             ONVIF_HTTP_PROFILE_2_CONTENT,
                             i + 1,
                             i == 0 ? "Main" : ((i == 1) ? "Sub" : "Third"),
                             //VideoSource
                             profiles->viAttr.Width, profiles->viAttr.Height,
                             //AudioSource

                             //VideoEncoder
                             i + 1, profiles->stream[i].Iinterval,
                             i == 0 ? "Main" : ((i == 1) ? "Sub" : "Third"),
                             profiles->stream[i].encodeFormat == 0 ? "H264" :
                             ((profiles->stream[i].encodeFormat == 1 ||
                               profiles->stream[i].encodeFormat == 8) ? "H265" : "MJPEG"),
                             /* "H264", */
                             profiles->stream[i].resolution[0],
                             profiles->stream[i].resolution[1],
                             profiles->stream[i].bitrateCtrlMode == 0 ? "false" : "true",
                             profiles->stream[i].resolution[2] == 0 ? profiles->viAttr.Fps :
                             profiles->stream[i].resolution[2],
                             profiles->stream[i].bitrate,
                             profiles->stream[i].encQuality,
                             //AudioEncoder

                             //Analytics
                             profiles->motion.enable == 0 ? 0 :
                             ovfs_ovf_analytics_motion_sensitivity_local2net(profiles->motion.Sensitivity),
                             pBase64,
                             //AudioOutput
                             zoom_max*1.0

                            );
                 iLen += iFill;
        }
    }
    else
    {
        int i = 0;
        int iLen = 0;
        for(i = 0; i < 3; i++)
        {
            if(profiles->stream[i].isValid == 0)
            {
                continue;
            }

            //int encodeFormat = profiles->stream[i].encodeFormat;
            int encodeprofiles = profiles->stream[i].profiles;

            int encQuality = profiles->stream[i].encQuality;

            LOGW("chProfile[0]=%c   xmlDetail->actionNs=%s\n", chProfile[0],
                 xmlDetail->actionNs);
            if(chProfile[0] != 's')
            {
                encQuality = profiles->stream[i].encQuality;
            }
            else if(strstr(xmlDetail->actionNs , "trt") != NULL)
            {
                encQuality = profiles->stream[i].encQuality;
            }

            iLen += snprintf(profilesbody + iLen, ONVIF_MAX_MSG_BUF_LEN - iLen,
                             ONVIF_HTTP_PROFILE_1_CONTENT,
                             chProfile,
                             i == 0 ? "CH01" : ((i == 1) ? "CH01_sub" : "CH01_third"),
                             i == 0 ? "Main" : ((i == 1) ? "Sub" : "Third"),
                             //VideoSourceConfiguration
                             profiles->viAttr.Width, profiles->viAttr.Height,
                             //AudioSourceConfiguration

                             //VideoEncoderConfiguration
                             i + 1,
                             i == 0 ? "Main" : ((i == 1) ? "Sub" : "Third"),
                            /*profiles->stream[i].encodeFormat == 0 ? "H264" :
                            ((profiles->stream[i].encodeFormat == 1 ||
                              profiles->stream[i].encodeFormat == 8) ? "H265" : "MJPEG"),*/
                             "H264",
                             profiles->stream[i].resolution[0],
                             profiles->stream[i].resolution[1],
                             encQuality,
                             profiles->stream[i].bitrateCtrlMode == 0 ? "false" : "true",
                             profiles->stream[i].resolution[2] == 0 ? profiles->viAttr.Fps :
                             profiles->stream[i].resolution[2],
                             profiles->stream[i].bitrate,
                             profiles->stream[i].Iinterval,
                             encodeprofiles == 0 ? "Baseline" : ((encodeprofiles == 1) ? "Main" : "High"),

                             //AudioEncoderConfiguration
                             i == 0?"238.255.0.2":"238.255.0.3",
                             i == 0?28080:28084,

                             i == 0?"238.255.0.5":"238.255.0.6",
                             i == 0?28080:28084,

                             //VideoAnalyticsConfiguration
                             profiles->motion.enable == 0 ? 0 :
                             ovfs_ovf_analytics_motion_sensitivity_local2net(profiles->motion.Sensitivity),
                             pBase64,
                             //PTZConfiguration
                             //"ptz0","ptz0","ptz0",
                             //MetadataConfiguration

                             //Extension
                             zoom_max*1.0,
                             chProfile);
        }
    }

    if(pBase64 != NULL)
    {
        ONVIF_FREE(pBase64);
    }

    if(chProfile[0] == 's')
    {
        if (isMedia20)//strstr(xmlDetail->actionNs , "ns1") != NULL)
        {
            snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_PROFILE_2_BODY,
                     ONVIF_HTTP_XMLNS, "s", profilesbody, "s");
        }
        else
        {
            snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_PROFILES_BODY,
                     ONVIF_HTTP_XMLNS, profilesbody);
        }
    }
    else
    {
        if (isMedia20)//strstr(xmlDetail->actionNs , "ns1") != NULL)
        {
            snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_PROFILE_2_BODY,
                     ONVIF_HTTP_XMLNS, "", profilesbody , "");
        }
        else
        {
            snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_PROFILE_1_BODY,
                     ONVIF_HTTP_XMLNS, profilesbody);
        }
    }
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));

    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
    ONVIF_FREE(profilesbody);
}

void HttpRsp_StreamUri(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_MultiCast_T *rtspAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char uri[128] = { 0 };
    char ipaddr[32] = {0 };
    char tokenName[32] = { 0 };

    int isMedia20 = 0;
    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        LOGW("IS Media20\n");
        isMedia20 = 1;
    }

    snprintf(ipaddr, xmlDetail->httpResult->hostAddrLen, "%s",
             xmlDetail->httpResult->hostAddrStr);

    if (strlen(xmlDetail->actionNs) > 0)
        snprintf(tokenName, sizeof(tokenName), "%s:ProfileToken", xmlDetail->actionNs);
    else
        snprintf(tokenName, sizeof(tokenName), "ProfileToken");

    mxml_node_t *token = mxmlFindElement((mxml_node_t *)xmlDetail->xmlParam,
                                         (mxml_node_t *)xmlDetail->xmlParam,
                                         tokenName, NULL, NULL, MXML_DESCEND_ALL);

    char *pStr = (char *)mxmlGetText(token, NULL);
    if (token != NULL && pStr != NULL && strcasecmp(pStr, "Profile2_1_1_2") == 0)
        snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_sub.264", ipaddr,
                 rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort);
    else if (token != NULL && pStr != NULL
             && strcasecmp(pStr, "Profile2_1_1_3") == 0)
        snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_third.264", ipaddr,
                 rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort);
    else if (token != NULL && pStr != NULL
             && strcasecmp(pStr, "CH01_sub") == 0)
        snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_sub.264", ipaddr,
                 rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort);
    else if (token != NULL && pStr != NULL
             && strcasecmp(pStr, "CH01_third") == 0)
        snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_third.264", ipaddr,
                 rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort);
    /* else if (token == NULL) */
    /* { */
    /*     token = mxmlFindElement((mxml_node_t *)xmlDetail->xmlParam, */
    /*                             (mxml_node_t *)xmlDetail->xmlParam, */
    /*                             "ProfileToken", NULL, NULL, MXML_DESCEND_ALL); */
    /*     if (token != NULL && strcasecmp(mxmlGetText(token, NULL), "CH01_sub") == 0) */
    /*         snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_sub.264", ipaddr, */
    /*                  rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort); */
    /*     else if (token != NULL */
    /*              && strcasecmp(mxmlGetText(token, NULL), "CH01_third") == 0) */
    /*         snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01_third.264", ipaddr, */
    /*                  rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort); */
    /*     else */
    /*         snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01.264", ipaddr, */
    /*                  rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort); */
    /* } */

    if (strlen(uri) == 0)
        snprintf(uri, sizeof(uri), "rtsp://%s:%d/ch01.264", ipaddr,
                 rtspAttr->rtspPort == 0 ? 554 : rtspAttr->rtspPort);

    if (isMedia20)//strstr(xmlDetail->actionNs, "ns1") != NULL)
        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_STREAM_URI_2_BODY,
                 ONVIF_HTTP_XMLNS, uri);
    else
        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_STREAM_URI_1_BODY,
                 ONVIF_HTTP_XMLNS, uri);


    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Servcies(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                      ONVIF_WEB_ATTR_T *webAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MAX_MSG_BUF_LEN);

    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    char *capability = NULL;
    XmlParserOne_vague(param, param, "IncludeCapability", &capability);
    LOGD("capability:[%s]\n",capability);

    char ipaddr[32] = { 0 };
    snprintf(ipaddr, xmlDetail->httpResult->hostAddrLen, "%s",
             xmlDetail->httpResult->hostAddrStr);

    char https[32] = { 0 };
    snprintf(https, xmlDetail->httpResult->schemeLen, "%s",
             xmlDetail->httpResult->scheme);
    LOGD("https:[%s]\n",https);
    int isHttps = (Common_StrnCmp(https, "https", 5) == 0);
    LOGD("isHttps:[%d]\n",isHttps);
    char url[128] = {0};
    snprintf(url, sizeof(url), "http%s://%s:%d",
             isHttps?"s":"",ipaddr,isHttps?webAttr->httpsPort:webAttr->httpPort);
    LOGD("url:[%s]\n",url);

    if(capability != NULL && strstr(capability,"true") != NULL)
    {
        snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_GET_SERVICE_WITH_CAP_BODY,
                 ONVIF_HTTP_XMLNS,
                 url,url,url,url,url,url,url
                 /*ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort*/);
    }
    else
    {
        snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_GET_SERVICE_BODY,
                 ONVIF_HTTP_XMLNS,
                 url,url,url,url,url,url,url
                 /*ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort,
                 ipaddr, webAttr->httpPort*/);
    }
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void ovfs_ovf_urldecode(char *p)
{
	int i = 0;

	while(*(p+i))
	{
		 if ((*p=*(p+i)) == '%')
		 {
			  *p=*(p+i+1) >= 'A' ? ((*(p+i+1) & 0XDF) - 'A') + 10 : (*(p+i+1) - '0');
			  *p=(*p) * 16;
			  *p+=*(p+i+2) >= 'A' ? ((*(p+i+2) & 0XDF) - 'A') + 10 : (*(p+i+2) - '0');
			  i+=2;
		 }
		 else if (*(p+i)=='+')
		 {
			  *p=' ';
		 }
		 p++;
	}
	*p='\0';

	return ;
}

void HttpRsp_Scope(char *buf, int len,ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *country, char *city, char *name)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SCOPE_BODY, ONVIF_HTTP_XMLNS,country, city, name);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SetScope(char *buf, int len,
								ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *outName)
{
	XmlParser_SetScope(xmlDetail,outName);

	memset(buf, 0, len);
	char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
	char *httpbody	= (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
	memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
	memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SET_SCOPE_BODY, ONVIF_HTTP_XMLNS);
	snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		 	(int)strlen(httpbody));

	snprintf(buf, len, "%s%s", httpheader, httpbody);

	ONVIF_FREE(httpheader);
	ONVIF_FREE(httpbody);
}

int XmlParser_SetScope(ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *outName)
{
	mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;
	mxml_node_t *tmpScope = NULL;
	tmpScope = mxmlFindElement(param, param, "Scopes", NULL, NULL, MXML_DESCEND_ALL);
	if (tmpScope == NULL)
	{
		return -1;
	}
	char* subStr = NULL;
	while(tmpScope)
	{
		char* txtStr = (char *)mxmlGetText(tmpScope, NULL);
		if(txtStr && (subStr = strstr(txtStr,"onvif://www.onvif.org/name/")))
		{
			if(subStr)
				subStr += strlen("onvif://www.onvif.org/name/");
			LOGD("XmlParser_SetScopeParam:name=[%s]\n",subStr);
		}
		//tmpScope = tmpScope->next;
        tmpScope = mxmlWalkNext(tmpScope, param, MXML_DESCEND_ALL);
	}
	if(subStr == NULL)
		return -1;

	int nameLen = strlen(subStr);
	char* nameP = malloc(nameLen+1);
	memset(nameP,0,nameLen+1);
	strcpy(nameP,subStr);

	ovfs_ovf_urldecode(nameP);			//URL解码
	RestOnvif_SetScope(nameP);
	strcpy(outName,nameP);

	return 0;
}

void HttpRsp_Dns(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                 ONVIF_NETWORK_ATTR_T *networkAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_DNS_BODY,
             ONVIF_HTTP_XMLNS, networkAttr->Dns1V4, networkAttr->Dns2V4);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_VideoSource(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                         ONVIF_VI_ATTR_T *viAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actionNs[64] = {0};
    snprintf(actionNs,sizeof(actionNs),"%s",
        (Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)?"tr2":"trt");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_VIDEO_SOURCE_BODY, ONVIF_HTTP_XMLNS,
        actionNs,actionNs,viAttr->Fps, viAttr->Width, viAttr->Height,actionNs,actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_VideoSourceCfg(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                            ONVIF_VI_ATTR_T *viAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_VIDEO_SOURCE_CFG_BODY,
             ONVIF_HTTP_XMLNS, viAttr->Width, viAttr->Height);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetOsds(char *buf, int len,
                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                     ONVIF_OSD_ATTR_T *osdAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char *token1  = NULL;
    char *token2  = NULL;
    char *token3  = NULL;
    char channelString1[128] = {0};

    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);
    OsdString2HttpString(osdAttr->channelOsdAttr.osdString,
                         strlen(osdAttr->channelOsdAttr.osdString),
                         sizeof(channelString1), channelString1);

    memset(osdAttr->channelOsdAttr.osdString, 0, sizeof(osdAttr->channelOsdAttr.osdString));
    snprintf(osdAttr->channelOsdAttr.osdString, sizeof(osdAttr->channelOsdAttr.osdString),
             "%s", channelString1);

    char *xmlns = (char *)mxmlElementGetAttr((mxml_node_t *)xmlDetail->xmlParam, "xmlns");
    if(NULL != xmlns && NULL != strstr(xmlns, "ver20"))
    {
        snprintf(xmlDetail->actionNs, sizeof(xmlDetail->actionNs), "tr2");

        if(osdAttr->datetimeOsdAttr.osdX > 0)
        {
            osdAttr->datetimeOsdAttr.osdX = 1.0;
        }
        if(osdAttr->datetimeOsdAttr.osdY > 0)
        {
            osdAttr->datetimeOsdAttr.osdY = 1.0;
        }
    }

    if (osdAttr->channelOsdAttr.osdEnable > 0)
    {
        token1  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token1, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
                 xmlDetail->actionNs, "s", "ChannelOSD",
                 osdAttr->channelOsdAttr.osdX, osdAttr->channelOsdAttr.osdY,
                 osdAttr->channelOsdAttr.osdSize / 10,
                 osdAttr->channelOsdAttr.osdString, xmlDetail->actionNs, "s");
    }

    if (osdAttr->datetimeOsdAttr.osdEnable > 0)
    {
        token2  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token2, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_2,
                 xmlDetail->actionNs, "s", "DateTimeOSD",
                 osdAttr->datetimeOsdAttr.osdX, osdAttr->datetimeOsdAttr.osdY,
                 osdAttr->datetimeOsdAttr.osdDataFormat->formatStr,
                 osdAttr->datetimeOsdAttr.osdTimeFormat->formatStr,
                 osdAttr->datetimeOsdAttr.osdSize / 10,
                 xmlDetail->actionNs, "s");
    }

#if (defined ONVIF_EXT_CUSTOM_OSD)
    /*FSAN 版本ONVIF 返回osd需要三行自定义， 一直开启，不能关闭，可以读写配置*/
    char *tokenCust0 = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *tokenCust1 = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *tokenCust2 = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    snprintf(tokenCust0, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
             xmlDetail->actionNs, "s", "CustomOSD0",
             osdAttr->custOsdAttr[0].osdX, osdAttr->custOsdAttr[0].osdY,
             osdAttr->custOsdAttr[0].osdSize / 10,
             osdAttr->custOsdAttr[0].osdString, xmlDetail->actionNs, "s");
    snprintf(tokenCust1, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
             xmlDetail->actionNs, "s", "CustomOSD1",
             osdAttr->custOsdAttr[1].osdX, osdAttr->custOsdAttr[1].osdY,
             osdAttr->custOsdAttr[1].osdSize / 10,
             osdAttr->custOsdAttr[1].osdString, xmlDetail->actionNs, "s");
    snprintf(tokenCust2, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
             xmlDetail->actionNs, "s", "CustomOSD2",
             osdAttr->custOsdAttr[2].osdX, osdAttr->custOsdAttr[2].osdY,
             osdAttr->custOsdAttr[2].osdSize / 10,
             osdAttr->custOsdAttr[2].osdString, xmlDetail->actionNs, "s");
#endif
    if (osdAttr->multiOsdAttr.osdEnable > 0)
    {
        token3  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token3, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
                 xmlDetail->actionNs, "s", "MultiOSD",
                 osdAttr->multiOsdAttr.osdX, osdAttr->multiOsdAttr.osdY,
                 osdAttr->multiOsdAttr.osdSize / 10,
                 osdAttr->multiOsdAttr.osdString, xmlDetail->actionNs, "s");
    }

#if (defined ONVIF_EXT_CUSTOM_OSD)
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_OSD_2_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, (token1 != NULL) ? token1 : "",
             (token2 != NULL) ? token2 : "",
             (tokenCust0 != NULL) ? tokenCust0 : "",
             (tokenCust1 != NULL) ? tokenCust1 : "",
             (tokenCust2 != NULL) ? tokenCust2 : "",
             (token3 != NULL) ? token3 : ""
             , xmlDetail->actionNs);

    if (tokenCust0)
        ONVIF_FREE(tokenCust0);
    if (tokenCust1)
        ONVIF_FREE(tokenCust1);
    if (tokenCust2)
        ONVIF_FREE(tokenCust2);

#else
    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_OSD_2_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, (token1 != NULL) ? token1 : "",
             (token2 != NULL) ? token2 : "", (token3 != NULL) ? token3 : ""
             , xmlDetail->actionNs);
#endif
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    if (token1)
        ONVIF_FREE(token1);
    if (token2)
        ONVIF_FREE(token2);

    if (token3)
        ONVIF_FREE(token3);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SetOsds(char *buf, int len,
                     ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char *xmlns = (char *)mxmlElementGetAttr((mxml_node_t *)xmlDetail->xmlParam, "xmlns");
    if(NULL != xmlns && NULL != strstr(xmlns, "ver20"))
    {
        snprintf(xmlDetail->actionNs, sizeof(xmlDetail->actionNs), "tr2");

        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SET_OSD_2_BODY_ver20,
                 ONVIF_HTTP_XMLNS);
    }
    else
    {
        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SET_OSD_2_BODY,
                 ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs);
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_ImageSet(char *buf, int len, ONVIF_IMAGE_SET_T *image,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_IMAGE_SET_BODY,
             ONVIF_HTTP_XMLNS,
             image->brightness, image->saturation, image->contrast, image->sharpness);
    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_Options(char *buf, int len, ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GET_OPTIONS_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_SetImaging(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SET_IMAGING_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetVideoEncoderConfigurationOptions(char *buf, int len,
        ONVIF_VideoEncoderCapability_T *video,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    char *strAbility  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char *options  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MAX_MSG_BUF_LEN);
    memset(strAbility, 0, ONVIF_MSG_BUF_LEN);
    memset(options, 0, ONVIF_MSG_BUF_LEN);

    int isMedia20 = 0;
    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        LOGW("IS Media20\n");
        isMedia20 = 1;
    }

    if (isMedia20)//strstr(xmlDetail->actionNs , "ns1") != NULL)
    {
        int iLen = 0;
        int iLoop = 0;

        for(iLoop = 0; iLoop < COMMON_ARRAY_ELEMENT_COUNT(video->resolution); iLoop++)
        {
            if(video->resolution[iLoop][0] != 0 && video->resolution[iLoop][1] != 0)
            {
                iLen += snprintf(strAbility + iLen, ONVIF_MSG_BUF_LEN - iLen, ResolutionsStr2,
                                 video->resolution[iLoop][0], video->resolution[iLoop][1]);
            }
        }

        char fpsStr[256] = {0};
        iLen = 0;
        for(iLoop = video->fpsRange[1]; iLoop >= video->fpsRange[0]; iLoop--)
        {
            iLen += snprintf(fpsStr + iLen, sizeof(fpsStr) - iLen, "%d%s", iLoop,
                iLoop == video->fpsRange[0]?"":" ");
        }

        iLen = 0;
        for(iLoop = 0; iLoop < COMMON_ARRAY_ELEMENT_COUNT(video->encType); iLoop++)
        {
            char encoding[16] = {0};

            if (strstr(video->encType[iLoop], "H264") != NULL)
            {
                snprintf(encoding, sizeof(encoding), "H264");
            }
            else if (strstr(video->encType[iLoop], "H265+") != NULL)
            {
                snprintf(encoding, sizeof(encoding), "H265+");
            }
            else if (strstr(video->encType[iLoop], "H265") != NULL)
            {
                snprintf(encoding, sizeof(encoding), "H265");
            }
            else if (strstr(video->encType[iLoop], "MJPEG") != NULL)
            {
                snprintf(encoding, sizeof(encoding), "JPEG");
            }
            else
            {
                break;
            }

            iLen += snprintf(options + iLen, ONVIF_MSG_BUF_LEN - iLen,
                             ONVIF_HTTP_GetVideoEncoderConfigurationOptions_2,
                             video->Iinterval[0], video->Iinterval[1],
                             fpsStr,
                             encoding,
                             video->encQuality[0], video->encQuality[1],
                             strAbility,
                             video->bitrateRange[0], video->bitrateRange[1]);
        }

        snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN - iLen,
                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_2_BODY,
                 ONVIF_HTTP_XMLNS,
                 options);
    }
    else
    {
        int iLen = 0;
        int iLoop = 0;
        for(iLoop = 0; iLoop < COMMON_ARRAY_ELEMENT_COUNT(video->resolution); iLoop++)
        {
            if(video->resolution[iLoop][0] != 0 && video->resolution[iLoop][1] != 0)
            {
                iLen += snprintf(strAbility + iLen, ONVIF_MSG_BUF_LEN - iLen, ResolutionsStr,
                                 video->resolution[iLoop][0], video->resolution[iLoop][1]);
            }
        }

        char *enc  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        char *extension  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        memset(enc, 0, ONVIF_MSG_BUF_LEN);

        iLen = 0;
        for(iLoop = 0; iLoop < COMMON_ARRAY_ELEMENT_COUNT(video->encType); iLoop++)
        {
            if (strstr(video->encType[iLoop], "H264") != NULL)
            {
                iLen += snprintf(enc + iLen, ONVIF_MSG_BUF_LEN - iLen,
                                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_H264,
                                 strAbility,
                                 video->Iinterval[0], video->Iinterval[1],
                                 video->fpsRange[0], video->fpsRange[1],
                                 video->bitrateRange[0], video->bitrateRange[1]);
            }
            /*else if (strstr(video->encType[iLoop], "H265") != NULL)
            {
                iLen += snprintf(enc + iLen, ONVIF_MSG_BUF_LEN,
                                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_H264,
                                 strAbility,
                                 video->Iinterval[0], video->Iinterval[1],
                                 video->fpsRange[0], video->fpsRange[1],
                                 video->bitrateRange[0], video->bitrateRange[1]);

            }*/
            else if (strstr(video->encType[iLoop], "MJPEG") != NULL)
            {
                iLen += snprintf(enc + iLen, ONVIF_MSG_BUF_LEN - iLen,
                                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_MJPEG,
                                 strAbility,
                                 video->fpsRange[0], video->fpsRange[1],
                                 video->bitrateRange[0], video->bitrateRange[1]);
            }
            else
            {
                break;
            }
        }

        memset(extension, 0, ONVIF_MSG_BUF_LEN);
        snprintf(extension, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_Extension, enc);

        snprintf(options, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions,
                 video->encQuality[0], video->encQuality[1],
                 enc,
                 extension);

        ONVIF_FREE(enc);
        ONVIF_FREE(extension);

        snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN,
                 ONVIF_HTTP_GetVideoEncoderConfigurationOptions_BODY,
                 ONVIF_HTTP_XMLNS,
                 options);
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));

    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(strAbility);
    ONVIF_FREE(options);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_SetVideoEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);


    char actionNs[64] = {0};
    snprintf(actionNs,sizeof(actionNs),"%s",
        (Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)?"tr2":"trt");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_SetVideoEncoderConfiguration_BODY,
             ONVIF_HTTP_XMLNS, actionNs, actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_SetNetworkInterfaces(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SetNetworkInterfaces_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_SetNetworkDefaultGateway(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SetNetworkDefaultGateway_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}


void HttpRsp_GetOSDOptions(char *buf, int len,
                           ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_GET_OSDOPTIONS,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs,
             xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetVideoEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        int devNo, int channelNo, int streamNo, ONVIF_VideoEncoderCfg_T *pVideCfg)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    /*media1 version , need set encode format to H264*/
    if (strstr(xmlDetail->actionNs, "trt") != NULL)
    {
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATION,
                 ONVIF_HTTP_XMLNS,
                 xmlDetail->actionNs, xmlDetail->actionNs, "",
                 devNo + 1, channelNo + 1, streamNo + 1,
                 streamNo?"SubStream":"MainStream",
                 pVideCfg->encodeFormat == 0 ? "H264" : ((pVideCfg->encodeFormat == 1) ? "H264" :
                         (pVideCfg->encodeFormat == 8) ? "H264" : "MJPEG"),
                 pVideCfg->resolution[0],
                 pVideCfg->resolution[1],
                 pVideCfg->encQuality,
                 pVideCfg->resolution[2],
                 pVideCfg->bitrate,
                 pVideCfg->Iinterval,
                 pVideCfg->profiles == 0 ? "Baseline" : ((pVideCfg->profiles == 1) ? "Main" :
                         "High"),
                 streamNo == 0?"238.255.0.2":"238.255.0.3",
                 streamNo == 0?28080:28084,
                 xmlDetail->actionNs, xmlDetail->actionNs
                );
    }
    else
    {
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATION,
                 ONVIF_HTTP_XMLNS,
                 xmlDetail->actionNs, xmlDetail->actionNs, "2",
                 devNo + 1, channelNo + 1, streamNo + 1,
                 streamNo?"SubStream":"MainStream",
                 pVideCfg->encodeFormat == 0 ? "H264" : ((pVideCfg->encodeFormat == 1) ? "H265" :
                         (pVideCfg->encodeFormat == 8) ? "H265" : "MJPEG"),
                 pVideCfg->resolution[0],
                 pVideCfg->resolution[1],
                 pVideCfg->encQuality,
                 pVideCfg->resolution[2],
                 pVideCfg->bitrate,
                 pVideCfg->Iinterval,
                 pVideCfg->profiles == 0 ? "Baseline" : ((pVideCfg->profiles == 1) ? "Main" :
                         "High"),
                 xmlDetail->actionNs, xmlDetail->actionNs
                );
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetVideoEncoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_VideoEncoderCfg_T *pVideCfg, int streamNo)
{

    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char *httpbodyitem1  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN / 2);
    char *httpbodyitem2  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN / 2);

    ONVIF_VideoEncoderCfg_T *pOf = pVideCfg;

    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);
    memset(httpbodyitem1, 0, ONVIF_MSG_BUF_LEN / 2);
    memset(httpbodyitem2, 0, ONVIF_MSG_BUF_LEN / 2);

    int isMedia20 = 0;

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        LOGW("IS Media20\n");
        isMedia20 = 1;
    }

    /*media1 version , need set encode format to H264*/
    //if (strstr(xmlDetail->actionNs, "trt") != NULL)
    if(!isMedia20)
    {
        snprintf(xmlDetail->actionNs,sizeof(xmlDetail->actionNs),"trt");
        if(streamNo)
        {
            if(streamNo == 2)pOf ++ ;
            snprintf(httpbodyitem1, ONVIF_MSG_BUF_LEN / 2,
                     ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                     xmlDetail->actionNs, "",
                     1, 1, streamNo, pOf->Iinterval, streamNo-1?"SubStream":"MainStream",
                     pOf->encodeFormat == 0 ?
                     "H264" : ((pOf->encodeFormat == 1) ?
                               "H264" :
                               (pOf->encodeFormat == 8) ?
                               "H264" : "MJPEG"),
                     pOf->resolution[0],
                     pOf->resolution[1],
                     pOf->encQuality,
                     pOf->resolution[2],
                     pOf->bitrate,
                     xmlDetail->actionNs
                    );
        }
        else
        {
            snprintf(httpbodyitem1, ONVIF_MSG_BUF_LEN / 2,
                     ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                     xmlDetail->actionNs, "",
                     1, 1, 1, pOf->Iinterval, "MainStream",
                     pOf->encodeFormat == 0 ?
                     "H264" : ((pOf->encodeFormat == 1) ?
                               "H264" :
                               (pOf->encodeFormat == 8) ?
                               "H264" : "MJPEG"),
                     pOf->resolution[0],
                     pOf->resolution[1],
                     pOf->encQuality,
                     pOf->bitrateCtrlMode == 0 ? "false" : "true",
                     pOf->resolution[2],
                     pOf->bitrate,
                     xmlDetail->actionNs
                    );
            pOf ++ ;
            snprintf(httpbodyitem2, ONVIF_MSG_BUF_LEN / 2,
                     ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                     xmlDetail->actionNs, "",
                     1, 1, 2, pOf->Iinterval , "SubStream",
                     pOf->encodeFormat == 0 ?
                     "H264" : ((pOf->encodeFormat == 1) ?
                               "H264" :
                               (pOf->encodeFormat == 8) ?
                               "H264" : "MJPEG"),
                     pOf->resolution[0],
                     pOf->resolution[1],
                     pOf->encQuality,
                     pOf->bitrateCtrlMode == 0 ? "false" : "true",
                     pOf->resolution[2],
                     pOf->bitrate,
                     xmlDetail->actionNs
                    );
        }
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS,
                 ONVIF_HTTP_XMLNS,                  xmlDetail->actionNs,
                 httpbodyitem1, httpbodyitem2,
                 xmlDetail->actionNs
                );
    }
    else
    {
        //if(xmlDetail->actionNs==NULL || strlen(xmlDetail->actionNs) == 0)
        {
            snprintf(xmlDetail->actionNs,sizeof(xmlDetail->actionNs),"tr2");
        }

        if(streamNo)
        {
            if(streamNo == 2)pOf ++ ;
            snprintf(httpbodyitem1, ONVIF_MSG_BUF_LEN / 2,
                     ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                     xmlDetail->actionNs, "2",
                     1, 1, streamNo, pOf->Iinterval, streamNo-1?"SubStream":"MainStream",
                     pOf->encodeFormat == 0 ?
                     "H264" : ((pOf->encodeFormat == 1) ?
                               "H265" :
                               (pOf->encodeFormat == 8) ?
                               "H265" : "MJPEG"),
                     pOf->resolution[0],
                     pOf->resolution[1],
                     pOf->encQuality,
                     pOf->bitrateCtrlMode == 0 ? "false" : "true",
                     pOf->resolution[2],
                     pOf->bitrate,
                     xmlDetail->actionNs
                    );
        }
        else
        {
            snprintf(httpbodyitem1, ONVIF_MSG_BUF_LEN / 2,
                         ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                         xmlDetail->actionNs, "2",
                         1, 1, 1, pOf->Iinterval, "MainStream",
                         pOf->encodeFormat == 0 ?
                         "H264" : ((pOf->encodeFormat == 1) ?
                                   "H265" :
                                   (pOf->encodeFormat == 8) ?
                                   "H265" : "MJPEG"),
                         pOf->resolution[0],
                         pOf->resolution[1],
	                     pOf->encQuality,
	                     pOf->bitrateCtrlMode == 0 ? "false" : "true",
                         pOf->resolution[2],
                         pOf->bitrate,
                         xmlDetail->actionNs
                        );
            pOf ++ ;
            snprintf(httpbodyitem2, ONVIF_MSG_BUF_LEN / 2,
                     ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS_ITEM,
                     xmlDetail->actionNs, "2",
                     1, 1, 2, pOf->Iinterval, "SubStream",
                     pOf->encodeFormat == 0 ?
                     "H264" : ((pOf->encodeFormat == 1) ?
                               "H265" :
                               (pOf->encodeFormat == 8) ?
                               "H265" : "MJPEG"),
                     pOf->resolution[0],
                     pOf->resolution[1],
                     /* 704,576, */
                     pOf->encQuality,
                     pOf->bitrateCtrlMode == 0 ? "false" : "true",
                     pOf->resolution[2],
                     pOf->bitrate,
                     xmlDetail->actionNs
                    );
        }
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_VIDEO_ENCODER_CONFIGURATIONS,
                 ONVIF_HTTP_XMLNS,                  xmlDetail->actionNs,
                 httpbodyitem1, httpbodyitem2,
                 xmlDetail->actionNs
                );
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpbodyitem1);
    ONVIF_FREE(httpbodyitem2);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetAudioEncoderConfiguration(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    if (strstr(xmlDetail->actionNs, "trt") != NULL)
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATION,
                 ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs, "",
                 xmlDetail->actionNs, xmlDetail->actionNs);
    else
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATION,
                 ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs, "2",
                 xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetAudioEncoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);


    char actionNs[64] = {0};
    snprintf(actionNs,sizeof(actionNs),"%s",
        (Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)?"tr2":"trt");

    /*if (strstr(xmlDetail->actionNs, "trt") != NULL)
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATIONS,
                 ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs, "",
                 xmlDetail->actionNs, xmlDetail->actionNs);
    else*/
        snprintf(httpbody, ONVIF_MSG_BUF_LEN,
                 ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATIONS,
                 ONVIF_HTTP_XMLNS, actionNs, actionNs, strstr(actionNs,"trt")?"":"2",
                 actionNs, actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}


void HttpRsp_GetAudioEncoderConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actionNs[64] = {0};
    snprintf(actionNs,sizeof(actionNs),"%s",
        (Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)?"tr2":"trt");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_GET_AUDIOENCODER_CONFIGURATION_OPTIONS,
             ONVIF_HTTP_XMLNS, actionNs, actionNs,actionNs, actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetMoveOptions(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_GETMOVEOPTIONS,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetVideoAnalyticsConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    int sizeReturn = 0;
    int blockW = pMotion->blockW;
    int blockH = pMotion->blockH;
    int i, j, iByteNum, iBitNum;
    char *pBase64 = NULL;
    char *strValue = pMotion->rect;
    char str[512] = {0};
    char cell[512] = {0};
    for(i = 0; i < 18; i++)
    {
        for(j = 0; j < 22; j++)
        {
            iByteNum = (i * 22 + j) / 8;
            iBitNum = 7 - ((i * 22 + j) % 8);
            int x = (j * blockW + 11) / 22;
            int y = (i * blockH + 9) / 18;

            if(*(strValue + y * blockW + x) != '0')
                cell[iByteNum] |= (1 << iBitNum);
        }
    }
    /*for (i = 0; i < iByteNum + 1; i ++)
    {
        printf("%x ", cell[i]);
    }
    printf("\n");*/
    sizeReturn = ovfs_onvif_Packbits((unsigned char *)cell, (unsigned char *)str,
                                     (unsigned int)(iByteNum + 1));
    pBase64 = ovfs_onvif_BASE64Encode((const char *)str, sizeReturn);
    //LOGW("Base64:%s\n", pBase64);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_GET_VIDEOANALYTICSCONFIGURATIONS,
             ONVIF_HTTP_XMLNS,
             pMotion->enable == 0 ? 0 : ovfs_ovf_analytics_motion_sensitivity_local2net(
                 pMotion->Sensitivity),
             pBase64);

    if(pBase64 != NULL)
    {
        ONVIF_FREE(pBase64);
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SetVideoAnalyticsConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_SetVideoAnalyticsConfigurations_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_ModifyAnalyticsModules(char *buf, int len,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_ModifyAnalyticsModules_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_ModifyRules(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_ModifyRules_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}


void HttpRsp_SetSystemDateAndTime(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_SET_SYSTEMDATEANDTIME,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetAnalyticsModules(char *buf, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTP_GET_ANALYTICSMODULES,
             ONVIF_HTTP_XMLNS,
             pMotion->enable == 0 ? 0 : ovfs_ovf_analytics_motion_sensitivity_local2net(
                 pMotion->Sensitivity));

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetRules(char *buf, int len,
                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail, ONVIF_MOTION_ATTR_T *pMotion)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    int sizeReturn = 0;
    int blockW = pMotion->blockW;
    int blockH = pMotion->blockH;
    int i, j, iByteNum, iBitNum;
    char *pBase64 = NULL;
    char *strValue = pMotion->rect;
    char str[512] = {0};
    char cell[512] = {0};
    for(i = 0; i < 18; i++)
    {
        for(j = 0; j < 22; j++)
        {
            iByteNum = (i * 22 + j) / 8;
            iBitNum = 7 - ((i * 22 + j) % 8);
            int x = (j * blockW + 11) / 22;
            int y = (i * blockH + 9) / 18;

            if(*(strValue + y * blockW + x) != '0')
                cell[iByteNum] |= (1 << iBitNum);
        }
    }
    /*for (i = 0; i < iByteNum + 1; i ++)
    {
        printf("%x ", cell[i]);
    }
    printf("\n");*/
    sizeReturn = ovfs_onvif_Packbits((unsigned char *)cell, (unsigned char *)str,
                                     (unsigned int)(iByteNum + 1));
    pBase64 = ovfs_onvif_BASE64Encode((const char *)str, sizeReturn);
    //LOGW("Base64:%s\n", pBase64);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN,
             ONVIF_HTTP_GET_RULES,
             ONVIF_HTTP_XMLNS,
             pBase64);

    if(pBase64 != NULL)
    {
        ONVIF_FREE(pBase64);
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Subscribe(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_SUBSCRIBE_NODE_T *subNode, ONVIF_WEB_ATTR_T *webAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char ipaddr[32] = { 0};
    char timeCStr[32] = { 0 };
    char timeTStr[32] = { 0 };
    char uri[64] = { 0 };
    snprintf(ipaddr, xmlDetail->httpResult->hostAddrLen, "%s",
             xmlDetail->httpResult->hostAddrStr);

    time_t curTime = time(NULL);
    TimeT2StrISO8601(&curTime, timeCStr);
    time_t termTime = curTime + 120; //120 seconds
    TimeT2StrISO8601(&termTime, timeTStr);

    snprintf(uri, sizeof(uri), "%s_%u", timeCStr, subNode->token);

    snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_SUBSCRIBE,
             ONVIF_HTTP_XMLNS, ipaddr, webAttr->httpPort, uri,
             timeCStr, timeTStr);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Renew(char *buf, int len,
                   ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char timeCStr[32] = { 0 };
    char timeTStr[32] = { 0 };

    time_t curTime = time(NULL);
    TimeT2StrISO8601(&curTime, timeCStr);
    time_t termTime = curTime + 120; //120 seconds
    TimeT2StrISO8601(&termTime, timeTStr);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_RENEW,
             ONVIF_HTTP_XMLNS,
             timeCStr, timeTStr);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Unsubscribe(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_UNSUBSCRIBE,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetServiceCapability(char *buf, int len,
                                  ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_Service_Capabilities_BODY/*,
             ONVIF_HTTP_XMLNS*/);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetEventProperties(char *buf, int len,
                                ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);

    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(ONVIF_HTTP_GetEventProperties));
    snprintf(buf, len, "%s%s", httpheader, ONVIF_HTTP_GetEventProperties);

    ONVIF_FREE(httpheader);
}

void HttpRsp_CreateOSD(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *fieldName)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char *xmlns = (char *)mxmlElementGetAttr((mxml_node_t *)xmlDetail->xmlParam, "xmlns");
    if(NULL != xmlns && NULL != strstr(xmlns, "ver20"))
    {
        snprintf(xmlDetail->actionNs, sizeof(xmlDetail->actionNs), "tr2");
    }
    else
    {
        snprintf(xmlDetail->actionNs, sizeof(xmlDetail->actionNs), "trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_Create_OSD_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs,
             fieldName, xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_DeleteOSD(char *buf, int len,
                       ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char *xmlns = (char *)mxmlElementGetAttr((mxml_node_t *)xmlDetail->xmlParam, "xmlns");
    if(NULL != xmlns && NULL != strstr(xmlns, "ver20"))
    {
        snprintf(xmlDetail->actionNs, sizeof(xmlDetail->actionNs), "tr2");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_Delete_OSD_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));

    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_CreatePullPointSubscription(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_SUBSCRIBE_NODE_T *subNode, ONVIF_WEB_ATTR_T *webAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MAX_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char ipaddr[32] = { 0};
    char timeCStr[32] = { 0 };
    char timeTStr[32] = { 0 };
    char uri[64] = { 0 };
    snprintf(ipaddr, xmlDetail->httpResult->hostAddrLen, "%s",
             xmlDetail->httpResult->hostAddrStr);

    time_t curTime = time(NULL);
    TimeT2StrISO8601(&curTime, timeCStr);
    time_t termTime = curTime + 120; //120 seconds
    TimeT2StrISO8601(&termTime, timeTStr);

    snprintf(uri, sizeof(uri), "%s_%u", timeCStr, subNode->token);

    snprintf(httpbody, ONVIF_MAX_MSG_BUF_LEN, ONVIF_HTTP_CreatePullPointSubscription,
             ONVIF_HTTP_XMLNS, ipaddr, webAttr->httpPort, uri,
             timeCStr, timeTStr);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}
/*isHappen == -1 , means alarm message need to send , so send a empty content string*/
void HttpRsp_PullMessage(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                         ONVIF_SUBSCRIBE_NODE_T *subNode, int isHappen)
{
    char *msg = NULL;
    char *head = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *body = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char timeStr[32] = { 0 };
    char timeTStr[32] = { 0 };

    time_t curTime = time(NULL);
    TimeT2StrISO8601(&curTime, timeStr);

    time_t termTime = curTime + 120; //120 seconds
    TimeT2StrISO8601(&termTime, timeTStr);

    if (isHappen == 1)
    {
        msg = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr,
                 (subNode->isSend != 1) ? "Initialized" : "Changed", "true",
                 timeStr, "Changed", "true");
        subNode->isSend = 1;
    }
    else if (isHappen == 0)
    {
        msg = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr,
                 "Deleted", "false", timeStr, "Deleted", "false");
    }

    snprintf(body, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PullMessage_BODY,
             ONVIF_HTTP_XMLNS, timeStr, timeTStr, (msg != NULL) ? msg : "");
    snprintf(head, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD, strlen(body));
    snprintf(buf, len, "%s%s", head, body);

    if (msg != NULL)
        ONVIF_FREE(msg);
    ONVIF_FREE(head);
    ONVIF_FREE(body);

}

static int ConnectAndSend(char *ip, int port, char *buffer, int len)
{
    int ret = -1;
    struct sockaddr_in sa;
    struct timeval tm;
    int sockfd = -1;
    sa.sin_family = AF_INET;
    sa.sin_addr.s_addr = inet_addr(ip);
    sa.sin_port = htons(port);

    sockfd = socket(sa.sin_family, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        ret = -1;
        LOGW("socket create failed.\n");
        return -1;
    }

    int flags = fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);

    int const_int_1 = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &const_int_1, sizeof(const_int_1));

    if (connect(sockfd, (struct sockaddr *)&sa, sizeof(struct sockaddr_in)) < 0)
    {
        if (errno == EINPROGRESS)
        {
            tm.tv_sec = 5;
            tm.tv_usec = 0;
            fd_set wset;
            FD_ZERO(&wset);
            FD_SET(sockfd, &wset);
            ret = select(sockfd + 1, NULL, &wset, NULL, &tm);
            if (ret  > 0 && FD_ISSET(sockfd, &wset))
                ret = 0;
            else
            {
                LOGW("connect failed %d\n", ret);
                ret = -1;
            }

        }
    }
    else
    {
        ret = 0;
    }


    if (ret == 0)
    {
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) & O_NONBLOCK); //set back to block mode
        if (send(sockfd, buffer, len, 0) < 0)
        {
            ret = -1;
            LOGW("send error[%d](%s)\n", errno, strerror(errno));
        }
    }

    close(sockfd);
    return ret;
}

int HttpRsp_PostMotionNotify(char *buf, int len,
                             ONVIF_SUBSCRIBE_NODE_T *subNode, int isHappen)
{
    char *msg = NULL;
    char *head = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *body = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char timeStr[32] = { 0 };

    time_t tt = time(NULL);
    TimeT2StrISO8601(&tt, timeStr);

    int ret = 0;

    if (isHappen > 0)
    {
        msg = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        /* snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr, */
        /*          (subNode->isSend == 0) ? "Initialized" : "Changed", "true", */
        /*          timeStr, "Changed", "true"); */
        snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr,
                 (subNode->isSend == 0) ? "Changed" : "Changed", "true",
                 timeStr, "Changed", "true");

    }
    else
    {
        msg = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
        snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr,
                 "Changed", "false", timeStr, "Changed", "false");
        /* snprintf(msg, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_MSG_MOTION, timeStr, */
        /*          "Deleted", "false", timeStr, "Deleted", "false"); */
    }
    snprintf(body, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_NOTIFY_BODY,
             subNode->ip, subNode->port, subNode->uri, (msg != NULL) ? msg : "");
    snprintf(head, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_NOTIFY_HEADER, subNode->uri, subNode->ip,
             subNode->port,
             strlen(body));
    snprintf(buf, len, "%s%s", head, body);
    ret = ConnectAndSend(subNode->ip, subNode->port, buf, strlen(buf));
    subNode->isSend = 1;
    DBGS("%s:%d @%s@\n", subNode->ip, subNode->port, buf);
    if (msg != NULL)
        ONVIF_FREE(msg);
    ONVIF_FREE(head);
    ONVIF_FREE(body);

    return ret;
}

int XmlParser_UdpDiscovery(char *buf, int len,
                           char *maca, char *ip, int httpPort,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    char msgIdStr[64] = { 0 };
    char typesStr[64] = { 0 };
    int ret = -1;

    mxml_node_t *newNode = mxmlNewXML("1.0");
    if (newNode == NULL)
    {
        LOGE("new node is null\n");
        return -1;
    }

    if (mxmlLoadString(newNode, NULL, buf) == NULL)
    {
        LOGE("parser xml string failed\n");
        mxmlDelete(newNode);
        return -1;
    }
    /* printf("%s\n",onvifDiscoveryBuf); */
    mxml_node_t *msgenv = mxmlGetFirstChild(newNode);
    do
    {
        if (msgenv != NULL)
        {
            char *ele_name = mxmlGetElement(msgenv);
            if (ele_name) {
                if (strstr(ele_name, "MessageID") != NULL)
                {
                    /* LOGD("mssage id %s\n",mxmlGetText(msgenv,NULL)); */
                    snprintf(msgIdStr, sizeof(msgIdStr), "%s", mxmlGetText(msgenv, NULL));
                }
                if (strstr(ele_name, "Types") != NULL)
                {
                    /* LOGD("types  %s\n",mxmlGetText(msgenv, NULL)); */
                    snprintf(typesStr, sizeof(typesStr), "%s", mxmlGetText(msgenv, NULL));
                }
            }
        }
        msgenv = mxmlWalkNext(msgenv, newNode, MXML_DESCEND_ALL);
    }
    while(msgenv);

    if (strlen(msgIdStr) == 0 || strlen(typesStr) == 0)
    {
        DBGR("note match msg id and types or has relates id @%s@\n", buf);
        mxmlDelete(newNode);
        return -1;
    }

    char messId[16] = { 0 };
    snprintf(messId, sizeof(messId), "%04x%02x", Common_Rand32(), Common_Rand16());

    char rspTname[64] = {0 };
    char *rspType = strstr(typesStr, ":");
    if (rspType)
        snprintf(rspTname, sizeof(rspTname), "dn:%s", rspType + 1);
    else
        snprintf(rspTname, sizeof(rspTname), "dn:%s", typesStr);

    memset(buf, 0, len);

	if((ptzCapability->ptzInfo.IsOfDome > 0) || (ptzCapability->ptzInfo.LensSupport== 1))
	{
		snprintf(buf, len, ONVIF_SCAN_RSP, messId, msgIdStr, maca, //rspTname,
				"onvif://www.onvif.org/type/ptz",
				ip, httpPort);
	}
	else
	{
		snprintf(buf, len, ONVIF_SCAN_RSP, messId, msgIdStr, maca, //rspTname,
				"",
				ip, httpPort);
	}

    ret = 0;

    mxmlDelete(newNode);
    return ret;
}



static int XmlParserActionAndDetail(mxml_node_t *msgenv,
                                    ONVIF_XML_ACTION_T *xmlAction,
                                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    int actionId = -1, actionCnt = 0, i = 0, nsLen = 0;
    actionCnt = COMMON_ARRAY_ELEMENT_COUNT(s_xml_action);
    char *ele_name = mxmlGetElement(msgenv);
    /*tr2:GetStreamUri tr2 is namespace , GetStreamUri is action name*/
    char *ename = strstr(ele_name, ":");
    if (ename == NULL)
        ename = ele_name;
    else
        ename = ename + 1;

    nsLen = ename - ele_name - 1;

    if (nsLen >= (int)sizeof(xmlDetail->actionNs))
    {
        LOGE("name space is too big %d\n", nsLen);
        return -1;
    }

    for (i = 0; i < actionCnt; i++)
    {
        if (strlen(ename) == strlen(s_xml_action[i].actionName) &&
                strncmp(ename, s_xml_action[i].actionName,
                        strlen(s_xml_action[i].actionName)) == 0)
        {
            LOGD("match id %s %d\n", s_xml_action[i].actionName, s_xml_action[i].actionId);
            actionId = s_xml_action[i].actionId;
            memcpy(xmlAction , &s_xml_action[i], sizeof(ONVIF_XML_ACTION_T));

            if (ename != ele_name)
                strncpy(xmlDetail->actionNs, ele_name, nsLen);

            LOGD("xmlDetail->actionNs = @%s@\n", xmlDetail->actionNs);

            xmlDetail->actionName = s_xml_action[i].actionName;
            xmlDetail->actionId = actionId;
            xmlDetail->xmlParam = msgenv;

            int num_attrs = mxmlElementGetAttrCount(msgenv);
            LOGW("[%s][%d]\n",ename,num_attrs);
            if (num_attrs)
            {
                char *token = (char *)mxmlElementGetAttr(msgenv, "xmlns");
                if (token == NULL)
                    continue;
                LOGW("[%d]token:[%s]\n",num_attrs,token);
                if(strstr(token,"ver20/media"))
                {
                    xmlDetail->media2 = 1;
                    break;
                }
            }

            break;
        }
    }

    return actionId;
}

int XmlParser_FreeXmlDetail(ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    if (xmlDetail != NULL)
    {
        if (xmlDetail->xmlRoot != NULL)
        {
            mxmlDelete((mxml_node_t *)xmlDetail->xmlRoot);
            xmlDetail->xmlRoot = NULL;
        }
    }
    return 0;
}

int XmlParser_OnvifAction(char *buf, int len, ONVIF_AUTH_INFO_T *authResult,
                          ONVIF_XML_ACTION_T *xmlAction,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    int actionId = -1, ret = -1;
    mxml_node_t *newNode = mxmlNewXML("1.0");
    if (newNode == NULL)
    {
        LOGE("new node is null\n");
        return -1;
    }

    if (mxmlLoadString(newNode, NULL, buf) == NULL)
    {
        LOGE("parser xml string failed\n");
        mxmlDelete(newNode);
        return -1;
    }

    xmlDetail->xmlRoot = newNode;
    mxml_node_t *msgenv = mxmlGetFirstChild(newNode);

    do
    {
        if (msgenv == NULL || mxmlGetType(msgenv) != MXML_TYPE_ELEMENT
                || mxmlGetElement(msgenv) == NULL)
        {
            msgenv = mxmlWalkNext(msgenv, newNode, MXML_DESCEND_ALL);
            continue;
        }

         char *ele_name = mxmlGetElement(msgenv);

        if (strstr(ele_name, "Username") != NULL)
        {
            char *tp = (char *)mxmlGetText(msgenv, NULL);
            if (tp != NULL)
            {
                if(authResult->userName)
                {
                    LOGW("free username [%s]\n",authResult->userName);
                    ONVIF_FREE(authResult->userName);
                }
                authResult->userName = ONVIF_STRDUP(tp);
                /* LOGD("username %s\n", authResult->userName); */
            }
        }
        else if (strstr(ele_name, "Nonce") != NULL)
        {
            char *tp = (char *)mxmlGetText(msgenv, NULL);
            if (tp != NULL)
            {
                if(authResult->nonce)
                {
                    ONVIF_FREE(authResult->nonce);
                }
                authResult->nonce =  ONVIF_STRDUP(tp);
                /* LOGD("nonce  %s\n", authResult->nonce); */
            }
        }
        else if (strstr(ele_name, "Password") != NULL)
        {
            char *tp = (char *)mxmlGetText(msgenv, NULL);
            if (tp != NULL)
            {
                if(authResult->passwordDigest)
                {
                    ONVIF_FREE(authResult->passwordDigest);
                }
                authResult->passwordDigest = ONVIF_STRDUP(tp);
                /* LOGD("password  %s\n", authResult->passwordDigest); */
            }
        }
        else if (strstr(ele_name, "Created") != NULL)
        {
            char *tp = (char *)mxmlGetText(msgenv, NULL);
            if (tp != NULL)
            {
                if(authResult->created)
                {
                    ONVIF_FREE(authResult->created);
                }
                authResult->created = ONVIF_STRDUP(tp);
                /* LOGD("created  %s\n", authResult->created); */
            }
        }
        else
        {
            ret = XmlParserActionAndDetail(msgenv, xmlAction, xmlDetail);
            if (ret >= 0)
            {
                actionId = ret;
                break;
            }

        }

        /* LOGW("%s %s\n", msgenv->value.element.name, mxmlGetText(msgenv, NULL)); */

        msgenv = mxmlWalkNext(msgenv, newNode, MXML_DESCEND_ALL);
    }
    while(msgenv);

    /* mxmlDelete(newNode); */

    if (authResult->userName != NULL && authResult->nonce != NULL
            && authResult->passwordDigest != NULL)
    {
        authResult->hasAuth = 1;
        authResult->authMethod = ONVIF_AUTH_TYPE_WS;
    }

    return actionId;
}

int XmlParser_DeleteOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *tmpNode = NULL;

    char EleName[32] = {0};

    snprintf(EleName,31,"%s:OSDToken",xmlDetail->actionNs);
    tmpNode = mxmlFindElement(param, param, EleName, NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
    {
        return -1;
    }

    char *token = (char *)mxmlGetText(tmpNode, NULL);
    if (token == NULL)
        return -1;

    if (strcmp(token, "OSDToken_ChannelOSD") == 0) //ChanName
    {
        osdAttr->channelOsdAttr.osdEnable = 0;
    }

    if (strcmp(token, "OSDToken_DateTimeOSD") == 0) //date time
    {
        osdAttr->datetimeOsdAttr.osdEnable = 0;
    }

    if (strcmp(token, "OSDToken_MultiOSD") == 0)// multi osd
    {
        osdAttr->multiOsdAttr.osdEnable = 0;
    }

    return 0;
}
#if (defined ONVIF_EXT_CUSTOM_OSD)
static int XmlParser_SeCustomOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail, int cusIdx,
                                      mxml_node_t *tmpNode,
                                      ONVIF_OSD_ATTR_T *osdAttr, char *contentStr)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *tNode = NULL;

    tNode = mxmlFindElement(tmpNode, tmpNode, "tt:PlainText", NULL, NULL,
                            MXML_DESCEND_ALL);
    if (tNode == NULL)
        return -1;

    char *cnStart = strstr(contentStr, "<tt:PlainText>");
    char *cnStop = strstr(contentStr, "</tt:PlainText>");
    if (cnStart == NULL || cnStop == NULL)
    {
        LOGE("search PlainText failed\n");
        return -1;
    }

    cnStart += strlen("<tt:PlainText>");
    if (cnStop - cnStart > sizeof(osdAttr->custOsdAttr[cusIdx].osdString) - 1)
        cnStop = cnStart + sizeof(osdAttr->custOsdAttr[cusIdx].osdString) - 1;

    if (cnStop <= cnStart)
    {
        LOGE("search PlainText failed\n");
        return -1;
    }

    tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
    if (tNode != NULL)
    {
        osdAttr->custOsdAttr[cusIdx].osdSize = 10 * atoi(mxmlGetText(tNode, NULL));
    }

    if (osdAttr->custOsdAttr[cusIdx].osdSize > 50)
        osdAttr->custOsdAttr[cusIdx].osdSize = 50;

    char chStr[128] = {};
    snprintf(chStr, cnStop - cnStart + 1, "%s", cnStart);
    HttpString2OsdString(chStr, strlen(chStr),
                         sizeof(osdAttr->custOsdAttr[cusIdx].osdString),
                         osdAttr->custOsdAttr[cusIdx].osdString);

    tNode = mxmlFindElement(param, param, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
    if (tNode != NULL)
    {
        osdAttr->custOsdAttr[cusIdx].osdX = atof(mxmlElementGetAttr(tNode, "x"));
        osdAttr->custOsdAttr[cusIdx].osdY = atof(mxmlElementGetAttr(tNode, "y"));

    }
    osdAttr->custOsdAttr[cusIdx].osdEnable = 1;
    return 0;
}
#endif

int XmlParser_SetOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                          ONVIF_OSD_ATTR_T *osdAttr, char *contentStr)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *tmpNode = NULL, *tNode = NULL;
    char EleName[32] = {0};
    if(strlen(xmlDetail->actionNs))
    {
        snprintf(EleName,31,"%s:OSD",xmlDetail->actionNs);
    }
    else
    {
        snprintf(EleName,31,"OSD");
    }
    tmpNode = mxmlFindElement(param, param, EleName, NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
    {
        return -1;
    }

    int i = 0;
    int num_attrs = mxmlElementGetAttrCount(tmpNode);
    for (i = 0; i < num_attrs; i++)
    {
        char *token = (char *)mxmlElementGetAttr(tmpNode, "token");
        if (token == NULL)
            continue;

        if (strcmp(token, "OSDToken_ChannelOSD") == 0 || strcmp(token, "ChanName") == 0)
        {
            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:PlainText", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "PlainText", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode == NULL)
                continue;

            char *cnStart = strstr(contentStr, "PlainText>");
            if (cnStart == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            cnStart += strlen("PlainText>");

            char *cnStop = strstr(cnStart, "<");
            if (cnStop == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop - cnStart > sizeof(osdAttr->channelOsdAttr.osdString) - 1)
                cnStop = cnStart + sizeof(osdAttr->channelOsdAttr.osdString) - 1;

            if (cnStop < cnStart)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            char chStr[128] = {};
            memset(osdAttr->channelOsdAttr.osdString, 0,
                   sizeof(osdAttr->channelOsdAttr.osdString));

            /*if cnStop == cnStart 说明Txt是空的*/
            if (cnStop != cnStart)
            {
                snprintf(chStr, cnStop - cnStart + 1, "%s", cnStart);
                HttpString2OsdString(chStr, strlen(chStr),
                                     sizeof(osdAttr->channelOsdAttr.osdString),
                                     osdAttr->channelOsdAttr.osdString);
            }

            tNode = mxmlFindElement(param, param, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "Pos", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->channelOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->channelOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }

            tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "FontSize", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                LOGE("fon size %d\n", atoi(mxmlGetText(tNode, NULL)));
                osdAttr->channelOsdAttr.osdSize = 10 * atoi(mxmlGetText(tNode, NULL));
            }

            if (osdAttr->channelOsdAttr.osdSize > 50)
                osdAttr->channelOsdAttr.osdSize = 50;

            osdAttr->channelOsdAttr.osdEnable = 1;
        }
        //SetOsdParam的DateTime设值
        else if (strcmp(token, "OSDToken_DateTimeOSD") == 0 || strcmp(token, "DateTime") == 0)
        {
            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "Pos", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->datetimeOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->datetimeOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }

            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:DateFormat", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "DateFormat", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                char *dformat  = (char *)mxmlGetText(tNode, NULL);
                osdAttr->datetimeOsdAttr.osdDataFormat = RestOnvif_ParserDate(dformat, 0);
            }

            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:TimeFormat", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "TimeFormat", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                char *hformat  = (char *)mxmlGetText(tNode, NULL);
                osdAttr->datetimeOsdAttr.osdTimeFormat = RestOnvif_ParserHour(hformat, 0);
            }
            osdAttr->datetimeOsdAttr.osdEnable = 1;
        }
        else if (strcmp(token, "OSDToken_MultiOSD") == 0) //multi osd
        {
            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:PlainText", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "PlainText", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode == NULL)
                continue;

            char *cnStart = strstr(contentStr, "PlainText>");
            if (cnStart == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            cnStart += strlen("PlainText>");

            char *cnStop = strstr(cnStart, "<");
            if (cnStop == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop - cnStart > sizeof(osdAttr->multiOsdAttr.osdString) - 1)
                cnStop = cnStart + sizeof(osdAttr->multiOsdAttr.osdString) - 1;

            if (cnStop <= cnStart)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "FontSize", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->multiOsdAttr.osdSize = 10 * atoi(mxmlGetText(tNode, NULL));
            }

            if (osdAttr->multiOsdAttr.osdSize > 50)
                osdAttr->multiOsdAttr.osdSize = 50;

            char chStr[128] = {};
            snprintf(chStr, cnStop - cnStart + 1, "%s", cnStart);
            HttpString2OsdString(chStr, strlen(chStr),
                                 sizeof(osdAttr->multiOsdAttr.osdString),
                                 osdAttr->multiOsdAttr.osdString);

            tNode = mxmlFindElement(param, param, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if(NULL == tNode)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "Pos", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->multiOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->multiOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }
            osdAttr->multiOsdAttr.osdEnable = 1;
        }
#if (defined ONVIF_EXT_CUSTOM_OSD)
        else if (strcmp(token, "OSDToken_CustomOSD0") == 0)
        {
            XmlParser_SeCustomOsdParam(xmlDetail, 0, tmpNode, osdAttr, contentStr);
        }
        else if (strcmp(token, "OSDToken_CustomOSD1") == 0)
        {
            XmlParser_SeCustomOsdParam(xmlDetail, 1, tmpNode, osdAttr, contentStr);
        }
        else if (strcmp(token, "OSDToken_CustomOSD2") == 0)
        {
            XmlParser_SeCustomOsdParam(xmlDetail, 2, tmpNode, osdAttr, contentStr);
        }
#endif
    }

    return 0;
}

int XmlParser_CreateOsdParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr, char *contentStr)
{
    int type = -1;
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *tmpNode = NULL, *tNode = NULL;
    char EleName[32] = {0};
    if(strlen(xmlDetail->actionNs))
    {
    snprintf(EleName,31,"%s:OSD",xmlDetail->actionNs);
    }
    else
    {
        snprintf(EleName,31,"OSD");
    }

    tmpNode = mxmlFindElement(param, param, EleName, NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
    {
        return -1;
    }

    int i = 0;
    int num_attrs = mxmlElementGetAttrCount(tmpNode);    
    for (i = 0; i < num_attrs; i++)
    {
        char *token = (char *)mxmlElementGetAttr(tmpNode, "token");
        if (token == NULL)
            continue;

        tNode = mxmlFindElement(tmpNode, tmpNode, "tt:PlainText", NULL, NULL,
                                MXML_DESCEND_ALL);
        if(NULL == tNode)
        {
            tNode = mxmlFindElement(tmpNode, tmpNode, "PlainText", NULL, NULL,
                                    MXML_DESCEND_ALL);
        }

        if (mxmlFindElement(tmpNode, tmpNode, "tt:DateFormat", NULL, NULL, MXML_DESCEND_ALL) != NULL ||
                mxmlFindElement(tmpNode, tmpNode, "DateFormat", NULL, NULL, MXML_DESCEND_ALL) != NULL)
        {
            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if (tNode != NULL)
            {
                osdAttr->datetimeOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->datetimeOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }

            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:DateFormat", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == NULL)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "DateFormat", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                char *dformat  = (char *)mxmlGetText(tNode, NULL);
                osdAttr->datetimeOsdAttr.osdDataFormat = RestOnvif_ParserDate(dformat, 0);
            }

            tNode = mxmlFindElement(tmpNode, tmpNode, "tt:TimeFormat", NULL, NULL,
                                    MXML_DESCEND_ALL);
            if(NULL == NULL)
            {
                tNode = mxmlFindElement(tmpNode, tmpNode, "TimeFormat", NULL, NULL,
                                        MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                char *hformat  = (char *)mxmlGetText(tNode, NULL);
                osdAttr->datetimeOsdAttr.osdTimeFormat = RestOnvif_ParserHour(hformat, 0);
            }

            tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
            if(tNode == NULL)
            {
                tNode = mxmlFindElement(param, param, "FontSize", NULL, NULL, MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->datetimeOsdAttr.osdSize =
                    10 * atoi(mxmlGetText(tNode, NULL));
            }

            if (osdAttr->datetimeOsdAttr.osdSize > 50)
                osdAttr->datetimeOsdAttr.osdSize = 50;

            osdAttr->datetimeOsdAttr.osdEnable = 1;

            type = 1;
        }
        else if (tNode != NULL && osdAttr->channelOsdAttr.osdEnable != 1)
        {
            char *cnStart = strstr(contentStr, "PlainText>");
            if (cnStart == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            cnStart += strlen("PlainText>");

            char *cnStop = strstr(cnStart, "<");
            if (cnStop == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop - cnStart > sizeof(osdAttr->channelOsdAttr.osdString) - 1)
                cnStop = cnStart + sizeof(osdAttr->channelOsdAttr.osdString) - 1;

            if (cnStop < cnStart)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop > cnStart)
            {
                snprintf(osdAttr->channelOsdAttr.osdString, cnStop - cnStart + 1, "%s", cnStart);
            }
            else
            {
                memset(osdAttr->channelOsdAttr.osdString, 0, sizeof(osdAttr->channelOsdAttr.osdString));
            }

            tNode = mxmlFindElement(param, param, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if(tNode == NULL)
            {
                tNode = mxmlFindElement(param, param, "Pos", NULL, NULL, MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->channelOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->channelOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }

            tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
            if(tNode == NULL)
            {
                tNode = mxmlFindElement(param, param, "FontSize", NULL, NULL, MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->channelOsdAttr.osdSize = 10 * atoi(mxmlGetText(tNode, NULL));
            }

            if (osdAttr->channelOsdAttr.osdSize > 50)
                osdAttr->channelOsdAttr.osdSize = 50;

            osdAttr->channelOsdAttr.osdEnable = 1;

            type = 0;
        }
        else if (tNode != NULL && osdAttr->multiOsdAttr.osdEnable != 1)
        {
            char *cnStart = strstr(contentStr, "PlainText>");
            if (cnStart == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            cnStart += strlen("PlainText>");

            char *cnStop = strstr(cnStart, "<");
            if (cnStop == NULL)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop - cnStart > sizeof(osdAttr->multiOsdAttr.osdString) - 1)
                cnStop = cnStart + sizeof(osdAttr->multiOsdAttr.osdString) - 1;

            if (cnStop < cnStart)
            {
                LOGE("search PlainText failed\n");
                continue;
            }

            if (cnStop > cnStart)
            {
                snprintf(osdAttr->multiOsdAttr.osdString, cnStop - cnStart + 1, "%s", cnStart);
            }
            else
            {
                memset(osdAttr->multiOsdAttr.osdString, 0, sizeof(osdAttr->multiOsdAttr.osdString));
            }


            tNode = mxmlFindElement(param, param, "tt:FontSize", NULL, NULL, MXML_DESCEND_ALL);
            if(tNode == NULL)
            {
                tNode = mxmlFindElement(param, param, "FontSize", NULL, NULL, MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->multiOsdAttr.osdSize =
                    10 * atoi(mxmlGetText(tNode, NULL));
            }

            if (osdAttr->multiOsdAttr.osdSize > 50)
                osdAttr->multiOsdAttr.osdSize = 50;

            tNode = mxmlFindElement(param, param, "tt:Pos", NULL, NULL, MXML_DESCEND_ALL);
            if(tNode == NULL)
            {
                tNode = mxmlFindElement(param, param, "Pos", NULL, NULL, MXML_DESCEND_ALL);
            }
            if (tNode != NULL)
            {
                osdAttr->multiOsdAttr.osdX = atof(mxmlElementGetAttr(tNode, "x"));
                osdAttr->multiOsdAttr.osdY = atof(mxmlElementGetAttr(tNode, "y"));
            }
            osdAttr->multiOsdAttr.osdEnable = 1;

            type = 2;
        }
    }
    return type;
}


typedef struct
{
    int TZ;
    int bias_enable;
    int bias;
} LOCAL_TZ_T;

typedef struct
{
    int mon;
    int weekIdx;
    int weekDay;
    int hour;
    int min;
    int sec;
} LOCAL_DayTime_T;

typedef struct
{
    int set;    // 1-set 0-unset
    int enable;
    int mode;
    int Bias;   // min
    LOCAL_DayTime_T start;
    LOCAL_DayTime_T end;
} LOCAL_DST_T;

typedef struct
{
    int sign;
    int offsetHour;
    int offsetMinute;
} NET_TZ_T;


typedef struct TZ_NODE
{
    ovfs_time_zone tz_name;
    int time_offset;
} TZ_NODE_T;

TZ_NODE_T arry_zone[] =
{
    {OVFS_TIME_ZONE_WEST_LINE, -1200},
    {OVFS_TIME_ZONE_SAMOA, -1100},
    {OVFS_TIME_ZONE_HAWAII, -1000},
    {OVFS_TIME_ZONE_ALASKA, -900},
    {OVFS_TIME_ZONE_PACIFIC_OCEAN, -800},
    {OVFS_TIME_ZONE_MOUNTAIN, -700},
    {OVFS_TIME_ZONE_CENTRAL_CANADA, -600},
    {OVFS_TIME_ZONE_EASTERN_TIME_CANADA, -500},
    {OVFS_TIME_ZONE_CARACAS, -430},
    {OVFS_TIME_ZONE_ATLANTIC_CANADA, -400},
    {OVFS_TIME_ZONE_NEWFOUNDLAND, -330},
    {OVFS_TIME_ZONE_GEORGETOWN, -300},
    {OVFS_TIME_ZONE_ATLANTIC_OCEAN, -200},
    {OVFS_TIME_ZONE_ANGLE_ISLANDS, -100},
    {OVFS_TIME_ZONE_GREENWICH, 0},
    {OVFS_TIME_ZONE_AMSTERDAM, 100},
    {OVFS_TIME_ZONE_ATHENS, 200},
    {OVFS_TIME_ZONE_BAGHDAD, 300},
    {OVFS_TIME_ZONE_TEHERAN, 330},
    {OVFS_TIME_ZONE_MOSCOW, 400},
    {OVFS_TIME_ZONE_KABUL, 430},
    {OVFS_TIME_ZONE_ISB, 500},
    {OVFS_TIME_ZONE_MADRAS, 530},
    {OVFS_TIME_ZONE_KATHMANDU, 545},
    {OVFS_TIME_ZONE_NOVOSIBIRSK, 600},
    {OVFS_TIME_ZONE_RANGOON, 630},
    {OVFS_TIME_ZONE_BANGKOK, 700},
    {OVFS_TIME_ZONE_BEIJING, 800},
    {OVFS_TIME_ZONE_OSAKA, 900},
    {OVFS_TIME_ZONE_ADELAIDE, 930},
    {OVFS_TIME_ZONE_CANBERRA, 1000},
    {OVFS_TIME_ZONE_SOLOMON_ISLANDS, 1100},
    {OVFS_TIME_ZONE_OSKLAND, 1200},
    {OVFS_TIME_ZONE_NUKUALOFA, 1300}
};

static void ovfs_TZ_translate_ntol(NET_TZ_T net_TZ, LOCAL_TZ_T *local_TZ)
{
    int iTime;
    int nloop;
    int size;

    iTime = (net_TZ.sign) * (net_TZ.offsetHour * 100 + net_TZ.offsetMinute);
    size = sizeof(arry_zone) / sizeof(TZ_NODE_T);
    for (nloop = 0; nloop < size; nloop++)
    {
        if (((nloop == 0) && (iTime <= arry_zone[nloop].time_offset)) ||
                ((nloop == size - 1) && (iTime <= arry_zone[nloop].time_offset)) ||
                ((iTime >= arry_zone[nloop].time_offset)
                 && (iTime < arry_zone[nloop + 1].time_offset)))
        {
            local_TZ->TZ = arry_zone[nloop].tz_name;
            if (iTime != arry_zone[nloop].time_offset)
            {
                local_TZ->bias_enable = 1;
                local_TZ->bias = iTime - arry_zone[nloop].time_offset;
            }
            else
            {
                local_TZ->bias_enable = 0;
                local_TZ->bias = 0;
            }
            break;
        }
    }

    return ;
}


static int ParserTimeZoneAndDst(char *str,  ONVIF_TIME_ALL_T *timeAll,
                                char *tzName, int *tzOffset, char *tzDstName)
{
    int hour = 0;
    int min = 0;
    int sec = 0;
    char *strValue = NULL;

    char tzOffsetStr[32] = {0};
    char dstStr[32] = {0};
    char startName[32] = {0};
    char endName[32] = {0};
    NET_TZ_T TZInfo;
    LOCAL_TZ_T tz;

    memset(&tz, 0, sizeof(LOCAL_TZ_T));
    memset(&TZInfo, 0, sizeof(NET_TZ_T));

    if (str == NULL)
    {
        LOGE("Param Error!\n");
        return -1;
    }

    memset(&TZInfo, 0, sizeof(NET_TZ_T));
    /*strValue = (char *)strchr(str, '-');
    if (strValue)
    {
        TZInfo.sign = 1;
    }
    else
    {
        TZInfo.sign = -1;
    }*/
    sscanf(str, "%32[^+-1234567890]%32[+-1234567890:]%32[^,],%32[^,],%32[^,]",
           tzName,
           tzOffsetStr, dstStr, startName, endName);
    LOGW("str[%s]\n",str);
    LOGW("tzName[%s]tzOffsetStr[%s]dstStr[%s]startName[%s]endName[%s]\n",tzName,tzOffsetStr, dstStr, startName, endName);
    // TZ
    sscanf(tzOffsetStr, "%d:%d", &TZInfo.offsetHour, &TZInfo.offsetMinute);
    LOGW("TZInfo[%d][%d]\n",TZInfo.offsetHour, TZInfo.offsetMinute);
    if(Common_StrniCmp(tzName, "GMT", 3) == 0)
    {
        TZInfo.sign = TZInfo.offsetHour<0?-1:1;
    }
    else
    {
        TZInfo.sign = TZInfo.offsetHour<0?1:-1;
    }
    TZInfo.offsetHour = abs(TZInfo.offsetHour);
    ovfs_TZ_translate_ntol(TZInfo, &tz);
    timeAll->timezone.EnableBias = tz.bias_enable;
    timeAll->timezone.Zone = tz.TZ;
    timeAll->timezone.ZoneBias = tz.bias;

    // DST (time offset not negative)
    sscanf(dstStr, "%32[^+-1234567890]%d:%d:%d", tzDstName, &hour, &min, &sec);
    timeAll->dst.Bias = hour * 60 + min;

    int second = 0;
    sscanf(startName, "M%d.%d.%d/%d:%d:%d", &timeAll->dst.StartTime.Month,
           &timeAll->dst.StartTime.WeekIdx,
           &timeAll->dst.StartTime.WeekDay, &timeAll->dst.StartTime.Hour,
           &timeAll->dst.StartTime.Min, &second);

    sscanf(endName, "M%d.%d.%d/%d:%d:%d", &timeAll->dst.StopTime.Month,
           &timeAll->dst.StopTime.WeekIdx,
           &timeAll->dst.StopTime.WeekDay, &timeAll->dst.StopTime.Hour,
           &timeAll->dst.StopTime.Min, &second);

    *tzOffset = (TZInfo.offsetHour * 100 + TZInfo.offsetMinute);
    return 0;
}

int XmlParser_DateTimeParam(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                            ONVIF_TIME_ALL_T *timeAll,
                            char *tzName, int *tzOffset, char *tzDstName)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *tmpNode = NULL;
    tmpNode = mxmlFindElement(param, param, "DateTimeType", NULL, NULL,
                              MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tds:DateTimeType", NULL, NULL,
                                  MXML_DESCEND_ALL);

    char *vstr = NULL;
    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            if (strstr(vstr, "NTP") != NULL)
                timeAll->ntp.Enable = 1;
            else
                timeAll->ntp.Enable = 0;
        }
    }

    tmpNode = mxmlFindElement(param, param, "DaylightSavings", NULL, NULL,
                              MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tds:DaylightSavings", NULL, NULL,
                                  MXML_DESCEND_ALL);

    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            if (strstr(vstr, "true") != NULL)
                timeAll->dst.Enable = 1;
            else
                timeAll->dst.Enable = 0;
        }
    }

    tmpNode = mxmlFindElement(param, param, "TZ", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:TZ", NULL, NULL, MXML_DESCEND_ALL);

    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            ParserTimeZoneAndDst(vstr,
                                 timeAll, tzName, tzOffset, tzDstName);
        }
    }

    tmpNode = mxmlFindElement(param, param, "Hour", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Hour", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Hour = atoi(vstr);
        }
    }

    tmpNode = mxmlFindElement(param, param, "Minute", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Minute", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Min = atoi(vstr);
        }
    }

    tmpNode = mxmlFindElement(param, param, "Second", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Second", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Sec = atoi(vstr);
        }
    }


    tmpNode = mxmlFindElement(param, param, "Year", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Year", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Year = atoi(vstr);
        }
    }

    tmpNode = mxmlFindElement(param, param, "Month", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Month", NULL, NULL, MXML_DESCEND_ALL);

    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Month = atoi(vstr);
        }
    }

    tmpNode = mxmlFindElement(param, param, "Day", NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
        tmpNode = mxmlFindElement(param, param, "tt:Day", NULL, NULL, MXML_DESCEND_ALL);

    if (tmpNode != NULL)
    {
        vstr  = (char *)mxmlGetText(tmpNode, NULL);
        if (vstr != NULL)
        {
            timeAll->utc.Day = atoi(vstr);
        }
    }

    return 0;
}

int XmlParser_Subscrib(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                       ONVIF_SUBSCRIBE_NODE_T *subNode)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    mxml_node_t *msgenv = mxmlGetFirstChild(param);
    do
    {
        char *ele_name = NULL;
        if (msgenv != NULL && mxmlGetType(msgenv) == MXML_TYPE_ELEMENT
                && (ele_name = mxmlGetElement(msgenv)) != NULL)
        {
            if (strstr(ele_name, "Address") != NULL)
            {
                char *vstr = (char *)mxmlGetText(msgenv, NULL);
                if (vstr)
                {
                    //sscanf(vstr, "%*[^://]://%32[^:]:%d/%32s",
                    //       subNode->ip, &subNode->port, subNode->uri);
                    char host[32]={0};
                    char *strtmp = NULL;
                    sscanf(vstr, "%*[^://]://%32[^/]/%32s",
                           host, subNode->uri);
                    if(strtmp = strstr(host,":"))
                    {
                        sscanf(host,"%32[^:]:%d",subNode->ip,&subNode->port);
                    }
                    else
                    {
                        Common_Strncpy(subNode->ip, host, strlen(host));
                        subNode->port = 80;
                    }

                    subNode->updateTime = Common_GetSystemCount64();
                    subNode->token = Common_Rand32();
                    return 0;
                }

            }
        }
        msgenv = mxmlWalkNext(msgenv, param, MXML_DESCEND_ALL);
    }
    while(msgenv);

    return -1;
}

static inline int XmlParserOne(mxml_node_t *param, const char *keyStr1,
                               const char *keyStr2, char **value)
{
    mxml_node_t *tmpNode = NULL;
    char *vstr = NULL;
    tmpNode = mxmlFindElement(param, param, keyStr1, NULL, NULL, MXML_DESCEND_ALL);
    if (tmpNode == NULL)
    {
        tmpNode = mxmlFindElement(param, param, keyStr2, NULL, NULL, MXML_DESCEND_ALL);
        if (tmpNode == NULL)
        {
            return -1;
        }
    }

    vstr  = (char *)mxmlGetText(tmpNode, NULL);
    if (vstr == NULL)
        return -1;

    *value = vstr;

    return 0;
}

mxml_node_t *XmlParserOne_vague(mxml_node_t *param, mxml_node_t *top, const char *keyStr, char **value)
{

    mxml_node_t *msgenv = mxmlGetFirstChild(param);
    do
    {
    	if (msgenv == NULL || mxmlGetType(msgenv) != MXML_TYPE_ELEMENT
                || mxmlGetElement(msgenv) == NULL)
    	{
    		msgenv = mxmlWalkNext(msgenv, top, MXML_DESCEND_ALL);
    		continue;
    	}

        char *ele_name = mxmlGetElement(msgenv);

    	if (strstr(ele_name, keyStr) != NULL)
    	{
            if(value)
            {
                *value = (char *)mxmlGetText(msgenv, NULL);
            }

            break;
    	}

    	/* LOGW("%s %s\n", msgenv->value.element.name, mxmlGetText(msgenv, NULL)); */

    	msgenv = mxmlWalkNext(msgenv, top, MXML_DESCEND_ALL);
    }
    while(msgenv);

    return msgenv;
}

int XmlParser_PushAnalogInfo(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                             ONVIF_OSD_ATTR_T *osdAttr, ONVIF_TIME_ALL_T *timeAll,
                             ONVIF_GPS_EXT_INFO_T *gpsinfo,
                             char *contentStr)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;
    char *vstr = NULL;

    /*utc parser*/
    XmlParserOne(param, "extXsd:utctime", "utctime", &vstr);
    if (vstr != NULL)
    {
        sscanf(vstr, "%4d-%2d-%2dT%2d:%2d:%2d", &timeAll->utc.Year, &timeAll->utc.Month,
               &timeAll->utc.Day, &timeAll->utc.Hour, &timeAll->utc.Min, &timeAll->utc.Sec);
    }

    osdAttr->datetimeOsdAttr.osdEnable = -1;

    /*avalueable*/
    XmlParserOne(param, "extXsd:availability", "availability", &vstr);
    if (vstr != NULL)
        snprintf(gpsinfo->availability, sizeof(gpsinfo->availability), "%s", vstr);

    XmlParserOne(param, "extXsd:nsindicator", "nsindicator", &vstr);
    if (vstr != NULL)
        snprintf(gpsinfo->nsindicator, sizeof(gpsinfo->nsindicator), "%s", vstr);

    XmlParserOne(param, "extXsd:ewindicator", "ewindicator", &vstr);
    if (vstr != NULL)
        snprintf(gpsinfo->ewindicator, sizeof(gpsinfo->ewindicator), "%s", vstr);

    XmlParserOne(param, "extXsd:latitudehigh", "latitudehigh", &vstr);
    if (vstr != NULL)
        gpsinfo->latitude = (double)atoi(vstr);

    XmlParserOne(param, "extXsd:latitudelow", "latitudelow", &vstr);
    if (vstr != NULL)
        gpsinfo->latitude += (double)atoi(vstr) / (double)(pow(10, strlen(vstr)));

    XmlParserOne(param, "extXsd:longitudehigh", "longitudehigh", &vstr);
    if (vstr != NULL)
        gpsinfo->longitude = (double)atoi(vstr);

    XmlParserOne(param, "extXsd:longitudelow", "longitudelow", &vstr);
    if (vstr != NULL)
        gpsinfo->longitude += (double)atoi(vstr) / (double)(pow(10, strlen(vstr)));

    XmlParserOne(param, "extXsd:speed", "speed", &vstr);
    if (vstr != NULL)
        gpsinfo->speed = (double)atoi(vstr);

    XmlParserOne(param, "extXsd:speedlow", "speedlow", &vstr);
    if (vstr != NULL)
        gpsinfo->speed += (double)atoi(vstr) / (double)(pow(10, strlen(vstr)));

    XmlParserOne(param, "extXsd:directionhigh", "directionhigh", &vstr);
    if (vstr != NULL)
        gpsinfo->direction = (double)atoi(vstr);

    XmlParserOne(param, "extXsd:directionlow", "directionlow", &vstr);
    if (vstr != NULL)
        gpsinfo->direction += (double)atoi(vstr) / (double)(pow(10, strlen(vstr)));

    XmlParserOne(param, "extXsd:altitudehigh", "altitudehigh", &vstr);
    if (vstr != NULL)
        gpsinfo->altitude = (double)atoi(vstr);

    XmlParserOne(param, "extXsd:altitudelow", "altitudelow", &vstr);
    if (vstr != NULL)
        gpsinfo->altitude += (double)atoi(vstr) / (double)(pow(10, strlen(vstr)));

    XmlParserOne(param, "extXsd:satellitenum", "satellitenum", &vstr);
    if (vstr != NULL)
        gpsinfo->satellitenum = atoi(vstr);

    /* osdAttr->ChannelEnable = 1; */
    /* osdAttr->ChannelX = -1.0f; */
    /* osdAttr->ChannelY = 1.0f; */

    if (strcmp(gpsinfo->availability, "V") == 0)
    {
        gpsinfo->latitude = (double)0;
        gpsinfo->longitude = (double)0;
        gpsinfo->speed = (double)0;
    }

    if (strcmp(gpsinfo->nsindicator, "N") == 0)
    {
        snprintf(gpsinfo->ns, sizeof(gpsinfo->ns), "北纬:%.1f°", gpsinfo->latitude);
    }
    else
    {
        snprintf(gpsinfo->ns, sizeof(gpsinfo->ns), "南纬:%.1f°", gpsinfo->latitude);
    }

    if (strcmp(gpsinfo->ewindicator, "E") == 0)
    {
        snprintf(gpsinfo->ew, sizeof(gpsinfo->ew), "东经:%.1f°", gpsinfo->longitude);//°
    }
    else
    {
        snprintf(gpsinfo->ew, sizeof(gpsinfo->ew), "西经:%.1f°", gpsinfo->longitude);
    }

    snprintf(gpsinfo->spd, sizeof(gpsinfo->spd), "速度:%.2f km/h", gpsinfo->speed);

    snprintf(osdAttr->multiOsdAttr.osdString, sizeof(osdAttr->multiOsdAttr.osdString),
             "%s %s\n%s\n%s", gpsinfo->ns, gpsinfo->ew, gpsinfo->spd, gpsinfo->station);

    osdAttr->multiOsdAttr.osdX = -0.960f;
    osdAttr->multiOsdAttr.osdY = -0.730f;
    osdAttr->multiOsdAttr.osdEnable = 1;
    osdAttr->multiOsdAttr.osdSize = 20;

    LOGW("%s %s%s Lat %.3f Lon %.3f Spd %.1f \nDir %.1f Alt %.1f Sat %d",
         gpsinfo->availability, gpsinfo->nsindicator, gpsinfo->ewindicator,
         gpsinfo->latitude, gpsinfo->longitude, gpsinfo->speed, gpsinfo->direction,
         gpsinfo->altitude, gpsinfo->satellitenum);
    return 0;
}


int XmlParser_PushStationInfo(ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                              ONVIF_OSD_ATTR_T *osdAttr, ONVIF_TIME_ALL_T *timeAll,
                              ONVIF_GPS_EXT_INFO_T *gpsinfo,
                              char *contentStr)
{
    mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;
    char *vstr = NULL;
    char statusStr[32] = {};

    XmlParserOne(param, "extXsd:status", "status", &vstr);
    if (vstr != NULL)
    {
        if (strcmp(vstr, "IN") == 0)
        {
            snprintf(statusStr, sizeof(statusStr), "%s", "进站:");
        }
        else
        {
            snprintf(statusStr, sizeof(statusStr), "%s", "出站:");
        }
    }
    else
        snprintf(statusStr, sizeof(statusStr), "%s", "进站:");

    XmlParserOne(param, "extXsd:station", "station", &vstr);
    if (vstr != NULL)
    {
        snprintf(gpsinfo->station, sizeof(gpsinfo->station) , "%s%s", statusStr, vstr);
        LOGW("station is %s\n", gpsinfo->station);
    }
    else if (strlen(gpsinfo->station) <
             1) //if gpsinfo->station was empty , so show "进站:" in the station position
        snprintf(gpsinfo->station, sizeof(gpsinfo->station) , "%s%s", statusStr, "");

    snprintf(osdAttr->multiOsdAttr.osdString, sizeof(osdAttr->multiOsdAttr.osdString),
             "%s %s\n%s\n%s", gpsinfo->ns, gpsinfo->ew, gpsinfo->spd, gpsinfo->station);

    osdAttr->multiOsdAttr.osdX = -0.960f;
    osdAttr->multiOsdAttr.osdY = -0.730f;
    osdAttr->multiOsdAttr.osdEnable = 1;
    osdAttr->multiOsdAttr.osdSize = 20;

    return 0;
}

void HttpRsp_PushAnalogInfo(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PushAnalogGpsInfo_BODY,
             ONVIF_HTTP_XMLNS,  xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_PushStationInfo(char *buf, int len,
                             ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PushStationInfo_BODY,
             ONVIF_HTTP_XMLNS,  xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioSource(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioSources_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioSourceConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioSourceConfigurations_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioSourceConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioSourceConfigurationOptions_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetMetadataConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetMetadataConfigurations_BODY,
             ONVIF_HTTP_XMLNS,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioOutputConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioOutputConfigurationOptions_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioOutputConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioOutputConfigurations_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioDecoderConfigurationOptions(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioDecoderConfigurationOptions_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetAudioDecoderConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actions[16] = {0};

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        snprintf(actions,sizeof(actions),"tr2");
    }
    else
    {
        snprintf(actions,sizeof(actions),"trt");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetAudioDecoderConfigurations_BODY,
             ONVIF_HTTP_XMLNS,actions,actions,actions,actions);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}

void HttpRsp_GetSnapshotUri(char *buf, int len,
                            ONVIF_XML_ACTION_DETAIL_T *xmlDetail, char *picUrl)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    if(Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)
    {
        LOGW("IS Media20\n");
        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetSnapshotUri_BODY2,
                 ONVIF_HTTP_XMLNS, picUrl);
    }
    else
    {
        snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetSnapshotUri_BODY,
                 ONVIF_HTTP_XMLNS, picUrl);
    }

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);
}



void HttpRsp_GetOsd(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                    ONVIF_OSD_ATTR_T *osdAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    char *token1  = NULL;
    char *token2  = NULL;
    char *token3  = NULL;
    char channelString1[128] = {0};

    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);
    OsdString2HttpString(osdAttr->channelOsdAttr.osdString, strlen(osdAttr->channelOsdAttr.osdString),
                         sizeof(osdAttr->channelOsdAttr.osdString), channelString1);

    char *vstr = NULL;
    XmlParserOne((mxml_node_t *)xmlDetail->xmlParam, "OSDToken", "trt:OSDToken", &vstr);

    memset(osdAttr->channelOsdAttr.osdString, 0, sizeof(osdAttr->channelOsdAttr.osdString));
    snprintf(osdAttr->channelOsdAttr.osdString, sizeof(osdAttr->channelOsdAttr.osdString),
             "%s", channelString1);

    if (osdAttr->channelOsdAttr.osdEnable > 0 && strcmp(vstr, "OSDToken_ChannelOSD") == 0)
    {
        token1  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token1, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
                 xmlDetail->actionNs, "", "ChannelOSD",
                 osdAttr->channelOsdAttr.osdX, osdAttr->channelOsdAttr.osdY,
                 osdAttr->channelOsdAttr.osdSize / 10,
                 osdAttr->channelOsdAttr.osdString, xmlDetail->actionNs, "");
    }

    if (osdAttr->datetimeOsdAttr.osdEnable > 0 && strcmp(vstr, "OSDToken_DateTimeOSD") == 0)
    {
        token2  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token2, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_2,
                 xmlDetail->actionNs, "", "DateTimeOSD",
                 osdAttr->datetimeOsdAttr.osdX, osdAttr->datetimeOsdAttr.osdY,
                 osdAttr->datetimeOsdAttr.osdDataFormat->formatStr,
                 osdAttr->datetimeOsdAttr.osdTimeFormat->formatStr,
                 osdAttr->datetimeOsdAttr.osdSize / 10,
                 xmlDetail->actionNs, "");
    }

    if (osdAttr->multiOsdAttr.osdEnable > 0 && strcmp(vstr, "OSDToken_MultiOSD") == 0)
    {
        token3  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
        snprintf(token3, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_GET_OSD_TOKEN_1,
                 xmlDetail->actionNs, "", "MultiOSD",
                 osdAttr->multiOsdAttr.osdX, osdAttr->multiOsdAttr.osdY,
                 osdAttr->multiOsdAttr.osdSize / 10,
                 osdAttr->multiOsdAttr.osdString, xmlDetail->actionNs, "");
    }

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetOsd_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, (token1 != NULL) ? token1 : "",
             (token2 != NULL) ? token2 : "",
             (token3 != NULL) ? token3 : "", xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    if (token1)
        ONVIF_FREE(token1);
    if (token2)
        ONVIF_FREE(token2);

    if (token3)
        ONVIF_FREE(token3);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetConfigurations(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetConfigurations_BODY,"s","s");

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetConfiguration(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    /*if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:GetConfigurationResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "GetConfigurationResponse");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);*/

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetConfigurations_BODY,"","");

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetConfigurationOptions(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetConfigurationOptions_BODY);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetPresetTours(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    return -1;
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:GetPresetToursResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "GetPresetToursResponse");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetPresetTour(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:GetPresetTourResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "GetPresetTourResponse");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetNode(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, "%s", ONVIF_HTTP_GetNode_BODY);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetNodes(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, "%s", ONVIF_HTTP_GetNodes_BODY);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetPresets(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	int i = 0;
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
	char *httppreset = (char *)ONVIF_MALLOC(ONVIF_NAMESTR_MAX_LEN*ptzCapability->ptzInfo.presetsNum);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	memset(httppreset,0,ONVIF_NAMESTR_MAX_LEN*ptzCapability->ptzInfo.presetsNum);
	LOGD("presetsNum=%d\n",ptzCapability->ptzInfo.presetsNum);
	for(i=0;i<ptzCapability->ptzInfo.presetsNum;i++)
	{
		LOGD("Name=%s,Token=%s\n",ptzCapability->ptzInfo.presetsAttr[i].token,ptzCapability->ptzInfo.presetsAttr[i].name);
		char tempStr[ONVIF_NAMESTR_MAX_LEN];
		snprintf(tempStr,ONVIF_NAMESTR_MAX_LEN,ONVIF_HTTP_GetPresets_ITEM,
			ptzCapability->ptzInfo.presetsAttr[i].token,ptzCapability->ptzInfo.presetsAttr[i].name);
		httppreset = strcat(httppreset,tempStr);
	}

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:GetPresetsResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:GetPresetsResponse");

	snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
             ONVIF_HTTP_XMLNS, action, httppreset,action);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

	ONVIF_FREE(httppreset);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SetPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	char *vstrToken = NULL,*vstrName = NULL;
    XmlParserOne((mxml_node_t *)xmlDetail->xmlParam, "PresetToken", "tptz:PresetToken", &vstrToken);
	XmlParserOne((mxml_node_t *)xmlDetail->xmlParam, "PresetName", "tptz:PresetName", &vstrName);
	LOGD("Token=%s,Name:%s\n",vstrToken,vstrName);

	int preset_num = -1;

	/*if((vstrToken == NULL) && (vstrName != NULL))
		preset_num = Utils_PickDecNumber(vstrName);
	else if ((vstrToken != NULL) && (vstrName == NULL))
		preset_num = Utils_PickDecNumber(vstrToken);*/
	if (vstrToken != NULL)
	{
        preset_num = Utils_PickDecNumber(vstrToken);
    }

    if(preset_num == -1 && (vstrName != NULL))
    {
        LOGD("preset_num=-1 and parse vstrName\n");
        preset_num = Utils_PickDecNumber(vstrName);
    }

	LOGD("preset_num=%d\n",preset_num);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz");

	if(RestOnvif_RequestSetPreset(&preset_num) != 0)
	{
		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
				 (int)strlen(httpbody));
	}
	else
	{
		snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SetPreset_BODY,
					 ONVIF_HTTP_XMLNS, action, action,
					 preset_num ,
					 action,action);

		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
				 (int)strlen(httpbody));
	}
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GotoPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	char *vstr = NULL;
    XmlParserOne((mxml_node_t *)xmlDetail->xmlParam, "PresetToken", "tptz:PresetToken", &vstr);
	LOGD("Name:%s\n",vstr);

	int preset_num = Utils_PickDecNumber(vstr);
	LOGD("preset_num=%d\n",preset_num);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:GotoPresetResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:GotoPresetResponse");

	if(RestOnvif_RequestGotoPreset(preset_num) != 0)
	{
		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
				 (int)strlen(httpbody));
	}
	else
	{
	    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

	    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
	             (int)strlen(httpbody));
	}
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_DelPreset(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	char *vstr = NULL;
    XmlParserOne((mxml_node_t *)xmlDetail->xmlParam, "PresetToken", "tptz:PresetToken", &vstr);
	LOGD("Name:%s\n",vstr);

	int preset_num = Utils_PickDecNumber(vstr);
	LOGD("preset_num=%d\n",preset_num);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:RemovePresetResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:RemovePresetResponse");

	if(RestOnvif_RequestDelPreset(preset_num) != 0)
	{
		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
				 (int)strlen(httpbody));
	}
	else
	{
	    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

	    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
	             (int)strlen(httpbody));
	}
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetStatus(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	float z;
	int max = 1;
	double type=0,speed=0;
	char uri[64];
	char action[32] = {0};
    char timeStr[32] = { 0 };

    memset(buf, 0, len);
	memset(uri, 0, sizeof(uri));
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    RestOnvif_RequestGetZoomCfg(&z,&max);

    LOGE("zoom:[%f][%f]\n",z,z/20);

    time_t curTime = time(NULL);
    TimeT2StrISO8601(&curTime, timeStr);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetStatus_BODY,
             ONVIF_HTTP_XMLNS, "","",z/20,timeStr, "" ,"");

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));

	snprintf(buf, len, "%s%s", httpheader, httpbody);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_AbsoluteMove(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	float x,y,z;
	int bFirst = 1;
	int type=0,speed=0;
	char uri[64];
	char action[32] = {0};
    memset(buf, 0, len);
	memset(uri, 0, sizeof(uri));
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	mxml_node_t *tmpNode = NULL,*tmpNode2 = NULL;
	mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    //Position

    tmpNode = XmlParserOne_vague(param, param, "Position", NULL);
    if(tmpNode)
    {
        tmpNode2 = XmlParserOne_vague(tmpNode, param, "Zoom", NULL);
        if(NULL != tmpNode2)
    	{
            z = atof(mxmlElementGetAttr(tmpNode2, "x"));

            LOGE("zoom x:[%f][%f]\n",z,z*20);

            RestOnvif_RequestSetZoomCfg(z*20);
    	}
    }

    if (strlen(xmlDetail->actionNs) > 0)
        snprintf(action, sizeof(action), "%s:AbsoluteMoveResponse", xmlDetail->actionNs);
    else
        snprintf(action, sizeof(action), "tptz:AbsoluteMoveResponse");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));

	snprintf(buf, len, "%s%s", httpheader, httpbody);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}


void HttpRsp_ContinuousMove(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	float x,y,z;
	int bFirst = 1;
	int type=0,speed=0;
	char uri[64];
	char action[32] = {0};
    memset(buf, 0, len);
	memset(uri, 0, sizeof(uri));
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	mxml_node_t *tmpNode = NULL;
	mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;
    tmpNode = mxmlFindElement(param, param, NULL, "x", NULL, MXML_DESCEND_ALL);
    if(NULL != tmpNode)
	{
        char *ele_name = mxmlGetElement(tmpNode);
        LOGD("node name:[%s]\n",ele_name);
        if(strstr(ele_name, "PanTilt") != NULL)
        {
        	x = atof(mxmlElementGetAttr(tmpNode, "x"));
        	y = atof(mxmlElementGetAttr(tmpNode, "y"));
        	snprintf(uri, sizeof(uri),"/Ptz/Move/Continuous?Pan=%d&&Tilt=%d",(int)(x*10),(int)(y*10));
        	bFirst = 0;

            if(x != (float)0 || y != (float)0)
            {
                if(x == (float)0)
                {
                    if(y > (float)0)
                    {
                        type = 21;
                    }
                    else
                    {
                        type = 22;
                    }

                    speed = abs(y*10);
                }
                else if(y == (float)0)
                {
                    if(x > (float)0)
                    {
                        type = 24;
                    }
                    else
                    {
                        type = 23;
                    }

                    speed = abs(x*10);
                }
                else
                {
                    if(y > (float)0)
                    {
                        if(x < (float)0)
                        {
                            type = 25;
                        }
                        else
                        {
                            type = 26;
                        }

                        speed = abs(y*10);
                    }
                    else
                    {
                        if(x < (float)0)
                        {
                            type = 27;
                        }
                        else
                        {
                            type = 28;
                        }

                        speed = abs(y*10);
                    }
                }

            }
            else
            {
                tmpNode = mxmlFindElement(tmpNode,param,NULL,"x",NULL,MXML_DESCEND_NONE);
            }
        }

        if(tmpNode && strstr(ele_name, "Zoom") != NULL)
        {
    		z = atof(mxmlElementGetAttr(tmpNode, "x"));
    		type = z>0?11:12;
    	    speed = abs(z*10);
        }
	}

    /*
	char *vstr = NULL;
    XmlParserOne(param, "Timeout", "tptz:Timeout", &vstr);
	LOGD("Name:%s\n",vstr);

	int timeout = Utils_PickDecNumber(vstr);
	LOGD("timeout=%d\n",timeout);

	if((bFirst == 0) && timeout)
	{
        char tmp[64] = {};
        snprintf(tmp, sizeof(tmp), "%sTimeout=%d", uri, timeout / 1000);
        memcpy(uri, tmp, sizeof(uri));
	}
	LOGD("uri=%s\n",uri);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:ContinuousMoveResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:ContinuousMoveResponse");

	if(ptzCapability->ptzInfo.IsOfDome== 2)
	{
		if(RestOnvif_RequestUri(uri,REST_PUT) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
					 (int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	}
	else if(ptzCapability->ptzInfo.LensSupport== 1)
	{
		if(RestOnvif_RequestBoardLensZoom(type,speed) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
					 (int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	}
*/
    if (strlen(xmlDetail->actionNs) > 0)
        snprintf(action, sizeof(action), "%s:ContinuousMoveResponse", xmlDetail->actionNs);
    else
        snprintf(action, sizeof(action), "tptz:ContinuousMoveResponse");

    if(RestOnvif_RequestBoardLensZoom(type,speed) != 0)
	{
		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
				 (int)strlen(httpbody));
	}
	else
	{
	    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

	    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
	             (int)strlen(httpbody));
	}

	snprintf(buf, len, "%s%s", httpheader, httpbody);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Move(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	int speed=0;
	char uri[64];
	char action[32] = {0};
    memset(buf, 0, len);
	memset(uri, 0, sizeof(uri));
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	mxml_node_t *tmpNode = NULL, *tNode = NULL;
	mxml_node_t *param = (mxml_node_t *)xmlDetail->xmlParam;

    if(strlen(xmlDetail->actionNs))
    {
        snprintf(action, sizeof(action), "%s:Focus", xmlDetail->actionNs);

    }
    else
    {
        snprintf(action, sizeof(action), "Focus");
    }

    tmpNode = mxmlFindElement(param, param, action, NULL, NULL, MXML_DESCEND_ALL);
	if(NULL != tmpNode)
	{
        tNode = XmlParserOne_vague(tmpNode, param, "Continuous", NULL);//mxmlGetFirstChild(tmpNode);
        char *ele_name = mxmlGetElement(tNode);
        if(strstr(ele_name, "Continuous") != NULL)
        {
			char *vstr = NULL;
            XmlParserOne_vague(tNode, param, "Speed", &vstr);

			speed = (int)(atof(vstr)*10);
        }
	}

    if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:MoveResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:MoveResponse");
/*
	snprintf(uri, sizeof(uri), "/Ptz/Move/Continuous?Focus=%d&Timeout=60", speed);

	if(ptzCapability->ptzInfo.IsOfDome== 2)
	{
		if(RestOnvif_RequestUri(uri,REST_PUT) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,(int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	}
	else if(ptzCapability->ptzInfo.LensSupport== 1)
	{*/
		int type = speed>0?13:14;
		if(RestOnvif_RequestBoardLensZoom(type,abs(speed)) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
					 (int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	//}
	snprintf(buf, len, "%s%s", httpheader, httpbody);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_Stop(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
	char action[32] = {0};
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

	if (strlen(xmlDetail->actionNs) > 0)
		snprintf(action, sizeof(action), "%s:StopResponse", xmlDetail->actionNs);
	else
		snprintf(action, sizeof(action), "tptz:StopResponse");
/*
	if(ptzCapability->ptzInfo.IsOfDome== 2)
	{
		if(RestOnvif_RequestUri((char *)"/Ptz/Stop",REST_PUT) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,(int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	}
	else if(ptzCapability->ptzInfo.LensSupport== 1)
	{
		if(RestOnvif_RequestBoardLensZoom(11,0) != 0)
		{
			snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
					 (int)strlen(httpbody));
		}
		else
		{
		    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
		             ONVIF_HTTP_XMLNS, action,"" ,action);

		    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
		             (int)strlen(httpbody));
		}
	}*/

    if(RestOnvif_RequestBoardLensZoom(11,0) != 0)
	{
		snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_400_HEAD,
				 (int)strlen(httpbody));
	}
	else
	{
	    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_PTZ_BODY,
	             ONVIF_HTTP_XMLNS, action,"" ,action);

	    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
	             (int)strlen(httpbody));
	}

	snprintf(buf, len, "%s%s", httpheader, httpbody);
    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}


void HttpRsp_HK_MaskOptions(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_HK_MaskOptions_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_HK_PrivacyMask(char *buf, int len,
                        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,ONVIF_CAPABILITY_SET_T *ptzCapability)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_HK_PrivacyMask_BODY,
             ONVIF_HTTP_XMLNS, xmlDetail->actionNs, xmlDetail->actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetNetworkProtocols(char *buf, int len,
                                 ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetNetworkProtocols_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetVideoSourceConfigurations(char *buf, int len,
        ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
        ONVIF_VI_ATTR_T *viAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    char actionNs[64] = {0};
    snprintf(actionNs,sizeof(actionNs),"%s",
        (Common_StrnCmp(xmlDetail->httpResult->uriStr, "/onvif/Media20", 13) == 0 || xmlDetail->media2)?"tr2":"trt");

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetVideoSourceConfigurations_BODY,
             ONVIF_HTTP_XMLNS, actionNs,actionNs,
             viAttr->Width, viAttr->Height,actionNs,actionNs);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetNTP(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetNTP_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetDiscoveryMode(char *buf, int len,
                              ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetDiscoveryMode_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetNetworkDefaultGateway(char *buf, int len,
                                      ONVIF_XML_ACTION_DETAIL_T *xmlDetail,
                                      ONVIF_NET_INFO_T *networkAttr)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetNetworkDefaultGateway_BODY,
             ONVIF_HTTP_XMLNS, networkAttr->gateWayV4);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetHostname(char *buf, int len,
                         ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetHostname_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SetSynchronizationPoint(char *buf, int len,
                    ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SetSynchronizationPoint_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_SystemReboot(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SystemReboot_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpReq_SendHello(char *buf, int len, char *ip, char *mac, int http_port, int addptz)
{
    snprintf(buf, len,ONVIF_HELLO_REQ,mac,mac,
        addptz?"onvif://www.onvif.org/type/ptz":"",ip,http_port);
}

void HttpRsp_GetSupportedAnalyticsModules(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SupportedAnalyticsModules_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetSupportedRules(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_SupportedRules_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_AddVideoEncoderConfiguration(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    //media接口, media2不支持
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_AddVideoEncoderConfiguration_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetGuaranteedNumberOfVideoEncoderInstances(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    //media接口, media2不支持
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetGuaranteedNumberOfVideoEncoderInstances_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

void HttpRsp_GetVideoEncoderInstances(char *buf, int len,
                          ONVIF_XML_ACTION_DETAIL_T *xmlDetail)
{
    //media2接口, media不支持
    memset(buf, 0, len);
    char *httpheader  = (char *)ONVIF_MALLOC(ONVIF_MSG_HEAD_LEN);
    char *httpbody  = (char *)ONVIF_MALLOC(ONVIF_MSG_BUF_LEN);
    memset(httpheader, 0, ONVIF_MSG_HEAD_LEN);
    memset(httpbody, 0, ONVIF_MSG_BUF_LEN);

    snprintf(httpbody, ONVIF_MSG_BUF_LEN, ONVIF_HTTP_GetVideoEncoderInstances_BODY,
             ONVIF_HTTP_XMLNS);

    snprintf(httpheader, ONVIF_MSG_HEAD_LEN, ONVIF_HTTP_200_HEAD,
             (int)strlen(httpbody));
    snprintf(buf, len, "%s%s", httpheader, httpbody);

    ONVIF_FREE(httpheader);
    ONVIF_FREE(httpbody);

}

