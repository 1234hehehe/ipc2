#include "ovfs_web_func.h"
#include <sys/vfs.h>


extern int web_action_prepare(webs_t wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct **header, cJSON_Struct **indata, cJSON_Struct **outdata);

extern BOOL facedatabase_is_backuping;
static char s_queryUri[128] = {0};

//int enable_weekday[7] = {1, 0, 1, 0, 1, 0, 1};
//unsigned int daytime[7] = {0x0520, 0x0520, 0x0520, 0x0520, 0x0520, 0x0520, 0x0520};

static int web_semantic_get_autoreboot(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int i_year = 0;
    int i_mon = 0;
    int i_day = 0;
    int i_hour = 0;
    int i_min = 0;
    int i_sec = 0;
    int iloop = 0;
    //int ltime = 0;
    //struct tm lc_time;
    char *str_tmp = 0;
    int weekmask = 0;
    int monthmask = 0;
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *pArry_root = NULL;
    Common_cJSON_T *pArry_tmp = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Maintain");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            //AutoRebootMode:webAPI定义:0-禁用,1-每天,2-每周,3-单次,4-每月
            //AutoRebootMode:功能模块定义:0-禁用,1-每天,2-每周,3-每月,4-单次,
            if (Common_Json_GetAttrValueInt(pResult, "Mode", &i_num))
            {
                int table[5] = {0,1,2,4,3};
                i_num = i_num < 0 || i_num >= sizeof(table)/sizeof(table[0]) ? 0 : i_num;
                Common_Json_SetAttrValueInt(outdata, "AutoRebootMode", table[i_num]);
            }
            //EveryDayTime
            str_tmp = NULL;
            Common_Json_GetAttrValue(pResult, -1, "EveryDate", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                sscanf(str_tmp,"%02d%02d%02d",&i_hour,&i_min,&i_sec);
                Common_Json_SetAttrValue(outdata, -1, "EveryDayTime", Common_Json_Type_Number, NULL, (i_hour<<8)|i_min, 0);
            }
            //DateTime
            str_tmp = NULL;
            Common_Json_GetAttrValue(pResult, -1, "Once", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                sscanf(str_tmp,"%04d%02d%02d%02d%02d%02d",&i_year,&i_mon,&i_day,&i_hour,&i_min,&i_sec);
                pArry_tmp = Common_cJSON_CreateArray();
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_year));
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_mon));
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_day));
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_hour));
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_min));
                Common_cJSON_AddItemToArray(pArry_tmp, Common_cJSON_CreateNumber(i_sec));
                Common_Json_AddItem(outdata, -1, "DateTime", (cJSON_Struct *)pArry_tmp);
            }
            //Week
            Common_Json_GetAttrValue(pResult, -1, "Week.Mask", NULL, NULL, &weekmask, NULL);
            str_tmp = NULL;
            i_hour = 0;
            i_min = 0;
            i_sec = 0;
            Common_Json_GetAttrValue(pResult, -1, "Week.Time", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                sscanf(str_tmp,"%02d%02d%02d",&i_hour,&i_min,&i_sec);
            }
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "Week", Common_Json_Type_Array, NULL, 0, 0);
            for (iloop = 0; iloop < 7; iloop ++)
            {
                if (weekmask&(1<<iloop))
                {
                    Common_Json_SetAttrValue(pArry_root, iloop, "Enable", Common_Json_Type_Number, NULL, 1, 1);
                }
                else
                {
                    Common_Json_SetAttrValue(pArry_root, iloop, "Enable", Common_Json_Type_Number, NULL, 0, 0);
                }
                Common_Json_SetAttrValue(pArry_root, iloop, "Time", Common_Json_Type_Number, NULL, (i_hour<<8)|i_min, 0);
            }

            Common_Json_SetAttrValue(outdata, -1, "Month", Common_Json_Type_Object, NULL, 0, 0);

            monthmask = 0;
            Common_Json_GetAttrValue(pResult, -1, "Monthly.DaysMask", NULL, NULL, &monthmask, NULL);
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "Month.Days", Common_Json_Type_Array, NULL, 0, 0);
            for (iloop = 0; iloop < 31; iloop ++)
            {
                Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_Number, NULL, ((monthmask>>iloop)&1), 0);
            }

            str_tmp = NULL;
            i_hour = 0;
            i_min = 0;
            i_sec = 0;
            Common_Json_GetAttrValue(pResult, -1, "Monthly.Time", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                sscanf(str_tmp,"%02d%02d%02d",&i_hour,&i_min,&i_sec);
            }
            Common_Json_SetAttrValue(outdata, -1, "Month.Time", Common_Json_Type_Number, NULL, (i_hour<<8)|i_min, 0);
        }

        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return ret;
}

static int web_semantic_set_autoreboot(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int i_year = 0;
    int i_mon = 0;
    int i_day = 0;
    int i_hour = 0;
    int i_min = 0;
    int i_sec = 0;
    int iloop = 0;
    int weekmask = 0;
    int monthmask = 0;
    char time[32] = {0};
    cJSON_Struct *pArry_root = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        //AutoRebootMode:webAPI定义:0-禁用,1-每天,2-每周,3-单次,4-每月
        //AutoRebootMode:功能模块定义:0-禁用,1-每天,2-每周,3-每月,4-单次
        if (Common_Json_GetAttrValueInt(indata, "AutoRebootMode", &i_num))
        {
            int table[5] = {0,1,2,4,3};
            i_num = i_num < 0 || i_num >= sizeof(table)/sizeof(table[0]) ? 0 : i_num;
            Common_Json_SetAttrValueInt(lowerData, "Mode", table[i_num]);
        }
        //EveryDayTime
        if (Common_Json_GetAttrValueInt(indata, "EveryDayTime", &i_num))
        {
            i_sec = 0;
            i_min = i_num&0xff;
            i_hour = (i_num>>8)&0xff;
            snprintf(time, sizeof(time), "%02d%02d%02d", i_hour, i_min, i_sec);
            Common_Json_SetAttrValueStr(lowerData, "EveryDate", time);
        }
        //DateTime
        if ((pArry_root = Common_Json_GetAttrValueArr(indata, "DateTime")) != NULL)
        {
            Common_Json_GetAttrValue(pArry_root, 0, NULL, NULL, NULL, &i_year, NULL);
            Common_Json_GetAttrValue(pArry_root, 1, NULL, NULL, NULL, &i_mon, NULL);
            Common_Json_GetAttrValue(pArry_root, 2, NULL, NULL, NULL, &i_day, NULL);
            Common_Json_GetAttrValue(pArry_root, 3, NULL, NULL, NULL, &i_hour, NULL);
            Common_Json_GetAttrValue(pArry_root, 4, NULL, NULL, NULL, &i_min, NULL);
            Common_Json_GetAttrValue(pArry_root, 5, NULL, NULL, NULL, &i_sec, NULL);

            snprintf(time, sizeof(time), "%04d%02d%02d%02d%02d%02d", i_year, i_mon, i_day, i_hour, i_min, i_sec);

            Common_Json_SetAttrValueStr(lowerData, "Once", time);
        }
        //Week
        if ((pArry_root = Common_Json_GetAttrValue(indata, -1, "Week", NULL, NULL, NULL, NULL)) != NULL)
        {
            for (iloop = 0; iloop < 7; iloop ++)
            {
                Common_Json_GetAttrValue(pArry_root, iloop, "Enable", NULL, NULL, &i_num, NULL);
                if (1 == i_num)
                {
                    weekmask |= 1<<iloop;
                }
                Common_Json_GetAttrValue(pArry_root, iloop, "Time", NULL, NULL, &i_num, NULL);
            }
            Common_Json_SetAttrValue(lowerData, -1, "Week", Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(lowerData, -1, "Week.Mask", Common_Json_Type_Number, NULL, weekmask, 0);

            i_sec = 0;
            i_min = i_num&0xff;
            i_hour = (i_num>>8)&0xff;
            snprintf(time, sizeof(time), "%02d%02d%02d", i_hour, i_min, i_sec);
            Common_Json_SetAttrValue(lowerData, -1, "Week.Time", Common_Json_Type_String, time, 0, 0);
        }
        //Month
        if ((pArry_root = Common_Json_GetAttrValue(indata, -1, "Month.Days", NULL, NULL, NULL, NULL)) != NULL)
        {
            Common_Json_SetAttrValue(lowerData, -1, "Monthly", Common_Json_Type_Object, NULL, 0, 0);
            for (iloop = 0; iloop < 31; iloop ++)
            {
                i_num = 0;
                Common_Json_GetAttrValue(pArry_root, iloop, NULL, NULL, NULL, &i_num, NULL);
                if (i_num)
                {
                    monthmask |= 1<<iloop;
                }
            }
            Common_Json_SetAttrValue(lowerData, -1, "Monthly.DaysMask", Common_Json_Type_Number, NULL, monthmask, 0);

            if (Common_Json_GetAttrValueInt(indata, "Month.Time", &i_num))
            {
                i_sec = 0;
                i_min = i_num&0xff;
                i_hour = (i_num>>8)&0xff;
                snprintf(time, sizeof(time), "%02d%02d%02d", i_hour, i_min, i_sec);
                Common_Json_SetAttrValue(lowerData, -1, "Monthly.Time", Common_Json_Type_String, time, 0, 0);
            }
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Maintain");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

//自动维护
int frmAutoReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_autoreboot(header, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_autoreboot(header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

/*
 *1.清除DiskFormatList
 *2.DiskFormatList中添加需要格式化的diskNo
 *3.逐个获取DiskFormatList的位置和分区地址，调用record接口进行格式化,格式化完成progress设为100
 * p_json格式
 *
 */
static int web_semantic_hdformat(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    int nloop = 0;
    /*int hdno_total = 0;
    int i_num = -1;
    int iloop = 0;
    char uri_path[128] = {0};
    WEB_DISKFORMAT_NODE_T *format_node_tmp = NULL;
    WEB_DISK_NODE_T *disk_node_tmp = NULL;
    cJSON_Struct *pArray = NULL;

    // 1.清除DiskFormatList
    Common_DList_DeleteAll(g_ovfs_web->devInfo.DiskFormatList);

    // 2.DiskFormatList中添加需要格式化的diskNo
    hdno_total = Common_DList_GetCount(g_ovfs_web->devInfo.DiskList);
    pArray = Common_Json_GetAttrValue(indata, -1, "FormatList", NULL, NULL, NULL, NULL);
    for (nloop = 0; nloop < Common_Json_Size(pArray); nloop ++)
    {
        Common_Json_GetAttrValue(pArray, nloop, NULL, NULL, NULL, &i_num, NULL);
        if ((i_num < 0) || (i_num > hdno_total))
        {
            continue;
        }
        format_node_tmp = (WEB_DISKFORMAT_NODE_T *)Common_Calloc(1, sizeof(WEB_DISKFORMAT_NODE_T), __FUNCTION__, __LINE__);
        format_node_tmp->diskNo = i_num;
        format_node_tmp->progress = 0;
        Common_DList_InsertTail(g_ovfs_web->devInfo.DiskFormatList, (void *)format_node_tmp, sizeof(WEB_DISKFORMAT_NODE_T));
    }

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    // 3.逐个获取DiskFormatList的位置和分区地址，调用record接口进行格式化
    for (iloop = 0; iloop < Common_DList_GetCount(g_ovfs_web->devInfo.DiskFormatList); iloop ++)
    {
        format_node_tmp = Common_DList_GetNode(g_ovfs_web->devInfo.DiskFormatList, iloop);
        if (format_node_tmp)
        {
            disk_node_tmp = Common_DList_Search(g_ovfs_web->devInfo.DiskList, (void *)&format_node_tmp->diskNo, web_diskformat_nodecompare);
            if (disk_node_tmp)
            {
                Common_Json_SetAttrValue(lowerData, -1, "FormatPartition", Common_Json_Type_String, disk_node_tmp->path, 0, 0);
                Common_Json_SetAttrValue(lowerData, -1, "FormatType", Common_Json_Type_String, "ext4", 0, 0);

                snprintf(uri_path, sizeof(uri_path), "%s/Format", disk_node_tmp->disk_uri);
                Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            }
            format_node_tmp->progress = 100;
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }*/

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        for(nloop=0; nloop<2; nloop++)
        {
            Common_Json_SetAttrValueInt(lowerData, "PartitionId", nloop);

            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Record/DiskManage/Disk0/Format");
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            LOGD("nloop:[%d] ret:[%d]\n",nloop,ret);
        }
     }

     Common_Json_Delete(lowerData);
     lowerData = NULL;

    return ret;
}

static int web_storage_str2size(char *pstr)
{
    char *local = NULL;
    char *pos = NULL;
    int i_multiple = 1;
    int n_size = 0;

    local = strdup(pstr);
    pos = local;
    while (*pos != '\0')
    {
        if ((*pos == 'G') || (*pos == 'g'))
        {
            i_multiple = 1024;
        }
        pos++;
    }
    n_size = atoi(local);
    free(local);

    return (n_size*i_multiple);
}

/*
 * 1.获取Disk的信息(Uri)
 * 2.清空DiskList
 * 3.1 获取Disk的Uri路径
 * 3.2 获取分区的设备目录
 */
static int web_semantic_gethdinfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int nloop = 0;
    int iloop = 0;
    int hdNo = 0;
    char *str_tmp = NULL;
    int disk_num = 0;
    char pathname[128] = {0};
    WEB_DISK_NODE_T *disk_node_tmp = NULL;
    OVFS_WEB_DISK disk[MAX_DISK_NUM];
    cJSON_Struct *pResult = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pArry_root = NULL;

    memset(disk, 0, sizeof(disk));
    // 1.获取Disk的信息(Uri)
    Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/DiskManage");
    ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
    if (0 == ret)
    {
        //Common_Json_StandardPrint(pResult, NULL, NULL, NULL);
        //获取存储设备数目和对应的uri
        pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "ResList", NULL, NULL, NULL, NULL);
        disk_num = Common_Json_Size(pArry_tmp) - 1;
        for (nloop = 0; nloop < disk_num; nloop++)
        {
            str_tmp = NULL;
            Common_Json_GetAttrValue(pArry_tmp, nloop, "Uri", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                snprintf(disk[nloop].path, sizeof(disk[nloop].path), "%s", str_tmp);
            }
        }
    }
    Common_Json_Delete(pResult);
    pResult = NULL;

    if (0 == ret)
    {
        // 2.清空DiskList
        Common_DList_DeleteAll(g_ovfs_web->devInfo.DiskList);

        hdNo = 0;
        //HDInfoList
        pArry_root = Common_Json_SetAttrValue(outdata, -1, "HDInfoList", Common_Json_Type_Array, NULL, 0, 0);

        // 3.逐个Disk获取分区信息
        for (nloop = 0; nloop < disk_num; nloop ++)
        {
            snprintf(pathname, sizeof(pathname), "%s/Attribute", disk[nloop].path);
            Ovfs_Web_UpdateHeader(header, REST_GET, pathname);
            ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
            if (0 == ret)
            {
                //Common_Json_StandardPrint(pResult, NULL, NULL, NULL);
                pArry_tmp = Common_Json_GetAttrValue(pResult, -1, "Partition", NULL, NULL, NULL, NULL);

                Common_Json_GetAttrValue(pResult, -1, "PartitionNum", NULL, NULL, &disk[nloop].partitionNum, NULL);
                for (iloop = 0; iloop < disk[nloop].partitionNum; iloop ++)
                {
                    str_tmp = NULL;
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "MountPath", NULL, &str_tmp, NULL, NULL);
                    if (str_tmp)
                    {
                        if (!strcmp(str_tmp, "/tmp/mmc/mmc1"))
                        {
                            continue;
                        }
                    }

                    disk_node_tmp = (WEB_DISK_NODE_T *)Common_Calloc(1, sizeof(WEB_DISK_NODE_T), __FUNCTION__, __LINE__);
                    // 3.1 获取Disk的Uri路径
                    disk_node_tmp->disk_uri = Common_StrDup(disk[nloop].path, __FUNCTION__, __LINE__);

                    str_tmp = NULL;
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "NodePath", NULL, &str_tmp, NULL, NULL);
                    if (str_tmp)
                    {
                        // 3.2 获取分区的设备目录
                        disk_node_tmp->path = Common_StrDup(str_tmp, __FUNCTION__, __LINE__);
                    }
                    disk_node_tmp->diskNo = hdNo + 1;
                    Common_DList_InsertTail(g_ovfs_web->devInfo.DiskList, (void *)disk_node_tmp, sizeof(WEB_DISK_NODE_T));

                    //HDNo
                    Common_Json_SetAttrValue(pArry_root, hdNo, "HDNo", Common_Json_Type_Number, NULL, hdNo + 1, 0);

                    //HdStatus
                    //Common_Json_SetAttrValue(pArry_root, hdNo, "HdStatus", Common_Json_Type_Number, NULL, 0, 0);

                    //Capacity
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "TotalSpace", NULL, &str_tmp, NULL, NULL);
                    //将容量大小的字符串转换为以M为单位的整型大小
                    i_num = web_storage_str2size(str_tmp);
                    Common_Json_SetAttrValue(pArry_root, hdNo, "Capacity", Common_Json_Type_Number, NULL, i_num, i_num);

                    //FreeSpace
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "FreeSpace", NULL, &str_tmp, NULL, NULL);
                    //将容量大小的字符串转换为以M为单位的整型大小
                    i_num = web_storage_str2size(str_tmp);
                    Common_Json_SetAttrValue(pArry_root, hdNo, "FreeSpace", Common_Json_Type_Number, NULL, i_num, i_num);

                    //HDType
                    Common_Json_SetAttrValue(pArry_root, hdNo, "HDType", Common_Json_Type_Number, NULL, 0, 0);

                    hdNo++;
                }
            }

            Common_Json_Delete(pResult);
            pResult = NULL;
        }
    }

    //! 如果record进程出错,仍要返回0.避免webserver无法login.
    ret = 0;
    return ret;
}

