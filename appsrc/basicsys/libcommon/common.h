/**
 * @file common.h
 * @date create on: 2016年4月7日
 * @brief
 * @author eric
 * @defgroup ants_common 基本库相关定义
 * @{
 *  @note
 */
#ifndef __COMMON_H__
#define __COMMON_H__

#ifdef __cplusplus
extern "C"
{
#endif
#ifndef WIN32
#include <errno.h>
#include "common_ds.h"
#include "common_md5.h"
#include "common_mq.h"
#include "common_log.h"
#include "pthread_pool.h"
#include "common_timer.h"
#include "common_utils.h"
#include "cjson.h"
#include "mysystem.h"
#include "common_json_str_ops.h"
#include "common_utility_api.h"

/** the max size of one socket package size for user send and recv , the head and tail of user package take 32 bytes */
#define IPC_SOCK_PKG_MAX                                 (1024*64 -32)

/**  unix domain server max connecting number */
#define IPC_SOCK_CON_MAX                               16
/** send socket buffer size */
#define IPC_SOCK_SND_BUF_SIZE                        (512*1024)

/**unix domain socket path*/
#define IPC_SOCK_UDS_PATH                               "/var/run/ants_ipc_sock_uds"

/**  the max threads number in threads pool*/
#define IPC_THREAD_POOL_MAX                         (IPC_SOCK_CON_MAX + 2)

/** 线程池中空闲线程超时退出时间, 0 表示不退出*/
#define IPC_THREAD_POOL_TIMEOUT                  0

/** 32KB*/
#define IPC_THREAD_STACK_MIN                         (1024*32)

/**   request default timeout 3s*/
#define IPC_REQ_DEFAULT_TIMEOUT                  3000

/** IPC数据格式开始码*/
#define IPC_DATA_BEGIN_CODE                            0x27

/**  IPC数据头长度*/
#define IPC_DATA_HEAD_SIZE                               sizeof(IPC_DATA_HEAD_T)

/** IPC数据尾长度*/
#define IPC_DATA_TAIL_SIZE                                  sizeof(unsigned int)

/** IPC数据最小长度 头+尾*/
#define IPC_DATA_MIN_SIZE                                  ((int )IPC_DATA_HEAD_SIZE+(int)IPC_DATA_TAIL_SIZE)

/** epoll_wait 一次事件个数最大值 , 大部分情况来说接受的事件个数都为1*/
#define IPC_EPOLL_EVENT_MAX                            32

/** 进程间通讯发送心跳包最大间隔 */
#define IPC_KEEP_ALIVE_MAX_INTERVAL             30000

/** min length of MQ (message queue) , also is the default value*/
#define MQ_QUEUE_MIN_LEN                              100

/** if length of MQ larger than this , app will be set busy state */
#define MQ_QUEUE_WARN_MAX                          80

/**  if app state was busy , length of MQ little than this , app will be set idle state*/
#define MQ_QUEUE_WARN_MIN                          50

/** the max size of error list , error list will drop the old one and insert new one when list full*/
#define UTILS_ERROR_LIST_MAX                           100

/** the max threads number in threads pool */
#define JRPC_IPC_THREAD_POOL_MAX                 (32)

/** 线程池中空闲线程超时退出时间, 0 表示不退出*/
#define JRPC_IPC_THREAD_POOL_TIMEOUT         0

/**  32KB*/
#define JRPC_IPC_THREAD_STACK_MIN                 (1024*32)

/** 共享内存文件锁文件*/
#define SHM_QUEUE_DEFAULT_NAME                 "/var/run/ants_shm_queue.%d"

/** */
#define PIPE_DEFAULT_NAME                               "/var/run/ants_ipc_pipe.%d"

/** 包含输出的断言*/
#define ants_assert(x) do  \
  {  \
    if (!(x))  {\
        LOGE("ASSERT %s FAILED in %s line %d errno %d error info %s\n",#x,__FUNCTION__,__LINE__,errno,strerror(errno));\
    }\
  } while(0)
/**
 * 进程ID
 */
typedef enum
{
    IPC_APPID_COMMON_CORE = 0,                    //!< IPC_APPID_COMMON_CORE 为server,其他的id都表示client
    IPC_APPID_COMMON_BOARD_SYSTEM,
    IPC_APPID_COMMON_ALARM,
    IPC_APPID_COMMON_NETWORK,
    IPC_APPID_COMMON_EXTEND,
    IPC_APPID_COMMON_MISC,
    IPC_APPID_COMMON_MEDIA,
    IPC_APPID_COMMON_ACCESS,
    IPC_APPID_COMMON_CFG_TOOL,
	IPC_APPID_COMMON_RECORD,
	IPC_APPID_COMMON_WEB,

    IPC_APPID_COMMON_MAX = 32                  //!< not a real client app id , just use for getting the count of all clients
}IPC_APPID_E;

/** 进程间通讯的数据类型*/
typedef enum
{
    /*
     * IPC data type
     */
    IPC_TYPE_DATA_REQUEST = 0,     //!< send a request data to dst app , there are two types of request , sync request and async request
    IPC_TYPE_DATA_RESPONSE,         //!< dst app will send a response data to src app
    IPC_TYPE_DATA_REPORT,              //!< send a report data to dst app
    IPC_TYPE_DATA_BROADCAST,     //!< send a broadcast data , all apps will receive data
    IPC_TYPE_DATA_KEEPALIVE,         //!< send a keep alive data

    /*
     * IPC data type only for internal using
     */
    IPC_TYPE_DATA_APPID_REG = 100,       //!< client send a empty data ,server  just update app id with connect fd , only for internal using
    IPC_TYPE_DATA_CONN_LIST,      //!< server update connection info to all client , when connecting a new client or disconnecting a client , only for internal using
    IPC_TYPE_DATA_STATE,                //!< async request need send a sync request firstly to get peer's state , and then send async request ,only for internal using
    IPC_TYPE_DATA_APP_REFUSE,

    /*
     * IPC event
     */
    IPC_TYPE_EVENT_NEW_CONN = 200,   //!< client connect to the server , this event will generate in  server and client
    IPC_TYPE_EVENT_DISCONN,       //!< client disconnect from server, this event will generate in  server and client
    IPC_TYPE_EVENT_ERR_CONN,    //!< server socket broken , only server generate this event
    IPC_TYPE_EVENT_NO_CONN,    //!< server forward msg to client which doesn't connect to server , only server generate this event
    IPC_TYPE_EVENT_REFUSE,            //!< client connect refused by server , this means this app id already has connected to server
}IPC_TYPE_E;

/**
 * app 新连接IPC_TYPE_EVENT_NEW_CONN事件收到的消息格式
 */
#define IPC_APP_INFO_STR_LEN    (32)
typedef struct
{
    pid_t pid;
    char appInfo[IPC_APP_INFO_STR_LEN];
}IPC_NEW_CONN_INFO_T;

#define     EC_INIT                                -1001
#define     EC_INIT_STR                        "Interface need init first or already init"
#define     EC_MEM                               -1002
#define     EC_MEM_STR                       "Alloc memory failed"
#define     EC_FORMAL                        -1003
#define     EC_FORMAL_STR                "Formal error , receive a data with error format in IPC"
#define     EC_SRC_NO_CON              -1004
#define     EC_SRC_NO_CON_STR      "Local socket is not connected to server"
#define     EC_DST_NO_CON               -1005
#define     EC_DST_NO_CON_STR       "Remote socket is not connected to server"
#define     EC_APP_BUSY                      -1006
#define     EC_APP_BUSY_STR             "App is busy , MQ will drop new coming data"
#define     EC_LIST_FULL                     -1007
#define     EC_LIST_FULL_STR             "MQ list is full"
#define     EC_SHM_SEQ                     -1008
#define     EC_SHM_SEQ_STR             "Share memory queue seq error"
#define     EC_OP_FAILED                   -1009
#define     EC_OP_FAILED_STR           "Operate failed"
#define     EC_SHM_DESTROY             -1010
#define     EC_SHM_DESTROY_STR     "Share memory queue destroyed"

/*JSON-RPC ERROR CODE*/
#define  EC_JRPC_UNKNOWN_ERROR                     -32000
#define  EC_JRPC_UNKNOWN_ERROR_STR             "Unknown error"
#define  EC_JRPC_PARSE_ERROR                               -32700
#define  EC_JRPC_PARSE_ERROR_STR                       "Parse error"
#define  EC_JRPC_INVALID_REQUEST                      -32600
#define  EC_JRPC_INVALID_REQUEST_STR              "Invalid Request"
#define  EC_JRPC_METHOD_NOT_FOUND              -32601
#define  EC_JRPC_METHOD_NOT_FOUND_STR      "Method not found"
#define  EC_JRPC_INVALID_PARAMS                       -32603
#define  EC_JRPC_INVALID_PARAMS_STR               "Invalid params"
#define  EC_JRPC_INTERNAL_ERROR                       -32693
#define  EC_JRPC_INTERNAL_ERROR_STR               "Internal error"

/* CFG ERROR CODE*/
#define EC_CFG_BASE                                               -2000
#define EC_CFG_PARAM_INVALID                            (EC_CFG_BASE-1)
#define EC_CFG_PARAM_INVALID_STR                        "Input param Invalid"
#define EC_CFG_CREATE_DEF_FILE_FAIL                    (EC_CFG_BASE-2)
#define EC_CFG_CREATE_DEF_FILE_FAIL_STR                "Create default configture file fail!"
#define EC_CFG_SAVE_FILE_FAIL                            (EC_CFG_BASE-3)
#define EC_CFG_SAVE_FILE_FAIL_STR                        "Save configture to file fail!"
#define EC_CFG_ITEM_MIS_MATCH                            (EC_CFG_BASE-4)
#define EC_CFG_ITEM_MIS_MATCH_STR                        "Mismatch  item!Can't get/set item."
#define EC_CFG_SUB_SUCCESS                            (EC_CFG_BASE-5)
#define EC_CFG_SUB_SUCCESS_STR                        "A part item success, the others fail."
#define EC_CFG_TOO_MANY_CFG_FILE                        (EC_CFG_BASE-6)
#define EC_CFG_TOO_MANY_CFG_FILE_STR                    "Too many cfg file."
#define EC_CFG_PRASE_FILE_FAIL                            (EC_CFG_BASE-7)
#define EC_CFG_PRASE_FILE_FAIL_STR                        "Prase file to json obj fail."
#define EC_CFG_HAS_INITED                                (EC_CFG_BASE-8)
#define EC_CFG_HAS_INITED_STR                            "Cfg module has alreadly init."
#define EC_CFG_NO_INITE                                   (EC_CFG_BASE-9)
#define EC_CFG_NO_INITE_STR                            "Cfg module must init first."
#define EC_CFG_HAVE_NO_RIGHT                                (EC_CFG_BASE-10)
#define EC_CFG_HAVE_NO_RIGHT_STR                        "Have no right on cfg file."
#define EC_CFG_SET_PARENT_ITEM_FAIL                    (EC_CFG_BASE-11)
#define EC_CFG_SET_PARENT_ITEM_FAIL_STR                    "Can't set parent item."
#define EC_CFG_ARRAY_NOT_SUPPORT                            (EC_CFG_BASE-12)
#define EC_CFG_ARRAY_NOT_SUPPORT_STR                    "Array not support in cfg"
#define EC_CFG_OBJ_WITH_NO_CHILD                            (EC_CFG_BASE-13)
#define EC_CFG_OBJ_WIEH_NO_CHILD_STR                            "Object type with no child"
#define EC_CFG_ARRAY_ONLY_SUPPORT_LEAF_NODE                   (EC_CFG_BASE-14)
#define EC_CFG_ARRAY_ONLY_SUPPORT_LEAF_NODE_STR               "array operation supported only on leaf node when get/del item"

/* HAL ERROR CODE*/
#define EC_HAL_BASE                                     -3000
#define EC_HAL_PARAM_INVALID                            (EC_HAL_BASE-1)
#define EC_HAL_PARAM_INVALID_STR                        "Input param Invalid"
#define EC_HAL_OPRATE_FAIL                            (EC_HAL_BASE-2)
#define EC_HAL_OPRATE_FAIL_STR                        "hal operation fail"
#define EC_HAL_SENSOR_UNKNOW                            (EC_HAL_BASE-3)
#define EC_HAL_SENSOR_UNKNOW_STR                        "Unknow sensor type"
#define EC_HAL_MIPI_OPS_FAIL                            (EC_HAL_BASE-4)
#define EC_HAL_MIPI_OPS_FAIL_STR                        "open mipi dev or set mipi attr fail"
#define EC_HAL_MODULE_NO_INIT                            (EC_HAL_BASE-5)
#define EC_HAL_MODULE_NO_INIT_STR                        "module not init. init first"
#define EC_HAL_MODULE_INIT_FAIL                          (EC_HAL_BASE-6)
#define EC_HAL_MODULE_INIT_FAIL_STR                      "module init. fail"
#define EC_HAL_PLATFORM_UNKNOW                             (EC_HAL_BASE-7)
#define EC_HAL_PLATFORM_UNKNOW_STR                         "Unknow platform type"
#define EC_HAL_GET_HAL_RES_FAIL                             (EC_HAL_BASE-8)
#define EC_HAL_GET_HAL_RES_FAIL_STR                         "Get Hal global res fail"
#define EC_HAL_IDX_NOT_SUPPORT                             (EC_HAL_BASE-9)
#define EC_HAL_IDX_NOT_SUPPORT_STR                         "res idx not support"
#define EC_HAL_RE_OPERATION                                 (EC_HAL_BASE-10)
#define EC_HAL_RE_OPERATION_STR                            "repeat operation"
#define EC_HAL_VI_DEV_NOT_STOP                            (EC_HAL_BASE-11)
#define EC_HAL_VI_DEV_NOT_STOP_STR                        "vi dev and chans must stop first"
#define EC_HAL_RES_RUN_OUT                                 (EC_HAL_BASE-12)
#define EC_HAL_RES_RUN_OUT_STR                             "run out resource"
#define EC_HAL_MATCH_ITEM_FAIL                             (EC_HAL_BASE-13)
#define EC_HAL_MATCH_ITEM_FAIL_STR                         "Can't find match item"
#define EC_HAL_TIMEOUT										(EC_HAL_BASE-14)
#define EC_HAL_TIMEOUT_STR                         		    "operation time out"
#define EC_HAL_FUNCTION_NOT_SUPPORT							(EC_HAL_BASE-15)
#define EC_HAL_FUNCTION_NOT_SUPPORT_STR            		    "Function not support"



/*  CFG LABEL */
#define COMMON_CFG_LABEL_NETAPP_PROTOCOL  "NetAppProt"
#define COMMON_CFG_LABEL_PPPOE                         "Pppoe"
#define COMMON_CFG_LABEL_SNMP                           "Snmp"
#define COMMON_CFG_LABEL_DDNS                           "Ddns"
#define COMMON_CFG_LABEL_MAIL                             "Mail"
#define COMMON_CFG_LABEL_NTP                               "NTP"
#define COMMON_CFG_LABEL_FTP                               "FTP"
#define COMMON_CFG_LABEL_RTSP                             "RTSP"
#define COMMON_CFG_LABEL_RTMP                             "RTMP"
#define COMMON_CFG_LABEL_UPNP                            "Upnp"



#define COMMON_CFG_LABEL_IMAGE                            "Image"	
#define COMMON_CFG_LABEL_OSD                              "Osd"
#define COMMON_CFG_LABEL_OSDATTR                      "Attr"
#define COMMON_CFG_LABEL_TIMEOSD                      "TimeOsd"	
#define COMMON_CFG_LABEL_CHANOSD                      "ChanOsd"
#define COMMON_CFG_LABEL_MULTIOSD                      "MultiOsd"
#define COMMON_CFG_LABEL_SENSOR                     "Sensor"	
#define COMMON_CFG_LABEL_DAYNIGHT                      "DayNight"	
#define COMMON_CFG_LABEL_EXPOSURE                          "Exposure"
#define COMMON_CFG_LABEL_WHITEbALANCE                      "WhiteBalance"
#define COMMON_CFG_LABEL_FOCUS                             "Focus"
#define COMMON_CFG_LABEL_POSTPROCESS                          "PostProcess"
#define COMMON_CFG_LABEL_AUXATTR                            "AuxAttr"
#define COMMON_CFG_LABEL_PTZOSD                              "PtzOsd"
			
#define COMMON_CFG_LABEL_NETATTR                          "NetAttr"	
#define COMMON_CFG_LABEL_AUTOIP                               "AutoAdapterIP"
#define COMMON_CFG_LABEL_ETHERNET                        "EtherNet"
#define COMMON_CFG_LABEL_WIFINET                              "WifiNet"
#define COMMON_CFG_LABEL_WIFICFG                               "WifiCfg"

#define COMMON_CFG_LABEL_NETACC_PROTOCOL "NetAccProt"
#define COMMON_CFG_LABEL_ONVIF                            "Onvif"
#define COMMON_CFG_LABEL_WEB                                  "Web"
#define COMMON_CFG_LABEL_I8                                       "I8"
#define COMMON_CFG_LABEL_GB28281                          "Gb28181"
#define COMMON_CFG_LABEL_I8S                                      "I8s"
#define COMMON_CFG_LABEL_FSEYE                                "Fseye"

#define COMMON_CFG_LABEL_RECORD                           "Record"	
#define COMMON_CFG_LABEL_MEDIA                                "Media"
#define COMMON_CFG_LABEL_ENCODER                              "Encoder"
#define COMMON_CFG_LABEL_VIDEO                                "Video"
#define COMMON_CFG_LABEL_AUDIO                                "Audio"
#define COMMON_CFG_LABEL_STREAM0                              "Stream0"
#define COMMON_CFG_LABEL_STREAM1                              "Stream1"

#define COMMON_CFG_LABEL_ALARM_TRIGGER            "AlarmTrigger"
#define COMMON_CFG_LABEL_ALARMIN                         "AlarmIn"
#define COMMON_CFG_LABEL_MOTION                           "Motion"
#define COMMON_CFG_LABEL_VHIDE                                "Vhide"
#define COMMON_CFG_LABEL_VDIAGNOSE                        "Vdiagnose"
#define COMMON_CFG_LABEL_TRIG_COUNTER_WIRE      "CounterWire"	
#define COMMON_CFG_LABEL_TRIG_DETECT_WIRE           "DetectWire"
#define COMMON_CFG_LABEL_TRIG_DETECT_REGION      "DetectRegion"	
#define COMMON_CFG_LABEL_TRIG_OBJECT_REGION    "ObjectRegion"
#define COMMON_CFG_LABEL_TRIG_SOUND_DETECT       "SoundDetect"
	

#define COMMON_CFG_LABEL_ALARM_ACTION                 "AlarmAction"	
#define COMMON_CFG_LABEL_ALARM_OUT                        "AlarmOut"
#define COMMON_CFG_LABEL_ALARM_PTZ                        "Ptz"
#define COMMON_CFG_LABEL_ALARM_PTZ_ATTR                        "AttrParam"
#define COMMON_CFG_LABEL_ALARM_PTZ_PROT_NAME            "ProtocolName"
#define COMMON_CFG_LABEL_ALARM_GUARD                        "GuardPos"
#define COMMON_CFG_LABEL_ALARM_CRUISE                        "CruiseSets"


#define COMMON_CFG_LABEL_LINKAGE                              "Linkage"	
#define COMMON_CFG_LABEL_ALARMIN0                         "AlarmIn0"
#define COMMON_CFG_LABEL_MOTION	                        "Motion"
#define COMMON_CFG_LABEL_VHIDE	                        "Vhide"
#define COMMON_CFG_LABEL_VGIAGNOSE	                "Vdiagnose"
#define COMMON_CFG_LABEL_DISKFULL                         "DiskFull"
#define COMMON_CFG_LABEL_DISKERR	                        "DiskErr"
#define COMMON_CFG_LABEL_BABLE_BREAK	                "NetCableBreak"
#define COMMON_CFG_LABEL_IP_CONFLIT	                "IpConflit"
#define COMMON_CFG_LABEL_ILLEG_ACCESS	                "IllegallyAcc"
#define COMMON_CFG_LABEL_UNMATCH_FORMAT	        "UnmatchFormat"
#define COMMON_CFG_LABEL_RECORD_ERR	                "VideoRecordErr"
#define COMMON_CFG_LABEL_LINK_COUNTER_WIRE	"CounterWire"
#define COMMON_CFG_LABEL_LINK_DETECT_WIRE	"DetectWire"
#define COMMON_CFG_LABEL_LINK_DETECT_REGION	"DetectRegion"
#define COMMON_CFG_LABEL_LINK_OBJECT_REGION	"ObjectRegion"
#define COMMON_CFG_LABEL_LINK_SOUND_DETECT	"SoundDetect"

#define COMMON_CFG_LABEL_DEV                                          "Device"
#define COMMON_CFG_LABEL_MAINTAIN                                 "Maintain"
#define COMMON_CFG_LABEL_RS485                                        "RS485"
#define COMMON_CFG_LABEL_PPP                                              "PPP"
#define COMMON_CFG_LABEL_DEVINFO                                      "DevInfoCfg"

#define COMMON_CFG_LABEL_SMART                                        "Smart"	
#define COMMON_CFG_LABEL_SMART_PLATE                          "Plate"
#define COMMON_CFG_LABEL_FACE                                         "Face"
#define COMMON_CFG_LABEL_FIRE                                         "Fire"
#define COMMON_CFG_LABEL_SMART_COUNTER_WIRE	        "CounterWire"
#define COMMON_CFG_LABEL_SMART_DETECT_WIRE	        "DetectWire"
#define COMMON_CFG_LABEL_SMART_DETECT_REGION	"DetectRegion"
#define COMMON_CFG_LABEL_SMART_OBJECT_REGION	"ObjectRegion"
#define COMMON_CFG_LABEL_SMART_SOUND_DETECT	        "SoundDetect"

#define COMMON_CFG_LABEL_MISC "Misc"
#define COMMON_CFG_LABEL_TIME    "Time"
#define COMMON_CFG_LABEL_BEGIN_POINT "BeginPoint"
#define COMMON_CFG_LABEL_END_POINT    "EndPoint"
#define COMMON_CFG_LABEL_MASK    "Mask"

#define COMMON_CFG_LABEL_USERS "Users"

#define COMMON_CFG_LABEL_DEVINFO_RO                               "DevInfoRO"	
#define COMMON_CFG_LABEL_DEVINFO_DEV_TYPE                 "DeviceType"
#define COMMON_CFG_LABEL_DEVINFO_SERIAL_NUM            "SerialNumber"
#define COMMON_CFG_LABEL_DEVINFO_VERSION                      "Version"
#define COMMON_CFG_LABEL_DEVINFO_HW_INFO                      "HardwareInfo"
#define COMMON_CFG_LABEL_DEVINFO_IF_CONS                      "InterfaceConstraint"
#define COMMON_CFG_LABEL_DEVINFO_SERVICE_STATUS       "ServiceStatus"
#define COMMON_CFG_LABEL_DEVINFO_ENCODER_ABI           "EncoderAbility"
	
#define COMMON_CFG_LABEL_FACTORY_INFO                            "FactoryInfo"	

/* cfg changed lebels */
#define COMMON_CFG_CHANGED_LABEL_STATUS       "Status"
#define COMMON_CFG_CHANGED_LABEL_PARAMS       "Cfgparams"
#define COMMON_TIME_CHANGED_LABEL_PARAMS       "Timeparams"

#define COMMON_CHANGED_LABEL_LAST_RET       "LastStepRet"


#define COMMON_TIME_CHANGED_LABEL_OLD_SEC     "OldTimeSecond"
#define COMMON_TIME_CHANGED_LEAEL_OLD_USEC    "OldTimeUsecond"
#define COMMON_TIME_CHANGED_LABEL_NEW_SEC     "NewTimeSecond"
#define COMMON_TIME_CHANGED_LEAEL_NEW_USEC    "NewTimeUsecond"

#define COMMON_CFG_LABEL_CHANNEL              "Channel"
#define COMMON_CFG_LABEL_DEVICE				"Device"

#endif

#ifdef __cplusplus
}
#endif

#endif /* COMMON_COMMON_H_ */
/**
 * @}
 */