/*1.获取DiskFormatList
 *2.将DiskFormatList中的diskNo和进度填入json数组中
 */

static int web_semantic_gethdstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;
    int i_num = 0;
    char *str_tmp = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/Status");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(0 == ret)
    {
        if(NULL != Common_Json_GetAttrValue(lowerData, -1, "Status", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(outdata, -1, "Status", Common_Json_Type_Number, NULL, i_num, 0);
        }
        if(NULL != Common_Json_GetAttrValue(lowerData, -1, "Decribe", NULL, &str_tmp, NULL, NULL))
        {
            Common_Json_SetAttrValue(outdata, -1, "Decribe", Common_Json_Type_String, str_tmp, 0, 0);
        }

    }
    if(NULL != lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}


static int web_semantic_getprogess(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iloop = 0;
    int format_num = 0;
    Common_cJSON_T *pArry_root = NULL;
    WEB_DISKFORMAT_NODE_T *format_node_tmp = NULL;

    pArry_root = Common_Json_SetAttrValue(outdata, -1, "ProgressList", Common_Json_Type_Array, NULL, 0, 0);
    // 1.获取DiskFormatList
    format_num = Common_DList_GetCount(g_ovfs_web->devInfo.DiskFormatList);
    for (iloop = 0; iloop < format_num; iloop ++)
    {
        // 2.将DiskFormatList中的diskNo和进度填入json数组中
        format_node_tmp = Common_DList_GetNode(g_ovfs_web->devInfo.DiskFormatList, iloop);
        if (format_node_tmp)
        {
            Common_Json_SetAttrValue(pArry_root, iloop, "DiskNo", Common_Json_Type_Number, NULL, format_node_tmp->diskNo, 0);
            Common_Json_SetAttrValue(pArry_root, iloop, "DiskProgress", Common_Json_Type_Number, NULL, format_node_tmp->progress, 0);
        }
    }

    return ret;
}

static int web_semantic_update(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int transMode = 0;
    int needRestore = 0;
    char *str_tmp = NULL;
    char md5buf[128] = {0};
    char cmd[256] = {0};
    struct stat file_stat;
    cJSON_Struct *pResult = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        char *filePath = "/dev/ovfs_ipc.update";
        //     if (Common_Json_GetAttrValueStr(indata, "FilePath", &filePath))
        //     {
        snprintf(g_ovfs_web->updatePath, sizeof(g_ovfs_web->updatePath), "%s", filePath);
        //     }
        transMode = 1<<7;
        Common_Json_SetAttrValue(lowerData, -1, "TransMode", Common_Json_Type_Number, NULL, transMode, transMode);
        Common_Json_SetAttrValue(lowerData, -1, "FilePath", Common_Json_Type_String, g_ovfs_web->updatePath, 0, 0);
        stat(g_ovfs_web->updatePath, &file_stat);
        Common_Json_SetAttrValue(lowerData, -1, "FileSize", Common_Json_Type_Number, NULL, file_stat.st_size, 0);
        snprintf(cmd, sizeof(cmd), "md5sum %s | awk '{print $1}'", g_ovfs_web->updatePath);
        Common_Exe_Cmd(cmd, 5*1000, md5buf, sizeof(md5buf));
        md5buf[strlen(md5buf)-1] = '\0';
        Common_Json_SetAttrValue(lowerData, -1, "Md5sum", Common_Json_Type_String, md5buf, 0, 0);

        if(Common_Json_GetAttrValueInt(indata, "NeedRestore", &needRestore))
        {
            Common_Json_SetAttrValueInt(lowerData, "NeedRestore", needRestore);
        }

        Ovfs_Web_UpdateHeader(header, REST_POST, "/Update/Start");
        ret = Ovfs_Web_RestMethodA(header, lowerData, &pResult, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(pResult, -1, "Uri", NULL, &str_tmp, NULL, NULL);
            if (str_tmp)
            {
                snprintf(s_queryUri, sizeof(s_queryUri), "%s", str_tmp);
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_importcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    // 应用配置文件包.
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    int lockedParam = 0;
    char *filePath = NULL;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueStr(indata, "FilePath", &filePath))
        {
            Common_Json_SetAttrValueStr(lowerData, "FileName", filePath);
        }

        if (Common_Json_GetAttrValueInt(indata, "LockedParam", &lockedParam))
        {
            Common_Json_SetAttrValueInt(lowerData, "LockedParam", lockedParam);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/ImportCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (filePath)
    {
        remove(filePath);
    }

    return ret;
}

static int web_semantic_exportcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int type = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *outParam = NULL;

    Common_Json_GetAttrValueInt(indata, "ExportType", &type);

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    char *exportcfgPath = NGX_HTML "/exportcfg.tgz";
    // 生成配置文件包.
    if (0 == ret)
    {
        struct stat tmpStat;
        if (stat(exportcfgPath, &tmpStat) == 0)
        {
            remove(exportcfgPath);
        }

        Common_Json_SetAttrValueInt(lowerData, "ExportType", type);
        Common_Json_SetAttrValueStr(lowerData, "FileName", exportcfgPath);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/ExportCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, &outParam, 0);
    }


    if (0 == ret)
    {
        int export_type = 0;
        Common_Json_GetAttrValueInt(outParam, "ExportType", &export_type);
        if(export_type != type)
        {
            ret = WEB_CODE_Unsupported;
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    // 访问配置文件包.
    unsigned int fileSize = 0;
    FILE *fp = NULL;
    if (0 == ret)
    {
        struct stat tmpStat;
        int tryCount;

        for (tryCount = 0; tryCount < 5; tryCount++)
        {
            if ((stat(exportcfgPath, &tmpStat) != 0) || ((fp = fopen(exportcfgPath, "rb")) == NULL))
            {
                usleep(100000);
            }
            else
            {
                fclose(fp);
                break;
            }
        }
        if (tryCount == 5)
        {
            ret = WEB_CODE_FileNotAccess;
        }
        else
        {
            fileSize = tmpStat.st_size;
        }
    }

    // 写入将要发出的outdata中.
    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(outdata, "ExportType", type);
        Common_Json_SetAttrValueStr(outdata, "FileName", "exportcfg.tgz");
        Common_Json_SetAttrValueInt(outdata, "FileSize", fileSize);

        char cmd[128];
        char md5buf[128] = {0};
        snprintf(cmd, sizeof(cmd), "md5sum %s | awk '{print $1}'", exportcfgPath);
        Common_Exe_Cmd(cmd, 5*1000, md5buf, sizeof(md5buf));
        md5buf[strlen(md5buf)-1] = '\0';
        Common_Json_SetAttrValueStr(outdata, "Md5sum", md5buf);
    }

    return ret;
}

static int web_semantic_update_ptz(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Update");
    ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

    return ret;
}

static int web_semantic_get_update_progress(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_progress = 0;
    int i_num = 0;
    int status = 0;
    cJSON_Struct *pResult = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, s_queryUri);
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(pResult, -1, "Progress", NULL, NULL, &i_progress, NULL);
            Common_Json_GetAttrValue(pResult, -1, "Partial", NULL, NULL, &i_num, NULL);
            Common_Json_GetAttrValue(pResult, -1, "Status", NULL, NULL, &status, NULL);
            if (7 == i_num)
            {
                i_progress = 100;
            }
            else if (-1 == status)
            {
                i_progress = -1;
            }
            else
            {
            }
            Common_Json_SetAttrValue(outdata, -1, "Progress", Common_Json_Type_Number, NULL, i_progress, 0);
            if (i_progress == 100 || i_progress < 0)
            {
                remove(g_ovfs_web->updatePath);
            }
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return ret;
}

static int web_semantic_get_update_ptz_progress(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_progress = 0;
    int i_num = 0;
    int status = 0;
    cJSON_Struct *pResult = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/update");
        ret = Ovfs_Web_RestMethodA(header, NULL, &pResult, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValueInt(pResult, "updateState", &status);
            if(status == 6)
            {
                Common_Json_GetAttrValueInt(pResult, "updateProgress", &i_progress);
                Common_Json_SetAttrValueInt(outdata, "Progress", i_progress);
            }

            switch(status)
            {
                case 4://ready
                    status = 1;
                    break;

                case 6://updating
                    status = 2;
                    break;

                case 7://update ok
                    status = 3;
                    break;
                case 9://update failed
                    status = -1;
                    break;
            }

            Common_Json_SetAttrValueInt(outdata, "Status", status);
        }
        Common_Json_Delete(pResult);
        pResult = NULL;
    }

    return ret;
}

int web_semantic_devicereboot(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "CloseQuickStart", &i_num) && i_num == 1)
        {
            LOGW("CloseQuickStart!\n");
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Update/UbootConfig");
            Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
        }

        Common_Json_SetAttrValue(lowerData, -1, "Delay", Common_Json_Type_Number, NULL, 3, 0);
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Power/Reboot");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}
static int web_semantic_SysUpdateFree(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    //int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Update/PrepareUpdate");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            JsonOper_MergeObj(outdata, lowerData, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}


static int web_semantic_parasysrestore(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int iloop = 0;
    cJSON_Struct *pArry_root = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        iloop = 0;
        pArry_root = Common_Json_SetAttrValue(lowerData, -1, "ResList", Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_GetAttrValue(indata, -1, "Network", NULL, NULL, &i_num, NULL);
        if (1 == i_num)
        {
            Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_String, "NetConfig", 0, 0);
            iloop ++;
        }
        Common_Json_GetAttrValue(indata, -1, "Alarm", NULL, NULL, &i_num, NULL);
        if (1 == i_num)
        {
            Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_String, "AlarmConfig", 0, 0);
            iloop ++;
        }
        Common_Json_GetAttrValue(indata, -1, "Account", NULL, NULL, &i_num, NULL);
        if (1 == i_num)
        {
            Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_String, "UserConfig", 0, 0);
            iloop ++;
        }
        Common_Json_GetAttrValue(indata, -1, "OtherCfg", NULL, NULL, &i_num, NULL);
        if (1 == i_num)
        {
            Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_String, "Others", 0, 0);
            iloop ++;
        }
        Common_Json_SetAttrValueInt(lowerData, "NeedReboot", 1);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Restore/Items");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    i_num = 0;
    Common_Json_GetAttrValue(indata, -1, "VideoParam", NULL, NULL, &i_num, NULL);
    if (i_num)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Attribute/Restore");
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_factoryrestore(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, int deletclockcfg)
{
    int ret = 0;
    //int timeA = 0;
    //int timeB = 0;

    cJSON_Struct *pArry_root = NULL;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    int ret_tmp = web_semantic_aliiotrunbinging(header, indata, outdata);

    //Common_GetSystemCount(&timeA, NULL);

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(lowerData, "DeleteLockedCfg", deletclockcfg);
        Common_Json_SetAttrValueInt(lowerData, "NeedReboot", 1);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Restore/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }
    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_DELETE, "/NetWork/NetAttr/WifiConfig");
        Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(0 == ret)
    {
        Common_Json_SetAttrValueStr(lowerData, "Path", "/update/soundFile/reset_ok");
        Common_Json_SetAttrValueInt(lowerData, "Times", 1);
        Common_Json_SetAttrValueInt(lowerData, "Priority", 1);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Audio/Adec/PlayFile");
        Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    /*Common_GetSystemCount(&timeB, NULL);

    if(ret_tmp == 0)
    {
        int offset = timeB - timeA;
        LOGD("time offset:[%d]\n", offset);
        if(offset < 5)
        {
            LOGD("Sleep [%d]s!\n", 5 - offset);
            Common_Sleep(5 - offset, 0);
        }
    }*/

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_restore_resetkey(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Sys/SoftReset");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

static S32 LoadCfg2Json(const S8 *pCfgFilePath, cJSON_Struct **pTempJson)
{
	S32 nStrLen = 0;
	char *pConfigString = NULL;

    FILE *fp = NULL;

	if ((NULL == pCfgFilePath) || (NULL == pTempJson) || (*pTempJson != NULL))
	{
	    LOGE("Enter parameters error.\n");
		return -1;
	}

	fp = fopen(pCfgFilePath, "rb");
    if (NULL == fp)
    {
        LOGI("fopen %s error.\n", pCfgFilePath);
        return -1;
    }

	fseek(fp, 0, SEEK_END);
	nStrLen = ftell(fp);
	fseek(fp, 0, SEEK_SET);

    if (nStrLen > 0)
	{
		pConfigString = (char *)Common_Malloc(nStrLen, 0, __FUNCTION__, __LINE__);
		if (pConfigString != NULL)
		{
			if(nStrLen == fread(pConfigString, 1, (U32)nStrLen, fp))
			{
				*pTempJson = Common_Json_Parse(pConfigString, NULL, NULL);
			}
		}
	}
    fclose(fp);

    if (pConfigString != NULL)
	{
		Common_Free(pConfigString, __FUNCTION__, __LINE__);
		pConfigString = NULL;
	}

    if (NULL == *pTempJson)
	{
		return -1;
	}

	return 0;
}

static S32 SaveJson2Cfg(const S8 *pCfgFilePath, cJSON_Struct *pTempJson)
{
	S32 nStrLen = 0;
	char *pConfigString = NULL;

    FILE *fp = NULL;

	if ((NULL == pCfgFilePath) || (NULL == pTempJson))
	{
	    LOGE("Enter parameters error.\n");
		return -1;
	}

    pConfigString = Common_Json_Print(pTempJson, &nStrLen);
	fp = Common_File_fOpen(pCfgFilePath, "wb");
    if (NULL == fp)
    {
        LOGI("fopen %s error.\n", pCfgFilePath);
        return -1;
    }

    if(nStrLen != Common_File_fWrite(pConfigString, 1, (U32)nStrLen, fp))
	{
        LOGE("fWrite str len error.\n");
	}

    fclose(fp);

    if (pConfigString != NULL)
    {
        Common_Free(pConfigString, __FUNCTION__, __LINE__);
        pConfigString = NULL;
    }

	return 0;
}

int GetItemFromJsonFile(const S8 *pCfgFilePath, S8 *ItemStr,cJSON_Struct **pOutJson)
{
    int ret = 0;
    char *tmpStr = NULL;
    char *curStr = NULL;
    char *nextStr = NULL;
    cJSON_Struct *rootdata = NULL;
    cJSON_Struct *itemdata = NULL;
    cJSON_Struct *tmpdata = NULL;

    if ((NULL == pCfgFilePath) || (NULL == ItemStr) || (NULL == pOutJson) || (*pOutJson != NULL))
	{
	    LOGE("Enter parameters error.\n");
		return -1;
	}

    if (0 == ret)
    {
        if ((*pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        ret = LoadCfg2Json(pCfgFilePath, &rootdata);
    }

    if(ret == 0)
    {
        itemdata = Common_Json_GetAttrValueObj(rootdata, ItemStr);
        tmpStr = ItemStr;
        tmpdata = *pOutJson;
        while(tmpStr)
        {
            Common_UriOneParse(tmpStr, NULL, &curStr, &nextStr);
            //LOGD("curStr:[%s] nextStr:[%s]\n",curStr,nextStr);

            if(curStr != NULL)
    		{
                tmpdata = Common_Json_SetAttrValueObj(tmpdata, curStr);

    			Common_Free(curStr, __FUNCTION__, __LINE__);
    			curStr = NULL;
    		}

            if(nextStr)
            {
                tmpStr = nextStr;
            }
            else
            {
                break;
            }
        }

        JsonOper_MergeObj(tmpdata, itemdata, 0);
    }

    if (rootdata)
    {
        Common_Json_Delete(rootdata);
        rootdata = NULL;
    }

    return ret;

}

static int web_semantic_restore_simple(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *osddata = NULL;
    cJSON_Struct *netattrdata = NULL;

    if (0 == ret)
    {
        GetItemFromJsonFile("/usr/etc/cfgfiles/BoardSys.json", "BoardSys/Osd", &osddata);
        GetItemFromJsonFile("/usr/etc/cfgfiles/NetWork.json", "NetAttr", &netattrdata);
    }

    S8 szCmd[256];
    sprintf(szCmd, "pkill boardsys;cd /usr/etc/cfgfiles;mv Access.json Access.json_bak;mv Core.json Core.json_bak;rm -r *.json;\
        mv Access.json_bak Access.json;mv Core.json_bak Core.json;");//touch /usr/etc/restore_other
    Common_System(szCmd);

    if(osddata)
    {
        SaveJson2Cfg("/usr/etc/cfgfiles/BoardSys.json",osddata);
        Common_Json_Delete(osddata);
        osddata = NULL;
    }

    if(netattrdata)
    {
        SaveJson2Cfg("/usr/etc/cfgfiles/NetWork.json",netattrdata);
        Common_Json_Delete(netattrdata);
        netattrdata = NULL;
    }

    return ret;
}

static int web_semantic_get_sysupdateinfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Common_Json_SetAttrValue(outdata, -1, "HDCount", Common_Json_Type_Number, NULL, 0, 0);
    Common_Json_SetAttrValue(outdata, -1, "RemoteRight", Common_Json_Type_Number, NULL, 1, 1);
    Common_Json_SetAttrValue(outdata, -1, "Path", Common_Json_Type_String, g_ovfs_web->updatePath?g_ovfs_web->updatePath:"", 0, 0);
    Common_Json_SetAttrValue(outdata, -1, "FreeSize", Common_Json_Type_Number, NULL, 15728640, 15728640);

    return ret;
}

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)

static int web_semantic_gethdfsinfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerdata = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/DiskManage/FSInfo");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerdata, 0);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueStr(lowerdata, "FsSupport", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "FsSupport", str_tmp);
        }

        if(Common_Json_GetAttrValueStr(lowerdata, "DiskFs", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "DiskFs", str_tmp);
        }

        if(Common_Json_GetAttrValueInt(lowerdata, "NeedFormat", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "NeedFormat", i_num);
        }
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

static int web_semantic_getrecordconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerdata = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/RecordConfig/PreserveRecord");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerdata, 0);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerdata, "StorageSpaceMode", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "StorageSpaceMode", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerdata, "StorageSpacePercent", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "StorageSpacePercent", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerdata, "StorageSpaceSize", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "StorageSpaceSize", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerdata, "StorageSpaceReserveSize", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "StorageSpaceReserveSize", i_num);
        }
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

static int web_semantic_setrecordconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerdata = NULL;

    if(0 == ret)
    {
        if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if(0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "StorageSpaceMode", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "StorageSpaceMode", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "StorageSpacePercent", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "StorageSpacePercent", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "StorageSpaceSize", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "StorageSpaceSize", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "StorageSpaceReserveSize", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "StorageSpaceReserveSize", i_num);
        }
    }

    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Record/RecordConfig/PreserveRecord");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

// 硬盘格式化
int frmGetHDFormatProgress(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_getprogess(header, indata, outdata);
    }

    return ret;
}

//硬盘格式化
int frmHDFormat(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header, 5);
    if (0 == ret)
    {
        ret = web_semantic_hdformat(header, indata, outdata);
        if (ret != 0)
        {
            if (1 == ret)
            {
                ret = WEB_CODE_LackingMem;
                //errorString = "Malloc resource Failed!";
            }
            else if (2 == ret)
            {
                ret = WEB_CODE_LackingThread;
                //errorString="Create pthread Failed!";
            }
            else if (3 == ret)
            {
                ret = WEB_CODE_InvalidJson;
                //errorString="Invalid Format thread Object!";
            }
            else if (4 == ret)
            {
                ret = WEB_CODE_TaskExist;
                //errorString="Format thread Object is alive!,we have not done format yet!cancel this request!";
            }
            else
            {
                ret = WEB_CODE_GeneralMistake;
            }
        }
    }

    return ret;
}

// 硬盘信息
int frmGetHDInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header, 2);

    if (0 == ret)
    {
        ret = web_semantic_gethdinfo(header, indata, outdata);
    }

    return ret;
}

//硬盘状态
int frmGetHDStatus(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_gethdstatus(header, indata, outdata);
    }

    return ret;
}

// 硬盘信息
int frmGetHDFSInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header, 2);

    if (0 == ret)
    {
        switch(opt->type)
        {
        case 0:
            ret = web_semantic_gethdfsinfo(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
        }
    }

    return ret;
}
int frmRecordConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;


    if (0 == ret)
    {
        switch(opt->type)
        {
        case 0:
            ret = web_semantic_getrecordconfig(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_setrecordconfig(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
        }
    }

    if(0 == ret)
    {
        if(opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }


    return ret;
}

#endif


int frmDeviceReboot(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    ret = web_semantic_get_userright_remote(header, 13);

    if (0 == ret)
    {
        ret = web_semantic_devicereboot(header, indata, outdata);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

//恢复默认
int frmParaSysRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_parasysrestore(header, indata, outdata);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmFactoryRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
        ret = web_semantic_factoryrestore(header, indata, outdata, 1);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmDeviceRestore(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str = NULL;

    if (0 == ret)
	{
        Common_Json_GetAttrValueStr(indata, "RestoreType", &str);
        if(smatch(str, "Simple"))
        {
            LOGW("Simple Restore\n");
            ret = web_semantic_restore_simple(header, indata, outdata);
            if(ret == 0)
            {
                ovfs_web_write_log(header, "frmDeviceRestore_Simple", opt, "");
            }
        }
        else if(smatch(str, "All"))
        {
            LOGW("All Restore\n");
            ret = web_semantic_factoryrestore(header, indata, outdata, 0);
            if(ret == 0)
            {
                ovfs_web_write_log(header, "frmDeviceRestore_All", opt, "");
            }
        }
        else if(smatch(str, "Factory"))
        {
            LOGW("Factory Restore\n");
            ret = web_semantic_factoryrestore(header, indata, outdata, 1);
        }
        else if(smatch(str, "ResetKey"))
        {
            LOGW("ResetKey Restore\n");
            ret = web_semantic_restore_resetkey(header, indata, outdata);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if (0 == ret)
    {
        if(!smatch(str, "ResetKey"))
        {
            ret = web_semantic_devicereboot(header, indata, outdata);
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}


static int web_semantic_get_updateinfo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int size = 0;
    //int freeSize = 0;
    //cJSON_Struct *lowerData = NULL;

    Common_Json_GetAttrValueInt(indata, "UploadFielSize", &size);

    //Ovfs_Web_UpdateHeader(header, REST_PUT, "/Update/PrepareUpdate");
    //ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    //if (0 == ret)
    {
        struct statfs diskInfo;
        statfs("/dev",&diskInfo);
        unsigned long long totalBlocks = diskInfo.f_bsize;
        unsigned long long freeDisk = diskInfo.f_bfree*totalBlocks;
        //Common_Json_GetAttrValueInt(lowerData, "FreeSize", &freeSize);

        Common_Json_SetAttrValueInt(outdata, "CanDoUpload", size > freeDisk ? 0 : 1);

        Common_Json_SetAttrValue(outdata, -1, "AuthType", Common_Json_Type_Number, NULL, 0, 0);
        Common_Json_SetAttrValue(outdata, -1, "UploadAddr", Common_Json_Type_String, "/digest/upload", 0, 0);
    }

    //Common_Json_Delete(lowerData);
    //lowerData = NULL;

    return ret;
}

int frmUploadInfo(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
	{
        ret = web_semantic_get_updateinfo(header, indata, outdata);
    }

    return ret;
}

//获取设备参数
int frmSysUpdate(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    ret = web_semantic_get_userright_remote( header, 3);
    if (0 == ret)
    {
        ret = web_semantic_get_sysupdateinfo(header, indata, outdata);
    }

    return ret;
}
//升级释放内存
int frmSysUpdateFree(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        ret = web_semantic_SysUpdateFree(header, indata, outdata);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}



/*

don't let nginx send all post data to fastcgi server. it will eat the left memory.
may be we should try another way to avoid this!
*/


int upload(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    LOGD("%s %d\n",wp->upload_filename[0],wp->uploadType);
    if(wp->upload_filename[0] == NULL)
    {
        ret = WEB_CODE_InternalMistake;
    }

    /*if(ret == 0)
    {
        ret = web_semantic_get_userright_remote(header,5);
    }*/

    if (0 == ret)
    {

        Common_Json_SetAttrValueStr(indata, "FilePath", wp->upload_filename[0]);
        //PRINT_DBG("uploadType=%s\n", wp->uploadType);
        if (wp->uploadType == NULL || strcmp(wp->uploadType, "Update") == 0)
        {
            char cmd[512]= {0};

            if(wp->upload_filename[0])
            {
                LOGD("%s\n",wp->upload_filename[0] + slen(wp->upload_filename[0])-4);
                sprintf(cmd,"mv \"%s\" /dev/ovfs_ipc.update",wp->upload_filename[0]);
                LOGD("cmd:[%s]\n",cmd);
                system(cmd);
            }
            ret = web_semantic_update(header, indata, outdata);
        }
        else if (strcmp(wp->uploadType, "Config") == 0)
        {
            //LOGD("%s %s\n",wp->upload_filename,wp->upload_filename[0]);
            ret = web_semantic_importcfg(header, indata, outdata);
        }
        else if(strcmp(wp->uploadType, "Tls") == 0)
        {

            //copy crt key to ssl dir
            int i = 0;
            char cmd[128]= {0};
            for(i = 0; i < 6; ++i)
            {
                if(wp->upload_filename[i])
                {
                    memset(cmd,0,sizeof(cmd));
                    LOGD("%s\n",wp->upload_filename[i] + slen(wp->upload_filename[i])-4);
                    if(smatch(wp->upload_filename[i] + slen(wp->upload_filename[i]) - 4,".crt"))
                    {
                        sprintf(cmd,"mv \"%s\" /update/nginx/ssl/self.crt",wp->upload_filename[i]);
                    }
                    else if(smatch(wp->upload_filename[i] + slen(wp->upload_filename[i]) - 4,".key"))
                    {
                        sprintf(cmd,"mv \"%s\" /update/nginx/ssl/self.key",wp->upload_filename[i]);
                    }
                    LOGD("cmd:[%s]\n",cmd);
                    system(cmd);
                }
            }
        }
        else if (strcmp(wp->uploadType, "Ptz") == 0)
        {
            //LOGD("%s %s\n",wp->upload_filename,wp->upload_filename[0]);
            ret = web_semantic_update_ptz(header, indata, outdata);
        }
        else
        {
            ret = WEB_CODE_Unsupported;
        }
    }

    return ret;
}

//得到升级进度
int GetProgress(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char *str_tmp = NULL;

    if (0 == ret)
    {
        Common_Json_GetAttrValueStr(indata, "UploadType", &str_tmp);
        if(smatch(str_tmp, "Ptz"))
        {
            ret = web_semantic_get_update_ptz_progress(header, indata, outdata);
        }
        else
        {
            ret = web_semantic_get_update_progress(header, indata, outdata);
        }
    }

    return ret;
}

int frmChangeFileName(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        // filename中不能超长,不能有'/'字符.
        char *inFileName = NULL;
        if (Common_Json_GetAttrValueStr(indata, "FileName", &inFileName) == NULL || strlen(inFileName) > 256 || strchr(inFileName, '/'))
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            char *find = strrchr(g_ovfs_web->updatePath, '/');
            if (find)
            {
                snprintf(find + 1, sizeof(g_ovfs_web->updatePath) - (find + 1 - g_ovfs_web->updatePath), "%s", inFileName);
            }
        }
    }

    if(0 == ret)
    {
        ret = web_semantic_SysUpdateFree(header, indata, outdata);
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

//获取配置文件压缩包
int frmExportConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    ret = web_semantic_get_userright_remote(header,3);
    if(ret == 0)
    {
        ret = web_semantic_exportcfg(header, indata, outdata);
    }
    return ret;
}

int frmGetConfigFileV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    // 生成配置文件包.
    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(lowerData, "FileName", FILE_CONFIG_EXPORT);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/ExportCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    // 访问配置文件包.
    unsigned int fileSize = 0;
    FILE *fp = NULL;
    if (0 == ret)
    {
        struct stat tmpStat;
        int tryCount;

        for (tryCount = 0; tryCount < 5; tryCount++)
        {
            if ((stat(FILE_CONFIG_EXPORT, &tmpStat) != 0) || ((fp = fopen(FILE_CONFIG_EXPORT, "rb")) == NULL))
            {
                usleep(100000);
            }
            else
            {
                break;
            }
        }
        if (tryCount == 5)
        {
            ret = WEB_CODE_FileNotAccess;
        }
        else
        {
            fileSize = tmpStat.st_size;
        }
    }

    char *fileBuff = NULL;
    if (0 == ret)
    {
        fileBuff = (char*)malloc(sizeof(char)*(fileSize + 1));
        if (fileBuff == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    // 读入文件内容.
    if (0 == ret)
    {
        int readSize = 0;
        //printf("该文件的长度为%1d字节\n",length);
        if ((readSize = fread(fileBuff, fileSize,1, fp)) > 0)
        {
            fileBuff[fileSize] = '\0';
        }
    }

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    remove(FILE_CONFIG_EXPORT);

    // 转换为Base64编码写入将要发出的outdata中.
    if (0 == ret)
    {
        //websWriteBlock(wp, fileBuff, fileSize);
        unsigned int base64TextSize = 0;
        char *base64TextBuff = Common_Base64_Encode(fileBuff, fileSize, &base64TextSize);

        if (Common_Json_SetAttrValueStr(outdata, "Base64Text", base64TextBuff) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }

        Common_Free(base64TextBuff, __FUNCTION__, __LINE__);
    }

    if (fileBuff)
    {
        free(fileBuff);
        fileBuff = NULL;
    }

    return ret;
}

int frmSetConfigFileV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    // 取得配置文件包数据,将其Base64编码转换.
    char *fileBuff = NULL;
    unsigned int fileSize = 0;
    if (0 == ret)
    {
        //websWriteBlock(wp, fileBuff, fileSize);
        unsigned int base64TextSize = 0;
        char *base64TextBuff = NULL;

        if (Common_Json_GetAttrValueStr(indata, "Base64Text", &base64TextBuff) == NULL ||
                (base64TextSize = strlen(base64TextBuff)) < 32)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            fileBuff = Common_Base64_Decode(base64TextBuff, base64TextSize, &fileSize);
        }
    }

    // 访问配置文件包.
    FILE *fp = NULL;
    if (0 == ret)
    {
        struct stat tmpStat;
        int tryCount;

        for (tryCount = 0; tryCount < 5; tryCount++)
        {
            if ((stat(FILE_CONFIG_EXPORT, &tmpStat) == 0) || ((fp = fopen(FILE_CONFIG_EXPORT, "wb")) == NULL))
            {
                usleep(100000);
            }
            else
            {
                break;
            }
        }
        if (tryCount == 5)
        {
            ret = WEB_CODE_FileNotAccess;
        }
    }

    // 写入文件内容.
    if (0 == ret)
    {
        int readSize = 0;
        //printf("该文件的长度为%1d字节\n",length);
        if ((readSize = fwrite(fileBuff, fileSize,1, fp)) > 0)
        {
            fileBuff[fileSize] = '\0';
        }
    }

    if (fp)
    {
        fclose(fp);
        fp = NULL;
    }

    if (fileBuff)
    {
        Common_Free(fileBuff, __FUNCTION__, __LINE__);
        fileBuff = NULL;
    }

    // 应用配置文件包.
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(lowerData, "FileName", FILE_CONFIG_EXPORT);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/ImportCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    remove(FILE_CONFIG_EXPORT);

    return ret;
}

int frmSetUuid(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    // request: "Data":{"BurnParam":[{"Name":"TUTK_UID","Uuid":"abcd123456//id=%d//langure=%d//model=%s"}]}
    // response: "Data":{"BurnRes":[{"BurnResult":0,"Name":"TUTK_UID","Uuid":"abcd123456//id=%d//langure=%d//model=%s"}]}

    // request: "Data":{"BurnParam":[{"Name":"GOOLINK_UID","Uuid":"abcd9923456"}]}
    // response: "Data":{"BurnRes":[{"BurnResult":-1,"Name":"GOOLINK_UID","Uuid":"abcd9923456"}]}

    // request: "Data":{"BurnParam":[{"Name":"DeviceID","Uuid":"abcd9923456","HostIp":"app.umeye.cn","HostPort":5800}]}
    // response: "Data":{"BurnRes":[{"BurnResult":-2,"Name":"DeviceID","Uuid":"abcd123456"}]}

    // 其中,BurnResult的值意义为,0-成功,-1-调用接口失败,-2-表示已经烧录.

    cJSON_Struct *burnParam = NULL;
    if (0 == ret)
    {
        if ((burnParam = Common_Json_GetAttrValueArr(indata, "BurnParam")) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        int burnParamSize = Common_Json_ArraySize(burnParam);
        cJSON_Struct *outBurnres = Common_Json_SetAttrValueArr(outdata, "BurnRes");

        cJSON_Struct *burnParamEach = NULL;
        cJSON_Struct *outBurnresEach = NULL;

        int i;
        for (i = 0; i < burnParamSize; i++)
        {
            outBurnresEach = Common_Json_SetAttrValueArrObj(outBurnres, i);
            burnParamEach = Common_Json_GetAttrValueArrItem(burnParam, i);

            char *nameValue = NULL;
            char *uuidValue = NULL;
            if (0 == ret)
            {
                if (Common_Json_GetAttrValueStr(burnParamEach, "Name", &nameValue) == NULL ||
                        Common_Json_GetAttrValueStr(burnParamEach, "Uuid", &uuidValue) == NULL)
                {
                    ret = WEB_CODE_InvalidArg;
                }
                else
                {
                    Common_Json_SetAttrValueStr(lowerData, "ServerName", nameValue);
                    Common_Json_SetAttrValueStr(lowerData, "UUID", uuidValue);
                }
            }

            if (0 == ret)
            {
                if (strcmp(nameValue, "DeviceID") == 0)
                {
                    char *hostIp = NULL;
                    int hostPort = 0;
                    if (Common_Json_GetAttrValueStr(burnParamEach, "HostIp", &hostIp) == NULL)
                    {
                        ret = WEB_CODE_InvalidArg;
                    }
                    else
                    {
                        Common_Json_SetAttrValueStr(lowerData, "ServerAddr", hostIp);
                    }
                    if (Common_Json_GetAttrValueInt(burnParamEach, "HostPort", &hostPort) == NULL)
                    {
                        ret = WEB_CODE_InvalidArg;
                    }
                    else
                    {
                        Common_Json_SetAttrValueInt(lowerData, "ServerPort", hostPort);
                    }
                }
                else
                {
                    Common_Json_RemoveItem(lowerData, -1, "HostIp");
                    Common_Json_RemoveItem(lowerData, -1, "HostPort");
                }
            }

            if (0 == ret)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/AccessHost/Burn");
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
                Common_Json_SetAttrValueInt(outBurnresEach, "BurnResult", ret == 0 ? 0 : (ret == -12 ? -2 : -1));
                ret = 0;
            }
            Common_Json_SetAttrValueStr(outBurnresEach, "Name", nameValue);
            Common_Json_SetAttrValueStr(outBurnresEach, "Uuid", uuidValue);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


int web_semantic_get_bwlist(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int enable = -1;
    //char* str = NULL;

    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *arr = NULL;

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/AccessControl/AccessMode");
        ret = Ovfs_Web_RestMethodA(header, NULL,&lowerData, 0);
        if(ret == 0)
        {

            if (Common_Json_GetAttrValueInt(lowerData, "Mode",&enable))
            {
                Common_Json_SetAttrValueInt(outdata, "Mode", enable);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    LOGD("\n-------mode:%d ----ret:%d\n",enable,ret);
    if(ret == 0)
    {
        if(enable == 2)
        {

            Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/AccessControl/blacklist");
            ret = Ovfs_Web_RestMethodA(header, NULL,&lowerData, 0);
            if(ret == 0)
            {
                arr = Common_cJSON_DetachItemFromObject(lowerData,"ResList");
                if (arr)
                {
                    Common_Json_AddItem(outdata, -1,"BlackList",arr);
                }
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
        else if(enable == 1)
        {
            Ovfs_Web_UpdateHeader(header, REST_GET, "/Access/AccessControl/whitelist");
            ret = Ovfs_Web_RestMethodA(header, NULL,&lowerData, 0);
            if(ret == 0)
            {
                arr = Common_cJSON_DetachItemFromObject(lowerData,"ResList");
                if (arr)
                {
                    Common_Json_AddItem(outdata, -1,"WhiteList",arr);
                }
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    return ret;
}
int web_semantic_set_bwlist(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int mode = -1;
    //char* str = NULL;
    char* action = NULL;
    int method = 0;
    char* uri = NULL;

    cJSON_Struct *lowerData = NULL;
    //cJSON_Struct *data = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        //Common_Json_SetAttrValueInt(lowerData, "Enable", check_type);
        Common_Json_GetAttrValueInt(indata, "Mode",&mode);//mode type
        Common_Json_GetAttrValueStr(indata, "Action",&action);//add or del or set

        if(mode == 2)
        {
            uri = "/Access/AccessControl/blacklist";
        }
        else if(mode == 1)
        {
            uri = "/Access/AccessControl/whitelist";
        }
        LOGD("----------->mode:%d action:%s",mode,action);
        if(smatch(action,"Add"))
        {
            method = REST_POST;
            Common_cJSON_T* ip_list = Common_Json_GetAttrValueArr(indata, "IpList");
            Common_Json_AddItem(lowerData, -1, "ResList", Common_cJSON_Duplicate(ip_list,1));
        }
        else if(smatch(action,"Delete"))
        {
            method = REST_DELETE;
            Common_cJSON_T* ip_list = Common_Json_GetAttrValueArr(indata, "IpList");
            Common_Json_AddItem(lowerData, -1, "ResList", Common_cJSON_Duplicate(ip_list,1));
        }
        else if(smatch(action,"Set"))
        {
            method = REST_PUT;
            uri = "/Access/AccessControl/AccessMode";
            Common_Json_SetAttrValueInt(lowerData,"Mode",mode);
        }
        LOGD("----------->method:%d uri:%s",method,uri);

        Ovfs_Web_UpdateHeader(header, method, uri);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);


    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

//硬件检查
int frmBlackWhiteList(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    //int check_type = -1;
    //cJSON_Struct *burnParam = NULL;
    //cJSON_Struct *lowerData = NULL;



    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0://get mode wlist or blist
            //获取参数
            ret = web_semantic_get_bwlist(header,opt,header, indata, outdata);
            break;

        case 1://del items in white or black list
            //设置参数
            ret = web_semantic_set_bwlist(header,opt,header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }


    return ret;
}



int web_semantic_get_localsettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    //get
    Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.WndMode",  (S32*)&(g_ovfs_web->plugin_params.wm));

    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PrevCapture",  &(g_ovfs_web->plugin_params.prev_capture_path));
    LOGD("%s\n",g_ovfs_web->plugin_params.prev_capture_path);
    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PbCapture",  &g_ovfs_web->plugin_params.pb_capture_path);
    LOGD("%s\n",g_ovfs_web->plugin_params.pb_capture_path);

    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.FileCapture",  &g_ovfs_web->plugin_params.file_capture_path);
    LOGD("%s\n",g_ovfs_web->plugin_params.file_capture_path);
    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.BackupPath",  &g_ovfs_web->plugin_params.backup_path);
    LOGD("%s\n",g_ovfs_web->plugin_params.backup_path);

    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.RecPath",  &g_ovfs_web->plugin_params.rec_path);
    LOGD("%s\n",g_ovfs_web->plugin_params.rec_path);
    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.PlatePics",  &g_ovfs_web->plugin_params.plate_pics);
    Common_Json_GetAttrValueStr(g_ovfs_config, "PluginParams.FacePics",  &g_ovfs_web->plugin_params.face_pics);

    Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.RecFormat",	(S32*)&g_ovfs_web->plugin_params.rec_file_format);
    Common_Json_GetAttrValueInt(g_ovfs_config, "PluginParams.PicQuality",	&g_ovfs_web->plugin_params.prev_buf_val);

    Common_Json_GetAttrValueInt(g_ovfs_config, "PasswordTips",   &g_ovfs_web->password_tips);

    Common_Json_GetAttrValueStr(g_ovfs_config, "FacePicFormat",   &g_ovfs_web->face_picture_format);

    //set
    Common_Json_SetAttrValueInt(outdata, "WndMode",  g_ovfs_web->plugin_params.wm);

    Common_Json_SetAttrValueStr(outdata, "PrevCapture",  g_ovfs_web->plugin_params.prev_capture_path?g_ovfs_web->plugin_params.prev_capture_path:"");
    Common_Json_SetAttrValueStr(outdata, "PbCapture",  g_ovfs_web->plugin_params.pb_capture_path?g_ovfs_web->plugin_params.pb_capture_path:"");
    Common_Json_SetAttrValueStr(outdata, "FileCapture",  g_ovfs_web->plugin_params.file_capture_path?g_ovfs_web->plugin_params.file_capture_path:"");
    Common_Json_SetAttrValueStr(outdata, "BackupPath",  g_ovfs_web->plugin_params.backup_path?g_ovfs_web->plugin_params.backup_path:"");
    Common_Json_SetAttrValueStr(outdata, "RecPath",  g_ovfs_web->plugin_params.rec_path?g_ovfs_web->plugin_params.rec_path:"");
    Common_Json_SetAttrValueStr(outdata, "PlatePics",  g_ovfs_web->plugin_params.plate_pics?g_ovfs_web->plugin_params.plate_pics:"");
    Common_Json_SetAttrValueStr(outdata, "FacePics",  g_ovfs_web->plugin_params.face_pics?g_ovfs_web->plugin_params.face_pics:"");

    Common_Json_SetAttrValueInt(outdata, "RecFormat",	g_ovfs_web->plugin_params.rec_file_format);
    Common_Json_SetAttrValueInt(outdata, "PicQuality",	g_ovfs_web->plugin_params.prev_buf_val);

    Common_Json_SetAttrValueInt(outdata, "PasswordTips",  g_ovfs_web->password_tips);

    Common_Json_SetAttrValueStr(outdata, "FacePicFormat",  g_ovfs_web->face_picture_format?g_ovfs_web->face_picture_format:"");
    {
        char* out = Common_Json_Print(outdata, NULL);
        LOGD("%s\n",out);
        wfree(out);
    }
    return ret;
}

int web_semantic_set_localsettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int nval = 0;
    int tips = 0;
    char* str_tmp = NULL;
    char* str_format = NULL;
    int changed = FALSE;
    cJSON_Struct *pObj_tmp = NULL;
    //cJSON_Struct *data = NULL;

    OVFS_WEB_PLUGIN_PARAMS_T plugin_tmp;
    memset(&plugin_tmp,0,sizeof(OVFS_WEB_PLUGIN_PARAMS_T));

    if(ret == 0)
    {
        //set
        if(Common_Json_GetAttrValueInt(indata, "WndMode",  (S32*)(&plugin_tmp.wm)))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueInt(g_ovfs_config,  "PluginParams.WndMode", &nval);
            if(pObj_tmp)
            {
                if(nval != plugin_tmp.wm)
                {
                    ((Common_cJSON_T *)pObj_tmp)->valueint = plugin_tmp.wm;
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "PluginParams.WndMode",plugin_tmp.wm);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "PrevCapture",  &plugin_tmp.prev_capture_path))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.PrevCapture", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.prev_capture_path))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.prev_capture_path);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.PrevCapture",plugin_tmp.prev_capture_path);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "PbCapture",  &plugin_tmp.pb_capture_path))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.PbCapture", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.pb_capture_path))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.pb_capture_path);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.PbCapture",plugin_tmp.pb_capture_path);
                changed = TRUE;
            }


        }

        if(Common_Json_GetAttrValueStr(indata, "FileCapture",  &plugin_tmp.file_capture_path))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.FileCapture", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.file_capture_path))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.file_capture_path);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.FileCapture",plugin_tmp.file_capture_path);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "BackupPath",  &plugin_tmp.backup_path))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.BackupPath", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.backup_path))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.backup_path);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.BackupPath",plugin_tmp.backup_path);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "RecPath",  &plugin_tmp.rec_path))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.RecPath", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.rec_path))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.rec_path);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.RecPath",plugin_tmp.rec_path);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "PlatePics",  &plugin_tmp.plate_pics))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.PlatePics", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.plate_pics))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.plate_pics);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.PlatePics",plugin_tmp.plate_pics);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "FacePics",	&plugin_tmp.face_pics))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "PluginParams.FacePics", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,plugin_tmp.face_pics))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(plugin_tmp.face_pics);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "PluginParams.FacePics",plugin_tmp.face_pics);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "RecFormat", (S32*)(&plugin_tmp.rec_file_format)))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueInt(g_ovfs_config,  "PluginParams.RecFormat", &nval);
            if(pObj_tmp)
            {
                if(nval != plugin_tmp.rec_file_format)
                {
                    ((Common_cJSON_T *)pObj_tmp)->valueint = plugin_tmp.rec_file_format;
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "PluginParams.RecFormat",plugin_tmp.rec_file_format);
                changed = TRUE;
            }
        }



        if(Common_Json_GetAttrValueInt(indata, "PicQuality",	&plugin_tmp.prev_buf_val))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueInt(g_ovfs_config,  "PluginParams.PicQuality", &nval);
            if(pObj_tmp)
            {
                if(nval != plugin_tmp.prev_buf_val)
                {
                    ((Common_cJSON_T *)pObj_tmp)->valueint = plugin_tmp.prev_buf_val;
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "PluginParams.PicQuality",plugin_tmp.prev_buf_val);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "PasswordTips",	&tips))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueInt(g_ovfs_config,  "PasswordTips", &nval);
            if(pObj_tmp)
            {
                if(nval != tips)
                {
                    ((Common_cJSON_T *)pObj_tmp)->valueint = tips;
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueInt(g_ovfs_config, "PasswordTips",tips);
                changed = TRUE;
            }
        }

        if(Common_Json_GetAttrValueStr(indata, "FacePicFormat",  &str_format))
        {
            //set
            pObj_tmp = Common_Json_GetAttrValueStr(g_ovfs_config,  "FacePicFormat", &str_tmp);
            if(pObj_tmp)
            {
                if(!smatch(str_tmp,str_format))
                {
                    ((Common_cJSON_T *)pObj_tmp)->valuestring = sclone(str_format);
                    changed = TRUE;
                }
            }
            else
            {
                Common_Json_SetAttrValueStr(g_ovfs_config, "FacePicFormat",str_format);
                changed = TRUE;
            }
        }

        if(changed)
        {
            char* out = Common_Json_Print(g_ovfs_config, NULL);
            LOGD("%s\n",out);
            wfree(out);
            Access_SaveConfig(g_AccessHandle, g_ovfs_config);
        }
    }
    return ret;
}


//插件中的路径配置
int frmLocalSettings(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    //int check_type = -1;
    //cJSON_Struct *burnParam = NULL;
    //cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0://get localsettings
            //获取参数
            ret = web_semantic_get_localsettings(header,opt,header, indata, outdata);
            break;

        case 1://set localsettings
            //设置参数
            ret = web_semantic_set_localsettings(header,opt,header, indata, outdata);
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }


    return ret;
}

int root_file_status = -1;

int web_semantic_set_logopic(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
	int ret = 0;
	char* str = NULL;
    if(Common_Json_GetAttrValueStr(indata, "ImageData",&str) == NULL)
	{
        ret = WEB_CODE_InvalidArg;
	}

    if(ret != 0 || str == NULL || slen(str) == 0)
    {
        ret = WEB_CODE_InvalidArg;
    }

	if(ret == 0)
	{
        int len = 0;
        char *pic_decode = NULL;
        pic_decode = Common_Base64_Decode(str, slen(str), &len);
        LOGD("Pic len:%d\n",len);
        FILE *fd;
        fd = Common_File_fOpen("/tmp/logo.png", "wb");
        if(fd)
        {
            Common_File_fWrite(pic_decode, 1, len, fd);
            Common_File_fClose(fd);
        }

        Common_Free(pic_decode, __FUNCTION__, __LINE__);

        if(root_file_status == -1)
        {
            char cmdBuffer[1024];

            snprintf(cmdBuffer, sizeof(cmdBuffer),
                "rm -r /dev/root_file_status;"
                "cd /root/nginx/html/static/images;touch test_file;"
                "if [ $? == 0 ]; then echo \"ok\" > /dev/root_file_status;fi"
                "rm -r test_file"
                );

            Common_System(cmdBuffer);

            root_file_status = Common_File_IsExist("/dev/root_file_status")?1:0;
        }

        LOGD("root_file_status:[%d]\n",root_file_status);
        if(root_file_status)
        {

            int file_type = 0;
            file_type = getFileType("/root/nginx/html/static/images/logo.png");
            LOGW("logo.png file type:[%d]\n",file_type);

            if(file_type == 5)
            {
#if 0//def WIFIDOME
                Common_System("mv /tmp/logo.png /usr/etc/res/web/logo.png");
#else
                Common_System("mv /tmp/logo.png /usr/etc/logo.png");
#endif
            }
            else
            {
                Common_System("cp -r /tmp/logo.png /root/nginx/html/static/images/logo.png");
                Common_System("mv /tmp/logo.png /update/nginx/html/static/images/logo.png");
            }
        }
        else
        {
#if 0//def WIFIDOME
            Common_System("mv /tmp/logo.png /usr/etc/res/web/logo.png");
#else
            Common_System("mv /tmp/logo.png /usr/etc/logo.png");
#endif
        }
	}

	return ret;
}

int frmSetLogoPic(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch ( opt->type )
	{
		case 1:
			ret = web_semantic_set_logopic(header, indata, outdata);
			break;
		default:
			ret = WEB_CODE_InvalidArg;
			break;
	}

    if ( 0 == ret )
	{
		Common_Json_SetAttrValueStr ( outdata, STATUS_CODE, SAVE_OK );
	}

    return ret;
}

int web_semantic_setircut(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    char *mode = NULL;
    cJSON_Struct * lowerData = NULL;

    if(Common_Json_GetAttrValueStr(indata, "TestPara/Mode", &mode) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }


    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueObj(lowerData, "DayNightMode");
        if(smatch(mode, "Day"))
        {
            Common_Json_SetAttrValueInt(lowerData, "DayNightMode/ircutOutTrig", 0);
        }
        else if(smatch(mode, "Night"))
        {
            Common_Json_SetAttrValueInt(lowerData, "DayNightMode/ircutOutTrig", 1);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if(ret == 0)
    {
        ret = web_semantic_set_videoparaex(header, lowerData, outdata, opt);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

Common_Thread_T hLightThread = NULL;
int reset_flag = 0;     //0-none 1-resetting 2-has refreshed count
void *Fxn_Web_LightRest()
{
    int count = 0;
    cJSON_Struct *header = NULL;
    cJSON_Struct * lowerData = NULL;

    header = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueObj(header, "Auth");
    Common_Json_SetAttrValueInt(header, "Auth/Method", 1);
    Common_Json_SetAttrValueStr(header, "Auth/Username", "(null)");
    Common_Json_SetAttrValueStr(header, "Auth/Password", "ovfsZSJQZLHL");

    lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    Common_Json_SetAttrValueInt(lowerData, "IcrLightMode", 0);

    while(1)
    {
        Common_Sleep(1, 0);

        if(reset_flag == 1)
        {
            count = 0;
            reset_flag = 2;
        }
        else if(reset_flag == 0)
        {
            continue;
        }

        if(count > 2)
        {
            count = 0;
            web_semantic_set_videoparaex(header, lowerData, NULL, NULL);
            reset_flag = 0;
        }

        count++;
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(header)
    {
        Common_Json_Delete(header);
        header = NULL;
    }
}

int web_semantic_setlight(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    char *type = NULL;
    cJSON_Struct * lowerData = NULL;

    if(Common_Json_GetAttrValueStr(indata, "TestType", &type) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }


    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(lowerData, "IcrLightMode", 1);
        Common_Json_SetAttrValueInt(lowerData, "IcrLightAue", 100);
        if(smatch(type, "AlarmLight"))
        {
            Common_Json_SetAttrValueStr(lowerData, "LightType", "Warm");
        }
        else if(smatch(type, "FillLight"))
        {
            Common_Json_SetAttrValueStr(lowerData, "LightType", "Ir");
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    if(ret == 0)
    {
        ret = web_semantic_set_videoparaex(header, lowerData, outdata, opt);
        /*reset_flag = 1;
        if(hLightThread == NULL)
        {
            Common_Thread_Create(&hLightThread, __FUNCTION__, 0, 0, Fxn_Web_LightRest, NULL);
        }*/
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_getlightinfo(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    char *type = NULL;
    cJSON_Struct * lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Sys/BoardAbility");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueStr(lowerData, "IrBoardType", &type))
        {
            Common_Json_SetAttrValueStr(outdata, "IrBoardType", type);
        }
        else
        {
            ret = WEB_CODE_InternalMistake;
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_getwifiinfo(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    char *str_tmp = NULL;
    cJSON_Struct * lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/NetAttr/WifiSignal");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueStr(lowerData, "Ssid", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "Ssid", str_tmp);
        }
        else
        {
            ret = WEB_CODE_InternalMistake;
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_getmotorcfg(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct * lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Conf");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "PtzCustomParam/XMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "XMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "PtzCustomParam/YMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "YMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "ReverseMove", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "ReverseMove", i_num);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_setmotorcfg(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_SetAttrValueObj(lowerData, "PtzCustomParam");
        if(Common_Json_GetAttrValueInt(indata, "TestPara/XMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "PtzCustomParam/XMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "TestPara/YMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "PtzCustomParam/YMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "TestPara/ReverseMove", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "ReverseMove", i_num);
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Conf");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

#ifndef AWSIOT
    {
        if(!Common_File_IsExist((char *)"/usr/etc/default"))
        {
            Common_System("mkdir /usr/etc/default");
        }

        S8 szCmd[128] = {0};
        snprintf(szCmd, sizeof(szCmd), "cp -r /usr/etc/cfgfiles/Ptz.json /usr/etc/default/Ptz.json");
        Common_System(szCmd);
    }
#endif

    return ret;
}

int web_semantic_PtzReverseControl(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct * lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "TestPara/ReverseControl", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Reverse", i_num);
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/reverseControl");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

#ifndef AWSIOT
    {
        if(!Common_File_IsExist((char *)"/usr/etc/default"))
        {
            Common_System("mkdir /usr/etc/default");
        }

        S8 szCmd[128] = {0};
        snprintf(szCmd, sizeof(szCmd), "cp -r /usr/etc/cfgfiles/Ptz.json /usr/etc/default/Ptz.json");
        Common_System(szCmd);
    }
#endif

    return ret;
}

int web_set_cfg_and_lock(cJSON_Struct* indata)
{
    int ret = 0;
    int i_num = 0;
    int need_lock = 0;
    cJSON_Struct *fileDefaultCfg = NULL;
    cJSON_Struct *temp = NULL;

    Access_LoadConfigByType(g_AccessHandle,Access_ConfigType_Default,&fileDefaultCfg);

    if(Common_Json_GetAttrValueObj(fileDefaultCfg, "ThirdPartyProtocols") == NULL)
    {
        Common_Json_SetAttrValueObj(fileDefaultCfg, "ThirdPartyProtocols");
    }

    temp = Common_Json_GetAttrValueObj(indata, "Protocol");
    if(temp)
    {
        if(Common_Json_GetAttrValueInt(temp, "HK/Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(fileDefaultCfg, "ThirdPartyProtocols/HK", i_num);
            need_lock = 1;
        }

        if(Common_Json_GetAttrValueInt(temp, "TST/Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(fileDefaultCfg, "ThirdPartyProtocols/TST", i_num);
            need_lock = 1;
        }

        web_semantic_set_thirdpartyprotocols(NULL, temp, NULL);
    }

    temp = Common_Json_GetAttrValueObj(indata, "Protocol/Onvif");
    if(temp)
    {
        if(Common_Json_GetAttrValueInt(temp, "AllNet", &i_num))
        {
            Common_Json_SetAttrValueInt(fileDefaultCfg, "OnvifCfg/AdaptiveIp", i_num);
            Common_Json_SetAttrValueInt(temp, "adaptiveIp", i_num);
            need_lock = 1;
        }

        if(Common_Json_GetAttrValueInt(temp, "Timeout", &i_num))
        {
            Common_Json_SetAttrValueInt(fileDefaultCfg, "OnvifCfg/Timeout", i_num);
            need_lock = 1;
        }

        web_semantic_set_onvifpara(NULL,temp,NULL,NULL);
    }

    if(need_lock)
    {
        Access_SaveConfigByType(g_AccessHandle, Access_ConfigType_Default, fileDefaultCfg);
    }

    Common_Json_Delete(fileDefaultCfg);
    fileDefaultCfg = NULL;

    return ret;
}

int parse_lightcfg(cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int count = 0;
    int ret = 0;
    int i_lightcontrol = 1;
    int i_num = 0;
    char *str_mode = NULL;
    cJSON_Struct *tmp = NULL;

    if(outdata == NULL)return -1;

    if(ret == 0)
    {
        cJSON_Struct *list = Common_Json_SetAttrValueArr(outdata, "ImageList");
        if(Common_Json_GetAttrValueStr(indata, "Mode", &str_mode))
        {
            tmp = Common_Json_SetAttrValueArrObj(list, count++);
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Type", 31);
            Common_Json_SetAttrValueObj(tmp, "Param");
            Common_Json_SetAttrValueStr(tmp, "Param/Type", str_mode);
        }

        tmp = Common_Json_SetAttrValueArrObj(list, count++);
        Common_Json_SetAttrValueInt(tmp, "Device", 0);
        Common_Json_SetAttrValueInt(tmp, "Type", 0);
        Common_Json_SetAttrValueObj(tmp, "Param");

        cJSON_Struct *tmp2 = Common_Json_SetAttrValueArrObj(list, count);
        Common_Json_SetAttrValueInt(tmp2, "Device", 0);
        Common_Json_SetAttrValueInt(tmp2, "Type", 12);
        Common_Json_SetAttrValueObj(tmp2, "Param");

        if(Common_Json_GetAttrValueInt(indata, "IRCut", &i_num))
        {
            Common_Json_SetAttrValueInt(tmp, "Param/ircutOutTrig", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LightOnTime", &i_num))
        {
            Common_Json_SetAttrValueInt(tmp, "Param/DayEnd", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LightOffTime", &i_num))
        {
            Common_Json_SetAttrValueInt(tmp, "Param/DayStart", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Brightness", &i_num))
        {
            Common_Json_SetAttrValueInt(tmp2,"Param/Level",i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Control", &i_num))
        {
            if(i_num == 3)
            {
                Common_Json_SetAttrValueInt(tmp, "Param/Mode", 4);

                Common_Json_SetAttrValueInt(tmp2,"Param/Enable",1);
                Common_Json_SetAttrValueInt(tmp2,"Param/Mode",0);
            }
            else if(i_num == 4)
            {
                Common_Json_SetAttrValueInt(tmp, "Param/Mode", 0);

                Common_Json_SetAttrValueInt(tmp2,"Param/Enable",1);
                Common_Json_SetAttrValueInt(tmp2,"Param/Mode",0);
            }
            else
            {
                if(str_mode)
                {
                    if(smatch(str_mode, "Warm"))
                    {
                        i_lightcontrol = 2;
                    }
                    else
                    {
                        if(i_num == 1)
                        {
                            i_lightcontrol = 3;
                        }
                    }

                    //i_lightcontrol = smatch(str_mode, "Warm") ? 2 : 1;
                    Common_Json_SetAttrValueInt(tmp, "Param/Mode", i_lightcontrol);
                }

                int mode = 0;
                int enable = 0;
                if(i_num == 2)
                {
                    enable = 1;
                }
                else if(i_num == 1)
                {
                    enable = 1;
                    mode = 1;
                }

                Common_Json_SetAttrValueInt(tmp2,"Param/Enable",enable);
                Common_Json_SetAttrValueInt(tmp2,"Param/Mode",mode);
            }
        }

        cJSON_Struct *param = Common_Json_GetAttrValueObj(tmp2, "Param");
        if(Common_Json_Size(param) == 0)
        {
            Common_Json_RemoveItem(list, count, NULL);
        }

        count--;
        param = Common_Json_GetAttrValueObj(tmp, "Param");
        if(Common_Json_Size(param) == 0)
        {
            Common_Json_RemoveItem(list, count, NULL);
        }

    }

    return ret;
}

int web_semantic_lockcfg(cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    char url[128] = {0};
    cJSON_Struct * lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        cJSON_Struct *para = Common_Json_SetAttrValueObj(lowerData, "TestPara");
        if(Common_Json_GetAttrValueInt(indata, "Ptz/XMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "TestPara/XMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Ptz/YMaxStep", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "TestPara/YMaxStep", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "Ptz/ReverseMove", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "TestPara/ReverseMove", i_num);
        }

        if(Common_Json_Size(para) > 0)
        {
            ret = web_semantic_setmotorcfg(header, lowerData, outdata);
        }
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueStr(indata, "Web/Logo", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "ImageData", str_tmp);

            ret = web_semantic_set_logopic(header, lowerData, outdata);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(ret == 0)
    {
        ret = web_set_cfg_and_lock(indata);
    }

    if(ret == 0)
    {
        lowerData = Common_Json_GetAttrValueObj(indata, "DeviceInfo");
        if(lowerData)
        {
            cJSON_Struct *data = Common_Json_Duplicate(lowerData, 1);
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/Core/Version");
            ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);
            Common_Json_Delete(data);
        }
    }

    if(ret == 0)
    {
        snprintf(url,sizeof(url),"/BoardSys/Audio/Attribute/All");
        lowerData = Common_cJSON_CreateObject();
        Common_Json_SetAttrValueStr(lowerData, "uri", url);
        cJSON_Struct *param = Common_Json_SetAttrValueObj(lowerData, "param");
        if (Common_Json_GetAttrValueInt(indata, "Volume/InputVolume", &i_num))
        {
            Common_Json_SetAttrValueInt(param, "InputVol", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "Volume/OutputVolume", &i_num))
        {
            Common_Json_SetAttrValueInt(param, "OutputVol", i_num);
        }

        if(Common_Json_Size(param) > 0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, url);
            cJSON_Struct *data = Common_Json_Duplicate(param, 1);
            ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);
            Common_Json_Delete(data);
            data = NULL;
            if(ret == 0)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/boardsys/Sys/LockParam");
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            }
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    if(ret == 0)
    {
        snprintf(url,sizeof(url),"/BoardSys/Image/Attribute/All");
        lowerData = Common_cJSON_CreateObject();
        Common_Json_SetAttrValueStr(lowerData, "uri", url);
        cJSON_Struct *param = Common_Json_SetAttrValueObj(lowerData, "param");
        cJSON_Struct *light = Common_Json_GetAttrValueObj(indata, "Light");


        if (Common_Json_GetAttrValueInt(indata, "Image/IRCut", &i_num))
        {
            if(light == NULL)
            {
                light = Common_Json_SetAttrValueObj(indata, "Light");
            }
            Common_Json_SetAttrValueInt(light, "IRCut", i_num);
        }

        parse_lightcfg(light, param);

        if (Common_Json_GetAttrValueInt(indata, "Image/Mirror", &i_num))
        {
            cJSON_Struct *list = Common_Json_GetAttrValueArr(param, "ImageList");
            if(list == NULL)
            {
                list = Common_Json_SetAttrValueArr(param, "ImageList");
            }

            int size = Common_Json_ArraySize(list);
            cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(list, size);
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Type", 10);
            Common_Json_SetAttrValueObj(tmp, "Param");
            Common_Json_SetAttrValueInt(tmp, "Param/Mode", i_num);
        }

        if(Common_Json_Size(param) > 0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, url);
            cJSON_Struct *data = Common_Json_Duplicate(param, 1);
            ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);
            Common_Json_Delete(data);
            data = NULL;
            if(ret == 0)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/boardsys/Sys/LockParam");
                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            }
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    return ret;
}

int frmProductTest(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    char *testType = NULL;
    if(Common_Json_GetAttrValueStr(indata, "TestType", &testType) == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    if(ret == 0)
    {
        if(smatch(testType, "Ptz"))
        {
            int cmd = 0, speed = 0;
            if(Common_Json_GetAttrValueInt(indata, "TestPara/Cmd", &cmd) == NULL ||
                Common_Json_GetAttrValueInt(indata, "TestPara/Speed", &speed) == NULL)
            {
                ret = WEB_CODE_InvalidArg;
            }

            if(cmd<0 || cmd>6)
            {
                ret = WEB_CODE_InvalidArg;
            }

            if(speed<0 || speed>10)
            {
                ret = WEB_CODE_InvalidArg;
            }

            if(ret == 0)
            {
                cJSON_Struct *data = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
                int arr[7] = {21,21,22,23,24,29,10000};
                /*Common_Json_SetAttrValueInt(data, "Type", arr[cmd]);
                Common_Json_SetAttrValueObj(data, "CmdParam");
                Common_Json_SetAttrValueInt(data, "CmdParam/Device", opt->dev);
                Common_Json_SetAttrValueInt(data, "CmdParam/Stop", cmd ? 0: 1);
                Common_Json_SetAttrValueInt(data, "CmdParam/Speed", speed);
                Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Cmd");
                ret = Ovfs_Web_RestMethodA(header, data, NULL, 0);*/

                Common_Json_SetAttrValueInt(data, "Cmd", arr[cmd]);
                Common_Json_SetAttrValueInt(data, "IsStop", cmd ? 0: 1);
                Common_Json_SetAttrValueInt(data, "Speed", speed);
                ret = web_semantic_ptzcontrol(opt,header,data,NULL);

                Common_Json_Delete(data);
                data = NULL;
            }

        }
        else if(smatch(testType, "Preset"))
        {
            int preset = 0;
            char *str_tmp = NULL;
            char uri[64]= {0};
            if(Common_Json_GetAttrValueInt(indata, "TestPara/PresetNo", &preset) == NULL)
            {
                ret = WEB_CODE_InvalidArg;
            }

            Common_Json_GetAttrValueStr(indata, "TestPara/OperationType", &str_tmp);
            if(smatch(str_tmp, "Set"))
            {
                snprintf(uri, sizeof(uri), "/Ptz/Preset?Token=%d", preset);
            }
            else if(smatch(str_tmp, "Call"))
            {
                snprintf(uri, sizeof(uri), "/Ptz/Preset?Goto=%d", preset);
            }
            else
            {
                ret = WEB_CODE_InvalidArg;
            }

            if(ret == 0)
            {
                Ovfs_Web_UpdateHeader(header, REST_PUT, uri);
                ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
            }
        }
        else if(smatch(testType, "Restore"))
        {
            cJSON_Struct *data = Common_Json_GetAttrValueObj(indata, "TestPara");
            ret = frmDeviceRestore(wp, opt, header, data, outdata);

        }
        else if(smatch(testType, "SDInfo"))
        {
            ret = web_semantic_gethdinfo(header, indata, outdata);
        }
        else if(smatch(testType, "FormatSD"))
        {
            cJSON_Struct *data = Common_Json_GetAttrValueObj(indata, "TestPara");
            ret = web_semantic_hdformat(header, data, outdata);
        }
        else if(smatch(testType, "Horn"))
        {
            Common_Json_SetAttrValueInt(indata, "DetectType", 1);
            Common_Json_SetAttrValueInt(indata, "AudioSelected", 0);
            ret = frmAudioSpeech(wp, opt, header, indata, outdata);
        }
        else if(smatch(testType, "SyncTime"))
        {
            cJSON_Struct *data = Common_Json_GetAttrValueObj(indata, "TestPara");
            int offset = 0;
            if(Common_Json_GetAttrValueInt(data, "TimeZoneOffset", &offset))
            {
                Common_Json_SetAttrValueInt(data, "TimeOffsetHour", offset/60);
                Common_Json_SetAttrValueInt(data, "TimeOffsetMinute", offset%60);
            }

            ret = web_semantic_set_netntppara(header, data, outdata);
            if(ret == 0)
            {
                ret = web_semantic_set_devicetimectrl(header, data, outdata);
            }
        }
        else if(smatch(testType, "Ircut"))
        {
            ret = web_semantic_setircut(header, indata, outdata, opt);
        }
        else if(smatch(testType, "AlarmLight")|| smatch(testType, "FillLight"))
        {
            ret = web_semantic_setlight(header, indata, outdata, opt);
        }
        else if(smatch(testType, "LightTypeInfo"))
        {
            ret = web_semantic_getlightinfo(header, indata, outdata);
        }
        else if(smatch(testType, "Wifi"))
        {
            ret = web_semantic_getwifiinfo(header, indata, outdata);
        }
        else if(smatch(testType, "MotorCfg"))
        {
            ret = web_semantic_setmotorcfg(header, indata, outdata);
        }
        else if(smatch(testType, "PtzCfg"))
        {
            ret = web_semantic_setmotorcfg(header, indata, outdata);
        }
        else if(smatch(testType, "LockCfg"))
        {
            cJSON_Struct *data = Common_Json_GetAttrValueObj(indata, "TestPara");
            ret = web_semantic_lockcfg(header, data, outdata);
        }
        else if(smatch(testType, "GetPtzCfg"))
        {
            ret = web_semantic_getmotorcfg(header, indata, outdata);
        }
        else if(smatch(testType, "PtzReverseControl"))
        {
            ret = web_semantic_PtzReverseControl(header, indata, outdata);
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }
    }

    return ret;
}

static int web_semantic_get_iotsdstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int status = -1;
    int sdinfo = 0;
    int freespec = 0;
    char recordableDays[8] = {0};
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/Status");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        Common_Json_GetAttrValueInt(lowerData, "Status", &status);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/DiskManage/Disk0/Attribute");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            int i = 0;
            int SDNum = 0;
            char *str_tmp = NULL;
            Common_Json_GetAttrValueInt(lowerData, "PartitionNum", &SDNum);
            cJSON_Struct *part = Common_Json_GetAttrValueArr(lowerData, "Partition");
            for(i=0; i<SDNum; i++)
            {
                Common_Json_GetAttrValue(part, i, "MountPath", NULL, &str_tmp, NULL, NULL);

                if (str_tmp)
                {
                    if (!strcmp(str_tmp, "/tmp/mmc/mmc1"))
                    {
                        continue;
                    }
                }

                str_tmp = NULL;
                Common_Json_GetAttrValue(part, i, "TotalSpace", NULL, &str_tmp, NULL, NULL);
                sdinfo = web_storage_str2size(str_tmp);

                str_tmp = NULL;
                Common_Json_GetAttrValue(part, i, "FreeSpace", NULL, &str_tmp, NULL, NULL);
                freespec = web_storage_str2size(str_tmp);

                str_tmp = NULL;
                Common_Json_GetAttrValue(part, i, "SDRecordableDays", NULL, &str_tmp, NULL, NULL);
                if(str_tmp)
                {
                    snprintf(recordableDays,sizeof(recordableDays),"%s",str_tmp);
                }

            }
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    Common_Json_SetAttrValueInt(outdata,"SDStatus",status);
    Common_Json_SetAttrValueStr(outdata,"SDRecordableDays",recordableDays);
    Common_Json_SetAttrValueInt(outdata,"SDInfo",sdinfo);
    Common_Json_SetAttrValueInt(outdata,"SDFreeSpace",freespec);

    return ret;
}

static const char *const_schedModeTime = "{\"SchedModeTime\":[[0,0,23,59],[6,0,17,59],[18,0,5,59]]}";
cJSON_Struct *g_json_schedModeTime = NULL;
static int web_semantic_get_iotrecordcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int record_mode = 0;
    int start,stop,start2,stop2,schedMode = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    ret = web_semantic_get_iotsdstatus(header, indata, outdata);

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/RecordConfig/Device0/Channel0");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValueInt(lowerData, "RecordMode", &record_mode);
            if(record_mode>0)
            {
                if(Common_Json_GetAttrValueInt(lowerData, "Week0/Sched0/SchedMode",&record_mode))
                {
#ifdef AWSIOT
                    if(record_mode == 3)record_mode = 1;
#endif
                }
            }

            Common_Json_SetAttrValueInt(outdata,"RecordMode",record_mode);

            cJSON_Struct *recordModeCap = Common_Json_SetAttrValueArr(outdata, "RecordModeCapability");
            int cap_idx = 0;
#ifdef AWSIOT
            Common_Json_SetAttrValueArrInt(recordModeCap, cap_idx++, 0);
#endif
            Common_Json_SetAttrValueArrInt(recordModeCap, cap_idx++, 1);
            Common_Json_SetAttrValueArrInt(recordModeCap, cap_idx++, 2);

#ifndef AWSIOT
            Common_Json_SetAttrValueArrInt(recordModeCap, cap_idx++, 3);
#endif

            cJSON_Struct *time = Common_Json_SetAttrValueArr(outdata, "RecordSched");

            if(Common_Json_GetAttrValueInt(lowerData, "Week0/Sched0/StartTime",&i_num))
            {
                start = i_num;
                Common_Json_SetAttrValueArrInt(time, 0, i_num/100);
                Common_Json_SetAttrValueArrInt(time, 1, i_num%100);
            }

            if(Common_Json_GetAttrValueInt(lowerData, "Week0/Sched0/StopTime",&i_num))
            {
                stop = i_num;
                Common_Json_SetAttrValueArrInt(time, 2, i_num/100);
                Common_Json_SetAttrValueArrInt(time, 3, i_num%100);
            }

            Common_Json_GetAttrValueInt(lowerData, "Week0/Sched1/StartTime",&start2);
            Common_Json_GetAttrValueInt(lowerData, "Week0/Sched1/StopTime",&stop2);

            if(start== 0 && stop == 2359)
            {
                schedMode = 0;
            }
            else if(start== 600 && stop == 1759)
            {
                schedMode = 1;
            }
            else
            {
                if(start== 1800 && stop == 2359 && start2 == 0 && stop2 == 559)
                {
                    schedMode = 2;

                }
                else
                {
                    schedMode = 3;
                }

                if(stop == 2359 && start2 == 0)
                {
                    Common_Json_SetAttrValueArrInt(time, 2, stop2/100);
                    Common_Json_SetAttrValueArrInt(time, 3, stop2%100);
                }
            }

            Common_Json_SetAttrValueInt(outdata, "SchedMode", schedMode);

            if(g_json_schedModeTime == NULL)
            {
                g_json_schedModeTime = Common_Json_Parse(const_schedModeTime, NULL, NULL);
            }

            JsonOper_MergeObj(outdata, g_json_schedModeTime, 0);

        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_iotrecordcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int recordmode = -1;
    int schedMode = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueInt(lowerData, "RecordMode", 1);

        Common_Json_GetAttrValueInt(indata, "RecordMode", &recordmode);

        if(recordmode < 0 && recordmode > 3)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            if(recordmode == 0)
            {
                Common_Json_SetAttrValueInt(lowerData, "RecordMode", 0);
                recordmode = 1;
            }
#ifdef AWSIOT
            if(recordmode == 1)recordmode = 3;
#endif
        }
    }

    if (0 == ret)
    {
        int i = 0, j = 0;
        int i_hour = 0,i_min = 0;
        int start_time = 0, end_time = 2359;
        int start_time_2 = 0, end_time_2 = 0;
        char week_str[8] = {0}, sched_str[8] = {0};
        //if(recordmode == 1)
        {
            cJSON_Struct *sched = Common_Json_GetAttrValueArr(indata, "RecordSched");

            Common_Json_GetAttrValue(sched, 0, NULL, NULL, NULL, &i_hour, NULL);
            Common_Json_GetAttrValue(sched, 1, NULL, NULL, NULL, &i_min, NULL);
            start_time = i_hour*100 + i_min;

            i_hour = 0,i_min = 0;

            Common_Json_GetAttrValue(sched, 2, NULL, NULL, NULL, &i_hour, NULL);
            Common_Json_GetAttrValue(sched, 3, NULL, NULL, NULL, &i_min, NULL);
            end_time = i_hour*100 + i_min;
        }

        if(Common_Json_GetAttrValueInt(indata, "SchedMode", &schedMode))
        {

            if(schedMode == 0)
            {
                start_time = 0;
                end_time = 2359;
            }
            else if(schedMode == 1)
            {
                start_time = 600;
                end_time = 1759;
            }
            else if(schedMode == 2)
            {
                start_time = 1800;
                end_time = 2359;
                start_time_2 = 0;
                end_time_2 = 559;
            }
        }

        if(start_time>end_time)
        {
            end_time_2 = end_time;
            end_time = 2359;
        }

        for(i=0; i<MAX_DAYS; i++)
        {
            snprintf(week_str,sizeof(week_str),"Week%d",i);
            cJSON_Struct *week = Common_Json_SetAttrValueObj(lowerData, week_str);
            for(j=0; j<MAX_TIMESEGMENT; j++)
            {
                snprintf(sched_str,sizeof(sched_str),"Sched%d",j);
                cJSON_Struct *sched = Common_Json_SetAttrValueObj(week, sched_str);
                int start = 0, stop = 0;
                if(j==0)
                {
                    start = start_time;
                    stop = end_time;
                }
                else if(j==1)
                {
                    start = start_time_2;
                    stop = end_time_2;
                }
                Common_Json_SetAttrValueInt(sched, "StartTime", start);
                Common_Json_SetAttrValueInt(sched, "StopTime", stop);
                Common_Json_SetAttrValueInt(sched, "SchedMode", recordmode);
            }
        }

    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Record/RecordConfig/Device0/Channel0");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_iotsdformat(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_hdformat(header, indata, outdata);
    }

    if (0 == ret)
    {
        web_semantic_devicereboot(header, indata, outdata);
    }

    return ret;
}

int web_semantic_get_iotnetworkstatus(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int iRet = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    ret = web_semantic_get_networkstatus(header, indata, outdata);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueStr(outdata, "DefaultRoute",&str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata,"EthName",str_tmp);
        }

        if(Common_Json_GetAttrValueStr(outdata, "NetMask",&str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "Mask", str_tmp);
        }
    }

    //RTMP
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtmp/Attribute");
        iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iRet)
        {
            //RTMPPort
            Common_Json_GetAttrValue(lowerData, -1, "Rtmp.RtmpPort", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "RTMPPort", Common_Json_Type_Number, NULL, i_num, 0);

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_homepositioncfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/PTZ/HomePosition");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Token", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "PresetNo", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "HomePositionTime", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "ResetTime", i_num);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_homepositioncfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "PresetNo", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Token", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "ResetTime", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "HomePositionTime", i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/PTZ/HomePosition");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_del_homepositioncfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_DELETE, "/PTZ/HomePosition");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

static int web_semantic_get_preset(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int i = 0;
    int ret = 0;
    int i_num = 0;
    int presetMaxNum = 64;
    int preseNo = -1;
    int resetTime = 0;
    int version = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/ability");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        Common_Json_GetAttrValueInt(lowerData, "MaxPresetNum", &presetMaxNum);
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
		lowerData = NULL;
    }

    Common_Json_SetAttrValueInt(outdata, "MaxPresetNum", presetMaxNum);

    Ovfs_Web_UpdateHeader(header, REST_GET, "/PTZ/HomePosition");
    int iret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == iret)
    {
        Common_Json_GetAttrValueInt(lowerData, "Token", &preseNo);
        Common_Json_GetAttrValueInt(lowerData, "HomePositionTime", &resetTime);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(Common_Json_GetAttrValueInt(indata, "ApiVersion", &version) == NULL)
    {
        version = 0;
    }

    Ovfs_Web_UpdateHeader(header, REST_GET, version?"/Ptz/Presetv2":"/Ptz/Preset");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        cJSON_Struct *list_set = Common_Json_SetAttrValueArr(outdata, "PresetList");
        cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerData, "Presets");
        int size = Common_Json_ArraySize(list_get);
        for(i=0; i<size; i++)
        {
            cJSON_Struct *nloop = Common_Json_SetAttrValueArrObj(list_set, i);
            if(Common_Json_GetAttrValue(list_get, i, "Token", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(nloop, "No", i_num);
                Common_Json_SetAttrValueInt(nloop, "HomePosition", i_num == preseNo?1:0);
                Common_Json_SetAttrValueInt(nloop, "ResetTime", i_num == preseNo?resetTime:0);
            }

            if(Common_Json_GetAttrValue(list_get, i, "Title", NULL, &str_tmp, NULL, NULL))
            {
                Common_Json_SetAttrValueStr(nloop, "Title", str_tmp);
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_preset(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S* opt)
{
    int i = 0;
    int ret = 0;
    int presetidx = 0;
    int i_opt = 0;
    int homePos = 0;
    int version = 0;
    int method = REST_PUT;
    char *title = NULL;
    char uri[64]= {0};

    if(Common_Json_GetAttrValueInt(indata, "No", &presetidx) == NULL)
    {
        return WEB_CODE_InvalidArg;
    }

    if(Common_Json_GetAttrValueInt(indata, "ApiVersion", &version) == NULL)
    {
        version = 0;
    }

    Common_Json_GetAttrValueStr(indata, "Title", &title);

    Common_Json_GetAttrValueInt(indata, "Opt", &i_opt);

    snprintf(uri, sizeof(uri), "/Ptz/Preset%s?Token=%d", version?"v2":"",presetidx);

    if(opt->type == 1)//set
    {
        snprintf(uri, sizeof(uri), "%s&Title=%s&Opt=%d", uri,title,i_opt);
    }
    else if(opt->type == 2)//call
    {
        snprintf(uri, sizeof(uri), "/Ptz/Preset%s?Goto=%d", version?"v2":"", presetidx);
    }
    else if(opt->type == 3)//delete
    {
        method = REST_DELETE;
    }

    Ovfs_Web_UpdateHeader(header, method, uri);
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    if(Common_Json_GetAttrValueInt(indata, "HomePosition", &homePos) && homePos == 1)
    {
        Common_Json_SetAttrValueInt(indata, "PresetNo", presetidx);

        if(opt->type == 1)
        {
            ret = web_semantic_set_homepositioncfg(header, indata, NULL);
        }
        else if(opt->type == 3)
        {
            ret = web_semantic_del_homepositioncfg(header, indata, NULL);
        }
    }


    return ret;
}

int frmIotRecordCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            web_semantic_get_iotrecordcfg(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_iotrecordcfg(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }

    if (0 == ret)
    {
        if (opt->type == 1)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmIotSDStatus(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            web_semantic_get_iotsdstatus(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }

    return ret;
}

int frmIotSDFormat(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 1:
            ret = web_semantic_iotsdformat(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

int frmIotNetworkStatus(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_iotnetworkstatus(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }

    return ret;
}

int frmIotPresetControl(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;

    if(g_ovfs_web->devInfo.bPTZ != 4 && g_ovfs_web->devInfo.bPTZ != 5)
    {
        ret = WEB_CODE_Unsupported;
    }

    if (0 == ret)
    {
        switch (opt->type)
        {
            case 0:
                ret = web_semantic_get_preset(header, indata, outdata);
                break;
            case 1:
            case 2:
            case 3:
                ret = web_semantic_set_preset(header, indata, outdata,opt);
                break;
            default:
                ret = WEB_CODE_InvalidArg;
                break;
        }
    }

    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int frmIotHomePositionCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_homepositioncfg(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_homepositioncfg(header, indata, outdata);
            break;
        case 2:
            ret = web_semantic_del_homepositioncfg(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

int web_semantic_get_lightcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S* opt)
{
    int i = 0;
    int ret = 0;
    int i_type = 0;
    int i_lightcontrol = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Image/Attribute/All");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerData, "ImageList");
        int size = Common_Json_ArraySize(list_get);
        for(i=0; i<size; i++)
        {
            cJSON_Struct *tmp = Common_Json_GetAttrValueArrItem(list_get, i);
            if(Common_Json_GetAttrValueInt(tmp, "Type", &i_type) == NULL)
            {
                continue;
            }

            switch(i_type)
            {
                case 0:
                    if(i_lightcontrol == 0)
                    {
                        if(Common_Json_GetAttrValueInt(tmp, "Param/Mode", &i_num))
                        {
                            if(i_num == 0) i_lightcontrol = 4;
                            if(i_num == 4) i_lightcontrol = 3;
                            //Common_Json_SetAttrValueInt(outdata, "LightControl", i_num);
                        }
                    }

                    if(Common_Json_GetAttrValueInt(tmp, "Param/DayStart", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, "LightOffTime", i_num);
                    }

                    if(Common_Json_GetAttrValueInt(tmp, "Param/DayEnd", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, "LightOnTime", i_num);
                    }

                    break;

                case 12:
                    if(i_lightcontrol == 0)
                    {
                        Common_Json_GetAttrValueInt(tmp,"Param/Enable",&i_num);
                        if (0 == i_num)
                        {
                            i_lightcontrol = 0;
                        }
                        else
                        {
                            if(Common_Json_GetAttrValueInt(tmp,"Param/Mode",&i_num))
                            {
                                i_lightcontrol = i_num ? 1 : 2;
                            }
                        }
                    }

                    if(Common_Json_GetAttrValueInt(tmp,"Param/Level",&i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, "Brightness", i_num);
                    }

                    break;

                case 31:
                    if (Common_Json_GetAttrValueStr(tmp, "Param/Type", &str_tmp))
                    {
                        Common_Json_SetAttrValueStr(outdata, "Mode", str_tmp);
                    }

                    break;
            }
        }

        Common_Json_SetAttrValueInt(outdata, "Control", i_lightcontrol);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        OVFS_IMAGE_CAP_T imageCap = {0};
        ret = get_image_capability(header, opt, &imageCap);
        if (0 == ret)
        {
            cJSON_Struct *list_set = Common_Json_SetAttrValueArr(outdata, "ModeSupportList");
            for(i=0; i<8; i++)
            {
                if(slen(imageCap.lightList[i])>0)
                {
                    Common_Json_SetAttrValueArrStr(list_set, i, imageCap.lightList[i]);
                }
            }
        }
    }

    return ret;
}

int web_semantic_set_lightcfg(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        parse_lightcfg(indata, lowerData);

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "ImageList");

        if(Common_Json_ArraySize(list))
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Attribute/All");
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmIotLightCfg(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_lightcfg(header, indata, outdata, opt);
            break;
        case 1:
            ret = web_semantic_set_lightcfg(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_set_aovsleep(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int level = 0;
    char url[64] = {0};
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(indata, "Level", &level))
        {
            if(level == 0 || level == 1)
            {
                snprintf(url, sizeof(url), "/Boardsys/Sys/FrameMode");
                Common_Json_SetAttrValueInt(lowerData, "Level", level);
            }
            else if(level == 2)
            {
                snprintf(url, sizeof(url), "/Boardsys/Sys/AOVMode/Enable");
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
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, url);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmAovSleep(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 1:
            ret = web_semantic_set_aovsleep(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }

    if (0 == ret)
    {
        Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
    }

    return ret;
}

cJSON_Struct *g_testData = NULL;
int deal_test_data(int type, char *name, cJSON_Struct *indata, cJSON_Struct **outdata)
{
    LOGW("type:[%d] name:[%s]\n",type,name);
    if(g_testData == NULL)
    {
        g_testData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        Common_Json_SetAttrValueObj(g_testData, "Battery");
        Common_Json_SetAttrValueInt(g_testData, "Battery/LowBatteryAlarm", 0);
        Common_Json_SetAttrValueInt(g_testData, "Battery/Status", 0);
        Common_Json_SetAttrValueInt(g_testData, "Battery/Battery", 80);
        Common_Json_SetAttrValueInt(g_testData, "Battery/ChargingPower", 0);

        Common_Json_SetAttrValueObj(g_testData, "IndicatorLight");
        Common_Json_SetAttrValueInt(g_testData, "IndicatorLight/Enable", 0);

        Common_Json_SetAttrValueObj(g_testData, "SimCardSwitch");
        Common_Json_SetAttrValueInt(g_testData, "SimCardSwitch/Id", 1);
        Common_Json_SetAttrValueStr(g_testData, "SimCardSwitch/ISP", "China Telecom");
        Common_Json_SetAttrValueInt(g_testData, "SimCardSwitch/Signal", 95);
        Common_Json_SetAttrValueStr(g_testData, "SimCardSwitch/ICCID", "898608411924C1249046");
        Common_Json_SetAttrValueStr(g_testData, "SimCardSwitch/IMEI", "864775067711541");
        Common_Json_SetAttrValueInt(g_testData, "SimCardSwitch/Status", 1);
        Common_Json_SetAttrValueStr(g_testData, "SimCardSwitch/IP", "100.100.100.100");

        LOGD("g_testData: \n");
        ovfs_print_json(g_testData);
    }

    cJSON_Struct *node = Common_Json_GetAttrValueObj(g_testData, name);
    if(node == NULL)
    {
        LOGW("node == null \n");
        node = Common_Json_SetAttrValueObj(node, name);
    }
    else
    {
        LOGD("node: \n");
        ovfs_print_json(node);
    }

    if(type == 0)
    {
        *outdata = Common_Json_Duplicate(node, 1);
        LOGD("outdata: \n");
        ovfs_print_json(*outdata);
    }
    else
    {
        JsonOper_MergeObj(node, indata, 0);
        ovfs_print_json(outdata);
    }

    return 0;
}

int test_trig_alarm(cJSON_Struct *header, int status)
{
    int ret = 0;
    cJSON_Struct *indata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
    cJSON_Struct *arr = Common_Json_SetAttrValueArr(indata, "ResList");
    cJSON_Struct *item = Common_Json_SetAttrValueArrObj(arr, 0);
    //{\"ResList\":[{\"AlarmName\":\"LowBatteryAlarm\",\"Channel\":0,\"isHappent\":1}]}
    Common_Json_SetAttrValueStr(item, "AlarmName", "LowBatteryAlarm");
    Common_Json_SetAttrValueInt(item, "Channel", 0);
    Common_Json_SetAttrValueInt(item, "isHappent", status);
    Ovfs_Web_UpdateHeader(header, REST_PUT, "/alarm/collectdata");
    ret = Ovfs_Web_RestMethodA(header, indata, NULL, 0);

    Common_Json_Delete(indata);
    indata = NULL;
    return ret;
}

static int web_semantic_get_batteryconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *lowerData1 = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Sys/Battery");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData1, 0);
    deal_test_data(0, "Battery", NULL, &lowerData);
    //if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "LowBatteryAlarm", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "LowBatteryAlarm", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Status", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Status", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData1, "Battery", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Battery", i_num);
        }
        else
        {
            if(Common_Json_GetAttrValueInt(lowerData, "Battery", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "Battery", i_num);
            }
        }

        if(Common_Json_GetAttrValueInt(lowerData, "ChargingPower", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "ChargingPower", i_num);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (lowerData1)
    {
        Common_Json_Delete(lowerData1);
        lowerData1 = NULL;
    }

    return ret;
}

static int web_semantic_set_batteryconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "LowBatteryAlarm", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "LowBatteryAlarm", i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Sys/Battery");
        //ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        deal_test_data(1, "Battery", lowerData, NULL);
        test_trig_alarm(header, i_num);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmBatteryConfig(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_batteryconfig(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_batteryconfig(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_get_indicatorlightconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Sys/IndicatorLight");
    //ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    deal_test_data(0, "IndicatorLight", NULL, &lowerData);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_indicatorlightconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Sys/IndicatorLight");
        //ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        deal_test_data(1, "IndicatorLight", lowerData, NULL);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmIndicatorLightConfig(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_indicatorlightconfig(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_indicatorlightconfig(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_get_simcardswitch(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/Network/Netattr/LteCfg");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    //deal_test_data(0, "SimCardSwitch", NULL, &lowerData);
    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "SimSwitchSupport", &i_num) == NULL || i_num != 1)
        {
            ret = WEB_CODE_Unsupported;
        }
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Info/SimId", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Id", i_num+1);
        }

        if(Common_Json_GetAttrValueStr(lowerData, "Info/ServiceInfo", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "ISP", str_tmp);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Info/SignalStrength", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Signal", i_num);
        }

        if(Common_Json_GetAttrValueStr(lowerData, "Info/CCID", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "ICCID", str_tmp);
        }

        if(Common_Json_GetAttrValueStr(lowerData, "Info/IMEI", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "IMEI", str_tmp);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Info/LinkStatus", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Status", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Info/SimSwitchStatus", &i_num) && i_num == 1)
        {
            Common_Json_SetAttrValueInt(outdata, "Status", 3);
        }

        if(Common_Json_GetAttrValueStr(lowerData, "Info/IpAddrV4", &str_tmp))
        {
            Common_Json_SetAttrValueStr(outdata, "IP", str_tmp);
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_simcardswitch(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Id", &i_num))
        {
            Common_Json_SetAttrValueObj(lowerData, "Info");
            Common_Json_SetAttrValueInt(lowerData, "Info/SimId", i_num-1);
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Network/Netattr/LteCfg");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        /*if(i_num == 2)
        {
            Common_Json_SetAttrValueStr(lowerData, "ISP", "China Mobile");
            Common_Json_SetAttrValueStr(lowerData, "IP", "101.101.101.101");
        }
        else
        {
            Common_Json_SetAttrValueStr(lowerData, "ISP", "China Telecom");
            Common_Json_SetAttrValueStr(lowerData, "IP", "100.100.100.100");
        }
        deal_test_data(1, "SimCardSwitch", lowerData, NULL);*/
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmSimCardSwitch(Webs* wp, OVFS_WEB_OPTION_S* opt, cJSON_Struct* header, cJSON_Struct* indata, cJSON_Struct* outdata)
{
    int ret = 0;
    switch (opt->type)
    {
        case 0:
            ret = web_semantic_get_simcardswitch(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_simcardswitch(header, indata, outdata);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
    }


    if (0 == ret)
    {
        if (opt->type != 0)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}
