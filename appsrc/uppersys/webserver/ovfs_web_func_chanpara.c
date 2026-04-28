#include "ovfs_web_func.h"
#include <time.h>
extern int TimeStr2UnixTime(const char *timeStr, int *ltime);
extern cJSON_Struct * ovfs_web_parse_jsonfile(const char *filePath);
#define ANTS_TST_BIT(arg, bit) (((arg) & (1 << (bit))) != 0)
#define ANTS_U32_BIT_NUM	(32)

#define MKCOLOR_RGB555(r,g,b) (((((r) & 0xff)>>3) << 10) | ((((g) & 0xff)>>3) << 5) | (((b) & 0xff)>>3))
#define TRCOLOR_RGB888RGB555(rgb888) (((((((rgb888)>>16)&0xff))>>3) << 10) | (((((rgb888)>>8)&0xff)>>3) << 5) | (((rgb888)&0xff)>>3))
#define TRCOLOR_RGB555RGB888(rgb555) ((((((rgb555)>>10)&0x1f)<<3) << 16) | (((((rgb555)>>5)&0x1f)<<3) << 8) | (((((rgb555)>>0)&0x1f)<<3) << 0))

#define TEST_VIDEO_RESOLUTION 1

extern int s_libinfo_count;
extern THIRD_PROTOCOL_OPS_INFO_T s_libinfo[4];

#if 0//(TEST_VIDEO_RESOLUTION == 0)
struct
{
    const char *type;
    int id;
    const char *typeMinKbps;
    const char *typeMaxKbps;
} WEB_VIDEO_RESOLUTION_T[]= { {"(528*384)",0,"32Kbps","3 Mbps"},
    {"CIF(352*288)",1,"16Kbps","2 Mbps"},
    {"(176*144)",2,"16Kbps","2 Mbps"},
    {"D1(720*576)",3,"64Kbps","6 Mbps"},//{720,576}, // 3 NTSC{720,480}
    {"2CIF(704*288)",4,"64Kbps","5 Mbps"},
    {"Q720P(640*360)",5,"32Kbps","3 Mbps"},
    {"QVGA(320*240)",6,"16Kbps","2 Mbps"},
    {"Q1080P(960*544)",7,"64Kbps","6 Mbps"},
    {"D1(704*576)",8,"64Kbps","6 Mbps"},
    {"960P(1280*960)",9,"96Kbps","16Mbps"},
    {"(320*192)",10,"16Kbps","2 Mbps"},
    {"(288*192)",11,"16Kbps","2 Mbps"},
    {"(256*192)",12,"16Kbps","2 Mbps"},
    //{0*0 13},{0*0 14}
    {"(960*576)",15,"64Kbps","6 Mbps"},
    {"VGA(640*480)",16,"48Kbps","3 Mbps"},
    {"(1600*1200)",17,"128Kbps","16Mbps"},
    {"(800*600)",18,"64Kbps","8 Mbps"},
    {"HD720p(1280*720)",19,"80Kbps","12Mbps"},
    {"(1280*1024)",20,"128Kbps","14Mbps"},
    {"(1600*912)",21,"96Kbps","14Mbps"},
    {"3MP(3264*2448)",22,"512Kbps","20Mbps"},
    {"12MP(4000*3000)",23,"1024Kbps","32Mbps"},
    {"(2560*1440)",24,"160Kbps","20Mbps"},
    {"5MP(2592*1944)",25,"160Kbps","20Mbps"},
    {"(2592*1520)",26,"192Kbps","20Mbps"},
    {"HD1080p(1920*1080)",27,"144Kbps","16Mbps"},
    {"3MP(2560*1920)",28,"256Kbps","20Mbps"},
    {"(1600*1304)",29,"32Kbps","3 Mbps"},
    {"3MP(2048*1536)",30,"240Kbps","20Mbps"},
    {"5MP(2448*2048)",31,"256Kbps","20Mbps"},
    {"8MP(3840*2160)",32,"512Kbps","24Mbps"},
    {"9MP(3000*3000)",33,"640Kbps","28Mbps"},
};
#else
//static pthread_mutex_lock;
static cJSON_Struct *s_VideoResolutionTable[3] = {0};
#endif

// boardsys?¨ò?:0-h264,1-h265,2-mpeg4,3-mjpeg
// i8h?¨ò?:0-h264(??óD),1-h264(±ê×?),2-mpeg4,3-mjpeg,4-h265,5-h264+,6-h265+.
struct
{
    const char *type;
    int id;
} WEB_VIDEO_ENCTYPE_T[]= {   {"H264",0},
    {"MPEG4",2},
    {"MJPEG",3},
    {"H265",4},
    {"SVAC",5},
    {"H264+",6},
    {"H265+",7},
    {"S265",8},//265+ rename
};


struct
{
    int boardsys_id;
    int slink_id;
} Boardsys_Slink_enctype_code_Map[] =
{
    {0,0},
    {1,4},
    {2,2},
    {3,3},
    {4,0},
    {5,0},
    {6,0},
    {7,6},
    //{8,7},
    {9,5},
    {8,8}

};
int transcode_by_enctypemap(int boradsysid_or_slinkid,bool reserve_args)
{
    int i = 0;
    for(; i < sizeof(Boardsys_Slink_enctype_code_Map)/sizeof(Boardsys_Slink_enctype_code_Map[0]); ++i)
    {
        if(!reserve_args)
        {
            if(Boardsys_Slink_enctype_code_Map[i].boardsys_id == boradsysid_or_slinkid)
            {
                return Boardsys_Slink_enctype_code_Map[i].slink_id;
            }
        }
        else
        {
            if(Boardsys_Slink_enctype_code_Map[i].slink_id == boradsysid_or_slinkid)
            {
                return Boardsys_Slink_enctype_code_Map[i].boardsys_id;
            }
        }
    }
    return 0;
}




struct
{
    int boardSize;
    int webSize;
} WEB_OSDSIZE_T[] =     {{1,10},
    {2,15},
    {3,20},
    {4,30},
    {5,40},
    {6,50},
    {7,60},
    {8,70},
    {9,80}
};

struct
{
    const char *type;
    int id;
} WEB_VIDEO_BITTYPE_T[]= {   {"VBR",0},
    {"CBR",1},
    {"FIXQP",2},
    {"SMART", 3}
};


struct
{
    const char *type;
    int id;
} WEB_VIDEO_STREAMTYPE_T[]= {  {"Main Stream", 0},
    {"Aux Stream", 1},
    {"Third Stream", 3},
    {"Fourth Stream", 4},
    {"Fifth Stream", 5}
};


struct
{
    const char *matchto;
    int id;
} WEB_VIDEO_H264PROFILE_T[]= {   {"Baseline",0},
    {"MainProfile",1},
    {"HighProfile",2}
};

struct
{
    const char *type;
    int id;
} WEB_VIDEO_FRAMERATE_T[]= { {"1",5},
    {"2",6},
    {"4",7},
    {"6",8},
    {"8",9},
    {"10",10},
    {"12",11},
    {"15",14},
    {"16",12},
    {"18",15},
    {"20",13},
    {"22",16},
};

struct
{
    const char *type;
    int id;
    int valueInKbps;
} WEB_VIDEO_BITRAT_T[]= {    {"16Kbps",1,16},
    {"32Kbps",2,32},
    {"48Kbps",3,48},
    {"64Kbps",4,64},
    {"80Kbps",5,80},
    {"96Kbps",6,96},
    {"128Kbps",7,128},
    {"160Kbps",8,160},
    {"192Kbps",9,192},
    {"224Kbps",10,224},
    {"256Kbps",11,256},
    {"320Kbps",12,320},
    {"384Kbps",13,384},
    {"448Kbps",14,448},
    {"512Kbps",15,512},
    {"640Kbps",16,640},
    {"768Kbps",17,768},
    {"896Kbps",18,896},
    {"1024Kbps",19,1024},
    {"1280Kbps",20,1280},
    {"1536Kbps",21,1536},
    {"1792Kbps",22,1792},
    {"2 Mbps",23,2048},
    {"3 Mbps",24,3072},
    {"4 Mbps",25,4096},
    {"5 Mbps",26,5120},
    {"6 Mbps",27,6144},
    {"7 Mbps",28,7168},
    {"8 Mbps",29,8000},//8192
    {"9 Mbps",30,9000},
    {"10Mbps",31,10000},
    {"11Mbps",32,11000},
    {"12Mbps",33,12000},
    {"13Mbps",34,13000},
    {"14Mbps",35,14000},
    {"15Mbps",36,15000},
    {"16Mbps",37,16000},//1024*16
    {"20Mbps",38,20000},
    {"24Mbps",39,24000},
    {"28Mbps",40,28000},
    {"32Mbps",41,32000},
    {"Self-Define",-1,-1}//Self-Define(16-32000kbps)
};
static int pre_record_time[] = {0, 5, 10, 15, 20, 25, 30, 60};
static int record_delay[] = {5, 20, 30, 60, 120, 300, 600, 50};

static int web_semantic_set_disableallsmart(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata);

int GetBit32(int iBit32, int iIndex)
{
    if (iIndex< 0 || iIndex >= 32)
        return -1;
    return (0 != (iBit32 &  1<<iIndex));
}

int query_devchan_streamcount(cJSON_Struct *header, int devIndex, int chanIndex)
{
    int ret = 0;

    cJSON_Struct *lowerData = NULL;
    if(0 == ret)
    {
        /*
        {
        "DevTotalNum":  1,
        "ChanTotalNum": 1,
        "StreamNum":    3,
        "viDev0":{"viChanNum":1,"viChan0":3}
        }
        */
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Video/Ability/Number");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }
    int devCount = 0;
    if (0 == ret)
    {
        Common_Json_GetAttrValueInt(lowerData, "DevTotalNum", &devCount);
        if (devIndex >= devCount)
        {
            ret = -1;
        }
    }
    int chanCount = 0;
    if (0 == ret)
    {
        char labelName[32];
        snprintf(labelName, sizeof(labelName), "viDev%d.viChanNum", devIndex);
        Common_Json_GetAttrValueInt(lowerData, labelName, &chanCount);
        if (chanIndex >= chanCount)
        {
            ret = -1;
        }
    }
    int streamCount = 0;
    if (0 == ret)
    {
        char labelName[32];
        snprintf(labelName, sizeof(labelName), "viDev%d.viChan%d", devIndex, chanIndex);
        Common_Json_GetAttrValueInt(lowerData, labelName, &streamCount);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret < 0 ? 0 : streamCount;
}

static int web_semantic_get_videoshelterpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int iRet = 0;
    int i_num =0;
    int iloop = 0;
    int i_enable = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_root = NULL;
    char buf[256] = {0};

    if (0 == ret)
    {
        if ((pArry_root = Common_Json_SetAttrValue(outdata, -1, "Shelters", Common_Json_Type_Array, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }


    if (0 == ret)
    {
        int failed_count = 0;
        for (iloop=0; iloop<5; iloop++)
        {


            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Mask/Device%d/Channel%d/Rect%d",opt->dev,opt->ch,iloop);
            //printf("[%s]\n",buf);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);
            iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

            //Common_Json_StandardPrint(lowerData, NULL, NULL, NULL);
            if(iRet != 0)
            {
                if(iloop<4)
                {
                    ret = iRet;
                }
                failed_count++;
                continue;
            }
            Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_Object, NULL, 0, 0);
            //Enable
            Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num);
            if (i_num == 1)
            {
                i_enable = 1;
            }

            //Shelters( web [704*576] board[1024*1024])
            if (Common_Json_GetAttrValueInt(lowerData, "X", &i_num))
            {
                Common_Json_SetAttrValue(pArry_root, iloop, "HideAreaTopLeftX", Common_Json_Type_Number, NULL, i_num*704/1024, 0);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Y", &i_num))
            {
                Common_Json_SetAttrValue(pArry_root, iloop, "HideAreaTopLeftY", Common_Json_Type_Number, NULL, i_num*576/1024, 0);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "W", &i_num))
            {
                Common_Json_SetAttrValue(pArry_root, iloop, "HideAreaWidth", Common_Json_Type_Number, NULL, i_num*704/1024, 0);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "H", &i_num))
            {
                Common_Json_SetAttrValue(pArry_root, iloop, "HideAreaHeight", Common_Json_Type_Number, NULL, i_num*576/1024, 0);
            }
            //printf("------------%d-----\n",iloop);
            //Common_Json_StandardPrint(pArry_root, NULL,NULL,NULL);

            if (lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }
        }

        ret = failed_count == 5?ret:WEB_CODE_OK;
        if(ret == 0)
        {
            Common_Json_SetAttrValue(outdata, -1, "EnableHide", Common_Json_Type_Number, NULL, i_enable, i_enable);
        }
    }

    return ret;
}

static int web_semantic_set_videoshelterpara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    int i_width = 0;
    int i_height = 0;
    int iloop = 0;
    int i_enable = 0;
    cJSON_Struct *pArray_root = NULL;

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
        cJSON_Struct *lowerMaskList = Common_Json_SetAttrValueArr(lowerData, "MaskList");

        pArray_root = Common_Json_GetAttrValueArr(indata, "Shelters");
        int arraySize = Common_Json_ArraySize(pArray_root);
        //Common_Json_StandardPrint(indata,NULL,NULL,NULL);
        for (iloop=0; iloop < arraySize; iloop++)
        {
            Common_Json_SetAttrValue(lowerMaskList, iloop, NULL, Common_Json_Type_Object, NULL, 0, 0);
            Common_Json_SetAttrValue(lowerMaskList, iloop, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
            Common_Json_SetAttrValue(lowerMaskList, iloop, "Channel", Common_Json_Type_Number, NULL, opt->ch, 0);
            Common_Json_SetAttrValue(lowerMaskList, iloop, "Rect", Common_Json_Type_Number, NULL, iloop, 0);

            if (Common_Json_GetAttrValue(pArray_root, iloop, "HideAreaTopLeftX", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "X", Common_Json_Type_Number, NULL, i_num*1024/704, 0);
            }

            if (Common_Json_GetAttrValue(pArray_root, iloop, "HideAreaTopLeftY", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "Y", Common_Json_Type_Number, NULL, i_num*1024/576, 0);
            }

            if (Common_Json_GetAttrValue(pArray_root, iloop, "HideAreaWidth", NULL, NULL, &i_width, NULL))
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "W", Common_Json_Type_Number, NULL, i_width*1024/704, 0);
            }

            if (Common_Json_GetAttrValue(pArray_root, iloop, "HideAreaHeight", NULL, NULL, &i_height, NULL))
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "H", Common_Json_Type_Number, NULL, i_height*1024/576, 0);
            }

            if (i_width == 0 || i_height == 0)
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "Enable", Common_Json_Type_Number, NULL, 0, 0);
            }
            else if (Common_Json_GetAttrValue(indata, -1, "EnableHide", NULL, NULL, &i_enable, NULL))
            {
                Common_Json_SetAttrValue(lowerMaskList, iloop, "Enable", Common_Json_Type_Number, NULL, i_enable, 0);
            }
        }

        if (Common_Json_Size(lowerData) > 0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Mask/All");
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

static U32 IsLeapYear(U16 year)
{
    return (((year%4==0)&&(year%100!=0))||((year%400==0)));
}

static U32 LocalTime(WEB_TIME_T struTime, U32 timezone /*=8*/,int hourOffset,int minuteOffset)
{
    long res = 0 ;

    if(struTime.Month <= 2)
    {
        struTime.Month += 10 ;
        struTime.Year -= 1 ;
    }
    else
    {
        struTime.Month -= 2 ;
    }

    res = (long)(struTime.Year/4 - struTime.Year/100 + struTime.Year/400) + 367 * struTime.Month/12 + struTime.Day + struTime.Year*365 - 719499 ;

    res = ((res*24 + struTime.Hour )*60 + struTime.Minute)*60 + struTime.Second ;

//	res -= (hourOffset * 60 * 60  + minuteOffset* 60);

    return res ;
}

/*

mktime 一次 就会被减去 ipc时区的秒数.
返回时需要在加回来

*/
/*static time_t MkTime(struct tm* st,int minuteOffset)
{
    return mktime(st) - minuteOffset*60;
}*/

static VOID MkTimeOld(U32 res, WEB_TIME_T* time,  U32 timezone /*=8*/,int hourOffset, int minuteOffset)
{
    const int monthLengths[2][13] =
    {
        { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365},
        { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366},
    };
    const int yearLengths[2] = { 365, 366 };
    int year;
    int month;
    int minMonth;
    int maxMonth;
    int days;
    int clock;
    int isLeap;
    //cJSON_Struct* lowerData = NULL;
//	res += (hourOffset*60*60 + minuteOffset*60);
    days = res / 86400;
    clock = res % 86400;
    if(clock < 0)
    {
        clock += 86400;
        days -= 1;
    }
    if(days >= 0)
    {
        year = days/366;
        days -= year*365 + (year+1)/4 - (year+69)/100 + (year+369)/400;
        for(time->Year = year + 1970; ; time->Year++)
        {
            isLeap = IsLeapYear((unsigned short)time->Year);
            if(days < yearLengths[isLeap])
            {
                break;
            }
            days -= yearLengths[isLeap];
        }
    }
    else
    {
        year = days/366;
        days -= year*365 + (year-2)/4 - (year-30)/100 + (year-30)/400;
        for(time->Year = year + 1970 - 1; ; time->Year--)
        {
            isLeap = 0 ;
            days += yearLengths[isLeap];
            if(days >= 0)
            {
                break;
            }
        }
    }
    minMonth = 0;
    maxMonth = 12;
    for(month = 5; month < 12 && month > 0; month = (minMonth + maxMonth) / 2)
    {
        if(days < monthLengths[isLeap][month])
        {
            maxMonth = month;
        }
        else if(days >= monthLengths[isLeap][month + 1])
        {
            minMonth = month;
        }
        else
        {
            break;
        }
    }
    days -= monthLengths[isLeap][month];
    time->Month = month + 1;
    time->Day = days + 1;
    time->Hour = clock / 3600;        //3600s one hour
    clock = clock % 3600;
    time->Minute = clock / 60;        //60s one minute
    time->Second = clock % 60;     //ms
    time->Zone = (WORD)timezone ;
}

int trans_utctime_to_localtime_str(time_t st,char* out,int size)
{

    struct tm lc_start;
    localtime_r(&st, &lc_start);


    snprintf(out,size,"%04d%02d%02d%02d%02d%02d",lc_start.tm_year + 1900,lc_start.tm_mon + 1, lc_start.tm_mday,lc_start.tm_hour,lc_start.tm_min,lc_start.tm_sec);

    return 0;
}
unsigned long
utcMktime(const unsigned int year0, const unsigned int mon0,
          const unsigned int day, const unsigned int hour,
          const unsigned int min, const unsigned int sec)
{
    unsigned int mon = mon0, year = year0;
    /* 1..12 -> 11,12,1..10 */
    if (0 >= (int) (mon -= 2))
    {
        mon += 12;  /* Puts Feb last since it has leap day */
        year -= 1;
    }

    return ((((unsigned long)
              (year/4 - year/100 + year/400 + 367*mon/12 + day) +
              year*365 - 719499
             )*24 + hour /* now have hours */
            )*60 + min /* now have minutes */
           )*60 + sec; /* finally seconds */
}


int parse_datetime(char* datetime,time_t* utcsecs)
{
    WEB_TIME_T start;
    sscanf(datetime,"%4d%2d%2d%2d%2d%2d",&start.Year,&start.Month,&start.Day,
           &start.Hour,&start.Minute,&start.Second);

    *utcsecs = utcMktime(start.Year,start.Month,start.Day,start.Hour,start.Minute,start.Second);

    return 0;
}

void print_localtime(int ss)
{

    //localtime y-mm-dd hms
    struct tm begst = {0};
    gmtime_r(&ss, &begst);
    LOGD(" [%04d-%02d-%02d %02d:%02d:%02d]\n",begst.tm_year+1900,begst.tm_mon+1,begst.tm_mday,begst.tm_hour,begst.tm_min,begst.tm_sec);

}

static int ParseRecordFiles(cJSON_Struct *inData,cJSON_Struct *header, cJSON_Struct *outData, int dev,int channel,int begintime,int endtime)
{
    int ret = 0;

    int Key = -1;
    int DataSize = -1;
    int TZOffset = 0;
    int IsLocked = -1;
    int DataType = -1;
    int streamtype = -1;
    char *starttime = NULL;
    char *stoptime = NULL;
    cJSON_Struct *Segments = NULL;
    cJSON_Struct *SearchResults = NULL;

    Segments = Common_Json_GetAttrValue(inData,-1,"Segments",NULL,NULL,NULL,0);
    SearchResults = Common_Json_SetAttrValue(outData,-1,"SearchResults",Common_Json_Type_Array,NULL,0,0);

    // Common_Json_SetAttrValue(outData,-1,"UtcBegintime",Common_Json_Type_Number,NULL,begintime,0);

    //calc	dst seconds offset
    /* time_t delta = 0;
    	{
    struct tm now;
    time_t tp = time(NULL);
    localtime_r(&tp, &now);
    // LOGD("%d/%d/%d %d:%d:%d  is_dst:%d\n",now.tm_year+1900,now.tm_mon+1,now.tm_mday,now.tm_hour,now.tm_min,now.tm_sec,now.tm_isdst);
    now.tm_isdst = 0;
    delta = mktime(&now) - tp;
    LOGD("dst seconds:%d\n ",delta);

    Common_Json_SetAttrValue(outData,-1,"DstOffset",Common_Json_Type_Number,NULL,delta/60,0);

    }*/



    cJSON_Struct *Results = Common_Json_SetAttrValue(SearchResults, channel, NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueInt(Results,"Dev",dev+1);
    Common_Json_SetAttrValueInt(Results,"ChannelNo",channel+1);
    cJSON_Struct *Records = Common_Json_SetAttrValue(Results,-1,"Records",Common_Json_Type_Array,NULL,0,0);

    TZOffset = Common_GetTimeDiff()/60;
    TZOffset *= -1;

    int i = 0;
    int arraySize = Common_Json_ArraySize(Segments);
    for (i = 0; i < arraySize; i++)
    {
        cJSON_Struct *item = NULL;
        if ((item = Common_Json_GetAttrValue(Segments, i, NULL, NULL, NULL, NULL, NULL)) != NULL)
        {
            Common_Json_GetAttrValue(item, -1, "starttime", NULL, &starttime, NULL, 0);
            Common_Json_GetAttrValue(item, -1, "stoptime", NULL, &stoptime, NULL, 0);
            Common_Json_GetAttrValue(item, -1, "key", NULL, NULL, &Key, 0);
            Common_Json_GetAttrValue(item, -1, "streamtype", NULL, NULL, &streamtype, 0);
            Common_Json_GetAttrValue(item, -1, "DataType", NULL, NULL, &DataType, 0);
            Common_Json_GetAttrValue(item, -1, "IsLocked", NULL, NULL, &IsLocked, 0);
            Common_Json_GetAttrValue(item, -1, "DataSize", NULL, NULL, &DataSize, 0);

            time_t st1,st2;

            parse_datetime(starttime, &st1);
            //	print_localtime(st1 - TZOffset*60);
            parse_datetime(stoptime, &st2);

            cJSON_Struct *pArry_root3 = Common_cJSON_CreateArray();
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(DataType));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(st1));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(st2));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(0));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(TZOffset));
            Common_Json_AddItem(Records, i, NULL, pArry_root3);
            //}
        }
    }
    return ret;
}

static int ParseRecordFilesOld(cJSON_Struct *inData, cJSON_Struct *outData, int dev,int channel,int timeOffset,int minuteOffset)
{
    int ret = 0;
    int Key = -1;
    int DataSize = -1;
    int IsLocked = -1;
    int DataType = -1;
    int streamtype = -1;
    char *starttime = NULL;
    char *stoptime = NULL;
    cJSON_Struct *Segments = NULL;
    cJSON_Struct *SearchResults = NULL;
    Segments = Common_Json_GetAttrValue(inData,-1,"Segments",NULL,NULL,NULL,0);
    SearchResults = Common_Json_SetAttrValue(outData,-1,"SearchResults",Common_Json_Type_Array,NULL,0,0);
    cJSON_Struct *Results = Common_Json_SetAttrValue(SearchResults, channel, NULL, Common_Json_Type_Object, NULL, 0, 0);
    Common_Json_SetAttrValueInt(Results,"Dev",dev+1);
    Common_Json_SetAttrValueInt(Results,"ChannelNo",channel+1);
    cJSON_Struct *Records = Common_Json_SetAttrValue(Results,-1,"Records",Common_Json_Type_Array,NULL,0,0);
    int i = 0;
    int arraySize = Common_Json_ArraySize(Segments);
    for (i = 0; i < arraySize; i++)
    {
        cJSON_Struct *item = NULL;
        if ((item = Common_Json_GetAttrValue(Segments, i, NULL, NULL, NULL, NULL, NULL)) != NULL)
        {
            Common_Json_GetAttrValue(item, -1, "starttime", NULL, &starttime, NULL, 0);
            Common_Json_GetAttrValue(item, -1, "stoptime", NULL, &stoptime, NULL, 0);
            Common_Json_GetAttrValue(item, -1, "key", NULL, NULL, &Key, 0);
            Common_Json_GetAttrValue(item, -1, "streamtype", NULL, NULL, &streamtype, 0);
            Common_Json_GetAttrValue(item, -1, "DataType", NULL, NULL, &DataType, 0);
            Common_Json_GetAttrValue(item, -1, "IsLocked", NULL, NULL, &IsLocked, 0);
            Common_Json_GetAttrValue(item, -1, "DataSize", NULL, NULL, &DataSize, 0);
            WEB_TIME_T start;
            WEB_TIME_T stop;
            sscanf(starttime,"%4d%2d%2d%2d%2d%2d",&start.Year,&start.Month,&start.Day,
                   &start.Hour,&start.Minute,&start.Second);
            sscanf(stoptime,"%4d%2d%2d%2d%2d%2d",&stop.Year,&stop.Month,&stop.Day,
                   &stop.Hour,&stop.Minute,&stop.Second);
            cJSON_Struct *pArry_root3 = Common_cJSON_CreateArray();
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(DataType));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(LocalTime(start, 0,timeOffset,minuteOffset)));
            Common_cJSON_AddItemToArray(pArry_root3, Common_cJSON_CreateNumber(LocalTime(stop, 0, timeOffset,minuteOffset)));
            Common_Json_AddItem(Records, i, NULL, pArry_root3);
        }
    }
    return ret;
}

static int QueryVideoEncodeAbility(cJSON_Struct *header, int streamIdx,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;

    if (streamIdx < 0 || streamIdx > 4)
    {
        ret = WEB_CODE_InvalidArg;
    }

    if (0 == ret)
    {
        //if (s_VideoResolutionTable[streamIdx] == NULL)
        {
            char uri_path[128];
            snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Ability/Venc/Device%d/Channel%d/Stream%d",opt->dev,opt->ch, streamIdx);
            cJSON_Struct *lowerData = NULL;

            Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

            if ((ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0)) == 0)
            {
                cJSON_Struct *pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "Resolution", NULL, NULL, NULL, NULL);
                if(s_VideoResolutionTable[streamIdx])
                {
                    Common_Json_Delete(s_VideoResolutionTable[streamIdx]);
                    s_VideoResolutionTable[streamIdx] = NULL;
                }
                s_VideoResolutionTable[streamIdx] = Common_Json_Duplicate(pArry_tmp, 1);
            }
            if (lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }
        }
    }

    return ret;
}

typedef struct
{
    const char *wbName; // é?2?ê1ó?μ?°×??oaààDí??.
    int wbValue; // μ×2?ê1ó?μ?°×??oaμ?ààDí?μ.
} OVFS_WEB_WBNAMEMAP_S;

/*static OVFS_WEB_WBNAMEMAP_S s_whiteBalanceNameMap_dome[] =
{
    {"Auto", 1},
    {"Indoor", 2},
    {"Outdoor", 3},
};*/

static OVFS_WEB_WBNAMEMAP_S s_whiteBalanceNameMap[] =
{
    {"Auto", 0},
    {"Custom", 1},
    {"LockedWb", 2}, //???¨°×??oa
    {"IncandescentLamp", 3},//°×3?μ?
    {"WarmLight", 4},//?ˉ1aμ?
    {"NaturalLight", 5},//×?è?1a
    {"DaylightLamp", 6},//è?1aμ?
//    {"ColorTemperatureLamp4000K",//4000Ké???
//    {"ColorTemperatureLamp5000K",//5000Ké???
//    {"SunShine",//??1a
//    {"DarkCloud",//?ú??
//    {"FlashLight",//éá1aμ?
//    {"HighDaylightLamp",//??ááè?1aμ?
//    {"UnderWater",//??μ×
};

static int QueryShutterModeMap(int transLowUp, int srcCode, int *dstCode)
{
    /* ?ì???￡ê?:
    0-×??ˉ?ì??,ê??ˉ??ò?.
    0x1~0x7f:?ì??ó??è,ê??ˉ?ì??,×??ˉ??ò?.
    0x100:è?×??ˉ,?ì??×??ˉ,??ò?×??ˉ(×?′ó)￡?
    0x101~:è?ê??ˉ,?ì??ê??ˉ,??ò?ê??ˉ(??ò??μ?alGainMode)￡?
    0x200:1aè|ó??è￡?
    0x300~0x3ff:2??a???ì??ê±??,??ó|????é?value:0~0xff,í?′?μ??úD?μ??ì???μ
    */
    int ret = 0;
    int shutterModeMap[][2] =
    {
        {1, -25}, {2, -50}, {32, -50}, {33, -60}, {3, -100}, {34, -100}, {35, -120}, //! {1, -25}, {2, -50}, {3, -100},?aèy???μoíPN??ê?óD1?áa.
        {36, -150}, {37, -180}, {38, -200}, {39, -240}, {4, -250}, {40, -300}, {41, -360}, {42, -480}, {5, -500}, {43, -600}, {44, -700},
        {6, -1000}, {45, -1500}, {50, -1600}, {7, -2000}, {46, -2500}, {8, -5000}, {51, -7000},
        {9, -10000}, {47, -11000}, {48, -16000}, {52, -30000}, {49, -33000}, {10, -50000}, {53, -60000}, {54, -120000},
    };
    int count = sizeof(shutterModeMap)/sizeof(shutterModeMap[0]);

    if (transLowUp == 0)
    {
        // ′óé?2?D-òé×a??3éμ×2?D-òé?μ.
        if (srcCode == 0 || srcCode == 0x100)
        {
            *dstCode = 0;
        }
        else if ((srcCode > 0 && srcCode < 0x100) || (srcCode > 0x100 && srcCode < 0x200))
        {
            if (srcCode > 0x100 && srcCode < 0x200)
            {
                srcCode -= 0x100;
            }

            int i;
            for (i = 0; i < count; i++)
            {
                if (shutterModeMap[i][0] == srcCode)
                {
                    *dstCode = shutterModeMap[i][1];
                    break;
                }
            }
        }
        else if (srcCode >= 0x300 && srcCode < 0x400)
        {
            *dstCode = srcCode;
        }
        else //if (srcCode > 0x300 && srcCode < 0x400)
        {
            ret = -1;
            //*dstCode = srcCode - 0x300;
        }
    }
    else
    {
        // ′óμ×2?D-òé×a??3éé?2?D-òé?μ.
        if (srcCode < 0)
        {
            int i;
            for (i = 0; i < count; i++)
            {
                if (shutterModeMap[i][1] == srcCode)
                {
                    *dstCode = shutterModeMap[i][0];
                    break;
                }
            }
        }
        else if (srcCode >= 0x300 && srcCode < 0x400)
        {
            *dstCode = srcCode;
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}

static void GetCurTimeStr(char *timeStr, int len)
{
    if (access("/tmp/TZ", F_OK) == 0)
    {
        FILE *fp = fopen("/tmp/TZ", "rb");
        char line1[16],line2[16];
        fscanf(fp,"%s\n%s\n",line1,line2);
        fclose(fp);
        strcpy(timeStr, line2);
    }
    else
    {
        strcat(timeStr, "+28800");
    }
}
int frmCurTimeOffset(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    char timeStr[32] = {};
    int timeZone = 0;
    memset(timeStr, 0, sizeof(timeStr));
    GetCurTimeStr(timeStr, sizeof(timeStr));
    timeZone = atoi(timeStr);
    Common_Json_SetAttrValueInt(outdata,"TimeZone",timeZone);
    return ret;
}
static int web_semantic_get_videoRecordQueryOld(cJSON_Struct *header, cJSON_Struct *indata,cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;

    if (outdata == NULL || indata == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    cJSON_Struct *channelArray = NULL;
    int channelCount = 0;
    if (0 == ret)
    {
        channelArray = Common_Json_GetAttrValue(indata,-1,"Channels",NULL,NULL,0,0);
        if (channelArray == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            channelCount = Common_Json_ArraySize(channelArray);
            channelCount = MAX2(1, channelCount);
        }
    }

    int QueryType = 0;
    time_t BeginDateTime = 0;
    time_t EndDateTime = 0;
    int timeOffset;
    int minuteOffset;
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata,  "BeginDateTime", &BeginDateTime) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValueInt(indata,  "EndDateTime",  &EndDateTime) == NULL)
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
    if(ret == 0)
    {
        LOCAL_TZ_T local_TZ;
        NET_TZ_T net_TZ;
        cJSON_Struct *tt = NULL;
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Core/Time/TimeZone");
        int ret = Ovfs_Web_RestMethodA(header, NULL, &tt, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(tt, -1, "Zone", NULL, NULL, &local_TZ.TZ, NULL);
            Common_Json_GetAttrValue(tt, -1, "EnableBias", NULL, NULL, &local_TZ.bias_enable, NULL);
            Common_Json_GetAttrValue(tt, -1, "ZoneBias", NULL, NULL, &local_TZ.bias, NULL);
            web_TZ_translate_lton(&local_TZ, &net_TZ);
            timeOffset = net_TZ.offsetHour;
            minuteOffset = net_TZ.offsetMinute;
        }
        Common_Json_Delete(tt);
        tt = NULL;
        LOGD("Device TimeZone: hourOffset:[%d] minuteOffset:[%d] \n",timeOffset,minuteOffset);
    }
    if (0 == ret)
    {
        WEB_TIME_T startTime;
        WEB_TIME_T stopTime;
        MkTimeOld ( BeginDateTime,&startTime,0,0,0/*timeOffset,minuteOffset*/ );
        MkTimeOld ( EndDateTime,&stopTime,0,0,0/*timeOffset,minuteOffset*/ );
        char time[20] = {0};
        snprintf(time,sizeof(time),"%04d%02d%02d%02d%02d%02d",startTime.Year,startTime.Month,startTime.Day,
                 startTime.Hour,startTime.Minute,startTime.Second);
        Common_Json_SetAttrValue(lowerData, -1,"StartTime",Common_Json_Type_String,time,0,0);
        LOGD("StartTime:%s\n",time);
        snprintf(time,sizeof(time),"%04d%02d%02d%02d%02d%02d",stopTime.Year,stopTime.Month,stopTime.Day,
                 stopTime.Hour,stopTime.Minute,stopTime.Second);
        Common_Json_SetAttrValue(lowerData, -1, "StopTime", Common_Json_Type_String, time, 0, 0);
        LOGD("StopTime:%s\n",time);
        Common_Json_SetAttrValue(lowerData, -1, "StreamMask", Common_Json_Type_Number, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "QueryMode", Common_Json_Type_Number, NULL, QueryType, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Device", Common_Json_Type_Number,NULL,opt->dev,0);
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/Replay/RecordList");
        int i = 0;
        for (i = 0; i < channelCount; i++)
        {
            int channNum = 0;
            if (Common_Json_GetAttrValue(channelArray, i, NULL, NULL, NULL, &channNum, 0) == NULL)
            {
                if (channelCount == 1)
                {
                    channNum = 1;
                }
                else
                {
                    break;
                }
            }
            channNum--; // API中"Channels"从1开始.而录像模块中从0开始.
            Common_Json_SetAttrValue(lowerData, -1, "Channel", Common_Json_Type_Number,NULL, channNum, 0);
            cJSON_Struct *outParam = NULL;
            ret = Ovfs_Web_RestMethodA(header, lowerData, &outParam, 30000);
            if (ret == 0)
            {
                ParseRecordFilesOld(outParam, outdata, opt->dev, channNum,timeOffset,minuteOffset);
            }
            Common_Json_Delete(outParam);
            outParam = NULL;
            if (ret != 0)
            {
                break;
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

extern int trans_utctime_to_localtime_str(time_t st,char* out,int size);





//??ê?éè??
int frmVideoShowParaCtrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    return ret;
}

static char osdsize_arr[][32] =
{
    "FirstStreamOsdSize",
    "SecondStreamOsdSize",
    "ThirdStreamOsdSize",
    "FourthStreamOsdSize",
    "FifthStreamOsdSize",
};

static char osdsize_arr_lattice[][32] =
{
    "MainStreamFontSize",
    "SubStreamFontSize",
    "ThirdStreamFontSize",
    "FourthStreamFontSize",
    "FifthStreamFontSize",
};
static char bitmapsize_arr[][32] =
{
    "Main",
    "Sub",
    "Third",
    "Fourth",
    "Fifth",
};

//configtype 0-all 1-only size 2-only len
int get_channel_osd_cap(cJSON_Struct *header, int configtype, char *osdname, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char arr_name[32] = {0};
    cJSON_Struct *lowerData = NULL;

    if(outdata == NULL)return -1;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/boardsys/osd/channelname/ability");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if (configtype != 1 && Common_Json_GetAttrValueInt(lowerData, "MaxCharLen", &i_num))
            {
                if(osdname)
                {
                    snprintf(arr_name, sizeof(arr_name),"%s/MaxLength",osdname);
                }
                else
                {
                    snprintf(arr_name, sizeof(arr_name),"MaxLength");
                }
                Common_Json_SetAttrValueInt(outdata, arr_name, i_num);
            }

            if (configtype != 2)
            {
                cJSON_Struct *pMainSizeList = Common_Json_GetAttrValueArr(lowerData, "OsdSize/stream0");
                if(pMainSizeList)
                {
                    Common_Json_AddItem(outdata, -1, "MainStreamFontSizeCapability", Common_Json_Duplicate(pMainSizeList, 1));
                }

                cJSON_Struct *pSubSizeList = Common_Json_GetAttrValueArr(lowerData, "OsdSize/stream1");
                if(pSubSizeList)
                {
                    Common_Json_AddItem(outdata, -1, "SubStreamFontSizeCapability", Common_Json_Duplicate(pSubSizeList, 1));
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int get_multi_osd_cap(cJSON_Struct *header, int configtype, char *osdname, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    char arr_name[32] = {0};
    cJSON_Struct *lowerData = NULL;

    if(outdata == NULL)return -1;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/boardsys/osd/mulstring/ability");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            if (configtype != 1)
            {
                if(Common_Json_GetAttrValueInt(lowerData, "MaxCharLen", &i_num))
                {
                    if(osdname)
                    {
                        snprintf(arr_name, sizeof(arr_name),"%s/MaxLength",osdname);
                    }
                    else
                    {
                        snprintf(arr_name, sizeof(arr_name),"MaxLength");
                    }
                    Common_Json_SetAttrValueInt(outdata, arr_name, i_num);
                }

                if(Common_Json_GetAttrValueInt(lowerData, "MaxLines", &i_num))
                {
                    if(osdname)
                    {
                        snprintf(arr_name, sizeof(arr_name),"%s/MaxLines",osdname);
                    }
                    else
                    {
                        snprintf(arr_name, sizeof(arr_name),"MaxLines");
                    }
                    Common_Json_SetAttrValueInt(outdata, arr_name, i_num);
                }
            }

            if (configtype != 2)
            {
                cJSON_Struct *pMainSizeList = Common_Json_GetAttrValueArr(lowerData, "OsdSize/stream0");
                if(pMainSizeList)
                {
                    Common_Json_AddItem(outdata, -1, "MainStreamFontSizeCapability", Common_Json_Duplicate(pMainSizeList, 1));
                }

                cJSON_Struct *pSubSizeList = Common_Json_GetAttrValueArr(lowerData, "OsdSize/stream1");
                if(pSubSizeList)
                {
                    Common_Json_AddItem(outdata, -1, "SubStreamFontSizeCapability", Common_Json_Duplicate(pSubSizeList, 1));
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_singlelineOSD(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;

    // web apid?¨ò??a
    // "OSD":{"FirstStreamOsdSize":0,"SecondStreamOsdSize":0,"ThirdStreamOsdSize":0,"ChanName":"007",
    //		"IsShowChanName":1,"ChanNameTopLeftX":0,"ChanNameTopLeftY":0,"IsShowOSD":1,
    //		"OSDTopLeftX":316,"OSDTopLeftY":40,"OSDType":2,"OSDHourType":0}
    // 1|?ü?￡?é?¨ò??a
    // {"Enable":1,"X":0,"Y":0,"Size":10,"BitMapSize":10,"String":"IpCamera0"}
    char buf[256]= {0};
    cJSON_Struct *lowerData = NULL;

    Common_Json_SetAttrValue(outdata,-1,"OSD",Common_Json_Type_Object,NULL,0,0);
    get_channel_osd_cap(header, 0, "OSD", outdata);

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {

            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.IsShowChanName", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.ChanNameTopLeftX", Common_Json_Type_Number, NULL, (i_num*352)/1024, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.ChanNameTopLeftY", Common_Json_Type_Number, NULL, (i_num*288)/1024, 0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "OSD.ChanNameLocation", i_num);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "OSD.FontName", str_tmp);
            }

            int useBitmap = 0;
            if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
            {
                if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "OSD.FirstStreamOsdSize", i_num);
                }
                else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_num))
                {
                    Common_Json_SetAttrValueInt(outdata, "OSD.FirstStreamOsdSize", i_num);
                }
            }

            Common_Json_SetAttrValue(outdata, -1, "OSD.FirstStreamOsdSize", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "String", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.ChanName", Common_Json_Type_String, str_tmp, 0, 0);

            if (Common_Json_GetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", NULL, NULL, &i_num, 0))
            {
                i_num = TRCOLOR_RGB555RGB888(i_num);
                char tmpstr[16];
                snprintf(tmpstr, sizeof(tmpstr), "%06x", i_num);
                Common_Json_SetAttrValue(outdata, -1, "OSD.BitColor", Common_Json_Type_String, tmpstr, 0, 0);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, 0, 0);

        int i = 1;
        char labelName[32];
        for(i = 1; i < streamCount; i++)
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);

            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
            if (0 == ret)
            {
                int useBitmap = 0;
                memset(labelName,0,sizeof(labelName));
                snprintf(labelName, sizeof(labelName), "OSD.%s", osdsize_arr[i]);
                if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
                {
                    if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, labelName, i_num);
                    }
                    else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, labelName, i_num);
                    }
                }
            }

            if(lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }

            if(ret != 0)break;

        }
    }

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/Time/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.IsShowOSD", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.OSDTopLeftX", Common_Json_Type_Number, NULL, (i_num*352)/1024, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.OSDTopLeftY", Common_Json_Type_Number, NULL, (i_num*288)/1024, 0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(outdata, "OSD.TimeLocation", i_num);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "OSD.FontName", str_tmp);
            }

            Common_Json_GetAttrValue(lowerData, -1, "Style", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.OSDType", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "HourStyle", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "OSD.OSDHourType", Common_Json_Type_Number, NULL, i_num, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_singlelineOSD(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *lowerData_time = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }

        if ((lowerData_time = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    // éè???÷??á÷í¨μà??
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "OSD.IsShowChanName", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.ChanNameTopLeftX", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "X", (i_num*1024)/352);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.ChanNameTopLeftY", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Y", (i_num*1024)/288);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.ChanNameLocation", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Location", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSD.FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSD.ChanName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "String", str_tmp);

            if(slen(str_tmp) == 0)
            {
                Common_Json_SetAttrValueInt(lowerData, "Enable", 0);
            }

            Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 0);
        }

        Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 0);

        if (Common_Json_GetAttrValueStr(indata, "OSD.BitColor", &str_tmp))
        {
            i_num = (int)strtoul(str_tmp, NULL, 16);
            i_num = TRCOLOR_RGB888RGB555(i_num);
            if (i_num >= 0 && i_num <= 0x7fff)
            {
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr", Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", Common_Json_Type_Number, NULL, i_num, 0);
            }
        }
    }

    // éè??ê±??OSD.
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "OSD.IsShowOSD", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.OSDTopLeftX", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "X", (i_num*1024)/352);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.OSDTopLeftY", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Y", (i_num*1024)/288);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.TimeLocation", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Location", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSD.FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData_time, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.OSDType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Style", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSD.OSDHourType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "HourStyle", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSD.BitColor", &str_tmp))
        {
            i_num = (int)strtoul(str_tmp, NULL, 16);
            i_num = TRCOLOR_RGB888RGB555(i_num);
            if (i_num >= 0 && i_num <= 0x7fff)
            {
                Common_Json_SetAttrValue(lowerData_time, -1, "ColorAttr", Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValue(lowerData_time, -1, "ColorAttr/TextColorEx", Common_Json_Type_Number, NULL, i_num, 0);
            }
        }
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, 0, 0);

        int i = 0;
        char labelName[32];
        for(i = 0; i < streamCount; i++)
        {
            memset(labelName,0,sizeof(labelName));
            snprintf(labelName, sizeof(labelName), "OSD.%s", osdsize_arr[i]);
            if (Common_Json_GetAttrValueInt(indata, labelName, &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "Size", i_num);
                Common_Json_SetAttrValueInt(lowerData_time, "Size", i_num);
            }

            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

            if(ret != 0)break;

            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/Time/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

            ret = Ovfs_Web_RestMethodA(header, lowerData_time, NULL, 0);

            if(ret != 0)break;

        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (lowerData_time)
    {
        Common_Json_Delete(lowerData_time);
        lowerData_time = NULL;
    }

    return ret;
}

static int web_semantic_get_singlelineOSD_lattice(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pObj_root = NULL;
    char buf[256] = {0};


    pObj_root = Common_Json_SetAttrValue(outdata,-1,"OSDLattice", Common_Json_Type_Object, NULL, 0, 0);
    get_channel_osd_cap(header, 0, "OSDLattice", outdata);

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "PosX", Common_Json_Type_Number, NULL, (i_num*1000)/1024,0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_num, 0);
            Common_Json_SetAttrValue(pObj_root, -1, "PosY", Common_Json_Type_Number, NULL, (i_num*1000)/1024,0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(pObj_root, "ChanNameLocation", i_num);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(pObj_root, "FontName", str_tmp);
            }

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, -1, "String", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "Text", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);

            int useBitmap = 0;
            if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
            {
                if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_num))
                {
                    Common_Json_SetAttrValueInt(pObj_root, "MainStreamFontSize", i_num);
                }
                else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_num))
                {
                    Common_Json_SetAttrValueInt(pObj_root, "MainStreamFontSize", i_num);
                }
            }

            if (Common_Json_GetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", NULL, NULL, &i_num, 0))
            {
                i_num = TRCOLOR_RGB555RGB888(i_num);
                char tmpstr[16];
                snprintf(tmpstr, sizeof(tmpstr), "%06x", i_num);
                Common_Json_SetAttrValue(pObj_root, -1, "BitColor", Common_Json_Type_String, tmpstr, 0, 0);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
        LOGW("dev:[%d] ch:[%d] streamcount:[%d]\n",opt->dev, opt->ch,streamCount);
    }

    if (0 == ret && streamCount > 1)
    {
        int i = 1;
        for(i = 1; i < streamCount; i++)
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);

            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

            if (0 == ret)
            {
                int useBitmap = 0;
                if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
                {
                    if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObj_root, osdsize_arr_lattice[i], i_num);
                    }
                    else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObj_root, osdsize_arr_lattice[i], i_num);
                    }
                }
            }

            if(lowerData)
            {
                Common_Json_Delete(lowerData);
                lowerData = NULL;
            }
        }


    }

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/Time/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "EnableDateOSD", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "DateOSDPosX", Common_Json_Type_Number, NULL, (i_num*1000)/1024, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "DateOSDPosY", Common_Json_Type_Number, NULL, (i_num*1000)/1024, 0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(pObj_root, "TimeLocation", i_num);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(pObj_root, "FontName", str_tmp);
            }

            Common_Json_GetAttrValue(lowerData, -1, "Style", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "DateOSDType", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "HourStyle", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "DateOSDHourType", Common_Json_Type_Number, NULL, i_num, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_singlelineOSD_lattice(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pObj_tmp1 = NULL;
    char buf[256] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *lowerData_time = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }

        if ((lowerData_time = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    //éè???÷??á÷í¨μà??OSD.
    if (0 == ret)
    {
        pObj_tmp = Common_Json_GetAttrValue(indata, -1, "OSDLattice", NULL, NULL, NULL, NULL);

        if (Common_Json_GetAttrValueInt(pObj_tmp, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueInt(pObj_tmp, "PosX", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "X", (i_num*1024)/1000);
        }

        if (Common_Json_GetAttrValueInt(pObj_tmp, "PosY", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Y", (i_num*1024)/1000);
        }

        if (Common_Json_GetAttrValueInt(pObj_tmp, "ChanNameLocation", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Location", i_num);
        }

        if (Common_Json_GetAttrValueStr(pObj_tmp, "FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValueStr(pObj_tmp, "Text", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "String", str_tmp);
        }

        if (Common_Json_GetAttrValueStr(pObj_tmp, "BitColor", &str_tmp))
        {
            i_num = (int)strtoul(str_tmp, NULL, 16);
            i_num = TRCOLOR_RGB888RGB555(i_num);
            if (i_num >= 0 && i_num <= 0x7fff)
            {
                Common_Json_SetAttrValueObj(lowerData, "ColorAttr");
                Common_Json_SetAttrValueInt(lowerData, "ColorAttr/TextColorEx", i_num);
            }
        }
    }

    // éè??ê±??OSD.
    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.EnableDateOSD", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Enable", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.DateOSDPosX", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "X", (i_num*1024)/1000);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.DateOSDPosY", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Y", (i_num*1024)/1000);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.TimeLocation", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Location", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSDLattice.FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData_time, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.DateOSDType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "Style", i_num);
        }

        if (Common_Json_GetAttrValueInt(indata, "OSDLattice.DateOSDHourType", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData_time, "HourStyle", i_num);
        }

        if (Common_Json_GetAttrValueStr(indata, "OSDLattice.BitColor", &str_tmp))
        {
            i_num = (int)strtoul(str_tmp, NULL, 16);
            i_num = TRCOLOR_RGB888RGB555(i_num);
            if (i_num >= 0 && i_num <= 0x7fff)
            {
                Common_Json_SetAttrValue(lowerData_time, -1, "ColorAttr", Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValue(lowerData_time, -1, "ColorAttr/TextColorEx", Common_Json_Type_Number, NULL, i_num, 0);
            }
        }


    }

    if(ret == 0)
    {
        int streamCount = 0;
        if (0 == ret)
        {
            streamCount = query_devchan_streamcount(header, 0, 0);
        }

        int i = 0;
        for(i = 0; i< streamCount; i++)
        {

            if (Common_Json_GetAttrValueInt(pObj_tmp, osdsize_arr_lattice[i], &i_num))
            {
                Common_Json_SetAttrValueInt(lowerData, "BitMapSize", i_num);
                Common_Json_SetAttrValueInt(lowerData_time, "Size", i_num);
            }

            pObj_tmp1 = Common_Json_GetAttrValueObj(pObj_tmp, bitmapsize_arr[i]);
            if (pObj_tmp1)
            {
                if (Common_Json_GetAttrValueInt(pObj_tmp1, "Width", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "BitMapW", i_num);
                }

                if (Common_Json_GetAttrValueInt(pObj_tmp1, "Height", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "BitMapH", i_num);
                }

                if (Common_Json_GetAttrValueStr(pObj_tmp1, "BytesArray", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(lowerData, "BitMapArr", str_tmp?str_tmp:"");
                }

                if (Common_Json_GetAttrValueInt(pObj_tmp1, "BytesLength", &i_num))
                {
                    Common_Json_SetAttrValueInt(lowerData, "BitMapArrLen", i_num);
                }
            }

            if (Common_Json_Size(lowerData) > 0)
            {
                Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 1);

                memset(buf,0,sizeof(buf));
                snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
                Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

                if(ret != 0)break;
            }

            if (Common_Json_Size(lowerData_time) > 0)
            {
                memset(buf,0,sizeof(buf));
                snprintf(buf,sizeof(buf),"/BoardSys/Osd/Time/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
                Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

                ret = Ovfs_Web_RestMethodA(header, lowerData_time, NULL, 0);

                if(ret != 0)break;
            }

        }


    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (lowerData_time)
    {
        Common_Json_Delete(lowerData_time);
        lowerData_time = NULL;
    }

    return ret;
}


int frmSingleLineOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_singlelineOSD(header, indata, outdata,opt);
            break;

        case 1:
        //éè??2?êy
        case 12:
            //ò??ˉê±??OSD????.
            ret = web_semantic_set_singlelineOSD(header, indata, outdata,opt);
            break;

        case 10:
            //??è?2?êy
            ret = web_semantic_get_singlelineOSD_lattice(header, indata, outdata,opt);
            break;

        case 11:
            //éè??2?êy
            ret = web_semantic_set_singlelineOSD_lattice(header, indata, outdata,opt);
            if (0 == ret)
            {
                cJSON_Struct *pArry_copy = Common_Json_GetAttrValueArr(indata,"OSDLattice/CopyChan");
                if (pArry_copy)
                {
                    int copy_size = Common_Json_ArraySize(pArry_copy);
                    int i = 0;
                    int copy_loop = 0;
                    for(i=0; i<copy_size; i++)
                    {
                        Common_Json_GetAttrValue(pArry_copy, i, NULL, NULL, NULL, &copy_loop, 0);
                        if(i == opt->ch)continue;
                        if(copy_loop)
                        {
                            OVFS_WEB_OPTION_S opt_tmp;
                            memset(&opt_tmp,0,sizeof(OVFS_WEB_OPTION_S));
                            memcpy(&opt_tmp,opt,sizeof(OVFS_WEB_OPTION_S));
                            opt_tmp.ch = i;
                            web_semantic_set_singlelineOSD_lattice(header, indata, outdata,&opt_tmp);
                        }
                    }
                }
            }
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1 || opt->type == 11 || opt->type == 12)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}


//#ifndef SIMPLIFIED
static int web_semantic_get_multilineOSD(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_tmp = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    char buf[256] = {0};

    Common_Json_SetAttrValue(outdata,-1, "MultiOSD", Common_Json_Type_Object, NULL, 0, 0);
    get_multi_osd_cap(header, 0, "MultiOSD", outdata);

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MultiOSD.TopLeftX", Common_Json_Type_Number, NULL, (i_tmp*352)/1024, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MultiOSD.TopLeftY", Common_Json_Type_Number, NULL, (i_tmp*288)/1024, 0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_tmp))
            {
                Common_Json_SetAttrValueInt(outdata, "MultiOSD.Location", i_tmp);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "MultiOSD.FontName", str_tmp);
            }

            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MultiOSD.IsShowMultiOSD", Common_Json_Type_Number, NULL, i_tmp, 0);

            Common_Json_GetAttrValue(lowerData, -1, "String", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(outdata, -1, "MultiOSD/Text", Common_Json_Type_String, str_tmp, 0, 0);
            Common_Json_SetAttrValue(outdata, -1, "MultiOSD/TextSize", Common_Json_Type_Number, NULL, str_tmp?strlen(str_tmp)+1:0, 0);

            if (Common_Json_GetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", NULL, NULL, &i_tmp, 0))
            {
                i_tmp = TRCOLOR_RGB555RGB888(i_tmp);
                char tmpstr[16];
                snprintf(tmpstr, sizeof(tmpstr), "%06x", i_tmp);
                Common_Json_SetAttrValue(outdata, -1, "MultiOSD/BitColor", Common_Json_Type_String, tmpstr, 0, 0);
            }

            int useBitmap = 0;
            if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
            {
                if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_tmp))
                {
                    Common_Json_SetAttrValueInt(outdata, "MultiOSD.FirstStreamOsdSize", i_tmp);
                }
                else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_tmp))
                {
                    Common_Json_SetAttrValueInt(outdata, "MultiOSD.FirstStreamOsdSize", i_tmp);
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, 0, 0);
    }

    if (0 == ret && streamCount > 1)
    {
        int i = 1;
        char labelName[32];
        for(i = 1; i < streamCount; i++)
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);

            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

            if (0 == ret)
            {
                int useBitmap = 0;
                memset(labelName,0,sizeof(labelName));
                snprintf(labelName, sizeof(labelName), "MultiOSD.%s", osdsize_arr[i]);
                if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
                {
                    if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_tmp))
                    {
                        Common_Json_SetAttrValueInt(outdata, labelName, i_tmp);
                    }
                    else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_tmp))
                    {
                        Common_Json_SetAttrValueInt(outdata, labelName, i_tmp);
                    }
                }
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;

        }

    }

    return ret;
}


static int web_semantic_set_multilineOSD(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_tmp = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    char buf[256] = {0};
    int i = 0;
    char labelName[32];

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, 0, 0);
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        if (Common_Json_GetAttrValueInt(indata, "MultiOSD/TopLeftX", &i_tmp) ||
                Common_Json_GetAttrValueInt(indata, "MultiOSD/PosX", &i_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "X", (i_tmp*1024)/352);
        }

        if (Common_Json_GetAttrValueInt(indata, "MultiOSD/TopLeftY", &i_tmp) ||
                Common_Json_GetAttrValueInt(indata, "MultiOSD/PosY", &i_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "Y", (i_tmp*1024)/288);
        }

        if (Common_Json_GetAttrValueInt(indata, "MultiOSD.Location", &i_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "Location", i_tmp);
        }

        if (Common_Json_GetAttrValueStr(indata, "MultiOSD.FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValueInt(indata, "MultiOSD/IsShowMultiOSD", &i_tmp))
        {
            Common_Json_SetAttrValueInt(lowerData, "Enable", i_tmp);
        }

        str_tmp = NULL;
        if (Common_Json_GetAttrValueStr(indata, "MultiOSD/Text", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "String", str_tmp);
            if(slen(str_tmp) == 0)
            {
                Common_Json_SetAttrValueInt(lowerData, "Enable", 0);
            }
        }

        Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 0);

        if (Common_Json_GetAttrValue(indata, -1, "MultiOSD/BitColor", NULL, &str_tmp, 0, 0))
        {
            i_tmp = (int)strtoul(str_tmp, NULL, 16);
            i_tmp = TRCOLOR_RGB888RGB555(i_tmp);
            if (i_tmp >= 0 && i_tmp <= 0x7fff)
            {
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr", Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", Common_Json_Type_Number, NULL, i_tmp, 0);
            }
        }

        for(i = 0; i < streamCount; i++)
        {
            i_tmp = 0;
            memset(labelName,0,sizeof(labelName));
            snprintf(labelName, sizeof(labelName), "MultiOSD.%s", osdsize_arr[i]);
            if (Common_Json_GetAttrValueInt(indata, labelName, &i_tmp) == NULL)//Common_Json_GetAttrValue(indata, -1, "MainStreamFontSize", NULL, NULL, &i_tmp, NULL))
            {

                cJSON_Struct *lddata = NULL;
                memset(buf,0,sizeof(buf));
                snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
                Ovfs_Web_UpdateHeader(header, REST_GET, buf);

                ret = Ovfs_Web_RestMethodA(header, NULL, &lddata, 0);
                if (ret == 0)
                {
                    int useBitmap = 0;
                    Common_Json_GetAttrValueInt(lddata, "UseRemoteBm", &useBitmap);
                    if(useBitmap == 0)
                    {
                        Common_Json_GetAttrValueInt(lddata, "Size", &i_tmp);
                    }
                    else
                    {
                        Common_Json_GetAttrValueInt(lddata, "BitMapSize", &i_tmp);
                    }
                }
                else
                {
                    break;
                }

                Common_Json_Delete(lddata);
                lddata = NULL;

            }

            Common_Json_SetAttrValueInt(lowerData, "Size", i_tmp);

            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
            if(ret != 0)break;

        }

    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_multilineOSD_lattice(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_tmp = -1;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pObj_root = NULL;
    char buf[256] = {0};

    pObj_root = Common_Json_SetAttrValue(outdata, -1, "MultiOSDLattice", Common_Json_Type_Object, NULL, 0, 0);
    get_multi_osd_cap(header, 0, "MultiOSDLattice", outdata);

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "X", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "PosX", Common_Json_Type_Number, NULL,(i_tmp*1000)/1024,0);

            Common_Json_GetAttrValue(lowerData, -1, "Y", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "PosY", Common_Json_Type_Number, NULL, (i_tmp*1000)/1024,0);

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_tmp))
            {
                Common_Json_SetAttrValueInt(pObj_root, "Location", i_tmp);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "FontName", &str_tmp))
            {
                Common_Json_SetAttrValueStr(pObj_root, "FontName", str_tmp);
            }

            Common_Json_GetAttrValue(lowerData, -1, "Enable", NULL, NULL, &i_tmp, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "Enable", Common_Json_Type_Number, NULL, i_tmp, 0);

            str_tmp = NULL;
            Common_Json_GetAttrValue(lowerData, -1, "String", NULL, &str_tmp, NULL, NULL);
            Common_Json_SetAttrValue(pObj_root, -1, "Text", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);

            int useBitmap = 0;
            if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
            {
                if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_tmp))
                {
                    Common_Json_SetAttrValueInt(pObj_root, "MainStreamFontSize", i_tmp);
                }
                else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_tmp))
                {
                    Common_Json_SetAttrValueInt(pObj_root, "MainStreamFontSize", i_tmp);
                }
            }

            if (Common_Json_GetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", NULL, NULL, &i_tmp, 0))
            {
                i_tmp = TRCOLOR_RGB555RGB888(i_tmp);
                char tmpstr[16];
                snprintf(tmpstr, sizeof(tmpstr), "%06x", i_tmp);
                Common_Json_SetAttrValue(pObj_root, -1, "BitColor", Common_Json_Type_String, tmpstr, 0, 0);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamCount = query_devchan_streamcount(header, 0, 0);
    }


    if (0 == ret && streamCount > 1)
    {
        int i = 1;
        for(i = 1; i < streamCount; i++)
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);

            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
            if (0 == ret)
            {
                int useBitmap = 0;
                if (Common_Json_GetAttrValueInt(lowerData, "UseRemoteBm", &useBitmap))
                {
                    if (useBitmap == 0 && Common_Json_GetAttrValueInt(lowerData, "Size", &i_tmp))
                    {
                        Common_Json_SetAttrValueInt(pObj_root, osdsize_arr_lattice[i], i_tmp);
                    }
                    else if (useBitmap && Common_Json_GetAttrValueInt(lowerData, "BitMapSize", &i_tmp))
                    {
                        Common_Json_SetAttrValueInt(pObj_root, osdsize_arr_lattice[i], i_tmp);
                    }
                }
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;

        }
    }

    return ret;
}


static int web_semantic_set_multilineOSD_lattice(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *pObj_tmp = NULL;
    cJSON_Struct *pObj_tmp1 = NULL;
    char buf[256] = {0};
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
        pObj_tmp = Common_Json_GetAttrValue(indata, -1, "MultiOSDLattice", NULL, NULL, NULL, NULL);
        if (Common_Json_GetAttrValue(pObj_tmp, -1, "PosX", NULL, NULL, &i_num, NULL))
        {
            i_num = MIN2(1000, MAX2(0, i_num));
            Common_Json_SetAttrValue(lowerData, -1, "X", Common_Json_Type_Number, NULL, (i_num*1024)/1000, 0);
        }

        if (Common_Json_GetAttrValue(pObj_tmp, -1, "PosY", NULL, NULL, &i_num, NULL))
        {
            i_num = MIN2(1000, MAX2(0, i_num));
            Common_Json_SetAttrValue(lowerData, -1, "Y", Common_Json_Type_Number, NULL, (i_num*1024)/1000, 0);
        }

        if (Common_Json_GetAttrValueInt(pObj_tmp, "Location", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerData, "Location", i_num);
        }

        if (Common_Json_GetAttrValueStr(pObj_tmp, "FontName", &str_tmp))
        {
            Common_Json_SetAttrValueStr(lowerData, "FontName", str_tmp);
        }

        if (Common_Json_GetAttrValue(pObj_tmp, -1, "Enable", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Enable", Common_Json_Type_Number, NULL, i_num, 0);
        }

        if (Common_Json_GetAttrValue(pObj_tmp, -1, "Text", NULL, &str_tmp, 0, 0))
        {
            Common_Json_SetAttrValue(lowerData, -1, "String", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
        }

        if (Common_Json_GetAttrValue(pObj_tmp, -1, "BitColor", NULL, &str_tmp, 0, 0))
        {
            i_num = (int)strtoul(str_tmp, NULL, 16);
            i_num = TRCOLOR_RGB888RGB555(i_num);
            if (i_num >= 0 && i_num <= 0x7fff)
            {
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr", Common_Json_Type_Object, NULL, 0, 0);
                Common_Json_SetAttrValue(lowerData, -1, "ColorAttr/TextColorEx", Common_Json_Type_Number, NULL, i_num, 0);
            }
        }

        int streamCount = 0;
        if (0 == ret)
        {
            streamCount = query_devchan_streamcount(header, 0, 0);
        }

        int i = 0;
        for(i = 0; i < streamCount; i++)
        {
            if (Common_Json_GetAttrValue(pObj_tmp, -1, osdsize_arr_lattice[i], NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerData, -1, "BitMapSize", Common_Json_Type_Number, NULL, i_num, 0);
            }

            pObj_tmp1 = Common_Json_GetAttrValue(pObj_tmp, -1, bitmapsize_arr[i], NULL, NULL, 0, 0);

            if (Common_Json_GetAttrValue(pObj_tmp1, -1, "Width", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerData, -1, "BitMapW", Common_Json_Type_Number, NULL, i_num, 0);
            }

            if (Common_Json_GetAttrValue(pObj_tmp1, -1, "Height", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValue(lowerData, -1, "BitMapH", Common_Json_Type_Number, NULL, i_num, 0);
            }

            if (Common_Json_GetAttrValue(pObj_tmp1, -1, "BytesArray", NULL, &str_tmp, 0, 0))
            {
                Common_Json_SetAttrValue(lowerData, -1, "BitMapArr", Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
            }

            if (Common_Json_GetAttrValue(pObj_tmp1, -1, "BytesLength", NULL, NULL, &i_num, 0))
            {
                Common_Json_SetAttrValue(lowerData, -1, "BitMapArrLen", Common_Json_Type_Number, NULL, i_num, 0);
            }

            if (Common_Json_Size(lowerData) > 0)
            {
                Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 1);

                memset(buf,0,sizeof(buf));
                snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,i);
                Ovfs_Web_UpdateHeader(header, REST_PUT, buf);

                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

                if(ret != 0)break;
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



//?àDDOSD
int frmMultiLineOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_multilineOSD(header, indata, outdata,opt);
            break;

        case 1:
            //éè??2?êy
            ret = web_semantic_set_multilineOSD(header, indata, outdata,opt);
            break;

        case 10:
            //??è?2?êy
            ret = web_semantic_get_multilineOSD_lattice(header, indata, outdata,opt);
            break;

        case 11:
            //éè??2?êy
            ret = web_semantic_set_multilineOSD_lattice(header, indata, outdata,opt);
            if (0 == ret)
            {
                cJSON_Struct *pArry_copy = Common_Json_GetAttrValueArr(indata,"MultiOSDLattice/CopyChan");
                if (pArry_copy)
                {
                    int copy_size = Common_Json_ArraySize(pArry_copy);
                    int i = 0;
                    int copy_loop = 0;
                    for(i=0; i<copy_size; i++)
                    {
                        Common_Json_GetAttrValue(pArry_copy, i, NULL, NULL, NULL, &copy_loop, 0);
                        if(i == opt->ch)continue;
                        if(copy_loop)
                        {
                            OVFS_WEB_OPTION_S opt_tmp;
                            memset(&opt_tmp,0,sizeof(OVFS_WEB_OPTION_S));
                            memcpy(&opt_tmp,opt,sizeof(OVFS_WEB_OPTION_S));
                            opt_tmp.ch = i;
                            web_semantic_set_multilineOSD_lattice(header, indata, outdata,&opt_tmp);
                        }
                    }
                }
            }
            break;

        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }

    if (0 == ret)
    {
        if (opt->type == 1 || opt->type == 11)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_set_ReqIFrame(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int streamIdx = 0;
    char uri_path[128] = {0};
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
        if (Common_Json_GetAttrValueInt(indata, "StreamType", &streamIdx) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
    }
    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/ReqIFrame?Device=%d&Channel=%d&Stream=%d",opt->dev,opt->ch,streamIdx);
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);
        ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}

int frmReqIFrame(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            ret = web_semantic_set_ReqIFrame(header, indata, outdata, opt);
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


static int web_semantic_get_videoeffect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    char buf[256]= {0};
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    cJSON_Struct *outParam = NULL;
    if (0 == ret)
    {
        Common_Json_SetAttrValue(lowerData, -1, "Type", Common_Json_Type_Number, NULL, 15,0);
        memset(buf,0,sizeof(buf));

        /*if (g_ovfs_web->devInfo.bPTZ == 1)
        {

            snprintf(buf,sizeof(buf),"/Ptz/Image/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        }
        else*/
        {
            snprintf(buf,sizeof(buf),"/BoardSys/Image/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_GET, buf);
        }

        ret = Ovfs_Web_RestMethodA(header, lowerData, &outParam, 0);
    }

    if (0 == ret)
    {
        int i_tmp = -1;
        Common_Json_SetAttrValue(outdata, -1, "VideoEffect", Common_Json_Type_Object, NULL, 0, 0);

        Common_Json_GetAttrValue(outParam, -1, "Brightness", NULL, NULL, &i_tmp, 0);
        Common_Json_SetAttrValue(outdata, -1, "VideoEffect/Brightness", Common_Json_Type_Number, NULL, i_tmp, 0);

        Common_Json_GetAttrValue(outParam, -1, "Contrast", NULL, NULL, &i_tmp, 0);
        Common_Json_SetAttrValue(outdata, -1, "VideoEffect/Contrast", Common_Json_Type_Number, NULL, i_tmp, 0);

        Common_Json_GetAttrValue(outParam, -1, "Saturation", NULL, NULL, &i_tmp, 0);
        Common_Json_SetAttrValue(outdata, -1, "VideoEffect/Saturation", Common_Json_Type_Number, NULL, i_tmp, 0);

        Common_Json_GetAttrValue(outParam, -1, "Hue", NULL, NULL, &i_tmp, 0);
        Common_Json_SetAttrValue(outdata, -1, "VideoEffect/Hue", Common_Json_Type_Number, NULL, i_tmp, 0);
    }

    if (outParam)
    {
        Common_Json_Delete(outParam);
        outParam = NULL;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return  ret;
}

static int web_semantic_set_videoeffect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_tmp = -1;
    char buf[256] = {0};
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
        Common_Json_SetAttrValue(lowerData, -1, "Type", Common_Json_Type_Number, NULL, 15,0);
        Common_Json_SetAttrValue(lowerData, -1, "Param", Common_Json_Type_Object, NULL, 0,0);

        if(Common_Json_GetAttrValueInt(indata,  "VideoEffect/Brightness", &i_tmp))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Param/Brightness", Common_Json_Type_Number, NULL, i_tmp, 0);
        }

        if(Common_Json_GetAttrValueInt(indata,  "VideoEffect/Contrast", &i_tmp))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Param/Contrast", Common_Json_Type_Number, NULL, i_tmp, 0);
        }

        if(Common_Json_GetAttrValueInt(indata, "VideoEffect/Saturation", &i_tmp))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Param/Saturation", Common_Json_Type_Number, NULL, i_tmp, 0);
        }

        if(Common_Json_GetAttrValueInt(indata,  "VideoEffect/Hue",&i_tmp))
        {
            Common_Json_SetAttrValue(lowerData, -1, "Param/Hue", Common_Json_Type_Number, NULL, i_tmp, 0);
        }

        /*if (g_ovfs_web->devInfo.bPTZ == 1)
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/Ptz/Image/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
            ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
        }*/
        //else
        {
            memset(buf,0,sizeof(buf));
            snprintf(buf,sizeof(buf),"/BoardSys/Image/Attribute/Device%d",opt->dev);
            Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
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


//éè??áá?è??±è?è
int frmVideoEffect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_videoeffect(header, indata, outdata,opt);
            break;

        case 1:
            //éè??2?êy
            ret = web_semantic_set_videoeffect(header, indata, outdata,opt);
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

static int web_semantic_get_videopara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int streamtype = 0;
    int streamIdx = -1;
    int i_num = 0;
    int iloop = 0;
    int Width = 0;
    int Height = 0;
    int i_max = 1;
    int i_width = 0;
    int i_height = 0;
    char uri_path[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_tmp = NULL;

    if (header == NULL || indata == NULL || outdata == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }

    int streamCount = 0;
    if (0 == ret)
    {
        streamtype = 0;
        Common_Json_GetAttrValue(indata, -1, "AbilityType", NULL, NULL, &streamtype, NULL);
        if (3 <= streamtype)
        {
            streamIdx = streamtype - 1;
        }
        else
        {
            streamIdx = streamtype;
        }

        //int thirdStreamSupport = 0;
        streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
        if (streamIdx >= streamCount)
        {
            ret = WEB_CODE_InvalidArg;
        }
        /*
        Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "/CVStreamSelect/thirdStream/visible", &thirdStreamSupport);
        if (thirdStreamSupport == 0 && streamIdx == 2)
        {
            ret = WEB_CODE_InvalidArg;
        }*/
    }

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Attribute/Device%d/Channel%d/Stream%d", opt->dev,opt->ch,streamIdx);
        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Width", NULL, NULL, &i_width, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Height", NULL, NULL, &i_height, NULL);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Ability/Venc/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,streamIdx);

        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            if ((pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "Resolution", NULL, NULL, NULL, NULL)) != NULL)
            {
                int arraySize = Common_Json_ArraySize(pArry_tmp);
                for (iloop = 0; iloop < arraySize; iloop++)
                {
                    int fps = -1;
                    int width = -1;
                    int height = -1;

                    Common_Json_GetAttrValue(pArry_tmp, iloop, "W", NULL, NULL, &width, 0);
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "H", NULL, NULL, &height, 0);
                    Common_Json_GetAttrValue(pArry_tmp, iloop, "Fps", NULL, NULL, &fps, 0);

                    if (width == -1 || height == -1 || fps == -1)
                        continue;
                    if((width == i_width) && (height == i_height))
                    {
                        i_max = fps;
                        break;
                    }
                }
                if (iloop == arraySize)
                {
                    ret = WEB_CODE_InternalMistake;
                    LOGE("Failed to find Resolution from ability.\n");
                }
            }
            else
            {
                ret = WEB_CODE_InternalMistake;
                LOGE("Failed to match [Resolution].\n");
            }
        }

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch, streamIdx);
        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            Common_Json_SetAttrValue(outdata, -1, "AbilityType", Common_Json_Type_Number, NULL, streamtype, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Quality", NULL, NULL, &i_num, 0);
            Common_Json_SetAttrValue(outdata,-1, "PicQuality", Common_Json_Type_Number, NULL, i_num, 0);

            if (Common_Json_GetAttrValue(lowerData, -1, "Fps", NULL, NULL, &i_num, 0))
            {
                i_num = MIN2(i_num, i_max);
                printf("VideoFrameRate:[%d %d]\n",i_num,i_max);
                Common_Json_SetAttrValue(outdata, -1, "VideoFrameRate", Common_Json_Type_Number, NULL, i_num, 0);
            }

            Common_Json_GetAttrValue(lowerData,-1, "Iinterval", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata,-1, "IFrameInterval", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "BitrateCtrlMode", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "BitrateType", Common_Json_Type_Number, NULL, i_num, 0);

            // boardsys?¨ò?:0-h264,1-h265,2-mpeg4,3-mjpeg,7-h264+,8-h265+.
            // i8h?¨ò?:0-h264(??óD),1-h264(±ê×?),2-mpeg4,3-mjpeg,4-h265,5-SVAC,6-h264+,7-h265+.
            //int encodeFormatMap[] = {0, 4, 2, 3, 0, 0, 0, 6, 7};
            Common_Json_GetAttrValue(lowerData, -1, "EncodeFormat", NULL, NULL, &i_num, NULL);
            /*if (i_num < 0 || i_num >= sizeof(encodeFormatMap)/sizeof(encodeFormatMap[0]))
            {
                i_num = 0;
            }*/
            i_num = transcode_by_enctypemap(i_num,FALSE);
            Common_Json_SetAttrValue(outdata, -1, "VideoEncType", Common_Json_Type_Number, NULL, i_num, 0);

            Common_Json_GetAttrValue(lowerData, -1, "Profiles", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata,-1, "VideoH264Profile", Common_Json_Type_Number, NULL, i_num, 0);

            //Common_Json_GetAttrValue(lowerData, -1, "BitrateIsCustom", NULL, NULL, &i_num, NULL);
            //int bitrate = 0;
            Common_Json_GetAttrValue(lowerData, -1, "Bitrate", NULL, NULL, &i_num, 0);
            Common_Json_SetAttrValue(outdata, -1, "VideoBitrate", Common_Json_Type_Number, NULL, (1<<31)|i_num, 0);
            for (iloop = 0; iloop < sizeof(WEB_VIDEO_BITRAT_T)/sizeof(WEB_VIDEO_BITRAT_T[0]); iloop++)
            {
                if (WEB_VIDEO_BITRAT_T[iloop].valueInKbps == i_num)
                {
                    Common_Json_SetAttrValue(outdata,-1,"VideoBitrate",Common_Json_Type_Number,NULL,WEB_VIDEO_BITRAT_T[iloop].id,0);
                    break;
                }
            }

            Common_Json_GetAttrValue(lowerData, -1, "Width", NULL, NULL, &Width, NULL);
            Common_Json_GetAttrValue(lowerData, -1, "Height", NULL, NULL, &Height, NULL);
            if (Width == -1 || Height == -1)
            {
                LOGE("Get resolution failed!\n");
                return -1;
            }
#if 0//(TEST_VIDEO_RESOLUTION == 0)
            char rect[32] = {0};
            snprintf(rect,sizeof(rect),"%d*%d",Width,Height);
            for (iloop = 0; iloop < sizeof(WEB_VIDEO_RESOLUTION_T)/sizeof(WEB_VIDEO_RESOLUTION_T[0]); iloop++)
            {
                if (strstr(WEB_VIDEO_RESOLUTION_T[iloop].type,rect))
                {
                    Common_Json_SetAttrValue(outdata, -1, "Resolution", Common_Json_Type_Number, NULL, WEB_VIDEO_RESOLUTION_T[iloop].id,0);
                    break;
                }
            }
#else
            QueryVideoEncodeAbility(header, streamIdx,opt);
            cJSON_Struct *videoResolutionTable = s_VideoResolutionTable[streamIdx];
            int resolutionCount = Common_Json_ArraySize(videoResolutionTable);
            for (iloop = 0; iloop < resolutionCount; iloop++)
            {
                int abilityW;
                int abilityH;
                cJSON_Struct *resolutionEach = Common_Json_GetAttrValueArrItem(videoResolutionTable, iloop);
                Common_Json_GetAttrValueInt(resolutionEach, "W", &abilityW);
                Common_Json_GetAttrValueInt(resolutionEach, "H", &abilityH);
                if (abilityW == Width && abilityH == Height)
                {
                    Common_Json_SetAttrValue(outdata, -1, "Resolution", Common_Json_Type_Number, NULL, iloop,0);
                    break;
                }
            }
#endif
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Audio/Attribute/All");
        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(lowerData, -1, "AudioEnable", NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValue(outdata, -1, "StreamType", Common_Json_Type_Number, NULL, i_num, 0);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_videopara(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int streamtype = 0;
    int streamIdx = -1;
    int iloop = 0;
    int Resolution = 0;
    int i_num = 0;
    int VideoFrameRate = -1;
    int VideoBitrate = 0;
    char uri_path[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_tmp = NULL;
    cJSON_Struct *pArry_copy = NULL;

    if (0 == ret)
    {
        streamtype = 0;
        Common_Json_GetAttrValue(indata, -1, "AbilityType", NULL, NULL, &streamtype, NULL);
        if (3 <= streamtype)
        {
            streamIdx = streamtype - 1;
        }
        else
        {
            streamIdx = streamtype;
        }
    }

    if (0 == ret)
    {
        //int thirdStreamSupport = 0;
        int streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
        if (streamIdx >= streamCount)
        {
            ret = WEB_CODE_InvalidArg;
        }
        /*Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "/CVStreamSelect/thirdStream/visible", &thirdStreamSupport);
        if (thirdStreamSupport == 0 && streamIdx == 2)
        {
            ret = WEB_CODE_InvalidArg;
        }*/
    }

    int i_max = 0;
    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValue(indata, -1, "Resolution", NULL, NULL, &Resolution, NULL))
        {
#if 0// (TEST_VIDEO_RESOLUTION == 0)
            int Width = -1;
            int Height = -1;
            char *str_tmp = NULL;
            for (iloop = 0; iloop < sizeof(WEB_VIDEO_RESOLUTION_T)/sizeof(WEB_VIDEO_RESOLUTION_T[0]); iloop++)
            {
                if (WEB_VIDEO_RESOLUTION_T[iloop].id == Resolution)
                {
                    str_tmp = strchr(WEB_VIDEO_RESOLUTION_T[iloop].type, '(');
                    sscanf(str_tmp,"(%d*%d)",&Width,&Height);
                }
                if (Width == -1 || Height == -1)
                {
                    continue;
                }
                Common_Json_SetAttrValue(lowerData,-1,"Width",Common_Json_Type_Number,NULL,Width,0);
                Common_Json_SetAttrValue(lowerData,-1,"Height",Common_Json_Type_Number,NULL,Height,0);
                break;
            }
#else
            QueryVideoEncodeAbility(header, streamIdx,opt);
            cJSON_Struct *videoResolutionTable = s_VideoResolutionTable[streamIdx];
            int resolutionCount = Common_Json_ArraySize(videoResolutionTable);
            if (Resolution >= resolutionCount)
            {
                ret = WEB_CODE_InvalidArg;
            }
            else
            {
                //for (iloop = 0; iloop < resolutionCount; iloop++)
                iloop = Resolution;
                {
                    int abilityW;
                    int abilityH;
                    cJSON_Struct *resolutionEach = Common_Json_GetAttrValueArrItem(videoResolutionTable, iloop);
                    Common_Json_GetAttrValueInt(resolutionEach, "W", &abilityW);
                    Common_Json_GetAttrValueInt(resolutionEach, "H", &abilityH);
                    Common_Json_GetAttrValueInt(resolutionEach, "Fps", &i_max);
                    Common_Json_SetAttrValue(lowerData,-1,"Width",Common_Json_Type_Number,NULL,abilityW,0);
                    Common_Json_SetAttrValue(lowerData,-1,"Height",Common_Json_Type_Number,NULL,abilityH,0);
                }
            }
#endif
        }
    }

    if (0 == ret)
    {
        if(Common_Json_GetAttrValue(indata,-1,"BitrateType",NULL,NULL,&i_num,NULL))
        {
            Common_Json_SetAttrValue(lowerData,-1,"BitrateCtrlMode",Common_Json_Type_Number,NULL,i_num,0);
        }

        if(Common_Json_GetAttrValue(indata,-1,"VideoBitrate",NULL,NULL,&VideoBitrate,0))
        {
            if (VideoBitrate & (1<<31))
            {
                VideoBitrate &= 0x7fffffff;
                if(Common_Json_GetAttrValue(indata,-1,"Bit",NULL,NULL,&i_num,NULL))
                {
                    Common_Json_SetAttrValue(lowerData,-1,"Bitrate",Common_Json_Type_Number,NULL,i_num,0);
                }
            }
            else
            {
                for (iloop = 0; iloop < sizeof(WEB_VIDEO_BITRAT_T)/sizeof(WEB_VIDEO_BITRAT_T[0]); iloop++)
                {
                    if (VideoBitrate == WEB_VIDEO_BITRAT_T[iloop].id)
                    {
                        i_num = WEB_VIDEO_BITRAT_T[iloop].valueInKbps;
                        Common_Json_SetAttrValue(lowerData, -1, "Bitrate", Common_Json_Type_Number, NULL, i_num, 0);
                        break;
                    }
                }
            }
        }

        if (Common_Json_GetAttrValue(indata,-1,"PicQuality",NULL,NULL,&i_num,NULL))
        {
            Common_Json_SetAttrValue(lowerData,-1,"Quality",Common_Json_Type_Number,NULL,i_num,0);
        }

        if (Common_Json_GetAttrValue(indata, -1, "VideoFrameRate", NULL, NULL, &VideoFrameRate, 0))
        {
            VideoFrameRate = MIN2(VideoFrameRate, i_max);
            Common_Json_SetAttrValue(lowerData, -1, "Fps", Common_Json_Type_Number, NULL, VideoFrameRate, 0);
        }

        if (Common_Json_GetAttrValue(indata,-1,"IFrameInterval",NULL,NULL,&i_num,NULL))
        {
            Common_Json_SetAttrValue(lowerData,-1,"Iinterval",Common_Json_Type_Number,NULL,i_num,0);
        }

        if (Common_Json_GetAttrValue(indata,-1,"VideoEncType",NULL,NULL,&i_num,NULL))
        {
            // i8h?¨ò?:0-h264(??óD),1-h264(±ê×?),2-mpeg4,3-mjpeg,4-h265,5-SVAC,6-h264+,7-h265+.
            // boardsys?¨ò?:0-h264,1-h265,2-mpeg4,3-mjpeg,7-h264+,8-h265+.
            /*int encodeFormatMap[] = {0, 0, 2, 3, 1, 0, 7, 8};
            if (i_num < 0 || i_num >= sizeof(encodeFormatMap)/sizeof(encodeFormatMap[0]))
            {
                i_num = 0;
            }*/
            i_num = transcode_by_enctypemap(i_num,TRUE);
            Common_Json_SetAttrValue(lowerData,-1,"EncodeFormat",Common_Json_Type_Number,NULL,i_num,0);
        }

        if (Common_Json_GetAttrValue(indata,-1,"VideoH264Profile",NULL,NULL,&i_num,NULL))
        {
            Common_Json_SetAttrValue(lowerData,-1,"Profiles",Common_Json_Type_Number,NULL,i_num,0);
        }

        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Attribute/Device%d/Channel%d/Stream%d",opt->dev,opt->ch, streamIdx);
        Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);

        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (0 == ret)
    {
        pArry_copy = Common_Json_GetAttrValueArr(indata,"CopyChan");
        if (pArry_copy)
        {
            int copy_size = Common_Json_ArraySize(pArry_copy);
            int i = 0;
            int copy_loop = 0;
            for(i=0; i<copy_size; i++)
            {
                Common_Json_GetAttrValue(pArry_copy, i, NULL, NULL, NULL, &copy_loop, 0);
                if(i == opt->ch)continue;
                if(copy_loop)
                {
                    snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Attribute/Device%d/Channel%d/Stream%d",opt->dev,i, streamIdx);
                    Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);
                    Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
                }
            }
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        /*?ìo?á÷éè??*/
        if(Common_Json_GetAttrValue(indata, -1, "StreamType", NULL, NULL, &i_num, NULL))
        {
            Common_Json_SetAttrValue(lowerData, -1, "AudioEnable", Common_Json_Type_Number, NULL, i_num, 0);

            snprintf(uri_path, sizeof(uri_path), "/BoardSys/Audio/Attribute/All");
            Ovfs_Web_UpdateHeader(header, REST_PUT, uri_path);

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

//êó?μ2?êy
int frmVideoIPCSetPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_videopara(header, indata, outdata,opt);
            break;

        case 1:
            //éè??2?êy
            ret = web_semantic_set_videopara(header, indata, outdata,opt);
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


static int web_semantic_get_videocompressability(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int iloop = 0;
    int nloop = 0;
    int i_min = 1;
    int i_max = 1;
    int i_num = 0;
    int streamtype = 0;
    int streamIdx = -1;
    int fps_len = 0;
    char fps_name[8] = {0};
    char *str_tmp = NULL;
    char uri_path[128] = {0};
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pArry_root = NULL;
    cJSON_Struct *pArry_root1 = NULL;
    cJSON_Struct *pArry_tmp = NULL;

    if (Common_Json_GetAttrValue(indata, -1, "AbilityType", NULL, NULL, &streamtype, 0) == NULL)
    {
        LOGE("GetAttrValue AbilityType failed!\n");
        ret = WEB_CODE_InvalidArg;
    }
    else if (streamtype >= 3)
    {
        streamIdx = streamtype - 1;
    }
    else if (streamtype >= 0 && streamtype <= 1)
    {
        streamIdx = streamtype;
    }
    else
    {
        ret = WEB_CODE_InvalidArg;
    }

    // ??è???á÷êy??.
    int streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);
    if (0 == ret)
    {
        //Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "CVConfigVideoShow/thirdOSDFontSize", &thirdStreamSupport);
        //Common_Json_GetAttrValueBol(g_ovfs_uiconfig, "/CVStreamSelect/thirdStream/visible", &thirdStreamSupport);
        if (streamIdx >= streamCount)
        {
            ret = 1;
        }
    }

    /*
        // ??è?μ±?°·?±??ê
        if (0 == ret)
        {
            int i_width = 0;
            int i_height = 0;
            snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Attribute/Device0/Channel0/Stream%d", streamIdx);
            Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

            ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
            if (0 == ret)
            {
                Common_Json_GetAttrValue(lowerData, -1, "Width", NULL, NULL, &i_width, NULL);
                Common_Json_GetAttrValue(lowerData, -1, "Height", NULL, NULL, &i_height, NULL);
            }
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    */

    if (0 == ret)
    {
        snprintf(uri_path, sizeof(uri_path), "/BoardSys/Video/Ability/Venc/Device%d/Channel%d/Stream%d",opt->dev,opt->ch,streamIdx);

        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if (0 == ret)
    {
        //ret = ParseVideobility(lowerData, data, StreamNum);
        //BaudRate
        int count = 0;
        pArry_root = Common_Json_SetAttrValue(outdata, -1, "BaudRate", Common_Json_Type_Array, NULL, 0, 0);
        for (iloop = 0; iloop < sizeof(WEB_VIDEO_STREAMTYPE_T)/sizeof(WEB_VIDEO_STREAMTYPE_T[0]); iloop ++)
        {
            if (iloop < streamCount)
            {
                pArry_root1 = Common_Json_SetAttrValue(pArry_root, count++, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, WEB_VIDEO_STREAMTYPE_T[iloop].type, 0, 0);
                Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_STREAMTYPE_T[iloop].id, 0);
            }
        }

        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "Resolution", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "Resolution", Common_Json_Type_Array, NULL, 0, 0);

            int resolutionCount = Common_Json_Size(pArry_tmp);

            count = 0;
            for (iloop = 0; iloop < resolutionCount; iloop++)
            {

                pArry_root1 = Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_Array, NULL, 0, 0);

                int valueInt;
                char *valueStr;
                if (Common_Json_GetAttrValue(pArry_tmp, iloop, "ResoStr", NULL, &valueStr, NULL, 0))
                {
                    Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, valueStr, 0, 0);
                }
                Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, iloop, 0);
                if (Common_Json_GetAttrValue(pArry_tmp, iloop, "Fps", NULL, NULL, &valueInt, 0))
                {
                    i_max = MAX2(i_max, valueInt);
                    Common_Json_SetAttrValue(pArry_root1, 2, NULL, Common_Json_Type_Number, NULL, valueInt, 0);
                }

            }
        }

        //FrameRate
        if (i_max < 1)
        {
            i_max = 1;
        }
        i_min = 1;
        count = 0;
        pArry_root = Common_Json_SetAttrValue(outdata, -1, "FrameRate", Common_Json_Type_Array, NULL, 0, 0);
        fps_len = i_max - i_min + 1;
        for (iloop = 0; iloop < fps_len; iloop++)
        {
            pArry_root1 = Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_Array, NULL, 0, 0);
            snprintf(fps_name, sizeof(fps_name), "%d", i_min + iloop);
            Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, fps_name, 0, 0);
            Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, i_min + iloop, 0);
        }
        pArry_root1 = Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_SetAttrValue(pArry_root1, 0, NULL,Common_Json_Type_String, "FULL", 0, 0);
        Common_Json_SetAttrValue(pArry_root1, 1, NULL,Common_Json_Type_Number, NULL, 0, 0);

        //BitrateType
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "BitrateType", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "BitrateType", Common_Json_Type_Array, NULL, 0, 0);
            count = 0;
            for (iloop = 0; iloop < Common_Json_Size(pArry_tmp); iloop++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, iloop, NULL, NULL, &str_tmp, NULL, NULL);
                if (str_tmp)
                {
                    for (nloop = 0; nloop < sizeof(WEB_VIDEO_BITTYPE_T)/sizeof(WEB_VIDEO_BITTYPE_T[0]); nloop++)
                    {
                        if (strcmp(WEB_VIDEO_BITTYPE_T[nloop].type, str_tmp) != 0)
                        {
                            continue;
                        }
                        pArry_root1 = Common_Json_SetAttrValue(pArry_root, count, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, WEB_VIDEO_BITTYPE_T[nloop].type, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_BITTYPE_T[nloop].id, 0);
                        count ++;
                    }
                }
            }
        }

        pArry_tmp = Common_Json_GetItem(lowerData, -1, "FpsRange");
        if (pArry_tmp)
        {
            str_tmp = NULL;
            Common_Json_GetAttrValue(pArry_tmp, 0, NULL, NULL, &str_tmp, 0, 0);
            if (str_tmp)
            {
                sscanf(str_tmp,"%d-%d",&i_min,&i_max);
            }
        }
        pArry_root = Common_Json_SetAttrValue(outdata, -1, "FpsRange", Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_SetAttrValue(pArry_root, 0, NULL, Common_Json_Type_Number, 0, i_min, 0);
        Common_Json_SetAttrValue(pArry_root, 1, NULL, Common_Json_Type_Number, 0, i_max, 0);
        //Bitrate
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "BitrateRange", NULL, NULL, 0, 0);
        if (str_tmp)
        {
            str_tmp = NULL;
            Common_Json_GetAttrValue(pArry_tmp, 0, NULL, NULL, &str_tmp, 0, 0);
            if (str_tmp)
            {
                sscanf(str_tmp,"%d-%d",&i_min,&i_max);
                count = 0;
                pArry_root = Common_Json_SetAttrValue(outdata, -1, "Bitrate", Common_Json_Type_Array, NULL, 0, 0);
                for (iloop = 0; iloop < sizeof(WEB_VIDEO_BITRAT_T)/sizeof(WEB_VIDEO_BITRAT_T[0]); iloop++)
                {
                    i_num = WEB_VIDEO_BITRAT_T[iloop].valueInKbps;
                    if ((i_num >= i_min) && (i_num <= i_max))
                    {
                        pArry_root1 = Common_Json_SetAttrValue(pArry_root, count, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, WEB_VIDEO_BITRAT_T[iloop].type, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_BITRAT_T[iloop].id, 0);
                        count ++;
                    }
                    else if(i_num == -1)
                    {
                        char sefdefine[32] = {0};
                        snprintf(sefdefine, sizeof(sefdefine),"%s(%d-%dKbps)",WEB_VIDEO_BITRAT_T[iloop].type,i_min,i_max);
                        pArry_root1 = Common_Json_SetAttrValue(pArry_root, count, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, sefdefine, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_BITRAT_T[iloop].id, 0);
                        count ++;
                    }


                }
            }
        }

        //VideoEncType
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "EncType", NULL, NULL, 0, 0);
        if (pArry_tmp)
        {
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "VideoEncType", Common_Json_Type_Array, NULL, 0, 0);
            count = 0;
            for (iloop = 0; iloop < Common_Json_Size(pArry_tmp); iloop++)
            {
                str_tmp = NULL;
                Common_Json_GetAttrValue(pArry_tmp, iloop, NULL, NULL, &str_tmp, NULL, NULL);
                if (str_tmp)
                {
                    for (nloop = 0; nloop < sizeof(WEB_VIDEO_ENCTYPE_T)/sizeof(WEB_VIDEO_ENCTYPE_T[0]); nloop++)
                    {
                        if (strcmp(WEB_VIDEO_ENCTYPE_T[nloop].type, str_tmp) != 0)
                        {
                            continue;
                        }

                        if(strcmp("H265+", str_tmp) == 0)
                        {
                            nloop++;
                        }

                        pArry_root1 = Common_Json_SetAttrValue(pArry_root, count, NULL, Common_Json_Type_Array, NULL, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, WEB_VIDEO_ENCTYPE_T[nloop].type, 0, 0);
                        Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_ENCTYPE_T[nloop].id, 0);
                        count ++;
                        break;
                    }
                }
            }
        }

        //H264PROFILE
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "Profiles", NULL, NULL, 0, 0);
        if(pArry_tmp)
        {
            pArry_root = Common_Json_SetAttrValue(outdata, -1, "H264PROFILE", Common_Json_Type_Array, NULL, 0, 0);
            // for (iloop = 0; iloop < sizeof(WEB_VIDEO_H264PROFILE_T)/sizeof(WEB_VIDEO_H264PROFILE_T[0]); iloop ++)
            // {
            int kk = 0;

            for(kk = 0; kk < Common_Json_ArraySize(pArry_tmp); ++kk)
            {
                int matchIdx = -1;
                char* ss = NULL;
                Common_Json_GetAttrValue(pArry_tmp, kk, NULL, NULL, &ss, NULL, NULL);
                LOGD("%s\n",ss);
                if(smatch(ss,"Main"))
                {
                    matchIdx = 1;
                }
                else if(smatch(ss,"Hight") )
                {

                    matchIdx = 2;
                }
                else if(smatch(ss,"BaseLine"))
                {
                    matchIdx = 0;
                }
                if(matchIdx == -1)continue;
                pArry_root1 = Common_Json_SetAttrValue(pArry_root, matchIdx, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_SetAttrValue(pArry_root1, 0, NULL, Common_Json_Type_String, WEB_VIDEO_H264PROFILE_T[matchIdx].matchto, 0, 0);
                Common_Json_SetAttrValue(pArry_root1, 1, NULL, Common_Json_Type_Number, NULL, WEB_VIDEO_H264PROFILE_T[matchIdx].id, 0);

            }



            // }
        }

        pArry_tmp = Common_Json_GetAttrValueArr(lowerData, "Iinterval");
        str_tmp = NULL;
        Common_Json_GetAttrValue(pArry_tmp, 0, NULL, NULL, &str_tmp, 0, 0);
        if (str_tmp)
        {
            sscanf(str_tmp,"%d-%d",&i_min,&i_max);

            pArry_root = Common_Json_SetAttrValue(outdata, -1, "IFrameIntervalRange", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pArry_root, 0, NULL, Common_Json_Type_Number, 0, i_min, 0);
            Common_Json_SetAttrValue(pArry_root, 1, NULL, Common_Json_Type_Number, 0, i_max, 0);
        }

        pArry_tmp = Common_Json_GetAttrValueArr(lowerData, "EncQuality");
        str_tmp = NULL;
        Common_Json_GetAttrValue(pArry_tmp, 0, NULL, NULL, &str_tmp, 0, 0);
        if (str_tmp)
        {
            sscanf(str_tmp,"%d-%d",&i_min,&i_max);

            pArry_root = Common_Json_SetAttrValue(outdata, -1, "EncQualityRange", Common_Json_Type_Array, NULL, 0, 0);
            Common_Json_SetAttrValue(pArry_root, 0, NULL, Common_Json_Type_Number, 0, i_min, 0);
            Common_Json_SetAttrValue(pArry_root, 1, NULL, Common_Json_Type_Number, 0, i_max, 0);
        }

    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}




int frmVideoCompressAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_videocompressability(header, indata, outdata,opt);
    }

    return ret;
}


static int web_semantic_get_imagecapability(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int i=0;
    int ret = 0;
	char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *setArr =NULL;
    OVFS_IMAGE_CAP_T imageCap = {0};

    ret = get_image_capability(header, opt, &imageCap);
    if(ret == 0)
    {
        Common_Json_SetAttrValueInt(outdata, "SupportTWDR", imageCap.twdr);
        Common_Json_SetAttrValueInt(outdata, "SupportExposureLight", imageCap.exposureLight);
        Common_Json_SetAttrValueInt(outdata, "SupportISPScene", imageCap.ispScene);
        Common_Json_SetAttrValueInt(outdata, "SupportCorridor", imageCap.corridor);
        Common_Json_SetAttrValueInt(outdata, "SupportSwitchSensor", imageCap.switchsensor);

        setArr = Common_Json_SetAttrValueArr(outdata, "SupportLights");
        for(i=0; i<8; i++)
        {
            if(slen(imageCap.lightList[i])>0)
            {
                Common_Json_SetAttrValueArrStr(setArr, i, imageCap.lightList[i]);
            }
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/boardsys/sys/boardability");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

        if(ret == 0)
        {
            if(Common_Json_GetAttrValueStr(lowerData, "IrBoardType", &str_tmp))
            {
                Common_Json_SetAttrValueStr(outdata, "IrBoardType", str_tmp);
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


int frmImageCapability(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    ret = web_semantic_get_imagecapability(header, indata, outdata, opt);

    return ret;
}

int get_image_capability(cJSON_Struct *header, OVFS_WEB_OPTION_S *opt, OVFS_IMAGE_CAP_T *imageCap)
{
    int i = 0;
    int ret = 0;
    int idx = 0;
    int sizeList = 0;
    int curDevNo = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *list = NULL;
    cJSON_Struct *item = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/boardsys/image/ability");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        list = Common_Json_GetItem(lowerData, -1, "AbilityList");
        if (list)
        {
            sizeList = Common_Json_Size(list);
            for (idx = 0; idx < sizeList; idx ++)
            {
                item = Common_Json_GetItem(list, idx, NULL);
                if (item)
                {
                    //Common_Json_StandardPrint(item, NULL, NULL, NULL);
                    Common_Json_GetAttrValueInt(item, "Device", &curDevNo);
                    LOGW("DevNo:%d Opt:%d\n", curDevNo, opt->dev);
                    if (curDevNo != opt->dev)
                    {
                        continue;
                    }
                    else
                    {
                        Common_Json_GetAttrValueInt(item, "bSupportCorridor", &imageCap->corridor);
                        Common_Json_GetAttrValueInt(item, "bSupportTWDR", &imageCap->twdr);
                        Common_Json_GetAttrValueInt(item, "bSupportExposureLight", &imageCap->exposureLight);
                        Common_Json_GetAttrValueInt(item, "bSupportISPScene", &imageCap->ispScene);
                        Common_Json_GetAttrValueInt(item, "bSupportSwitch", &imageCap->switchsensor);

                        cJSON_Struct *light_list = Common_Json_GetAttrValueArr(item, "SupportLights");
                		int light_size = Common_Json_ArraySize(light_list);
                		for(i=0; i<light_size; i++)
                		{
                            if(i<8)
                            {
                                str_tmp = NULL;
                                Common_Json_GetAttrValue(light_list, i, NULL, NULL, &str_tmp, NULL, NULL);
                                if(str_tmp)
                                {
                                    snprintf(imageCap->lightList[i],sizeof(imageCap->lightList[i]),"%s",str_tmp);
                                }
                            }
                		}
                    }
                }
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

static int web_semantic_get_videoparaex(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i;

    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        //区别一组的机芯控制, ptz/image注释
        /*if (g_ovfs_web->devInfo.bPTZ == 1)
        {
            Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/Image/Attribute/All");
        }
        else*/
        {
            Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Image/Attribute/All");
        }
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }
    //Common_Json_StandardPrint(lowerData,NULL, NULL, NULL);
    //printf("ret[%d]\n",ret);
    if (ret == 0)
    {
        /*μ×2??Yê±2??§3?￡?D′?à*/
        Common_Json_SetAttrValue(outdata,-1,"WBMode",Common_Json_Type_Number,NULL,-1,0);
        //Common_Json_SetAttrValue(outdata,-1,"LightCorrectMode",Common_Json_Type_Number,NULL,1,0);//±31a?￡ê? 0-1?±? 1-?í?ˉì? 2-??1aò??? 3-±31a213￥
        //Common_Json_SetAttrValue(outdata,-1,"LightCorrectLevel",Common_Json_Type_Number,NULL,64,0);//64-μí 128-?D 192-??
        Common_Json_SetAttrValue(outdata,-1,"DarkCompensation",Common_Json_Type_Number,NULL,0,0);//°μ??213￥ 0-1?±? 1-μí 2-?D 3-??
        Common_Json_SetAttrValue(outdata,-1,"ShowZoomRate",Common_Json_Type_Number,NULL,0,0);//??ê?±?±? 0-òt2? 1??ê?
        Common_Json_SetAttrValue(outdata,-1,"ShowCoordinate",Common_Json_Type_Number,NULL,0,0);//??ê?×?±ê 0-òt2? 1??ê?
        Common_Json_SetAttrValue(outdata,-1,"ShowStatus",Common_Json_Type_Number,NULL,0,0);//??ê?×′ì? 0-òt2? 1??ê?
        Common_Json_SetAttrValue(outdata,-1,"FocusSpeed",Common_Json_Type_Number,NULL,1,0);//±??1?ù?è 0-μí 1-?D 2-??
        Common_Json_SetAttrValue(outdata,-1,"ZoomSpeed",Common_Json_Type_Number,NULL,0,0);//±??1±?±? 0-μí 1-?D 2-??
        Common_Json_SetAttrValue(outdata,-1,"ElecAntiQuake",Common_Json_Type_Number,NULL,0,0);//μ?×ó·à?? 0-1?±? 1-?a??
        Common_Json_SetAttrValue(outdata,-1,"CameraType",Common_Json_Type_Number,NULL,0,0);//MDIé????úààDí, ????è?
        Common_Json_SetAttrValue(outdata,-1,"EnableAutoConfig",Common_Json_Type_Number,NULL,-1,0);//ê?·??a??é????ú×??ˉ????,-12??§3?
        Common_Json_SetAttrValue(outdata,-1,"VideoType",Common_Json_Type_Number,NULL,1,0);//0￡oêó?μ2?êy 1￡o ?úD?éè??

        cJSON_Struct *pArry_root = NULL;
        pArry_root = Common_Json_GetAttrValue(lowerData, -1, "ImageList", NULL, NULL, NULL, NULL);

        int arraySize = Common_Json_Size(pArry_root);
        for (i = 0; i < arraySize; i++)
        {
            int type = 0;
            int value = 0;
            int mode = 0;
            int enable = 0;
            cJSON_Struct *tmp = NULL;
            char *str_tmp = NULL;

            if ((tmp = Common_Json_GetAttrValue(pArry_root, i, NULL, NULL, NULL, NULL, NULL)) == NULL ||
                    Common_Json_GetAttrValue(tmp, -1, "Type", NULL, NULL, &type, 0) == NULL)
            {
                LOGE("get type failed!\n");
                continue;
            }
            Common_Json_GetAttrValue(tmp, -1, "Device", NULL, NULL, &value, 0);
            if(opt->dev != value)
            {
                //LOGW("Device different!\n");
                continue;
            }
            switch (type)
            {
            //0-DayNight 1-Exposure 2-WhiteBalance 3-fouce 4-Sharpen 5-Nr3d 6-Wdr 7-Defog
            //8-Gamma 9-ColorStyle 10-Mirror 11-Flicker 12-IrLights 13-AutoLens
            //14-Iris 15-ColorAdjust 16-SlowFrame
            case 0:
            {
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode",Common_Json_Type_Object,NULL,0,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/DayNightMode",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/ircutOutTrig",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/ircutOutTrig",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Delay",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/Delay",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/NightToDayThreshold",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/NightToDayThreshold",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/DayToNightThreshold",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/DayToNightThreshold",Common_Json_Type_Number,NULL,value,0);

                //×??¨ò?è?ò1?D???aê???êy
                Common_Json_GetAttrValue(tmp,-1,"Param/DayStart",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/DayStart",Common_Json_Type_Number,NULL,value,0);

                //×??¨ò?è?ò1?D???áê???êy
                Common_Json_GetAttrValue(tmp,-1,"Param/DayEnd",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DayNightMode/DayEnd",Common_Json_Type_Number,NULL,value,0);

                Common_Json_GetAttrValue(tmp,-1,"Param/ExternTriggerMode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"ExtInTrig",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/IrCurTriggerDir",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"IcrOutTrig",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 1: // ??1a
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Shutter",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"ShutterMode",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Gain",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"GainMode",Common_Json_Type_Number,NULL,value,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/AeRouteMode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"AeRouteMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 2://°×??oa
            {
                OVFS_WEB_WBNAMEMAP_S *nameMap = s_whiteBalanceNameMap;
                int nameMapCount = sizeof(s_whiteBalanceNameMap)/sizeof(s_whiteBalanceNameMap[0]);
                /*if (g_ovfs_web->devInfo.bPTZ == 1)
                {
                    nameMap = s_whiteBalanceNameMap_dome;
                    nameMapCount = sizeof(s_whiteBalanceNameMap_dome)/sizeof(s_whiteBalanceNameMap_dome[0]);
                }*/

                cJSON_Struct *whiteBalance = Common_Json_SetAttrValueObj(outdata, "WhiteBalance");
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    int i;
                    for (i = 0; i < nameMapCount; i++)
                    {
                        if (nameMap[i].wbValue == value)
                        {
                            Common_Json_SetAttrValueStr(whiteBalance, "Mode", nameMap[i].wbName);
                            break;
                        }
                    }
                    if (i >= nameMapCount)
                    {
                        Common_Json_SetAttrValueStr(whiteBalance, "Mode", nameMap[0].wbName);
                    }
                }
                cJSON_Struct *wbGain = NULL;
                if (Common_Json_GetAttrValueInt(tmp, "Param/CurR", &value) ||
                        Common_Json_GetAttrValueInt(tmp, "Param/CustomR", &value))
                {
                    if (wbGain == NULL)
                    {
                        wbGain = Common_Json_SetAttrValueObj(whiteBalance, "Gain");
                    }
                    Common_Json_SetAttrValueInt(wbGain, "R", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/CurG", &value) ||
                        Common_Json_GetAttrValueInt(tmp, "Param/CustomG", &value))
                {
                    if (wbGain == NULL)
                    {
                        wbGain = Common_Json_SetAttrValueObj(whiteBalance, "Gain");
                    }
                    Common_Json_SetAttrValueInt(wbGain, "G", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/CurB", &value) ||
                        Common_Json_GetAttrValueInt(tmp, "Param/CustomB", &value))
                {
                    if (wbGain == NULL)
                    {
                        wbGain = Common_Json_SetAttrValueObj(whiteBalance, "Gain");
                    }
                    Common_Json_SetAttrValueInt(wbGain, "B", value);
                }
                break;
            }
            case 3://???1?￡ê?
            {
                break;
            }
            case 4: // è??è
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Level",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"SharpnessLevel",Common_Json_Type_Number,NULL,enable?value:0,0);
                break;
            }
            case 5:
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Level",NULL,NULL,&value,0);
                if (enable)
                {
                    Common_Json_SetAttrValue(outdata,-1,"NRTfode",Common_Json_Type_Number,NULL,value+1,0);
                }
                else
                {
                    Common_Json_SetAttrValue(outdata,-1,"NRTfode",Common_Json_Type_Number,NULL,0,0);
                }
                break;
            }
            case 6: // ?í?ˉì?
            {
                int WDLeve = -1;
                int WDType = -1;
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                if (Common_Json_GetAttrValue(tmp,-1,"Param/Level",NULL,NULL,&WDLeve,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"WDMode",Common_Json_Type_Number,NULL,WDLeve+1,0);
                }
                if (Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&WDType,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"WdrType",Common_Json_Type_Number,NULL,WDType,0);
                }
                break;
            }
            case 7: // è￥?í
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Level",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DefogLevel",Common_Json_Type_Number,NULL,value+1,0);
                Common_Json_SetAttrValue(outdata,-1,"Defog",Common_Json_Type_Number,NULL,enable,0);
                break;
            }
            case 8: // Gamma2?êy
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"GammaMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 9: // í????êá?
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"PicQualityMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 10: // ?μ??
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"MinorMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 11:
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"FreqMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 12: //	oìíaμ??￡ê?
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Enable",NULL,NULL,&enable,0);
                if (0 == enable)
                {
                    Common_Json_SetAttrValue(outdata,-1,"IcrLightMode",Common_Json_Type_Number,NULL,0,0);
                }
                else
                {
                    Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&mode,0);
                    if (0 == mode)
                    {
                        Common_Json_SetAttrValue(outdata,-1,"IcrLightMode",Common_Json_Type_Number,NULL,2,0);
                    }
                    else
                    {
                        Common_Json_SetAttrValue(outdata,-1,"IcrLightMode",Common_Json_Type_Number,NULL,1,0);
                    }
                }

                Common_Json_GetAttrValue(tmp,-1,"Param/Level",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"IcrLightAue",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 13: // ×??ˉ?μí·
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"AutoLensMode",Common_Json_Type_Number,NULL,value,0);
                if(Common_Json_GetAttrValue(tmp,-1,"Param/AfArea",NULL,NULL,&value,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"AfArea",Common_Json_Type_Number,NULL,value,0);
                }
                if(Common_Json_GetAttrValue(tmp,-1,"Param/AfSearch",NULL,NULL,&value,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"AfSearch",Common_Json_Type_Number,NULL,value,0);
                }
                if(Common_Json_GetAttrValue(tmp,-1,"Param/AfSensitivity",NULL,NULL,&value,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"AfSensitivity",Common_Json_Type_Number,NULL,value,0);
                }
                if(Common_Json_GetAttrValue(tmp,-1,"Param/EnableDigitZoom",NULL,NULL,&value,0))
                {
                    Common_Json_SetAttrValue(outdata,-1,"EnableDigitZoom",Common_Json_Type_Number,NULL,value,0);
                }
                break;
            }
            case 14: // 1aè|?￡ê?
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"IrisMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 15://áá?è.±￥oí?è
            {
                break;
            }
            case 16: //?y?ì??
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"AutoSlowShutter",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 17: //D???±è
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"SnMode",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 18: // ?ˉì??μμ????y
            {
                Common_Json_GetAttrValue(tmp,-1,"Param/Mode",NULL,NULL,&value,0);
                Common_Json_SetAttrValue(outdata,-1,"DynamicBpCali",Common_Json_Type_Number,NULL,value,0);
                break;
            }
            case 19: // PTZ-WDR
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/BackLightMode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "LightCorrectMode", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/Level", &value))
                {
                    int table[3] = {64, 128, 192};
                    if (value >= sizeof(table)/sizeof(table[0]) || value < 0)
                    {
                        value = 0;
                    }
                    Common_Json_SetAttrValueInt(outdata, "LightCorrectLevel", table[value]);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/SSDR", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "DarkCompensation", value);
                }
                break;
            }
            case 20: // PTZ-×??ˉ?y?ì??
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "AutoSlowShutter", value);
                }
                break;
            }
            case 21: //PTZ-êy×??μ??
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "DnrMode", value);
                }
                break;
            }
            case 22: //PTZ-OSD??ê?
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/Ratio", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ShowZoomRate", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/Position", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ShowCoordinate", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/Status", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ShowStatus", value);
                }
                break;
            }
            case 23: //PTZ-±?±??ù?è
            {
                // μ×2?ê? {1-μí,2-?D,3-??},api?¨ò?ê?{0-μí,1-?D,2-??}.
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ZoomSpeed", value-1);
                }
                break;
            }
            case 24: //PTZ-μ?×ó·à??
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ElecAntiQuake", value);
                }
                break;
            }
            case 25: // ?ò?ú??1a
            {
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "Scene", value+1);
                }

                break;
            }
            case 26:
            {
                //char* out = Common_Json_Print(tmp,NULL);
                //printf("\n[%s]\n",out);
                //wfree(out);
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "LightCorrectMode", value);
                }
                if (Common_Json_GetAttrValueInt(tmp, "Param/Level", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "LightCorrectLevel", value);
                }
                //	out = Common_Json_Print(outdata,NULL);
                //	printf("\n[%s]\n",out);
                //	wfree(out);
                break;
            }
            case 27:
            {
                Common_Json_SetAttrValue(outdata,-1,"AECompensation",Common_Json_Type_Object,NULL,0,0);
                if (Common_Json_GetAttrValueInt(tmp, "Param/Level", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "AECompensation/Level", value);
                }
                break;
            }
            case 28://hongwai shouguang
            {
                Common_Json_SetAttrValue(outdata,-1,"ExposureLight",Common_Json_Type_Object,NULL,0,0);
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "ExposureLight/Mode", value);
                }
                break;
            }
            case 29://NR2D
            {
                Common_Json_SetAttrValue(outdata,-1,"NR2D",Common_Json_Type_Object,NULL,0,0);
                if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &value))
                {
                    Common_Json_SetAttrValueInt(outdata, "NR2D/Mode", value);
                }
                break;
            }
            case 30://Dis
            {
                Common_Json_SetAttrValue(outdata,-1,"Dis",Common_Json_Type_Object,NULL,0,0);
                if (Common_Json_GetAttrValueInt(tmp, "Param/Enable", &enable) && Common_Json_GetAttrValueInt(tmp, "Param/Level", &value))
                {
                    mode = 0 ;
                    if(enable != 0)
                    {
                        mode = value + 1;
                    }

                    Common_Json_SetAttrValueInt(outdata, "Dis/Mode", mode);
                }

                break;
            }
            case 31://LightType
            {
                str_tmp = NULL;
                if (Common_Json_GetAttrValueStr(tmp, "Param/Type", &str_tmp))
                {
                    Common_Json_SetAttrValueStr(outdata, "LightType", str_tmp);
                }
                break;
            }
            }
        }
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if(ret == 0)
    {
        cJSON_Struct *minorModeCap = Common_Json_SetAttrValueArr(outdata, "MinorModeCapability");
        Common_Json_SetAttrValueArrInt(minorModeCap, 0, 0);
        Common_Json_SetAttrValueArrInt(minorModeCap, 1, 1);
        Common_Json_SetAttrValueArrInt(minorModeCap, 2, 2);
        Common_Json_SetAttrValueArrInt(minorModeCap, 3, 3);

        cJSON_Struct *wdrModeCap = Common_Json_SetAttrValueArr(outdata, "WDRModeCapability");
        Common_Json_SetAttrValueArrInt(wdrModeCap, 0, 0);
        Common_Json_SetAttrValueArrInt(wdrModeCap, 1, 2);

        cJSON_Struct *wdrLevelCap = Common_Json_SetAttrValueArr(outdata, "WDRLevelCapability");
        cJSON_Struct *lightCorrectLevelCap = Common_Json_SetAttrValueArr(outdata, "LightCorrectLevelCapability");
        for(i=0;i<10;i++)
        {
            if(i<3)
            {
                Common_Json_SetAttrValueArrInt(wdrLevelCap, i, i+1);
            }
            Common_Json_SetAttrValueArrInt(lightCorrectLevelCap, i, i+1);
        }

        cJSON_Struct *lightCorrectModeCap = Common_Json_SetAttrValueArr(outdata, "LightCorrectModeCapability");
        Common_Json_SetAttrValueArrInt(lightCorrectModeCap, 0, 0);
        Common_Json_SetAttrValueArrInt(lightCorrectModeCap, 1, 1);
        Common_Json_SetAttrValueArrInt(lightCorrectModeCap, 2, 2);

        cJSON_Struct *irCutModeCap = Common_Json_SetAttrValueArr(outdata, "IRCutModeCapability");
        Common_Json_SetAttrValueArrInt(irCutModeCap, 0, 0);
        Common_Json_SetAttrValueArrInt(irCutModeCap, 1, 1);

        cJSON_Struct *exposureLihtCap = Common_Json_SetAttrValueArr(outdata, "ExposureLihtCapability");
        Common_Json_SetAttrValueArrInt(exposureLihtCap, 0, 0);
        Common_Json_SetAttrValueArrInt(exposureLihtCap, 1, 1);

        cJSON_Struct *ispSceneCap = Common_Json_SetAttrValueArr(outdata, "IspSceneCapability");

        OVFS_IMAGE_CAP_T imageCap = {0};
        ret = get_image_capability(header, opt, &imageCap);
        if (0 == ret)
        {
            if(imageCap.corridor)
            {
                Common_Json_SetAttrValueArrInt(minorModeCap, 4, 5);
                Common_Json_SetAttrValueArrInt(minorModeCap, 4, 5);
            }

            if(imageCap.twdr)
            {
                Common_Json_SetAttrValueArrInt(wdrModeCap, 2, 3);
            }

            if(imageCap.exposureLight == 2)
            {
                Common_Json_SetAttrValueArrInt(exposureLihtCap, 2, 2);
            }

            if(imageCap.ispScene == 1)
            {
                Common_Json_SetAttrValueArrInt(ispSceneCap, 0, 1);
                Common_Json_SetAttrValueArrInt(ispSceneCap, 1, 2);
            }
        }
    }

    return ret;
}

int web_semantic_set_videoparaex(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
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

    cJSON_Struct *array= NULL;
    if (0 == ret)
    {
        array = Common_Json_SetAttrValue(lowerData, -1, "ImageList", Common_Json_Type_Array, NULL, 0, 0);
        if (array == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    cJSON_Struct *tmp = NULL;
    int value = 0;
    int enable = 0;
    int mode = 0;
    int segCount = 0;

    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/DayNightMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/ircutOutTrig",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/ircutOutTrig",Common_Json_Type_Number,NULL,value,0);
            }
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/Delay",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Delay",Common_Json_Type_Number,NULL,value,0);
            }
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/NighttoDayThreshold",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/NightToDayThreshold",Common_Json_Type_Number,NULL,value,0);
            }
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/DaytoNightThreshold",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/DayToNightThreshold",Common_Json_Type_Number,NULL,value,0);
			}
			//è?ò1?D??×??¨ò? ?aê??áê???êy
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/DayStart",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/DayStart",Common_Json_Type_Number,NULL,value,0);
            }
			if(Common_Json_GetAttrValue(indata,-1,"DayNightMode/DayEnd",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/DayEnd",Common_Json_Type_Number,NULL,value,0);
			}

            if(Common_Json_GetAttrValue(indata,-1,"ExtInTrig",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/ExternTriggerMode",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"IcrOutTrig",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/IrCurTriggerDir",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,1,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"ShutterMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Shutter",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"GainMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Gain",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"AeRouteMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/AeRouteMode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 2-°×??oa
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            cJSON_Struct *whiteBalance = Common_Json_GetAttrValueObj(indata, "WhiteBalance");
            cJSON_Struct *wbGain = Common_Json_GetAttrValueObj(whiteBalance, "Gain");
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,2,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            char *valueStr;
            if (Common_Json_GetAttrValueStr(whiteBalance, "Mode", &valueStr))
            {
                OVFS_WEB_WBNAMEMAP_S *nameMap = s_whiteBalanceNameMap;
                int nameMapCount = sizeof(s_whiteBalanceNameMap)/sizeof(s_whiteBalanceNameMap[0]);
                /*if (g_ovfs_web->devInfo.bPTZ == 1)
                {
                    nameMap = s_whiteBalanceNameMap_dome;
                    nameMapCount = sizeof(s_whiteBalanceNameMap_dome)/sizeof(s_whiteBalanceNameMap_dome[0]);
                }*/

                int foundIndex = nameMap[0].wbValue;
                int i;
                for (i = 0; i < nameMapCount; i++)
                {
                    if (strcmp(valueStr, nameMap[i].wbName) == 0)
                    {
                        foundIndex = nameMap[i].wbValue;
                        break;
                    }
                }

                Common_Json_SetAttrValueInt(tmp, "Param/Mode", foundIndex);
            }
            if (Common_Json_GetAttrValueInt(wbGain, "R", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/CustomR", value);
            }
            if (Common_Json_GetAttrValueInt(wbGain, "G", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/CustomG", value);
            }
            if (Common_Json_GetAttrValueInt(wbGain, "B", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/CustomB", value);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 3-???1?￡ê?
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            //Common_Json_GetAttrValue(indata,-1,"WbMode",NULL,NULL,&value,0);
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,3,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            //if (value >= 0)
            {
                //Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }
        }
    }

    // 4-è?à??è
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,4,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"SharpnessLevel",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value,0);
                Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 5-3D?μ??
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,5,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"NRTfode",NULL,NULL,&value,0))
            {
                if (0 == value)
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,0,0);
                }
                else
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
                    Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value-1,0);
                }
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 6-?í?ˉì?
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,6,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"WDMode",NULL,NULL,&value,0))
            {
                if (value)
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
                    Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value-1,0);
                }
                else
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,0,0);
                }
            }
            if(Common_Json_GetAttrValue(indata,-1,"WdrType",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 7-è￥?í
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,7,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"DefogLevel",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value-1,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"Defog",NULL,NULL,&enable,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,enable,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 8-gamma
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,8,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"GammaMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
                Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 9-±￥oí?è
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            if(Common_Json_GetAttrValue(indata,-1,"PicQualityMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,9,0);
                Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
                Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(tmp) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 10-?μ???￡ê?
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            if(Common_Json_GetAttrValue(indata,-1,"MinorMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,10,0);
                Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
                Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(tmp) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 11-?1éá??
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,11,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"FreqMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 12-Icr?￡ê?
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,12,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"IcrLightMode",NULL,NULL,&mode,0))
            {
                if (0 == mode)
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,0,0);
                }
                else if (1 == mode)
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
                    Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,1,0);
                }
                else
                {
                    Common_Json_SetAttrValue(tmp,-1,"Param/Enable",Common_Json_Type_Number,NULL,1,0);
                    Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,0,0);
                }
            }
            if(Common_Json_GetAttrValue(indata,-1,"IcrLightAue",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 13-μ??ˉ?μí·
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,13,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"AutoLensMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValueInt(indata, "AfArea", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/AfArea", value);
            }
            if(Common_Json_GetAttrValueInt(indata, "AfSearch", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/AfSearch", value);
            }
            if(Common_Json_GetAttrValueInt(indata, "AfSensitivity", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/AfSensitivity", value);
            }
            if(Common_Json_GetAttrValueInt(indata, "EnableDigitZoom", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Param/EnableDigitZoom", value);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 14-1aè|2?êy
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,14,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"IrisMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 15-é?2ê2?êy
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,15,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            //Common_Json_GetAttrValue(indata,-1,"AutoSlowShutter",NULL,NULL,&value,0);
            //Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
        }
    }

    // 16-?ì??2?êy
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,16,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"AutoSlowShutter",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 17-D???±è2?êy
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,17,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"SnMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 18-?ˉì??μμ????y
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,18,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"DynamicBpCali",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 19 PTZ-WDR
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,19,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"LightCorrectMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/BackLightMode",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"LightCorrectLevel",NULL,NULL,&value,0))
            {
                //int table[3] = {64, 128, 192};
                if (value > 0)
                {
                    value = (value - 1)/64;
                }
                Common_Json_SetAttrValue(tmp,-1,"Param/Level",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"DarkCompensation",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/SSDR",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 20 PTZ-×??ˉ?y?ì??
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,20,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"AutoSlowShutter",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 21 PTZ-êy×??μ??
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,21,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"DnrMode",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 22 PTZ-OSD??ê?
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,22,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"ShowZoomRate",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Ratio",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"ShowCoordinate",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Position",Common_Json_Type_Number,NULL,value,0);
            }
            if(Common_Json_GetAttrValue(indata,-1,"ShowStatus",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Status",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 23 PTZ-±?±??ù?è
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,23,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            // μ×2?ê? {1-μí,2-?D,3-??},api?¨ò?ê?{0-μí,1-?D,2-??}.
            if(Common_Json_GetAttrValue(indata,-1,"ZoomSpeed",NULL,NULL,&value,0))
            {
                if (value < 0 || value > 2)
                {
                    value = 2; // ??è??a??.
                }
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value+1,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 24 PTZ-μ?×ó·à??
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
            Common_Json_SetAttrValue(tmp,-1,"Type",Common_Json_Type_Number,NULL,24,0);
            Common_Json_SetAttrValue(tmp,-1,"Device",Common_Json_Type_Number,NULL,0,0);
            cJSON_Struct *para = Common_Json_SetAttrValue(tmp,-1,"Param",Common_Json_Type_Object,NULL,0,0);
            if(Common_Json_GetAttrValue(indata,-1,"ElecAntiQuake",NULL,NULL,&value,0))
            {
                Common_Json_SetAttrValue(tmp,-1,"Param/Mode",Common_Json_Type_Number,NULL,value,0);
            }

            if(Common_Json_Size(para) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 25 ?ò?ú??1a
    if (0 == ret)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {

            if(Common_Json_GetAttrValueInt(indata, "Scene", &value))
            {
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueInt(tmp, "Type", 25);
                Common_Json_SetAttrValueObj(tmp, "Param");
                Common_Json_SetAttrValueInt(tmp, "Param/Mode", value-1);
            }

            if(Common_Json_Size(tmp) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 26
    if(ret == 0)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
			int mode = 0;
			int level = 5;
			if (Common_Json_GetAttrValueInt(indata, "LightCorrectMode", &mode) &&
				Common_Json_GetAttrValueInt(indata, "LightCorrectLevel", &level))
			{
				Common_Json_SetAttrValueInt(tmp, "Type", 26);
				Common_Json_SetAttrValueInt(tmp, "Device", 0);
				Common_Json_SetAttrValueObj(tmp, "Param");
				Common_Json_SetAttrValueInt(tmp, "Param/Mode", mode);
				Common_Json_SetAttrValueInt(tmp, "Param/Level", level);
			}

            if(Common_Json_Size(tmp) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 27
    if(ret == 0)
    {
        if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else
        {
			int level = 50;
			if (Common_Json_GetAttrValueObj(indata, "AECompensation") &&
				Common_Json_GetAttrValueInt(indata, "AECompensation/Level", &level))
			{
				Common_Json_SetAttrValueInt(tmp, "Type", 27);
				Common_Json_SetAttrValueInt(tmp, "Device", 0);
				Common_Json_SetAttrValueObj(tmp, "Param");
				Common_Json_SetAttrValueInt(tmp, "Param/Level", level);
			}

            if(Common_Json_Size(tmp) == 0)
            {
                segCount--;
                Common_Json_RemoveItem(array, segCount, NULL);
            }
        }
    }

    // 28 hongwaishouguang
    if(ret == 0)
    {
        int level = 0;
        if (Common_Json_GetAttrValueObj(indata, "ExposureLight") &&
                Common_Json_GetAttrValueInt(indata, "ExposureLight/Mode", &level))
        {
            if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Common_Json_SetAttrValueInt(tmp, "Type", 28);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                Common_Json_SetAttrValueInt(tmp, "Param/Mode", level);
            }
        }

    }

    if(ret == 0)
    {
        int level = 0;
        if (Common_Json_GetAttrValueObj(indata, "NR2D") &&
                Common_Json_GetAttrValueInt(indata, "NR2D/Mode", &level))
        {
            if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Common_Json_SetAttrValueInt(tmp, "Type", 29);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                Common_Json_SetAttrValueInt(tmp, "Param/Mode", level);
            }
        }
    }
    if(ret == 0) //type = 30
    {
        int level = 0;
        enable = 0;
        if (Common_Json_GetAttrValueObj(indata, "Dis") &&
                Common_Json_GetAttrValueInt(indata, "Dis/Mode", &mode))
        {
            if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Common_Json_SetAttrValueInt(tmp, "Type", 30);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");
                if(mode != 0)
                {
                    enable = 1;
                    level = mode - 1;
                }
                Common_Json_SetAttrValueInt(tmp, "Param/Enable", enable);
                Common_Json_SetAttrValueInt(tmp, "Param/Level", level);
            }
        }

    }

    if(ret == 0) //type = 31
    {
        char *str_tmp = NULL;
        if (Common_Json_GetAttrValueStr(indata, "LightType", &str_tmp))
        {
            if ((tmp = Common_Json_SetAttrValue(array, segCount++, NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
            {
                ret = WEB_CODE_LackingMem;
            }
            else
            {
                Common_Json_SetAttrValueInt(tmp, "Type", 31);
                Common_Json_SetAttrValueInt(tmp, "Device", 0);
                Common_Json_SetAttrValueObj(tmp, "Param");

                Common_Json_SetAttrValueStr(tmp, "Param/Type", str_tmp);
            }
        }

    }

    /*if (g_ovfs_web->devInfo.bPTZ==1)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Ptz/Image/Attribute/All");
    }
    else*/
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Image/Attribute/All");
    }
    ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


// //IPC-??êó°25230êó?μ2?êy
int frmVideoParaEx(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_videoparaex(header, indata, outdata,opt);
            break;

        case 1:
            //éè??2?êy
            ret = web_semantic_set_videoparaex(header, indata, outdata,opt);
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

static int web_semantic_set_lenscheck(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    char buf[56]= {0};
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
        Common_Json_SetAttrValueInt(lowerData, "Type", 17);
        Common_Json_SetAttrValue(lowerData, -1, "CmdParam", Common_Json_Type_Object,NULL, 0, 0);
        snprintf(buf,sizeof(buf),"/boardsys/image/cmd");
        Ovfs_Web_UpdateHeader(header, REST_PUT, buf);
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return  ret;
}


//?μí·D￡×?
int frmAutoLensCorrection(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            //éè??2?êy
            ret = web_semantic_set_lenscheck(header, indata, outdata,opt);
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

static int web_semantic_autoaperturecorrection(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    return ret;
}

int frmAutoApertureCorrection(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_autoaperturecorrection(header, indata, outdata);
        if (0 == ret)
        {
            Common_Json_SetAttrValueStr(outdata, STATUS_CODE, SAVE_OK);
        }
    }

    return ret;
}

static int web_semantic_badpixeltest(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int status = 0;
    char url[128] = {0};

    if(Common_Json_GetAttrValueInt(indata, "Status", &status) == NULL)
    {
        return WEB_CODE_InvalidArg;
    }

    if(status)
    {
        snprintf(url, sizeof(url), "/boardsys/image/OpenBadPointDet");
    }
    else
    {
        snprintf(url, sizeof(url), "/boardsys/image/closeBadPointDet");
    }

    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

int frmBadPixelTest(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            ret = web_semantic_badpixeltest(header, indata, outdata);
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

//êó?μ?úμ2
int frmVideoShelterPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //??è?2?êy
            ret = web_semantic_get_videoshelterpara(header, indata, outdata,opt);
            break;

        case 1:
            //éè??2?êy
            ret = web_semantic_set_videoshelterpara(header, indata, outdata,opt);
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

#if 1//(!defined SIMPLIFIED) || (defined WIFIDOME)
static int web_semantic_get_VideoQueryByMonth(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    char time[16] = {0};
    int StartYear = -1;
    int StartMonth = -1;
    int queryResult = 0;
    int querySize = 0;
    int nloop = 0;
    int dayIndex = 0;
    cJSON_Struct *pArry_root = NULL;
    cJSON_Struct *pArry_tmp = NULL;

    if (0 == ret)
    {
        if (Common_Json_GetAttrValue(indata, -1, "StartYear", NULL, NULL, &StartYear, NULL) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        if (Common_Json_GetAttrValue(indata, -1, "StartMonth", NULL, NULL, &StartMonth, NULL) == NULL)
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

    cJSON_Struct *outParam = NULL;
    if (0 == ret)
    {
        snprintf(time, sizeof(time), "%04d%02d", StartYear, StartMonth);

        pArry_root = Common_Json_SetAttrValue(lowerData, -1, "ChannelInfo", Common_Json_Type_Array, NULL, 0, 0);
        Common_Json_SetAttrValue(pArry_root, 0, "Device", Common_Json_Type_Number, NULL, opt->dev, 0);
        Common_Json_SetAttrValue(pArry_root, 0, "Channel", Common_Json_Type_Number, NULL, opt->ch, 0);
        Common_Json_SetAttrValue(lowerData, -1, "StreamMask", Common_Json_Type_Number, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "QueryMode", Common_Json_Type_Number, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "Month", Common_Json_Type_String, time, 0, 0);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/Replay/RecordListByMonthly");

        ret = Ovfs_Web_RestMethodA(header, lowerData, &outParam, 30000);
    }

    if (0 == ret)
    {
        //     Common_Json_GetAttrValue(outParam, -1, "DayNum", NULL, NULL, &querySize, NULL);
        pArry_tmp = Common_Json_GetAttrValue(outParam, -1, "Day", NULL, NULL, NULL, NULL);
        if ((pArry_tmp != NULL))//&&(querySize > 0)&&(querySize < 32)
        {
            if (querySize = Common_Json_Size(pArry_tmp))
            {
                for (nloop = 0; nloop < querySize; nloop ++)
                {
                    dayIndex = 0;
                    Common_Json_GetAttrValue(pArry_tmp, nloop, "Day", NULL, NULL, &dayIndex, NULL);
                    if ((dayIndex < 1) || (dayIndex > 31))
                    {
                        LOGW("DayIndex is not valid!\n");
                        break;
                    }
                    else
                    {
                        queryResult |= 1 << (dayIndex - 1);
                    }
                }
            }
            else
            {
                LOGE("DayNum and Array not Match!\n");
            }
        }
        else
        {
            LOGE("Param is unvalid!\n");
        }

        Common_Json_SetAttrValueInt(outdata, "QueryResult", queryResult);
    }

    if (outParam)
    {
        Common_Json_Delete(outParam);
        outParam = NULL;
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

//2é?ˉò??????D??D?ììóD????
int frmVideoQueryByMonth(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_VideoQueryByMonth(header, indata, outdata,opt);
    }

    return ret;
}

static int web_semantic_get_videoRecordQuery(cJSON_Struct *header, cJSON_Struct *indata,cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    tzset();
    if (outdata == NULL || indata == NULL)
    {
        ret = WEB_CODE_InvalidArg;
    }
    cJSON_Struct *channelArray = NULL;
    int channelCount = 0;
    if (0 == ret)
    {
        channelArray = Common_Json_GetAttrValue(indata,-1,"Channels",NULL,NULL,0,0);
        if (channelArray == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            channelCount = Common_Json_ArraySize(channelArray);
            channelCount = MAX2(1, channelCount);
        }
    }
    int QueryType = 0;
    time_t BeginDateTime = 0;
    time_t EndDateTime = 0;
    char* localDTStr1 = NULL;
    char* localDTStr2 = NULL;
    if (0 == ret)
    {
        //Common_Json_GetAttrValue(indata, -1, "QueryType", NULL, NULL, &QueryType, 0);

        if(
            Common_Json_GetAttrValue(indata,-1,"DeviceLocalDateTimeStart", NULL,&localDTStr1,NULL,NULL) &&
            Common_Json_GetAttrValue(indata, -1,"DeviceLocalDateTimeStop",NULL,&localDTStr2,NULL,NULL))
        {

            LOGW("string -->localDateTime BeginDateTime:[%s] strin --> localDateTime EndDateTime:[%s]\n",localDTStr1,localDTStr2);
            TimeStr2UnixTime(localDTStr1,&BeginDateTime);
            TimeStr2UnixTime(localDTStr2,&EndDateTime);

            LOGW("string --> BeginDateTime:[%d] strin --> EndDateTime:[%d]\n",BeginDateTime,EndDateTime);
        }
        else if ( Common_Json_GetAttrValue(indata,-1, "BeginDateTime",NULL,NULL, &BeginDateTime,NULL) &&
                  Common_Json_GetAttrValue(indata,  -1,"EndDateTime", NULL,NULL, &EndDateTime,NULL))
        {
            LOGW("BeginDateTime:[%d] EndDateTime:[%d]\n",BeginDateTime,EndDateTime);

        }
        else
        {
            LOGE("Invalid Aargs,UTC time LocalDateTime find None\n");
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

        struct tm startTime = {0};
        struct tm stopTime = {0};

        gmtime_r(&BeginDateTime,&startTime);
        gmtime_r(&EndDateTime,&stopTime);

        char time[20] = {0};
        snprintf(time,sizeof(time),"%04d%02d%02d%02d%02d%02d",startTime.tm_year + 1900,startTime.tm_mon + 1,startTime.tm_mday,
                 startTime.tm_hour,startTime.tm_min,startTime.tm_sec);
        Common_Json_SetAttrValue(lowerData, -1,"StartTime",Common_Json_Type_String,time,0,0);
        LOGD("gmtime StartTime:%s\n",time);
        snprintf(time,sizeof(time),"%04d%02d%02d%02d%02d%02d",stopTime.tm_year + 1900,stopTime.tm_mon + 1,stopTime.tm_mday,
                 stopTime.tm_hour,stopTime.tm_min,stopTime.tm_sec);
        Common_Json_SetAttrValue(lowerData, -1, "StopTime", Common_Json_Type_String, time, 0, 0);
        LOGD("gmtime StopTime:%s\n",time);

        //?Yê±2??§3?°′?÷×ó??á÷2é?ˉ????￡?1ì?¨?a?÷??á÷
        Common_Json_SetAttrValue(lowerData, -1, "StreamMask", Common_Json_Type_Number, NULL, 0, 0);
        Common_Json_SetAttrValue(lowerData, -1, "QueryMode", Common_Json_Type_Number, NULL, QueryType, 0);
        //QueryCondition￡?0-è?òa?ú×?￡?1-í?ê±?ú×? ?Yê±2??§3?°′ì??t2é?ˉ￡?1ì?¨?aí?ê±?ú×?

        Common_Json_SetAttrValue(lowerData, -1, "Device", Common_Json_Type_Number,NULL,opt->dev,0);

        Ovfs_Web_UpdateHeader(header, REST_GET, "/Record/Replay/RecordList");

        int i = 0;
        for (i = 0; i < channelCount; i++)
        {
            int channNum = 0;
            if (Common_Json_GetAttrValue(channelArray, i, NULL, NULL, NULL, &channNum, 0) == NULL)
            {
                // è?1???óD???¨í¨μà,??è??????÷μúò???í¨μà.
                if (channelCount == 1)
                {
                    channNum = 1;
                }
                else
                {
                    break;
                }
            }
            channNum--; // API?D"Channels"′ó1?aê?.???????￡?é?D′ó0?aê?.
            //Common_Json_GetAttrValue(inData,-1,"QueryCondition",NULL,NULL,&QueryCondition,0);

            Common_Json_SetAttrValue(lowerData, -1, "Channel", Common_Json_Type_Number,NULL, channNum, 0);

            cJSON_Struct *outParam = NULL;
            ret = Ovfs_Web_RestMethodA(header, lowerData, &outParam, 30000);
            if (ret == 0)
            {
                ParseRecordFiles(outParam,header, outdata, opt->dev, channNum,BeginDateTime,EndDateTime);
            }

            Common_Json_Delete(outParam);
            outParam = NULL;

            if (ret != 0)
            {
                break;
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


//????2é?ˉ  D??ó?ú ?§3???ìì2é?ˉ
int frmVideoRecordsQuery(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int BeginDateTime,EndDateTime;
    char* localDTStr1,localDTStr2;
    int is_mkv_request = 0;
    if(
        Common_Json_GetAttrValueStr(indata,"DeviceLocalDateTimeStart", &localDTStr1) &&
        Common_Json_GetAttrValueStr(indata,"DeviceLocalDateTimeStop",&localDTStr2))
    {
        is_mkv_request = 1;
    }
    else if ( Common_Json_GetAttrValueInt(indata, "BeginDateTime", &BeginDateTime) &&
              Common_Json_GetAttrValueInt(indata, "EndDateTime", &EndDateTime))
    {

    }
    else
    {
        LOGE("Invalid Aargs,UTC time LocalDateTime find None\n");
        ret = WEB_CODE_InvalidArg;
    }
    if (0 == ret)
    {
        ret = is_mkv_request?web_semantic_get_videoRecordQuery(header, indata, outdata,opt):web_semantic_get_videoRecordQueryOld(header, indata, outdata,opt);
    }

    return ret;
}


static int web_semantic_get_FlashVideoQuery(cJSON_Struct *header, Common_cJSON_T *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    //int content = 0;
    char start_time[32] = {0};
    char end_time[32] = {0};
    int i_year = 0;
    int i_month = 0;
    int i_day = 0;
    /*int i_hour = 0;
    int i_min = 0;
    int i_sec = 0;
    int seg_num = 0;
    int iloop = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    Common_cJSON_T *pArry_root = NULL;
    Common_cJSON_T *pArry_root2 = NULL;
    Common_cJSON_T *pArry_root3 = NULL;
    Common_cJSON_T *pObj_tmp = NULL;*/
    Common_cJSON_T *ptmp = NULL;
    //cJSON_Struct *pArry_tmp = NULL;

    if (0 == ret)
    {
        //ptmp = Common_cJSON_GetObjectItem(indata, "Date");
        ptmp = Common_Json_GetAttrValueArr(indata, "Date");
        //if (Common_cJSON_GetArraySize(ptmp) == 3)
        if (ptmp && Common_Json_ArraySize(ptmp) != 3)
        {
            ret = WEB_CODE_InvalidArg;
        }
        else
        {
            //ptmp1 = Common_cJSON_GetArrayItem(ptmp, 0);
            //i_year = ptmp1->valueint;
            Common_Json_GetAttrValue(ptmp, 0, NULL, NULL, NULL, &i_year, NULL);
            //ptmp1 = Common_cJSON_GetArrayItem(ptmp, 1);
            //i_month= ptmp1->valueint;
            Common_Json_GetAttrValue(ptmp, 1, NULL, NULL, NULL, &i_month, NULL);
            //ptmp1 = Common_cJSON_GetArrayItem(ptmp, 2);
            //i_day = ptmp1->valueint;
            Common_Json_GetAttrValue(ptmp, 2, NULL, NULL, NULL, &i_day, NULL);
            snprintf(start_time, sizeof(start_time), "%04d%02d%02d%02d%02d%02d", i_year, i_month, i_day, 0, 0, 0);
            snprintf(end_time, sizeof(end_time), "%04d%02d%02d%02d%02d%02d", i_year, i_month, i_day, 23, 59, 59);
        }
    }

    Common_Json_SetAttrValueStr(indata, "DeviceLocalDateTimeStart", start_time);
    Common_Json_SetAttrValueStr(indata, "DeviceLocalDateTimeStop", end_time);
    ret = web_semantic_get_videoRecordQuery(header,indata,outdata,opt);



    return ret;
}


//????2é?ˉ
int frmFlashVideoQuery(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_FlashVideoQuery(header, indata, outdata,opt);
    }

    return ret;
}

static int web_semantic_get_SDCardPics(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    // 1.?a??2é?ˉμ?·??§
    int start_time;
    int end_time;
    char* localDTStr1;
    char* localDTStr2;

    if (Common_Json_GetAttrValue(indata,-1,"DeviceLocalDateTimeStart", NULL,&localDTStr1,NULL,NULL)&&
            Common_Json_GetAttrValue(indata, -1,"DeviceLocalDateTimeStop",NULL,&localDTStr2,NULL,NULL))
    {
        LOGW("string -->localDateTime BeginDateTime:[%s] strin --> localDateTime EndDateTime:[%s]\n",localDTStr1,localDTStr2);
        TimeStr2UnixTime(localDTStr1,&start_time);
        TimeStr2UnixTime(localDTStr2,&end_time);

        LOGW("string --> BeginDateTime:[%d] strin --> EndDateTime:[%d]\n",start_time,end_time);
    }
    else if (Common_Json_GetAttrValue(indata, -1, "StartTime", NULL, NULL, &start_time, NULL) && Common_Json_GetAttrValue(indata, -1, "EndTime", NULL, NULL, &end_time, NULL))
    {

    }
    else
    {

        ret = WEB_CODE_InvalidArg;

    }

    // 2.2é?ˉ
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        char uri_path[128] = {0};

        snprintf(uri_path, sizeof(uri_path), "/FileManage/QueryFile?StartTime=%d&&StopTime=%d", start_time, end_time);
        LOGW("[%s]\n",uri_path);

        struct tm startTime = {0};
        struct tm stopTime = {0};
        /* è?·??– GMT ?—?é—′ */
        gmtime_r(&start_time,&startTime);
        /* è?·??– GMT ?—?é—′ */
        gmtime_r(&end_time,&stopTime);

        //ovfs_record ??￥èˉ￠????”¨????????°?—?é—′
        //localtime_r(&BeginDateTime ,&startTime);
        /* è?·??– GMT ?—?é—′ */
        //localtime_r(&EndDateTime ,&stopTime);

        char time[20] = {0};


        trans_utctime_to_localtime_str(start_time,time,20);
        LOGD("local StartTime:%s\n",time);


        trans_utctime_to_localtime_str(end_time,time,20);

        LOGD("local StartTime:%s\n",time);


        Ovfs_Web_UpdateHeader(header, REST_GET, uri_path);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 30000);
    }

    if (0 == ret)
    {
        char *str_tmp = NULL;
        int iloop = 0;
        cJSON_Struct *pArry_tmp = NULL;
        cJSON_Struct *pArry_root = NULL;

        pArry_root = Common_Json_SetAttrValue(outdata, -1, "SearchResults", Common_Json_Type_Array, NULL, 0, 0);
        pArry_tmp = Common_Json_GetAttrValue(lowerData, -1, "ResList", NULL, NULL, NULL, NULL);
        if (pArry_tmp)
        {
            for (iloop = 0; iloop < Common_Json_Size(pArry_tmp); iloop ++)
            {
                Common_Json_GetAttrValue(pArry_tmp, iloop, NULL, NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValue(pArry_root, iloop, NULL, Common_Json_Type_String, str_tmp?str_tmp:"", 0, 0);
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


//
// 	===================================
// 		* Function    : ANTS_MID_SNAP_QueryImageRequest
// 		* Description : 2é?ˉ×￥í?μ?í?????3?.
// 		* Input Para  : int timeStart -- 2é?ˉμ??eê?ê±??.
// 		int timeLen -- 2é?ˉμ?ê±??3¤?è,μ￥???a??.
// 		* Output Para : char** fileName -- í??????t??3?,???t????ò?'\0'·???.
// 		int* count -- í??????tμ?êy??.
// 		* Return Value: 0 -- OK; Else -- Error.
// ×￠òa:è?1?2é?ˉ?á1??a??(??óDí???),?ò·μ??ret=0, *fileName=NULL, *count=0.
//?ì?÷SD?¨í???
int frmSearchSDCardPics(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        ret = web_semantic_get_SDCardPics(header, indata, outdata);
    }

    return ret;
}
#endif

static int web_semantic_get_SmartAbility(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int ret = 0;
    int i_num = 0;
    int ability_size = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *AbilityList = NULL;
    cJSON_Struct *outList = NULL;
    cJSON_Struct *AbilityVersionList = NULL;
    if(0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }
    if (0 == ret)
    {
        outList = Common_Json_SetAttrValueArr(outdata, "List");
        AbilityList = Common_Json_GetAttrValueArr(lowerData, "AbilityList");
        AbilityVersionList = Common_Json_GetAttrValueArr(lowerData, "AbilityVersionList");
        ability_size = Common_Json_ArraySize(AbilityList);
        for(i=0; i<ability_size; i++)
        {
            Common_Json_GetAttrValue(AbilityList, i, NULL, NULL, &str_tmp, NULL, NULL);
            Common_Json_GetAttrValue(AbilityVersionList, i, NULL, NULL, NULL, &i_num, NULL);
            cJSON_Struct *out_loop = Common_Json_SetAttrValueArrObj(outList, i);
#if 0//def PLATFORM_JZT32
            //1001
            if(smatch(str_tmp, "DetectWire") ||
               smatch(str_tmp, "DetectAbsent") ||
               smatch(str_tmp, "Retrograde"))
            {
                if(i_num>1)i_num += 999;
            }
            else
            {
                i_num += 1001;
            }
#else
            if(smatch(str_tmp, "DetectWire") ||
               smatch(str_tmp, "DetectAbsent") ||
               smatch(str_tmp, "Retrograde"))
            {
                if(i_num>1)i_num += 998;
            }

            if(smatch(str_tmp, "RegionalInvasion"))
            {
                i_num += 1; //target track
            }


#endif

            Common_Json_SetAttrValueInt(out_loop, str_tmp, i_num);
        }
    }
    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }
    return ret;
}
int frmGetSmartAbility(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_SmartAbility(header, indata, outdata,opt);
            break;
        default:
            ret = WEB_CODE_InvalidArg;
            break;
        }
    }
    return ret;
}

int CaptureV2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int type = 0;
    int stream = 0;

    LOGD("method:[%s]\n",wp->method);

    if(smatch(wp->method,"GET"))
    {
        char *begin = NULL,*end = NULL, *str_tmp = NULL;

        if ((begin = strstr(wp->queryString, "DataType=")))
        {
            begin += strlen("DataType=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                str_tmp = strndup(begin, end - begin);
                type = strtoi(str_tmp);
                free(str_tmp);
                str_tmp = NULL;
            }
        }

        if ((begin = strstr(wp->queryString, "StreamNo=")))
        {
            begin += strlen("StreamNo=");
            for (end = begin; *end != '\0' && *end != ' ' && *end != '&'; end++);

            if (end > begin)
            {
                str_tmp = strndup(begin, end - begin);
                stream = strtoi(str_tmp);
                free(str_tmp);
                str_tmp = NULL;
            }
        }

        LOGD("querystring [%d][%d]\n",type,stream);
    }
    else
    {
        Common_Json_GetAttrValueInt(indata, "DataType", &type);
        Common_Json_GetAttrValueInt(indata, "StreamNo", &stream);
        LOGD("body [%d][%d]\n",type,stream);
    }

    cJSON_Struct *lowerData = NULL;
	if(0 == ret)
	{
        char uri[64];
        snprintf(uri, sizeof(uri), "/BoardSys/Snap/Device%d/Channel%d/Stream%d",opt->dev ,opt->ch > 0 ? opt->ch-1 : 0,stream);
		Ovfs_Web_UpdateHeader(header, REST_GET, uri);

		ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
	}

    char *fileBuffer = NULL;
    int fileBufferSize = 0;
    if (0 == ret)
    {
        char *path = NULL;
        struct stat tmpStat;
        int fd = -1;
        if (Common_Json_GetAttrValueStr(lowerData, "Path", &path) == NULL)
        {
            ret = WEB_CODE_InternalMistake;
        }
        else if (stat(path, &tmpStat) != 0 || (fd = open(path, O_RDWR)) < 0)
        {
            ret = WEB_CODE_FileNotAccess;
        }
        else if ((fileBuffer = malloc(tmpStat.st_size)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
        else if (read(fd, fileBuffer, tmpStat.st_size) < tmpStat.st_size)
        {
            ret = WEB_CODE_IamBusy;
        }
        else
        {
            if(type == 1)
            {
                int len = 0;
                char *pic_encode = NULL;
                pic_encode = Common_Base64_Encode((char*)fileBuffer, tmpStat.st_size, &len);

                Common_Json_SetAttrValueStr(outdata, "ImageData", pic_encode);

                wp->ext = ".text";

                if(pic_encode){
                    Common_Free(pic_encode, __FUNCTION__, __LINE__);
                }
            }
            else
            {
                fileBufferSize = tmpStat.st_size;
                Common_Json_SetItemExtData(outdata, fileBuffer, fileBufferSize);
                if (g_ovfs_web && g_ovfs_web->debugPrint)
                {
                    PRINT_DBG("fileBufferSize=%d\n", fileBufferSize);
                }
            }
        }

        if (fd >= 0)
        {
            close(fd);
            fd = -1;
            remove(path);
        }
    }

    if (fileBuffer)
    {
        free(fileBuffer);
        fileBuffer = NULL;
        fileBufferSize = 0;
    }

    if (lowerData)
    {
    	Common_Json_Delete(lowerData);
    	lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_onvifpara(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    /*
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/onvif/OnvifCfg");

    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (0 == ret)
    {
        ret = JsonOper_MergeObj(outdata, lowerData, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }*/

    if(Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/AuthEnable", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "authEnable", i_num);
    }

    i_num = 0;
    if(Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/AdaptiveIp", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "adaptiveIp", i_num);
    }

    if(Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "Timeout", i_num);
    }

    if(Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/FixedIp", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "FixedIp", i_num);
    }

    if(Common_Json_GetAttrValueStr(g_ovfs_config, "OnvifCfg/FixedIpAddr", &str_tmp))
    {
        Common_Json_SetAttrValueStr(outdata, "FixedIpAddr", str_tmp);
    }

    return ret;
}

int web_semantic_set_onvifpara(cJSON_Struct *header, cJSON_Struct *indata, Common_cJSON_T *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int adaptive_cur = 0;
    int enable = -1;
    int adaptive = -1;
    int timeout = -1;
    int fixedip = -1;

    if(Common_Json_GetAttrValueInt(indata, "authEnable", &enable))
    {
        Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/AuthEnable", enable);
    }

    if(Common_Json_GetAttrValueInt(indata, "adaptiveIp", &adaptive))
    {
        Common_Json_GetAttrValueInt(g_ovfs_config, "OnvifCfg/AdaptiveIp", &adaptive_cur);
        Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/AdaptiveIp", adaptive);
    }

    if(Common_Json_GetAttrValueInt(indata, "Timeout", &timeout))
    {
        Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/Timeout", timeout);
    }
    LOGD("enable:[%d] adaptive:[%d] timeout:[%d]\n",enable,adaptive,timeout);
    if(enable == -1 && adaptive == -1 && timeout == -1)return 0;

    int i = 0;
    for(i=0; i<s_libinfo_count; i++)
    {
        if(strstr(s_libinfo[i].libName,"libonvif.so") && s_libinfo[i].fSetConfig != NULL)
		{
            THIRD_ONVIF_CFG cfg = {0};

            if(enable != -1)
            {
                cfg.AuthEnable = enable;
                cfg.UseMask |= 0x1;
            }

            if(adaptive != -1 && adaptive != adaptive_cur)
            {
                cfg.AdaptiveIp = adaptive;
                cfg.UseMask |= 0x2;
                if(adaptive == 1)
                {

                    Common_Json_SetAttrValueInt(g_ovfs_config, "OnvifCfg/FixedIp", 0);
                    Common_Json_SetAttrValueStr(g_ovfs_config, "OnvifCfg/FixedIpAddr", "");
                    cfg.UseMask |= 0x8;
                    cfg.UseMask |= 0x10;
                }
            }


            if(timeout != -1)
            {
                cfg.Timeout = timeout;
                cfg.UseMask |= 0x4;
            }

			s_libinfo[i].fSetConfig(&cfg);
		}
    }

    Access_SaveConfig(g_AccessHandle, g_ovfs_config);

    return ret;
}


int frmOnvifPara(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:

            ret = web_semantic_get_onvifpara(header, indata, outdata,opt);
            break;

        case 1:

            ret = web_semantic_set_onvifpara(header, indata, outdata,opt);
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

const char * base64char2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
int base64_decodev2( const char * base64,  char * bindata )
{
    int i, j;
    unsigned char k;
    unsigned char temp[4];
    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char2[k] == base64[i] )
                temp[0]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char2[k] == base64[i+1] )
                temp[1]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char2[k] == base64[i+2] )
                temp[2]= k;
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char2[k] == base64[i+3] )
                temp[3]= k;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                       ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                       ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
            break;

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                       ((unsigned char)(temp[3]&0x3F));
    }
    return j;
}


static int web_semantic_get_url_realplay(cJSON_Struct *header, const char *ifaddr, int type, cJSON_Struct *indata, Common_cJSON_T *outdata)
{
    int ret = 0;

    int rtsp_port = 554;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        int channel = 0;
        int stream_type = 0;
        char rtsp_url[256] = {0};


        Common_Json_GetAttrValue(indata, -1, "Channel", NULL, NULL, &channel, NULL);
        Common_Json_GetAttrValue(indata, -1, "StreamType", NULL, NULL, &stream_type, NULL);

        if (type < 10)
        {
            snprintf(rtsp_url, sizeof(rtsp_url), "rtsp://%s:%d/living_comb%02d%s.264", ifaddr, rtsp_port, channel, (stream_type==1)?"_sub":((stream_type==2)?"_third":""));
        }
        else
        {
            snprintf(rtsp_url, sizeof(rtsp_url), "rtsp://%s:%d/ch%02d%s.264", ifaddr, rtsp_port, channel, (stream_type==1)?"_sub":((stream_type==2)?"_third":""));
        }
        Common_Json_SetAttrValue(outdata, -1, "RtspUrl", Common_Json_Type_String, rtsp_url, 0, 0);
    }

    return ret;
}

static int web_semantic_get_url_talk(cJSON_Struct *header, const char *ifaddr, cJSON_Struct *indata, Common_cJSON_T *outdata)
{
    int ret = 0;

    int rtsp_port = 554;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        int channel = 0;
        int audio_type = 1;
        char rtsp_url[256] = {0};

        Common_Json_GetAttrValue(indata, -1, "Channel", NULL, NULL, &channel, NULL);
        Common_Json_GetAttrValue(indata, -1, "AudioType", NULL, NULL, &audio_type, NULL);

        if (audio_type == 1)
        {
            snprintf(rtsp_url, sizeof(rtsp_url), "rtsp://%s:%d/audioback/ch_%02d/type_g711a", ifaddr, rtsp_port, channel);
            Common_Json_SetAttrValue(outdata, -1, "RtspUrl", Common_Json_Type_String, rtsp_url, 0, 0);
        }
        else if (audio_type == 2)
        {
            snprintf(rtsp_url, sizeof(rtsp_url), "rtsp://%s:%d/audioback/ch_%02d/type_g711u", ifaddr, rtsp_port, channel);
            Common_Json_SetAttrValue(outdata, -1, "RtspUrl", Common_Json_Type_String, rtsp_url, 0, 0);
        }
        else
        {
            ret = -1;
        }
    }

    return ret;
}

static int web_semantic_get_url_alarm(cJSON_Struct *header, const char *ifaddr, cJSON_Struct *indata, Common_cJSON_T *outdata)
{
    int ret = 0;

    int rtsp_port = 554;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        char rtsp_url[256] = {0};
        snprintf(rtsp_url, sizeof(rtsp_url), "rtsp://%s:%d/ch01.264", ifaddr, rtsp_port);
        Common_Json_SetAttrValue(outdata, -1, "RtspUrl", Common_Json_Type_String, rtsp_url, 0, 0);
    }

    return ret;
}

static int web_semantic_get_url_replay(cJSON_Struct *header, const char *ifaddr, int type, cJSON_Struct *indata, Common_cJSON_T *outdata)
{
    int ret = 0;

    int rtsp_port = 554;
    cJSON_Struct *lowerData = NULL;
    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/MediaServer/Rtsp/Attribute");

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (ret == 0)
        {
            Common_Json_GetAttrValue(lowerData, -1, "Rtsp.RtspPort", NULL, NULL, &rtsp_port, NULL);
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if ( 0 == ret )
    {
        int channel = 0;
        int stream_type = 0;
        char* timemode = NULL;
        time_t BeginDateTime = 0;
        time_t EndDateTime = 0;
        char* start_time = NULL;
        char* stop_time = NULL;
        char rtsp_url[256] = {0};

        Common_Json_GetAttrValue ( indata, -1, "Channel", NULL, NULL, &channel, NULL );
        Common_Json_GetAttrValue ( indata, -1, "StreamType", NULL, NULL, &stream_type, NULL );
        Common_Json_GetAttrValue ( indata, -1, "StartTime", NULL, &start_time, NULL, NULL );
        Common_Json_GetAttrValue ( indata, -1, "StopTime", NULL, &stop_time, NULL, NULL );

        //兼容0时区时间和本地时区时间
        Common_Json_GetAttrValue ( indata, -1, "TimeMode", NULL, &timemode, NULL, NULL );
        if(timemode && smatch(timemode, "Local"))
        {

            LOGW ( "string -->localDateTime BeginDateTime:[%s] strin --> localDateTime EndDateTime:[%s]\n",start_time,stop_time );
            TimeStr2UnixTime ( start_time,&BeginDateTime );
            TimeStr2UnixTime ( stop_time,&EndDateTime );

            LOGW ( "string --> BeginDateTime:[%d] strin --> EndDateTime:[%d]\n",BeginDateTime,EndDateTime );

            struct tm startTime = {0};
            struct tm stopTime = {0};

            gmtime_r ( &BeginDateTime,&startTime );

            gmtime_r ( &EndDateTime,&stopTime );



            char time1[20] = {0};
            snprintf ( time1,sizeof ( time1 ),"%04d%02d%02d%02d%02d%02d",startTime.tm_year + 1900,startTime.tm_mon + 1,startTime.tm_mday,
                       startTime.tm_hour,startTime.tm_min,startTime.tm_sec );

            LOGD ( "gmtime StartTime:%s\n",time1 );


            char time2[20] = {0};
            snprintf ( time2,sizeof ( time2 ),"%04d%02d%02d%02d%02d%02d",stopTime.tm_year + 1900,stopTime.tm_mon + 1,stopTime.tm_mday,
                       stopTime.tm_hour,stopTime.tm_min,stopTime.tm_sec );

            LOGD ( "gmtime StopTime:%s\n",time2 );

            if ( type < 10 )
            {
                snprintf ( rtsp_url, sizeof ( rtsp_url ), "rtsp://%s:%d/recording_comb?ch=%d&stream=%d&start=%s&stop=%s&rec_inquiry_type=%d", ifaddr, rtsp_port, channel, stream_type, time1, time2, 0 );
            }
            else
            {
                snprintf ( rtsp_url, sizeof ( rtsp_url ), "rtsp://%s:%d/recording?ch=%d&stream=%d&start=%s&stop=%s&rec_inquiry_type=%d", ifaddr, rtsp_port, channel, stream_type,time1, time2, 0 );
            }

        }
        else
        {
            if ( type < 10 )
            {
                snprintf ( rtsp_url, sizeof ( rtsp_url ), "rtsp://%s:%d/recording_comb?ch=%d&stream=%d&start=%s&stop=%s&rec_inquiry_type=%d", ifaddr, rtsp_port, channel, stream_type, start_time, stop_time, 0 );
            }
            else
            {
                snprintf ( rtsp_url, sizeof ( rtsp_url ), "rtsp://%s:%d/recording?ch=%d&stream=%d&start=%s&stop=%s&rec_inquiry_type=%d", ifaddr, rtsp_port, channel, stream_type,start_time, stop_time, 0 );
            }

        }
        Common_Json_SetAttrValue ( outdata, -1, "RtspUrl", Common_Json_Type_String, rtsp_url, 0, 0 );
    }

    return ret;
}


int frmGetRtspUrl(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        //′|àíip?aíaí?ip?ò??óò??
        //?úíaí?ó3é?rtspμ??·Dè?àí?
        char * domain_or_ip = NULL;
        char * ifaddr = wp->ifaddr;
        Common_Json_GetAttrValueStr(indata, "Ip", &domain_or_ip);

        if(domain_or_ip)
        {
            ifaddr = domain_or_ip;
        }


        switch (opt->type)
        {
        // RealPlay
        case 0:
        case 10:
        {
            ret = web_semantic_get_url_realplay(header, ifaddr, opt->type, indata, outdata);
        }
        break;
        // Talk
        case 1:
        {
            ret = web_semantic_get_url_talk(header, ifaddr, indata, outdata);
        }
        break;
        // Alarm
        case 2:
        {
            ret = web_semantic_get_url_alarm(header, ifaddr, indata, outdata);
        }
        break;
        // Replay
        case 3:
        case 13:
        {
            ret = web_semantic_get_url_replay(header, ifaddr, opt->type, indata, outdata);
        }
        break;
        default:
        {
            ret = WEB_CODE_InvalidArg;
        }
        break;
        }

        char *rtsp_url = NULL;
        Common_Json_GetAttrValueStr(outdata, "RtspUrl", &rtsp_url);
        LOGW("type=%d ret=%x url=%s\n", opt->type, ret, rtsp_url);
    }

    return ret;
}

cJSON_Struct *g_OptimalConfig[3] = {0};

static int web_semantic_get_optimalvideoencode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int streamidx = 0;
    int i_num = 0;
    int i_num2 = 0;
    int status = 1;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmpdata = NULL;
    cJSON_Struct *loopdata = NULL;

    if(g_OptimalConfig[0] == NULL)
    {
        lowerData = ovfs_web_parse_jsonfile("/root/res/custom/OptimalVideoEncode.json");
        if(lowerData)
        {
            tmpdata = Common_Json_GetAttrValueArr(lowerData, "AttributeList");
            size = Common_Json_ArraySize(tmpdata);

            for(i=0; i<size; i++)
            {
                loopdata = Common_Json_GetAttrValueArrItem(tmpdata, i);

                if(Common_Json_GetAttrValueInt(loopdata, "Stream", &streamidx))
                {
                    Common_Json_SetAttrValueInt(loopdata, "EncodeFormat", 8);
                    if(Common_Json_GetAttrValueInt(loopdata, "Iinterval", &i_num) && i_num < 200)
                    {
                        Common_Json_SetAttrValueInt(loopdata, "Iinterval", 200);
                    }

                    if(Common_Json_GetAttrValueInt(loopdata, "BitrateCtrlMode", &i_num) && i_num == 1)
                    {
                        Common_Json_SetAttrValueInt(loopdata, "BitrateCtrlMode", 0);
                        Common_Json_SetAttrValueInt(loopdata, "Quality", 1);
                    }

                    g_OptimalConfig[streamidx] = Common_Json_Duplicate(loopdata, 1);
                }
            }

            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
        else
        {
            //Common_Json_SetAttrValueInt(outdata, "Status", status);
            return WEB_CODE_Unsupported;
        }

    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Video/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        tmpdata = Common_Json_GetAttrValueArr(lowerData, "AttributeList");
        size = Common_Json_ArraySize(tmpdata);

        for(i=0; i<size; i++)
        {
            loopdata = Common_Json_GetAttrValueArrItem(tmpdata, i);

            if(Common_Json_GetAttrValueInt(loopdata, "Stream", &streamidx) == NULL)
            {
                continue;
            }

            if(Common_Json_GetAttrValueInt(loopdata, "Iinterval", &i_num) &&
                Common_Json_GetAttrValueInt(g_OptimalConfig[streamidx], "Iinterval", &i_num2))
            {
                if(i_num != i_num2)
                {
                    status = 0;
                    break;
                }
            }

            if(Common_Json_GetAttrValueInt(loopdata, "BitrateCtrlMode", &i_num) &&
                Common_Json_GetAttrValueInt(g_OptimalConfig[streamidx], "BitrateCtrlMode", &i_num2))
            {
                if(i_num != i_num2)
                {
                    status = 0;
                    break;
                }
            }

            if(Common_Json_GetAttrValueInt(loopdata, "Quality", &i_num) &&
                Common_Json_GetAttrValueInt(g_OptimalConfig[streamidx], "Quality", &i_num2))
            {
                if(i_num != i_num2)
                {
                    status = 0;
                    break;
                }
            }

            if(Common_Json_GetAttrValueInt(loopdata, "EncodeFormat", &i_num) &&
                Common_Json_GetAttrValueInt(g_OptimalConfig[streamidx], "EncodeFormat", &i_num2))
            {
                if(i_num != i_num2)
                {
                    status = 0;
                    break;
                }
            }

            if(Common_Json_GetAttrValueInt(loopdata, "Bitrate", &i_num) &&
                Common_Json_GetAttrValueInt(g_OptimalConfig[streamidx], "Bitrate", &i_num2))
            {
                if(i_num != i_num2)
                {
                    status = 0;
                    break;
                }
            }
        }

        Common_Json_SetAttrValueInt(outdata, "Status", status);

    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_set_optimalvideoencode(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    int i = 0;
    int size = 0;
    int streamidx = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmpdata = NULL;
    cJSON_Struct *loopdata = NULL;

    if(g_OptimalConfig[0] == NULL)
    {
        return ret;
    }

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Video/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(0 == ret)
    {
        tmpdata = Common_Json_GetAttrValueArr(lowerData, "AttributeList");
        size = Common_Json_ArraySize(tmpdata);

        for(i=0; i<size; i++)
        {
            loopdata = Common_Json_GetAttrValueArrItem(tmpdata, i);

            if(Common_Json_GetAttrValueInt(loopdata, "Stream", &streamidx) == NULL)
            {
                continue;
            }

            if(g_OptimalConfig[streamidx])
            {
                JsonOper_MergeObj(loopdata, g_OptimalConfig[streamidx], 0);
            }
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Video/Attribute/All");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmOptimalVideoEncode(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_optimalvideoencode(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_optimalvideoencode(header, indata, outdata);
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

static char linkTypeV2[][32] =
{
    "",
    "Motion",// 1
    "Vhide",// 2
    "RegionalInvasion",
    "DetectWire",
    "PersonStaying",//5
    "DetectAbsent",
    "ParkingViolation",
    "Retrograde",//8
    "AlarmIn",
    "SensorAlarm"  //10
};

int web_get_alarmtime(cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int i_num = 0;
    cJSON_Struct *temp = NULL;

    if(Common_Json_GetAttrValueInt(indata, "AlarmMode", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "AlarmMode", i_num);
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "WeekList"))
    {
        cJSON_Struct *weeklist = Common_Json_SetAttrValueArr(outdata, "WeekList");

        size = Common_Json_ArraySize(temp);
        for(i=0; i<7; i++)
        {
            i_num = 0;
            Common_Json_GetAttrValue(temp, i, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValueArrInt(weeklist, i, i_num);
        }
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "TimeList"))
    {
        cJSON_Struct *timelist = Common_Json_SetAttrValueArr(outdata, "TimeList");
        for(i=0; i<4; i++)
        {
            cJSON_Struct *loop_get = Common_Json_GetAttrValueArrItem(temp, i);
            cJSON_Struct *loop_set = Common_Json_SetAttrValue(timelist, i, NULL, Common_Json_Type_Array, NULL, 0, 0);

            i_num = 0;
            Common_Json_GetAttrValue(loop_get, 0, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValueArrInt(loop_set, 0, i_num/100);
            Common_Json_SetAttrValueArrInt(loop_set, 1, i_num%100);

            i_num = 0;
            Common_Json_GetAttrValue(loop_get, 1, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValueArrInt(loop_set, 2, i_num/100);
            Common_Json_SetAttrValueArrInt(loop_set, 3, i_num%100);
        }
    }

    return ret;
}

int web_set_alarmtime(cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int i_num = 0, i_num2 = 0;
    cJSON_Struct *temp = NULL;

    if(Common_Json_GetAttrValueInt(indata, "AlarmMode", &i_num))
    {
        Common_Json_SetAttrValueInt(outdata, "AlarmMode", i_num);
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "WeekList"))
    {
        cJSON_Struct *weeklist = Common_Json_SetAttrValueArr(outdata, "WeekList");

        size = Common_Json_ArraySize(temp);
        for(i=0; i<7; i++)
        {
            i_num = 0;
            Common_Json_GetAttrValue(temp, i, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_SetAttrValueArrInt(weeklist, i, i_num);
        }
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "TimeList"))
    {
        cJSON_Struct *timelist = Common_Json_SetAttrValueArr(outdata, "TimeList");
        for(i=0; i<4; i++)
        {
            cJSON_Struct *loop_get = Common_Json_GetAttrValueArrItem(temp, i);
            cJSON_Struct *loop_set = Common_Json_SetAttrValue(timelist, i, NULL, Common_Json_Type_Array, NULL, 0, 0);

            i_num = 0;
            i_num2 = 0;
            Common_Json_GetAttrValue(loop_get, 0, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_GetAttrValue(loop_get, 1, NULL, NULL, NULL, &i_num2, NULL);
            Common_Json_SetAttrValueArrInt(loop_set, 0, i_num*100+i_num2);

            i_num = 0;
            i_num2 = 0;
            Common_Json_GetAttrValue(loop_get, 2, NULL, NULL, NULL, &i_num, NULL);
            Common_Json_GetAttrValue(loop_get, 3, NULL, NULL, NULL, &i_num2, NULL);
            Common_Json_SetAttrValueArrInt(loop_set, 1, i_num*100+i_num2);
        }
    }

    return ret;
}

int g_bSupportExpandAlarmOut = -1;
int web_get_expand_alarmout_list(cJSON_Struct *header, cJSON_Struct *outdata)
{
    int ret = -1;
    int i = 0;
    int dev = 0;
    int ch = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    char strName[32] = {0};
    char strDevNameList[8][32] = {0};
    cJSON_Struct *lowerData = NULL;

    if(outdata == NULL)
    {
        return ret;
    }

    if(g_bSupportExpandAlarmOut == -1)
    {
        g_bSupportExpandAlarmOut = 0;
        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/ability");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if(ret == 0)
        {
            cJSON_Struct *modeCap = Common_Json_GetAttrValueArr(lowerData, "ModeCapability");
            if(modeCap)
            {
                int size = Common_Json_ArraySize(modeCap);
                for(i=0; i<size; i++)
                {
                    i_num= 0;
                    Common_Json_GetAttrValue(modeCap, i, NULL, NULL, NULL, &i_num, NULL);
                    if(i_num == 2)
                    {
                        g_bSupportExpandAlarmOut = 1;
                    }
                }
            }
        }
        if (lowerData)
        {
            Common_Json_Delete(lowerData);
    		lowerData = NULL;
        }
    }

    if(g_bSupportExpandAlarmOut == 0)
    {
        return -1;
    }

    cJSON_Struct *cap = Common_Json_SetAttrValueArr(outdata, "ExpandAlarmOutCapability");


    Ovfs_Web_UpdateHeader(header, REST_GET, "/Ptz/AlarmOut/attribute");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if(ret == 0)
    {
        cJSON_Struct *devlist = Common_Json_GetAttrValueArr(lowerData, "DevList");
        if(devlist)
        {
            int size = Common_Json_ArraySize(devlist);
            for(i=0; i<size; i++)
            {
                dev = 0;
                str_tmp = NULL;
                Common_Json_GetAttrValue(devlist, i, "Dev", NULL, NULL, &dev, NULL);
                Common_Json_GetAttrValue(devlist, i, "Name", NULL, &str_tmp, NULL, NULL);
                LOGD("dev:[%d] Name:[%s]\n",dev, str_tmp);
                snprintf(strDevNameList[dev], sizeof(strDevNameList[dev]), "%s", str_tmp);
            }
        }

        cJSON_Struct *chlist = Common_Json_GetAttrValueArr(lowerData, "ChList");
        if(chlist)
        {
            int size = Common_Json_ArraySize(chlist);
            for(i=0; i<size; i++)
            {
                cJSON_Struct *item = Common_Json_SetAttrValueArrObj(cap, i);
                dev = 0;
                ch = 0;
                str_tmp = NULL;
                Common_Json_GetAttrValue(chlist, i, "Dev", NULL, NULL, &dev, NULL);
                Common_Json_SetAttrValueInt(item, "Dev", dev);
                Common_Json_GetAttrValue(chlist, i, "Ch", NULL, NULL, &ch, NULL);
                Common_Json_SetAttrValueInt(item, "Ch", ch);
                Common_Json_GetAttrValue(chlist, i, "Name", NULL, &str_tmp, NULL, NULL);
                if(slen(str_tmp) == 0)
                {
                    snprintf(strName, sizeof(strName), "%s_%d", strDevNameList[dev], ch);
                    str_tmp = strName;
                }

                Common_Json_SetAttrValueStr(item, "Name", str_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_get_linkcfg_v2(cJSON_Struct *header, int linktype, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = -1;
    int j = 0;
    int size = 0;
    int i_num = 0;
    int ret_expand_alarmout = 0;
    char url[32] = {0};
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if(linktype<0 || linktype >= sizeof(linkTypeV2)/sizeof(linkTypeV2[0]))
    {
        return WEB_CODE_InvalidArg;
    }

    snprintf(url,sizeof(url),"Alarm/%s",linkTypeV2[linktype]);

    Ovfs_Web_UpdateHeader(header, REST_GET, url);
    ret = Ovfs_Web_RestMethodA(header, indata, &lowerdata, 0);
    if (ret == 0)
    {
        cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerdata, "List");
        cJSON_Struct *list_set = outdata;
        if(list_get != NULL)
        {
            size = Common_Json_ArraySize(list_get);
            i = 0;
            list_set = Common_Json_SetAttrValueArr(outdata, "List");
        }
        else
        {
            list_get = lowerdata;
            LOGD("lowerdata has not list\n");
        }

        for(;i<size;i++)
        {
            LOGD("i[%d] size[%d]\n", i, size);
            cJSON_Struct *linkage_set = Common_Json_SetAttrValue(list_set, i, "LinkageCfg", Common_Json_Type_Object, NULL, 0, 0);
            cJSON_Struct *linkage_get = Common_Json_GetAttrValue(list_get, i, "LinkageCfg", NULL, NULL, NULL, NULL);
            if(Common_Json_GetAttrValueInt(linkage_get, "Email/Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(linkage_set, "Email", i_num);
            }

            if(Common_Json_GetAttrValueInt(linkage_get, "Ftp/Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(linkage_set, "Ftp", i_num);
            }

            if(Common_Json_GetAttrValueInt(linkage_get, "Snap/Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(linkage_set, "Snap", i_num);
            }

            if(Common_Json_GetAttrValueInt(linkage_get, "Snap/Count", &i_num))
            {
                Common_Json_SetAttrValueInt(linkage_set, "SnapCount", i_num);
            }

            if(Common_Json_GetAttrValueInt(linkage_get, "Snap/Interval", &i_num))
            {
                Common_Json_SetAttrValueInt(linkage_set, "SnapInterval", i_num);
            }

            if(temp = Common_Json_GetAttrValueArr(linkage_get, "AlarmOut/List"))
            {
                cJSON_Struct *alarmout = Common_Json_SetAttrValueArr(linkage_set, "AlarmOut");

                int alarmout_size = Common_Json_ArraySize(temp);
                for(j=0; j<alarmout_size; j++)
                {
                    Common_Json_GetAttrValue(temp, j, NULL, NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValueArrInt(alarmout, j, i_num);
                }
            }

            ret_expand_alarmout = web_get_expand_alarmout_list(header, linkage_set);

            if(ret_expand_alarmout == 0 && (temp = Common_Json_GetAttrValueArr(linkage_get, "ExpandAlarmOut/List")))
            {
                cJSON_Struct *alarmout = Common_Json_SetAttrValueArr(linkage_set, "ExpandAlarmOut");

                int alarmout_size = Common_Json_ArraySize(temp);
                for(j=0; j<alarmout_size; j++)
                {
                    cJSON_Struct *item = Common_Json_SetAttrValueArrObj(alarmout, j);
                    i_num = 0;
                    Common_Json_GetAttrValue(temp, j, "Dev", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValueInt(item, "Dev", i_num);
                    i_num = 0;
                    Common_Json_GetAttrValue(temp, j, "Ch", NULL, NULL, &i_num, NULL);
                    Common_Json_SetAttrValueInt(item, "Ch", i_num);
                }
            }

            web_get_alarmtime(linkage_get, linkage_set);

            cJSON_Struct *lightcfg_set = Common_Json_SetAttrValue(list_set, i, "LightCfg", Common_Json_Type_Object, NULL, 0, 0);
            cJSON_Struct *lightcfg_get = Common_Json_GetAttrValue(list_get, i, "LightCfg", NULL, NULL, NULL, NULL);
            if(Common_Json_GetAttrValueInt(lightcfg_get, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(lightcfg_set, "Enable", i_num);
            }

            if(Common_Json_GetAttrValueInt(lightcfg_get, "Duration", &i_num))
            {
                Common_Json_SetAttrValueInt(lightcfg_set, "Duration", i_num);
            }

            web_get_alarmtime(lightcfg_get, lightcfg_set);

            cJSON_Struct *audiocfg_set = Common_Json_SetAttrValue(list_set, i, "AudioCfg", Common_Json_Type_Object, NULL, 0, 0);
            cJSON_Struct *audiocfg_get = Common_Json_GetAttrValue(list_get, i, "AudioCfg", NULL, NULL, NULL, NULL);
            if(Common_Json_GetAttrValueInt(audiocfg_get, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(audiocfg_set, "Enable", i_num);
            }

            if(Common_Json_GetAttrValueInt(audiocfg_get, "PlayCount", &i_num))
            {
                Common_Json_SetAttrValueInt(audiocfg_set, "PlayCount", i_num);
            }

            if(Common_Json_GetAttrValueInt(audiocfg_get, "AudioSelected", &i_num))
            {
                Common_Json_SetAttrValueInt(audiocfg_set, "AudioSelected", i_num);
            }

            web_get_alarmtime(audiocfg_get, audiocfg_set);
        }
    }

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

int web_get_linkcfg(cJSON_Struct *header, int linktype, cJSON_Struct *outdata)
{
    return web_get_linkcfg_v2(header, linktype, NULL, outdata);
}

int web_set_linkcfg(cJSON_Struct *header, int linktype, cJSON_Struct *indata)
{
    int ret = 0;
    int i = 0;
    int size = 0;
    int i_num = 0;
    char url[32] = {0};
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if(linktype<1 || linktype >= sizeof(linkTypeV2)/sizeof(linkTypeV2[0]))
    {
        return WEB_CODE_InvalidArg;
    }

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    snprintf(url,sizeof(url),"Alarm/%s",linkTypeV2[linktype]);
    if(linktype == 10)
    {
        if(Common_Json_GetAttrValueInt(indata, "SensorId", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "SensorId", i_num-1);
        }
    }

    if (ret == 0)
    {
        cJSON_Struct *linkage = Common_Json_SetAttrValueObj(lowerdata, "LinkageCfg");
        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/Email", &i_num))
        {
            Common_Json_SetAttrValueObj(linkage, "Email");
            Common_Json_SetAttrValueInt(linkage, "Email/Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/Ftp", &i_num))
        {
            Common_Json_SetAttrValueObj(linkage, "Ftp");
            Common_Json_SetAttrValueInt(linkage, "Ftp/Enable", i_num);
        }

        Common_Json_SetAttrValueObj(linkage, "Snap");

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/Snap", &i_num))
        {
            Common_Json_SetAttrValueInt(linkage, "Snap/Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/SnapCount", &i_num))
        {
            Common_Json_SetAttrValueInt(linkage, "Snap/Count", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/SnapInterval", &i_num))
        {
            Common_Json_SetAttrValueInt(linkage, "Snap/Interval", i_num);
        }

        Common_Json_SetAttrValueObj(linkage, "Ptz");

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/Preset", &i_num))
        {
            Common_Json_SetAttrValueInt(linkage, "Ptz/Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "LinkageCfg/PresetNo", &i_num))
        {
            Common_Json_SetAttrValueInt(linkage, "Ptz/PresetNo", i_num);
        }

        if(temp = Common_Json_GetAttrValueArr(indata, "LinkageCfg/AlarmOut"))
        {
            Common_Json_SetAttrValueObj(linkage, "AlarmOut");
            cJSON_Struct *alarmout = Common_Json_SetAttrValueArr(linkage, "AlarmOut/List");

            size = Common_Json_ArraySize(temp);
            for(i=0; i<size; i++)
            {
                Common_Json_GetAttrValue(temp, i, NULL, NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueArrInt(alarmout, i, i_num);
            }
        }

        if(temp = Common_Json_GetAttrValueArr(indata, "LinkageCfg/ExpandAlarmOut"))
        {
            Common_Json_SetAttrValueObj(linkage, "ExpandAlarmOut");
            cJSON_Struct *alarmout = Common_Json_SetAttrValueArr(linkage, "ExpandAlarmOut/List");

            size = Common_Json_ArraySize(temp);
            for(i=0; i<size; i++)
            {
                cJSON_Struct *item = Common_Json_SetAttrValueArrObj(alarmout, i);
                i_num = 0;
                Common_Json_GetAttrValue(temp, i, "Dev", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueInt(item, "Dev", i_num);
                i_num = 0;
                Common_Json_GetAttrValue(temp, i, "Ch", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueInt(item, "Ch", i_num);
            }
        }

        temp = Common_Json_GetAttrValueObj(indata, "LinkageCfg");
        web_set_alarmtime(temp, linkage);

        cJSON_Struct *lightcfg = Common_Json_SetAttrValueObj(lowerdata, "LightCfg");
        temp = Common_Json_GetAttrValueObj(indata, "LightCfg");
        if(Common_Json_GetAttrValueInt(temp, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lightcfg, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(temp, "Duration", &i_num))
        {
            Common_Json_SetAttrValueInt(lightcfg, "Duration", i_num);
        }

        web_set_alarmtime(temp, lightcfg);

        cJSON_Struct *audiocfg = Common_Json_SetAttrValueObj(lowerdata, "AudioCfg");
        temp = Common_Json_GetAttrValueObj(indata, "AudioCfg");
        if(Common_Json_GetAttrValueInt(temp, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(audiocfg, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(temp, "PlayCount", &i_num))
        {
            Common_Json_SetAttrValueInt(audiocfg, "PlayCount", i_num);
        }

        if(Common_Json_GetAttrValueInt(temp, "AudioSelected", &i_num))
        {
            Common_Json_SetAttrValueInt(audiocfg, "AudioSelected", i_num);
        }

        web_set_alarmtime(temp, audiocfg);
    }

LOGW("url:[%s]\n",url);
ovfs_print_json(lowerdata);
    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    return ret;
}

static int web_semantic_get_motiondetect_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num =0;
    char *str_tmp = NULL;
    int iloop = 0;
    int nloop = 0;
    cJSON_Struct *pArray_tmp = NULL;
    cJSON_Struct *pArray_root = NULL;
    cJSON_Struct *lowerData = NULL;

    char buf[256]= {0};
    snprintf(buf,sizeof(buf),"/BoardSys/Event/Motion/Attribute/Device%d/Channel%d",opt->dev,opt->ch);
    Ovfs_Web_UpdateHeader(header, REST_GET, buf);
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        //Enable
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Sensitivity", &i_num))
        {
            i_num = i_num>3 ? 3 : (i_num>1 ? 2 : 1);
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num);
        }

        int blockW = 0;
        int blockH = 0;
        if (Common_Json_GetAttrValueInt(lowerData, "BlockW", &blockW) == NULL)
        {
            blockW = 22;
        }
        if (Common_Json_GetAttrValueInt(lowerData, "BlockH", &blockH) == NULL)
        {
            blockH = 18;
        }

        //MotionScope
        Common_Json_GetAttrValueStr(lowerData, "Rect", &str_tmp);
        if ((!str_tmp) || strlen(str_tmp) != (blockW*blockH))
        {
            LOGE("Rect Scope Not Match(%d %dx%d)!\n", strlen(str_tmp), blockW, blockH);
            ret = WEB_CODE_InternalMistake;
        }
        else
        {
            pArray_root = Common_Json_SetAttrValueArr(outdata, "MotionScope");
            for (iloop=0; iloop<18; iloop++)
            {
                pArray_tmp = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0);
                for (nloop=0; nloop<22; nloop++)
                {
                    int x = (nloop * blockW + 11) / 22;
                    int y = (iloop * blockH + 9) / 18;
                    Common_Json_SetAttrValueArrInt(pArray_tmp, nloop, *(str_tmp+y*blockW+x) != '0');
                }
                Common_Json_AddItem(pArray_root, iloop, NULL, pArray_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 1, outdata);

    return ret;
}

static int web_semantic_set_motiondetect_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i = 0, j = 0;
    int i_num =0;
    char *scope = NULL;
    char url[256]= {0};
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);
    }

    if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
    {
        if(i_num<1 || i_num>3)
        {
            if(lowerdata)
            {
                Common_Json_Delete(lowerdata);
                lowerdata = NULL;
            }
            return WEB_CODE_InvalidArg;
        }

        i_num = i_num*2 - 1;
        Common_Json_SetAttrValueInt(lowerdata, "Sensitivity", i_num);
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "MotionScope"))
    {
        scope = calloc(22*18, 1);
        for (i=0; i < 18; i++)
        {
            cJSON_Struct *pArray_tmp = Common_Json_GetAttrValue(temp, i, NULL, NULL, NULL, NULL, NULL);
            for (j=0; j < 22; j++)
            {
                if (Common_Json_GetAttrValue(pArray_tmp, j, NULL, NULL, NULL, &i_num, NULL))
                {
                    scope[i*22+j] = i_num;
                }
            }
        }

        char rect[1024] = {0};
        for (i=0; i < 32; i++)
        {
            for (j=0; j < 32; j++)
            {
                int x = (j * 22 + (32>>1)) / 32;
                int y = (i * 18 + (32>>1)) / 32;
                rect[i*32+j] = scope[y*22+x] ? '1' : '0';
            }
        }

        Common_Json_SetAttrValueStr(lowerdata, "Rect", rect);

        free(scope);
        scope = NULL;
    }

    snprintf(url,sizeof(url),"/BoardSys/Event/Motion/Attribute/Device%d/Channel%d",opt->dev,opt->ch);
    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 1, indata);
    }

    return ret;
}

int frmMotionDetect_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_motiondetect_v2(header, indata, outdata, opt);
            break;

        case 1:
            ret = web_semantic_set_motiondetect_v2(header, indata, outdata, opt);
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

static int web_semantic_get_videohide_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i_num =0;
    char *str_tmp = NULL;
    int iloop = 0;
    int nloop = 0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    char buf[256]= {0};
    snprintf(buf,sizeof(buf),"/BoardSys/Event/Hide/Attribute/Device%d/Channel%d",opt->dev,opt->ch);
    Ovfs_Web_UpdateHeader(header, REST_GET, buf);
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        //Enable
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");

        Common_Json_GetAttrValueInt(lowerData, "X", &i_num);
        Common_Json_SetAttrValue(pArray_area, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num/1024.0);

        Common_Json_GetAttrValueInt(lowerData, "Y", &i_num);
        Common_Json_SetAttrValue(pArray_area, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num/1024.0);

        Common_Json_GetAttrValueInt(lowerData, "W", &i_num);
        Common_Json_SetAttrValue(pArray_area, 2, NULL, Common_Json_Type_Double, NULL, 0, i_num/1024.0);

        Common_Json_GetAttrValueInt(lowerData, "H", &i_num);
        Common_Json_SetAttrValue(pArray_area, 3, NULL, Common_Json_Type_Double, NULL, 0, i_num/1024.0);

    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 2, outdata);

    return ret;
}

static int web_semantic_set_videohide_v2(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata,OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    int i = 0, j = 0;
    int i_num =0;
    double f_tmp = 0.0;
    char url[256]= {0};
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);
    }

    if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
    {
        if(i_num<1 || i_num>3)
        {
            if(lowerdata)
            {
                Common_Json_Delete(lowerdata);
                lowerdata = NULL;
            }
            return WEB_CODE_InvalidArg;
        }

        Common_Json_SetAttrValueInt(lowerdata, "Sensitivity", i_num-1);
    }

    if(temp = Common_Json_GetAttrValueArr(indata, "DetectArea"))
    {
        i_num = 0;
        f_tmp = 0.0;
        Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
        Common_Json_SetAttrValueInt(lowerdata, "X", (i_num?i_num:f_tmp)*1024);

        i_num = 0;
        f_tmp = 0.0;
        Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
        Common_Json_SetAttrValueInt(lowerdata, "Y", (i_num?i_num:f_tmp)*1024);

        i_num = 0;
        f_tmp = 0.0;
        Common_Json_GetAttrValue(temp, 2, NULL, NULL, NULL, &i_num, &f_tmp);
        Common_Json_SetAttrValueInt(lowerdata, "W", (i_num?i_num:f_tmp)*1024);

        i_num = 0;
        f_tmp = 0.0;
        Common_Json_GetAttrValue(temp, 3, NULL, NULL, NULL, &i_num, &f_tmp);
        Common_Json_SetAttrValueInt(lowerdata, "H", (i_num?i_num:f_tmp)*1024);
    }

    snprintf(url,sizeof(url),"/BoardSys/Event/Hide/Attribute/Device%d/Channel%d",opt->dev,opt->ch);
    Ovfs_Web_UpdateHeader(header, REST_PUT, url);
    ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);

    if(lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 2, indata);
    }

        return ret;
    }


int frmVideoHide_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_videohide_v2(header, indata, outdata, opt);
            break;

        case 1:
            ret = web_semantic_set_videohide_v2(header, indata, outdata, opt);
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

static int web_semantic_set_customaudio(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int i_detecttype = 0;
    int i_datatype = 0;
    int i_setcustom = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    int size = sizeof(linkTypeV2)/sizeof(linkTypeV2[0]);

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        Common_Json_GetAttrValueInt(indata, "SetAudioSelectedCustom", &i_setcustom);
#ifdef AWSIOT
        if(i_setcustom)
#endif
        {
            if(Common_Json_GetAttrValueInt(indata, "DetectType", &i_detecttype))
            {
                if(i_detecttype >= size || i_detecttype < 1)
                {
                    ret = WEB_CODE_InvalidArg;
                }
            }
        }
    }

    if(ret == 0)
    {
        if(
#ifndef AWSIOT
           Common_Json_GetAttrValueInt(indata, "DetectType", &i_detecttype) &&
#endif
           Common_Json_GetAttrValueInt(indata, "AudioDataType", &i_datatype) &&
           Common_Json_GetAttrValueStr(indata, "AudioData", &str_tmp))
        {
            int data_size = slen(str_tmp);
            if(
#ifndef AWSIOT
               i_detecttype >= size || i_detecttype < 1 ||
#endif
               data_size == 0 || data_size/4*3 > 100 * 1024 ||//audio file less than 100k
               i_datatype < 1 || i_datatype > 3)//1-G711u 2-wav
            {
                ret = WEB_CODE_InvalidArg;
            }
            else
            {
#ifndef AWSIOT
                Common_Json_SetAttrValueStr(lowerData, "AlarmName", linkTypeV2[i_detecttype]);
#endif
                int len = 0;
                char *data_decode = NULL;
                data_decode = Common_Base64_Decode(str_tmp, slen(str_tmp), &len);
                LOGD("AudioData len:%d [%s]\n",len,linkTypeV2[i_detecttype]);

                char *buf = NULL;

                if(i_datatype == 2)
                {
                    //pcm to g711u

                    data_decode += 44;//wav header
                    len -= 44;

                    buf=(char *)malloc(sizeof(char)*len+1);

                    pcm16_to_ulaw(len,data_decode,buf);

                }

                FILE *fd;
                fd = Common_File_fOpen("/tmp/audio_data", "wb");
                if(fd)
                {

                    if(i_datatype == 2)
                    {
                        Common_File_fWrite(buf, 1, len/2, fd);
                        data_decode -= 44;
                    }
                    else
                    {
                        Common_File_fWrite(data_decode, 1, len, fd);
                    }
                    Common_File_fClose(fd);
                }

                Common_Free(data_decode, __FUNCTION__, __LINE__);
                if(buf)
                {
                    free(buf);
                }
                Common_Json_SetAttrValueStr(lowerData, "Path", "/tmp/audio_data");
            }

            if(i_setcustom)
            {
                Common_Json_SetAttrValueObj(indata, "AudioCfg");
                Common_Json_SetAttrValueInt(indata, "AudioCfg/AudioSelected", 1);
                ret = web_set_linkcfg(header, i_detecttype, indata);
            }
        }
        else
        {
            ret = WEB_CODE_InvalidArg;
        }


    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Alarm/SetCustomAudio");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}


int frmSetCustomAudio(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            ret = web_semantic_set_customaudio(header, indata, outdata);
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

static int web_semantic_audiospeech(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_type = 0;
    int i_idx = 0;
    char path[128] = {0};
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
#ifndef AWSIOT
        if(Common_Json_GetAttrValueInt(indata, "DetectType", &i_type) == NULL)
        {
            ret = WEB_CODE_InvalidArg;
        }
#endif

        if(Common_Json_GetAttrValueInt(indata, "AudioSelected", &i_idx))
        {
            Common_Json_SetAttrValueInt(lowerData, "AudioSelected", i_idx);
        }

        snprintf(path, sizeof(path), "/update/soundFile/sound_%s_%d", linkTypeV2[i_type], i_idx);

#ifdef AWSIOT
        if(i_idx)
        {
            snprintf(path, sizeof(path), "/usr/etc/cfgfiles/custom_audio");
        }
        else
        {
            snprintf(path, sizeof(path), "/usr/etc/cfgfiles/default/custom_audio");
            if(!Common_File_IsExist(path))
            {
                snprintf(path, sizeof(path), "/update/soundFile/sound_alarm_default");
            }
        }
#endif
        Common_Json_SetAttrValueStr(lowerData, "Path", path);
        Common_Json_SetAttrValueInt(lowerData, "Times", 1);
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Audio/Adec/PlayFile");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int frmAudioSpeech(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 1:
            ret = web_semantic_audiospeech(header, indata, outdata);
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

static int web_semantic_get_rectonvideo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/Boardsys/Osd/Rect");
        Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "RectOnVideo", i_num);
        }

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }
    return ret;
}

static int web_trans_smart_result_code(int code)
{
    int ret = 0;
    if(code != 0)
    {
        if(code <=-5 && code >=-10)
        {
            ret = -655376 + code;
        }
        else
            ret = code;
    }

    return ret;
}

static int web_semantic_set_rectonvideo(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);
    if(Common_Json_GetAttrValueInt(indata, "RectOnVideo", &i_num))
    {
        Common_Json_SetAttrValueInt(lowerData, "Enable", i_num);

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Osd/Rect");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    if (lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

static int web_semantic_get_regionalinvasion(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num = 0;
    int i = 0;
    double f_tmp = 0.0;
    char *str_tmp = NULL;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/smartserver/attribute/RegionalInvasion");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportRegionNum", 1);
        Common_Json_SetAttrValueInt(outdata, "SupportPointNum", 6);

        web_semantic_get_rectonvideo(header, indata, outdata);

        if(Common_Json_GetAttrValueInt(lowerData, "dwDetectType", &i_num))
        {
            cJSON_Struct *targettype = Common_Json_SetAttrValueArr(outdata, "TargetType");
            Common_Json_SetAttrValueArrInt(targettype, 0, i_num & 0x1);
            Common_Json_SetAttrValueArrInt(targettype, 1, i_num >> 1 & 0x1);
        }


        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");
        cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(tmp, "AreaId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(tmp, "Point");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "RegionList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    i_num = -1;
    int personzoom = -1;

    if (0 == ret)
    {
        if(Common_Json_GetAttrValueObj(header, "RemoteServerInfo"))
        {
            Common_Json_RemoveItem(header, -1, "RemoteServerInfo");
        }

        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/conf");
        Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

        Common_Json_GetAttrValueInt(lowerData, "PersonTrack", &i_num);
        Common_Json_GetAttrValueInt(lowerData, "PersonZoom", &personzoom);

        if (lowerData)
        {
            Common_Json_Delete(lowerData);
            lowerData = NULL;
        }
    }

    Common_Json_SetAttrValueInt(outdata, "PersonTrack", i_num);
    Common_Json_SetAttrValueInt(outdata, "TrackZoom", personzoom);

    //get link
    ret = web_get_linkcfg(header, 3, outdata);

    return ret;
}

static int web_semantic_set_regionalinvasion(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    int force_enable = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        cJSON_Struct *targettype = Common_Json_GetAttrValueArr(indata, "TargetType");
        if(targettype)
        {
            int type = 0;
            if(Common_Json_GetAttrValue(targettype, 0, NULL,NULL,NULL, &i_num,NULL))
            {
                type = i_num;
            }
            if(Common_Json_GetAttrValue(targettype, 1, NULL,NULL,NULL, &i_num,NULL))
            {
                if(i_num)
                {
                    type |= 0x2;
                }
            }

            Common_Json_SetAttrValueInt(lowerdata, "dwDetectType", type);
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectArea"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectArea/AreaId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectArea/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "RegionList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            LOGD("size:[%d]\n",size);
            for(i=0; i<size; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                LOGD("[%d][%d][%f]\n",i,i_num,f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                LOGD("[%d][%d][%f]\n",i,i_num,f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", i_num?i_num*1.0:f_tmp);
            }
        }

    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/smartserver/attribute/RegionalInvasion");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 3, indata);
    }

    if (0 == ret)
    {
        lowerdata = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0);

        i_num = -1;
        if(Common_Json_GetAttrValueInt(indata, "PersonTrack", &i_num) && i_num != -1)
        {
            Common_Json_SetAttrValueInt(lowerdata, "PersonTrack", i_num);
        }

        i_num = -1;
        if(Common_Json_GetAttrValueInt(indata, "TrackZoom", &i_num) && i_num != -1)
        {
            Common_Json_SetAttrValueInt(lowerdata, "PersonZoom", i_num);
        }

        if(Common_Json_GetAttrValueObj(header, "RemoteServerInfo"))
        {
            Common_Json_RemoveItem(header, -1, "RemoteServerInfo");
        }

        if(Common_Json_Size(lowerdata)>0)
        {
            Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/conf");
            ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
        }

        if (lowerdata)
        {
            Common_Json_Delete(lowerdata);
            lowerdata = NULL;
        }
    }

    return web_trans_smart_result_code(ret);
}

int frmRegionalInvasion(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_regionalinvasion(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_regionalinvasion(header, indata, outdata);
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

static int web_semantic_get_traversedetect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int i = 0;
    char *str_tmp = NULL;
    double f_tmp = 0.0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Attribute/DetectWire");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportLineNum", 1);

        web_semantic_get_rectonvideo(header, indata, outdata);

        if(Common_Json_GetAttrValueInt(lowerData, "dwDetectType", &i_num))
        {
            cJSON_Struct *targettype = Common_Json_SetAttrValueArr(outdata, "TargetType");
            Common_Json_SetAttrValueArrInt(targettype, 0, i_num & 0x1);
            Common_Json_SetAttrValueArrInt(targettype, 1, i_num >> 1 & 0x1);
        }


        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectLine");
        cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(tmp, "LineId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(tmp, "Point");
        cJSON_Struct *pointB = Common_Json_SetAttrValueArr(tmp, "PointB");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "WireList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            }

            tmp = Common_Json_GetAttrValue(list, 0, "End_Point", NULL, NULL, NULL, NULL);
            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "X", NULL, NULL, &i_num, &f_tmp);
            Common_Json_SetAttrValue(pointB, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "Y", NULL, NULL, &i_num, &f_tmp);
            Common_Json_SetAttrValue(pointB, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);

        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 4, outdata);

    return ret;
}

static int web_semantic_set_traversedetect(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            int force_enable = 0;
            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        cJSON_Struct *targettype = Common_Json_GetAttrValueArr(indata, "TargetType");
        if(targettype)
        {
            int type = 0;
            if(Common_Json_GetAttrValue(targettype, 0, NULL,NULL,NULL, &i_num,NULL))
            {
                type = i_num;
            }
            if(Common_Json_GetAttrValue(targettype, 1, NULL,NULL,NULL, &i_num,NULL))
            {
                if(i_num)
                {
                    type |= 0x2;
                }
            }

            Common_Json_SetAttrValueInt(lowerdata, "dwDetectType", type);
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectLine"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectLine/LineId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectLine/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "WireList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            double pointA[2][2] = {0};
            double f_tmp2 = 0.0;
            for(i=0; i<size && i<2; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                f_tmp2 = i_num?i_num*1.0:f_tmp;
                pointA[i][0] = f_tmp2;
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", f_tmp2);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                f_tmp2 = i_num?i_num*1.0:f_tmp;
                pointA[i][1] = f_tmp2;
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", f_tmp2);
            }

            cJSON_Struct *startpoint = Common_Json_SetAttrValueObj(list_tmp, "Start_Point");
            f_tmp2 = (pointA[1][0] + pointA[0][0])/2;
            Common_Json_SetAttrValueFlt(startpoint, "X", f_tmp2);
            f_tmp2 = (pointA[1][1] + pointA[0][1])/2;
            Common_Json_SetAttrValueFlt(startpoint, "Y", f_tmp2);

            cJSON_Struct *pointB_get = Common_Json_GetAttrValue(indata, 0, "DetectLine/PointB",NULL,NULL, NULL,NULL);
            cJSON_Struct *endpoint = Common_Json_SetAttrValueObj(list_tmp, "End_Point");
            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(pointB_get, 0, NULL, NULL, NULL, &i_num, &f_tmp);
            Common_Json_SetAttrValueFlt(endpoint, "X", i_num?i_num*1.0:f_tmp);
            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(pointB_get, 1, NULL, NULL, NULL, &i_num, &f_tmp);
            Common_Json_SetAttrValueFlt(endpoint, "Y", i_num?i_num*1.0:f_tmp);
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/Attribute/DetectWire");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 4, indata);
    }

    return web_trans_smart_result_code(ret);
}

int frmTraverseDetect(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_traversedetect(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_traversedetect(header, indata, outdata);
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

static int web_semantic_get_personstaying(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int i = 0;
    char *str_tmp = NULL;
    double f_tmp = 0.0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Attribute/PersonStaying");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportRegionNum", 1);
        Common_Json_SetAttrValueInt(outdata, "SupportPointNum", 6);

        web_semantic_get_rectonvideo(header, indata, outdata);

        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "dwLingerTime", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "StayTime", i_num);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");
        cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(tmp, "AreaId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(tmp, "Point");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "RegionList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 5, outdata);

    return ret;
}

static int web_semantic_set_personstaying(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            int force_enable = 0;
            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueInt(indata, "StayTime", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwLingerTime", i_num);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectArea"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectArea/AreaId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectArea/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "RegionList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            for(i=0; i<size; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/Attribute/PersonStaying");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 5, indata);
    }
    return web_trans_smart_result_code(ret);
}

int frmPersonStaying(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_personstaying(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_personstaying(header, indata, outdata);
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

static int web_semantic_get_personabsent(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int i = 0;
    char *str_tmp = NULL;
    double f_tmp = 0.0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Attribute/DetectAbsent");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportRegionNum", 1);
        Common_Json_SetAttrValueInt(outdata, "SupportPointNum", 6);

        web_semantic_get_rectonvideo(header, indata, outdata);

		if(Common_Json_GetAttrValueInt(lowerData, "dwAbsentTime", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "AbsentTime", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "dwSleepingDetect", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "SleepingDetect", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "dwMinPresentNumber", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "MinPresentNumber", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");
        cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(tmp, "AreaId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(tmp, "Point");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "RegionList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 6, outdata);

    return ret;
}

static int web_semantic_set_personabsent(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            int force_enable = 0;
            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueInt(indata, "AbsentTime", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwAbsentTime", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "SleepingDetect", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSleepingDetect", i_num);
        }

        if(Common_Json_GetAttrValueInt(indata, "MinPresentNumber", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwMinPresentNumber", i_num);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectArea"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectArea/AreaId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectArea/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "RegionList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            for(i=0; i<size; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/Attribute/DetectAbsent");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 6, indata);
    }
    return web_trans_smart_result_code(ret);
}

int frmPersonAbsent(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_personabsent(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_personabsent(header, indata, outdata);
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

static int web_semantic_get_parkingviolation(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int i = 0;
    char *str_tmp = NULL;
    double f_tmp = 0.0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Attribute/ParkingViolation");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportRegionNum", 1);
        Common_Json_SetAttrValueInt(outdata, "SupportPointNum", 6);

        web_semantic_get_rectonvideo(header, indata, outdata);

		if(Common_Json_GetAttrValueInt(lowerData, "dwLingerTime", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "ParkingTime", i_num);
        }

        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");
        cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(tmp, "AreaId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(tmp, "Point");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "RegionList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 7, outdata);

    return ret;
}

static int web_semantic_set_parkingviolation(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            int force_enable = 0;
            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueInt(indata, "ParkingTime", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwLingerTime", i_num);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectArea"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectArea/AreaId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectArea/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "RegionList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            for(i=0; i<size; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", i_num?i_num*1.0:f_tmp);
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", i_num?i_num*1.0:f_tmp);
            }
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/Attribute/ParkingViolation");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 7, indata);
    }
    return web_trans_smart_result_code(ret);
}

int frmParkingViolation(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_parkingviolation(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_parkingviolation(header, indata, outdata);
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

typedef struct
{
    double x;
    double y;
}OVFS_WEB_POINT;

//功能：求点在有向直线左边还是右边
//返回：0共线、1左边、-1右边
int   left_right(OVFS_WEB_POINT   a,OVFS_WEB_POINT   b,double   x,double   y)
{
      double   t;
      a.x   -=   x;   b.x   -=   x;
      a.y   -=   y;   b.y   -=   y;
      t   =   a.x*b.y-a.y*b.x;
      return   t==0   ?   0   :   t>0?1:-1;
}

//功能：线段c,d和直线a,b是否相交
bool   intersect1(OVFS_WEB_POINT   a,OVFS_WEB_POINT   b,OVFS_WEB_POINT   c,OVFS_WEB_POINT   d)
{
      return   (left_right(a,b,c.x,c.y)^left_right(a,b,d.x,d.y))!=0;//==-2;
}

//功能：判断线段c,d和线段a,b是否相交
bool   intersect(OVFS_WEB_POINT   a,OVFS_WEB_POINT   b,OVFS_WEB_POINT   c,OVFS_WEB_POINT   d)
{
      return   intersect1(a,b,c,d)   &&   intersect1(c,d,a,b);
}

static int web_semantic_get_vehicleretrograde(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i_num =0;
    int i = 0;
    char *str_tmp = NULL;
    double f_tmp = 0.0;
    cJSON_Struct *pArray_area = NULL;
    cJSON_Struct *lowerData = NULL;

    Ovfs_Web_UpdateHeader(header, REST_GET, "/SmartServer/Attribute/Retrograde");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Enable", i_num);
        }

        Common_Json_SetAttrValueInt(outdata, "SupportRegionNum", 1);
        Common_Json_SetAttrValueInt(outdata, "SupportPointNum", 4);

        web_semantic_get_rectonvideo(header, indata, outdata);

        if(Common_Json_GetAttrValueInt(lowerData, "dwSensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(outdata, "Sensitivity", i_num+1);
        }

        pArray_area = Common_Json_SetAttrValueArr(outdata, "DetectArea");
        cJSON_Struct *area_tmp = Common_Json_SetAttrValueArrObj(pArray_area, 0);
        Common_Json_SetAttrValueInt(area_tmp, "AreaId", 1);
        cJSON_Struct *point = Common_Json_SetAttrValueArr(area_tmp, "Point");

        cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "RegionList");
        int size = Common_Json_ArraySize(list);
        if(size > 0)
        {
            cJSON_Struct *tmp = Common_Json_GetAttrValueArrItem(list, 0);

            OVFS_WEB_POINT pointArr[4] = {0};
            OVFS_WEB_POINT pointA = {0};
            OVFS_WEB_POINT pointB = {0};

            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "Start_Point/X", NULL, NULL, &i_num, &f_tmp);
            pointA.x = i_num?i_num*1.0:f_tmp;

            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "Start_Point/Y", NULL, NULL, &i_num, &f_tmp);
            pointA.y = i_num?i_num*1.0:f_tmp;

            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "End_Point/X", NULL, NULL, &i_num, &f_tmp);
            pointB.x = i_num?i_num*1.0:f_tmp;

            i_num = 0;
            f_tmp = 0.0;
            Common_Json_GetAttrValue(tmp, -1, "End_Point/Y", NULL, NULL, &i_num, &f_tmp);
            pointB.y = i_num?i_num*1.0:f_tmp;

            LOGD("pointA:[%f][%f] pointB:[%f][%f]\n",pointA.x,pointA.y,pointB.x,pointB.y);

            tmp = Common_Json_GetAttrValue(list, 0, "Point", NULL, NULL, NULL, NULL);
            size = 0;
            size = Common_Json_ArraySize(tmp);

            int line = -1;
            LOGD("size:[%d]\n",size);
            for(i=0; i<size; i++)
            {
                i_num = 0;
                f_tmp = 0.0;
                cJSON_Struct *point_loop = Common_Json_SetAttrValue(point, i, NULL, Common_Json_Type_Array, NULL, 0, 0);
                Common_Json_GetAttrValue(tmp, i, "X", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 0, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                pointArr[i].x = i_num?i_num*1.0:f_tmp;
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(tmp, i, "Y", NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValue(point_loop, 1, NULL, Common_Json_Type_Double, NULL, 0, i_num?i_num*1.0:f_tmp);
                pointArr[i].y = i_num?i_num*1.0:f_tmp;

                if(line==-1 && i>0)
                {
                    bool result = intersect(pointA,pointB,pointArr[i],pointArr[i-1]);
                    LOGD("result:[%d]\n",result);
                    if(result)
                    {
                        line = i;
                    }
                }
            }

            if(line==-1)line = size;

            Common_Json_SetAttrValueInt(area_tmp, "DetectLine", line);
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    //get link
    ret = web_get_linkcfg(header, 8, outdata);

    return ret;
}

static int getCenterPoint(OVFS_WEB_POINT *pointArr, int size,OVFS_WEB_POINT *centerPoint)
{
    int i = 0,len = size;
    double temp = 0,area = 0,cx = 0,cy = 0;
    if (len < 3) return 0;
    for (i = 0; i < len - 1; i++) {
        temp = pointArr[i].x * pointArr[i + 1].y - pointArr[i].y * pointArr[i + 1].x;
        area += temp;
        cx += temp * (pointArr[i].x + pointArr[i + 1].x);
        cy += temp * (pointArr[i].y + pointArr[i + 1].y);
    }
    temp = pointArr[len - 1].x * pointArr[0].y - pointArr[len - 1].y * pointArr[0].x;
    area += temp;
    cx += temp * (pointArr[len - 1].x + pointArr[0].x);
    cy += temp * (pointArr[len - 1].y + pointArr[0].y);
    area = area / 2;
    cx = (cx / (6 * area));
    cy = (cy / (6 * area));

    if(cx<0)cx=0;
    if(cx>1)cx=1;
    if(cy<0)cy=0;
    if(cy>1)cy=1;

    centerPoint->x = cx;
    centerPoint->y = cy;
    return 0;
}

static int web_semantic_set_vehicleretrograde(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    int line = 0;
    double f_tmp = 0.0;
    cJSON_Struct *lowerdata = NULL;
    cJSON_Struct *temp = NULL;

    if ((lowerdata = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    if (ret == 0)
    {
        if(Common_Json_GetAttrValueInt(indata, "Enable", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "Enable", i_num);

            int force_enable = 0;
            if(i_num == 1 && Common_Json_GetAttrValueInt(indata, "ForceEnable", &force_enable))
            {
                if(force_enable == 1)
                {
                    web_semantic_set_disableallsmart(header, indata, outdata);
                }
            }
        }

        if(Common_Json_GetAttrValueInt(indata, "Sensitivity", &i_num))
        {
            Common_Json_SetAttrValueInt(lowerdata, "dwSensitivity", i_num-1);
        }

        if(Common_Json_GetAttrValueArr(indata, "DetectArea"))
        {
            if(Common_Json_GetAttrValue(indata, 0, "DetectArea/AreaId",NULL,NULL, &i_num,NULL) == NULL ||
                i_num != 1)
            {
                ret = WEB_CODE_InvalidArg;
            }

            Common_Json_GetAttrValue(indata, 0, "DetectArea/DetectLine",NULL,NULL, &line,NULL);
            OVFS_WEB_POINT pointArr[4] = {0};
            OVFS_WEB_POINT pointA = {0};
            OVFS_WEB_POINT pointB = {0};

            cJSON_Struct *point_get = Common_Json_GetAttrValue(indata, 0, "DetectArea/Point",NULL,NULL, NULL,NULL);
            cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerdata, "RegionList");
            cJSON_Struct *list_tmp = Common_Json_SetAttrValueArrObj(list, 0);
            cJSON_Struct *point_set = Common_Json_SetAttrValueArr(list_tmp, "Point");
            int size = Common_Json_ArraySize(point_get);
            LOGD("size:[%d] line:[%d]\n",size,line);
            for(i=0; i<size; i++)
            {
                temp = Common_Json_GetAttrValue(point_get, i, NULL,NULL,NULL, NULL,NULL);
                cJSON_Struct *point_set_tmp = Common_Json_SetAttrValueArrObj(point_set, i);
                Common_Json_GetAttrValue(temp, 0, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "X", i_num?i_num*1.0:f_tmp);
                pointArr[i].x = i_num?i_num*1.0:f_tmp;
                i_num = 0;
                f_tmp = 0.0;
                Common_Json_GetAttrValue(temp, 1, NULL, NULL, NULL, &i_num, &f_tmp);
                Common_Json_SetAttrValueFlt(point_set_tmp, "Y", i_num?i_num*1.0:f_tmp);
                pointArr[i].y = i_num?i_num*1.0:f_tmp;
            }

            getCenterPoint(&pointArr,size, &pointA);

            OVFS_WEB_POINT pointMid = {0};
            pointMid.x = (pointArr[line-1].x + pointArr[line==size?0:line].x)/2;
            pointMid.y = (pointArr[line-1].y + pointArr[line==size?0:line].y)/2;

            pointB.x = pointMid.x - (pointA.x - pointMid.x)/2;
            pointB.y = pointMid.y - (pointA.y - pointMid.y)/2;

            if(pointB.x<0)pointB.x=0;
            if(pointB.x>1)pointB.x=1;
            if(pointB.y<0)pointB.y=0;
            if(pointB.y>1)pointB.y=1;

            LOGD("pointA:[%f][%f] pointB:[%f][%f]\n",pointA.x,pointA.y,pointB.x,pointB.y);

            Common_Json_SetAttrValueObj(list_tmp, "Start_Point");
            Common_Json_SetAttrValueFlt(list_tmp, "Start_Point/X", pointA.x);
            Common_Json_SetAttrValueFlt(list_tmp, "Start_Point/Y", pointA.y);

            Common_Json_SetAttrValueObj(list_tmp, "End_Point");
            Common_Json_SetAttrValueFlt(list_tmp, "End_Point/X", pointB.x);
            Common_Json_SetAttrValueFlt(list_tmp, "End_Point/Y", pointB.y);
        }
    }

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/Attribute/Retrograde");
        ret = Ovfs_Web_RestMethodA(header, lowerdata, NULL, 0);
    }

    if (lowerdata)
    {
        Common_Json_Delete(lowerdata);
        lowerdata = NULL;
    }

    if(ret == 0)
    {
        ret = web_semantic_set_rectonvideo(header, indata, outdata);
    }

    if(ret == 0)
    {
        ret = web_set_linkcfg(header, 8, indata);
    }
    return web_trans_smart_result_code(ret);
}

int frmVehicleRetrograde(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_vehicleretrograde(header, indata, outdata);
            break;

        case 1:
            ret = web_semantic_set_vehicleretrograde(header, indata, outdata);
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

static int web_semantic_set_disableallsmart(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/SmartServer/StopAll");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

int frmDisableAllSmart(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
            case 1:
                ret = web_semantic_set_disableallsmart(header, indata, outdata);
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

static int web_semantic_get_exposurelight(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int ret = 0;
    int i_type = 0;
    int i_num = 0;
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
                case 28:
                    if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, "Mode", i_num);
                    }

                    break;
            }
        }

        cJSON_Struct *exposureLihtCap = Common_Json_SetAttrValueArr(outdata, "ModeCapability");
        Common_Json_SetAttrValueArrInt(exposureLihtCap, 0, 0);
        Common_Json_SetAttrValueArrInt(exposureLihtCap, 1, 1);

        OVFS_IMAGE_CAP_T imageCap = {0};
        ret = get_image_capability(header, opt, &imageCap);
        if(ret == 0)
        {
            if(imageCap.exposureLight == 2)
            {
                Common_Json_SetAttrValueArrInt(exposureLihtCap, 2, 2);
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

static int web_semantic_set_exposurelight(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int count = 0;
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmp = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerData, "ImageList");
        if(Common_Json_GetAttrValueInt(indata, "Mode", &i_num))
        {
            tmp = Common_Json_SetAttrValueArrObj(list, count++);
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Type", 28);
            Common_Json_SetAttrValueObj(tmp, "Param");
            Common_Json_SetAttrValueInt(tmp, "Param/Mode", i_num);
        }

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


int frmExposureLight(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_exposurelight(header, indata, outdata, opt);
            break;

        case 1:
            ret = web_semantic_set_exposurelight(header, indata, outdata);
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

static int web_semantic_get_alarminpara_v2(cJSON_Struct *header, int ch, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int iRet = 0;
    char *str_tmp = NULL;
    int i_num = 0;

    cJSON_Struct * lowerData = NULL;

    if (0 == iRet)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/BoardSys/Event/AlarmIn/Attribute/All");
        iRet = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == iRet)
        {
            cJSON_Struct *list_get = Common_Json_GetAttrValueArr(lowerData, "EventList");
            cJSON_Struct *list_set = Common_Json_SetAttrValueArr(outdata, "AlarmInList");
            int size = Common_Json_ArraySize(list_get);
            for(int i=0; i<size; i++)
            {
                cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(list_set, i);
                Common_Json_GetAttrValue(list_get, i, "Channel", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueInt(tmp, "No", i_num+1);

                Common_Json_GetAttrValue(list_get, i, "Enable", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueInt(tmp, "Enable", i_num);

                Common_Json_GetAttrValue(list_get, i, "Name", NULL, &str_tmp, NULL, NULL);
                Common_Json_SetAttrValueStr(tmp, "Name", str_tmp);

                Common_Json_GetAttrValue(list_get, i, "TriggerMode", NULL, NULL, &i_num, NULL);
                Common_Json_SetAttrValueInt(tmp, "TriggerMode", i_num);
            }

        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == iRet)
    {
        iRet = web_get_linkcfg(header, 9, outdata);
    }

    return iRet;
}

static int web_semantic_set_alarminpara_v2(cJSON_Struct *header, int ch, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int iRet = 0;
    int AlarmCh = 0;
    char *str_tmp = NULL;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;

    if ((lowerData = Common_Json_New(NULL,Common_Json_Type_Object,NULL,0,0)) == NULL)
    {
        iRet = WEB_CODE_LackingMem;
    }

    if (0 == iRet)
    {
        cJSON_Struct *list_get = Common_Json_GetAttrValueArr(indata, "AlarmInList");
        cJSON_Struct *list_set = Common_Json_SetAttrValueArr(lowerData, "EventList");
        int size = Common_Json_ArraySize(list_get);
        for(int i=0; i<size; i++)
        {
            cJSON_Struct *tmp = Common_Json_SetAttrValueArrObj(list_set, i);
            if(Common_Json_GetAttrValue(list_get, i, "No", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "Channel", i_num-1);
            }
            else
            {
                continue;
            }

            if(Common_Json_GetAttrValue(list_get, i, "Enable", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "Enable", i_num);
            }

            if(Common_Json_GetAttrValue(list_get, i, "Name", NULL, &str_tmp, NULL, NULL))
            {
                Common_Json_SetAttrValueStr(tmp, "Name", str_tmp);
            }

            if(Common_Json_GetAttrValue(list_get, i, "TriggerMode", NULL, NULL, &i_num, NULL))
            {
                Common_Json_SetAttrValueInt(tmp, "TriggerMode", i_num);
            }
        }

        Ovfs_Web_UpdateHeader(header, REST_PUT, "/BoardSys/Event/AlarmIn/Attribute/All");
        iRet = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);

        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == iRet)
    {
        iRet = web_set_linkcfg(header, 9, indata);
    }

    return iRet;
}

int frmAlarmInPara_V2(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_alarminpara_v2(header, opt->ch, indata, outdata);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_alarminpara_v2(header, opt->ch, indata, outdata);
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

static int web_semantic_get_ispconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int ret = 0;
    int i_type = 0;
    int i_num = 0;
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
                case 25:
                    if (Common_Json_GetAttrValueInt(tmp, "Param/Mode", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, "Scene", i_num+1);
                    }

                    break;
            }
        }

        cJSON_Struct *ispSceneCap = Common_Json_SetAttrValueArr(outdata, "SceneCapability");

        OVFS_IMAGE_CAP_T imageCap = {0};
        ret = get_image_capability(header, opt, &imageCap);
        if(ret == 0)
        {
            if(imageCap.ispScene)
            {
                Common_Json_SetAttrValueArrInt(ispSceneCap, 0, 1);
                Common_Json_SetAttrValueArrInt(ispSceneCap, 1, 2);
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

static int web_semantic_set_ispconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{

    int count = 0;
    int ret = 0;
    int i_num = 0;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *tmp = NULL;

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
    {
        ret = WEB_CODE_LackingMem;
    }

    if(ret == 0)
    {
        cJSON_Struct *list = Common_Json_SetAttrValueArr(lowerData, "ImageList");
        if(Common_Json_GetAttrValueInt(indata, "Scene", &i_num))
        {
            tmp = Common_Json_SetAttrValueArrObj(list, count++);
            Common_Json_SetAttrValueInt(tmp, "Device", 0);
            Common_Json_SetAttrValueInt(tmp, "Type", 25);
            Common_Json_SetAttrValueObj(tmp, "Param");
            Common_Json_SetAttrValueInt(tmp, "Param/Mode", i_num-1);
        }

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

int frmIspConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            //获取参数
            ret = web_semantic_get_ispconfig(header, indata, outdata,opt);
            break;

        case 1:
            //设置参数
            ret = web_semantic_set_ispconfig(header, indata, outdata);
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

int web_semantic_get_extendosd(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int id = -1;
    int i_num = 0;
    double f_tmp = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    get_channel_osd_cap(header, 1, NULL, outdata);

    Common_Json_GetAttrValueInt(indata, "SensorId", &id);

    Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/uartosd");
    ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);

    if(ret == 0)
    {
        cJSON_Struct *list_get = lowerData;
        cJSON_Struct *list_set = NULL;
        if(id == -1)
        {
            list_set = Common_Json_SetAttrValueArr(outdata, "List");
        }
        int size = Common_Json_ArraySize(list_get);
        if(size > 0)
        {
            for(i=0; i<size; i++)
            {
                cJSON_Struct *tmp = outdata;
                if(id == -1)
                {
                    tmp = Common_Json_SetAttrValueArrObj(list_set, i);
                }

                if(Common_Json_GetAttrValue(list_get, i, "UartOsdId", NULL, NULL, &i_num, NULL))
                {
                    if(id != -1)
                    {
                        if(i_num != id)
                        {
                            LOGD("continue\n");
                            continue;
                        }
                        else
                        {
                            size = i;
                        }
                    }

                    Common_Json_SetAttrValueInt(tmp, "SensorId", i_num+1);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Enable", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Enable", i_num);
                }
                if(Common_Json_GetAttrValue(list_get, i, "X", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "OSDX", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Y", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "OSDY", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "IsUpperThreshold", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Symbol", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "ThresholdValue", NULL, NULL, &i_num, &f_tmp))
                {
                    f_tmp = i_num?i_num*1.0:f_tmp;
                    Common_Json_SetAttrValueFlt(tmp, "Threshold", f_tmp);
                }

                if(id == -1 && i > 0)continue;
                tmp = outdata;

                if(Common_Json_GetAttrValue(list_get, i, "MainSize", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "MainStreamFontSize", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "SubSize", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "SubStreamFontSize", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "TextColor", NULL, NULL, &i_num, NULL))
                {

                    i_num = TRCOLOR_RGB555RGB888(i_num);
                    char tmpstr[16];
                    snprintf(tmpstr, sizeof(tmpstr), "#%06x", i_num);
                    Common_Json_SetAttrValueStr(tmp, "TextColor", tmpstr);
                }
            }

            if(id != -1 && i == size)
            {
                ret = WEB_CODE_InvalidArg;
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_set_extendosd(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int i_num = 0;
    int mainSize = 0;
    int subSize = 0;
    int textColor = -1;
    double f_tmp = 0.0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if (0 == ret)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0)) == NULL)
        {
            ret = WEB_CODE_LackingMem;
        }
    }

    if (0 == ret)
    {
        Common_Json_GetAttrValueInt(indata, "MainStreamFontSize", &mainSize);
        Common_Json_GetAttrValueInt(indata, "SubStreamFontSize", &subSize);

        if(Common_Json_GetAttrValueStr(indata, "TextColor", &str_tmp))
        {
            i_num = (int)strtoul(str_tmp+1, NULL, 16);
            textColor = TRCOLOR_RGB888RGB555(i_num);
        }

        int size = 1;
        cJSON_Struct *tmp;
        cJSON_Struct *list_get = Common_Json_GetAttrValueArr(indata, "List");
        if(list_get)
        {
            size = Common_Json_ArraySize(list_get);
        }
        LOGD("size:[%d]\n",size);

        for(i=0; i<size; i++)
        {
            tmp = Common_Json_SetAttrValueArrObj(lowerData, i);

            cJSON_Struct *tmp_item = indata;
            if(list_get)
            {
                tmp_item = Common_Json_GetAttrValueArrItem(list_get, i);
            }

            if (Common_Json_GetAttrValueInt(tmp_item, "SensorId", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "UartOsdId", i_num-1);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "Enable", i_num);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "OSDX", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "X", i_num);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "OSDY", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "Y", i_num);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "Symbol", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "IsUpperThreshold", i_num);
            }

            if(Common_Json_GetAttrValue(tmp_item, -1, "Threshold", NULL, NULL, &i_num, &f_tmp))
            {
                f_tmp = i_num?i_num*1.0:f_tmp;
                Common_Json_SetAttrValueFlt(tmp, "ThresholdValue", f_tmp);
            }

            if(mainSize)
            {
                Common_Json_SetAttrValueInt(tmp, "MainSize", mainSize);
            }

            if(subSize)
            {
                Common_Json_SetAttrValueInt(tmp, "SubSize", subSize);
            }

            if (textColor >= 0 && textColor <= 0x7fff)
            {
                Common_Json_SetAttrValueInt(tmp, "TextColor", textColor);
            }

            if(Common_Json_ArraySize(tmp) == 0 && size == 1)
            {
                Common_Json_RemoveItem(lowerData, 0, NULL);
            }
        }
    }

    if (0 == ret)
    {
        char *str = Common_Json_Print(lowerData, NULL);
        LOGD("lowerData:%s\n",str);
        free(str);
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/uartosd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmExtendOSD(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_extendosd(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_extendosd(header, indata, outdata);
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

static int web_semantic_get_lightconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int ret = 0;
    cJSON_Struct *lowerData = NULL;

    Common_Json_SetAttrValueInt(outdata, "SupportLight", 1);
    Common_Json_SetAttrValueInt(outdata, "LightVersion", 2);
    ret = web_semantic_get_lightcfg(header, indata, outdata, opt);
    if (0 == ret)
    {
        lowerData = Common_Json_DetachItem(outdata, -1, "ModeSupportList");
        Common_Json_AddItem(outdata, -1, "ModeCapability", lowerData);
    }

    return ret;
}

static int web_semantic_set_lightconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    ret = web_semantic_set_lightcfg(header, indata, outdata);

    return ret;
}

int frmLightConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_lightconfig(header, indata, outdata, opt);
            break;
        case 1:
            ret = web_semantic_set_lightconfig(header, indata, outdata);
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

static int web_semantic_get_allosdconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0;
    int ret = 0;
    int i_num = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *pObjRoot = NULL;
    char buf[256] = {0};

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/ChannelName/Attribute/All");
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            pObjRoot = Common_Json_SetAttrValueObj(outdata, "ChannelOSD");
            cJSON_Struct *list = Common_Json_GetAttrValueArr(lowerData, "NameOsdList");
            int size = Common_Json_ArraySize(list);
            for(i=0; i<size; i++)
            {
                cJSON_Struct *pObjItem = Common_Json_GetAttrValueArrItem(list, i);
                if(i == 0)
                {
                    if (Common_Json_GetAttrValueInt(pObjItem, "Enable", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObjRoot, "Enable", i_num);
                    }

                    if (Common_Json_GetAttrValueInt(pObjItem, "X", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObjRoot, "X", i_num);
                    }

                    if (Common_Json_GetAttrValueInt(pObjItem, "Y", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObjRoot, "Y", i_num);
                    }

                    if (Common_Json_GetAttrValueInt(pObjItem, "Location", &i_num))
                    {
                        Common_Json_SetAttrValueInt(pObjRoot, "Location", i_num);
                    }

                    if (Common_Json_GetAttrValueStr(pObjItem, "FontName", &str_tmp))
                    {
                        Common_Json_SetAttrValueStr(outdata, "FontName", str_tmp);
                    }

                    if (Common_Json_GetAttrValueStr(pObjItem, "String", &str_tmp))
                    {
                        Common_Json_SetAttrValueStr(pObjRoot, "Text", str_tmp);
                    }

                    if (Common_Json_GetAttrValueInt(pObjItem, "ColorAttr/TextColorEx", &i_num))
                    {
                        i_num = TRCOLOR_RGB555RGB888(i_num);
                        char tmpstr[16];
                        snprintf(tmpstr, sizeof(tmpstr), "#%06x", i_num);
                        Common_Json_SetAttrValueStr(outdata, "TextColor", tmpstr);
                    }
                }

                int useBitmap = 0;
                if (Common_Json_GetAttrValueInt(pObjItem, "UseRemoteBm", &useBitmap))
                {
                    if (useBitmap == 0 && Common_Json_GetAttrValueInt(pObjItem, "Size", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, osdsize_arr_lattice[i], i_num);
                    }
                    else if (useBitmap && Common_Json_GetAttrValueInt(pObjItem, "BitMapSize", &i_num))
                    {
                        Common_Json_SetAttrValueInt(outdata, osdsize_arr_lattice[i], i_num);
                    }
                }
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    get_channel_osd_cap(header, 0, "ChannelOSD", outdata);

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/Time/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            pObjRoot = Common_Json_SetAttrValueObj(outdata, "TimeOSD");

            if (Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Enable", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "X", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "X", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Y", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Y", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Location", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Style", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "DateType", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "HourStyle", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "HourType", i_num);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        memset(buf,0,sizeof(buf));
        snprintf(buf,sizeof(buf),"/BoardSys/Osd/MulString/Attribute/Device%d/Channel%d/Stream0",opt->dev,opt->ch);
        Ovfs_Web_UpdateHeader(header, REST_GET, buf);

        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
        if (0 == ret)
        {
            pObjRoot = Common_Json_SetAttrValueObj(outdata, "MultiOSD");

            if (Common_Json_GetAttrValueInt(lowerData, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Enable", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "X", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "X", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Y", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Y", i_num);
            }

            if (Common_Json_GetAttrValueInt(lowerData, "Location", &i_num))
            {
                Common_Json_SetAttrValueInt(pObjRoot, "Location", i_num);
            }

            if (Common_Json_GetAttrValueStr(lowerData, "String", &str_tmp))
            {
                Common_Json_SetAttrValueStr(pObjRoot, "Text", str_tmp);
            }
        }
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    if (0 == ret)
    {
        get_multi_osd_cap(header, 2, "MultiOSD", outdata);
    }

    return ret;
}

static char osd_name_list[][16] = {"TimeOSD", "ChannelOSD", "MultiOSD"};
static char url_name_list[][16] = {"Time", "ChannelName", "Mulstring"};


static int web_semantic_set_allosdconfig(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata, OVFS_WEB_OPTION_S *opt)
{
    int i = 0,
        j = 0,
        ret = 0,
        mainFontSize = 0,
        subFontSize = 0,
        textColor = -1,
        streamCount = 0,
        numTmp = 0;
    char *fontName = NULL,
         *strTmp = NULL;
    char url[128] = {0};
    cJSON_Struct *lowerData = NULL,
                 *tmpData = NULL;

    Common_Json_GetAttrValueInt(indata, "MainStreamFontSize", &mainFontSize);
    Common_Json_GetAttrValueInt(indata, "SubStreamFontSize", &subFontSize);
    Common_Json_GetAttrValueStr(indata, "FontName", &fontName);
    if(Common_Json_GetAttrValueStr(indata, "TextColor", &strTmp))
    {
        textColor = (int)strtoul(strTmp+1, NULL, 16);
        textColor = TRCOLOR_RGB888RGB555(textColor);
    }

    streamCount = query_devchan_streamcount(header, opt->dev, opt->ch);

    for(i=0; i<sizeof(osd_name_list)/sizeof(osd_name_list[0]); i++)
    {
        if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0)) == NULL)
        {
            return WEB_CODE_LackingMem;
        }

        if(tmpData = Common_Json_GetAttrValueObj(indata, osd_name_list[i]))
        {
            if(Common_Json_GetAttrValueInt(tmpData, "Enable", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "Enable", numTmp);
            }

            if(Common_Json_GetAttrValueInt(tmpData, "Location", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "Location", numTmp);
            }

            if(Common_Json_GetAttrValueInt(tmpData, "X", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "X", numTmp);
            }

            if(Common_Json_GetAttrValueInt(tmpData, "Y", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "Y", numTmp);
            }

            if(Common_Json_GetAttrValueStr(tmpData, "Text", &strTmp))
            {
                Common_Json_SetAttrValueStr(lowerData, "String", strTmp);
            }

            if(Common_Json_GetAttrValueInt(tmpData, "DateType", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "Style", numTmp);
            }

            if(Common_Json_GetAttrValueInt(tmpData, "HourType", &numTmp))
            {
                Common_Json_SetAttrValueInt(lowerData, "HourStyle", numTmp);
            }

            if(textColor >= 0 && textColor <= 0x7fff)
            {
                Common_Json_SetAttrValueObj(lowerData, "ColorAttr");
                Common_Json_SetAttrValueInt(lowerData, "ColorAttr/TextColorEx", textColor);
            }

            cJSON_Struct *lattice = Common_Json_GetAttrValueObj(tmpData, "Lattice");

            for(j=0; j<streamCount; j++)
            {

                if (Common_Json_GetAttrValueInt(indata, osdsize_arr_lattice[j], &numTmp))
                {
                    Common_Json_SetAttrValueInt(lowerData, "BitMapSize", numTmp);
                    Common_Json_SetAttrValueInt(lowerData, "Size", numTmp);
                }

                cJSON_Struct *pObj_tmp1 = Common_Json_GetAttrValueObj(lattice, bitmapsize_arr[j]);
                if (pObj_tmp1)
                {
                    Common_Json_SetAttrValueInt(lowerData, "UseRemoteBm", 1);
                    if (Common_Json_GetAttrValueInt(pObj_tmp1, "Width", &numTmp))
                    {
                        Common_Json_SetAttrValueInt(lowerData, "BitMapW", numTmp);
                    }

                    if (Common_Json_GetAttrValueInt(pObj_tmp1, "Height", &numTmp))
                    {
                        Common_Json_SetAttrValueInt(lowerData, "BitMapH", numTmp);
                    }

                    if (Common_Json_GetAttrValueStr(pObj_tmp1, "Data", &strTmp))
                    {
                        Common_Json_SetAttrValueStr(lowerData, "BitMapArr", strTmp);
                    }

                    if (Common_Json_GetAttrValueInt(pObj_tmp1, "Length", &numTmp))
                    {
                        Common_Json_SetAttrValueInt(lowerData, "BitMapArrLen", numTmp);
                    }
                }

                memset(url,0,sizeof(url));
                snprintf(url,sizeof(url),"/BoardSys/Osd/%s/Attribute/Device%d/Channel%d/Stream%d",url_name_list[i], opt->dev,opt->ch,j);
                Ovfs_Web_UpdateHeader(header, REST_PUT, url);

                ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
                LOGW("url:[%s] ret:[%d]\n",url,ret);
                char *str = Common_Json_Print(lowerData, NULL);
                printf("%s\n",str);
                free(str);

                if(ret != 0)break;
            }
        }

        Common_Json_Delete(lowerData);
        lowerData = NULL;

        if(ret != 0)break;
    }
    return ret;
}

int frmAllOSDConfig(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_allosdconfig(header, indata, outdata, opt);
            break;
        case 1:
            ret = web_semantic_set_allosdconfig(header, indata, outdata, opt);
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

static int web_semantic_set_switchsensor(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    Ovfs_Web_UpdateHeader(header, REST_PUT, "/Boardsys/Video/changesensor");
    ret = Ovfs_Web_RestMethodA(header, NULL, NULL, 0);

    return ret;
}

int frmSwitchSensor(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
            case 1:
                ret = web_semantic_set_switchsensor(header, indata, outdata);
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

int web_semantic_get_sensoralarm(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int id = -1;
    int i_num = 0;
    double f_tmp = 0;
    char *str_tmp = NULL;
    cJSON_Struct *lowerData = NULL;

    if(Common_Json_GetAttrValueInt(indata, "SensorId", &i_num))
    {
        LOGD("SensorId GET:[%d]\n",i_num);
        if(i_num<1 || i_num>8)
        {
            return WEB_CODE_InvalidArg;
        }
        else
        {
            id = i_num-1;
        }
    }
    LOGD("SensorId:[%d]\n",id);

    get_channel_osd_cap(header, 1, NULL, outdata);

    if(id != -1)
    {
        Common_Json_SetAttrValueInt(indata, "SensorId", id);
    }

    ret = web_get_linkcfg_v2(header, 10, indata, outdata);

    if(ret == 0)
    {
        Ovfs_Web_UpdateHeader(header, REST_GET, "/ptz/uartosd");
        ret = Ovfs_Web_RestMethodA(header, NULL, &lowerData, 0);
    }

    if(ret == 0)
    {
        cJSON_Struct *list_get = lowerData;
        cJSON_Struct *list_set = NULL;
        if(id == -1)
        {
            if((list_set = Common_Json_GetAttrValueArr(outdata, "List")) == NULL)
            {
                list_set = Common_Json_SetAttrValueArr(outdata, "List");
            }
        }
        int size = Common_Json_ArraySize(list_get);
        if(size > 0)
        {
            for(i=0; i<size; i++)
            {
                cJSON_Struct *tmp = outdata;
                if(id == -1)
                {
                    if((tmp = Common_Json_GetAttrValueArrItem(list_set, i)) == NULL)
                    {
                        tmp = Common_Json_SetAttrValueArrObj(list_set, i);
                    }
                }

                if(Common_Json_GetAttrValue(list_get, i, "UartOsdId", NULL, NULL, &i_num, NULL))
                {
                    LOGD("UartOsdId:[%d]\n",i_num);
                    if(id != -1)
                    {
                        if(i_num != id)
                        {
                            LOGD("continue\n");
                            continue;
                        }
                        else
                        {
                            size = i;
                        }
                    }

                    Common_Json_SetAttrValueInt(tmp, "SensorId", i_num+1);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Enable", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Enable", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Name", NULL, &str_tmp, NULL, NULL))
                {
                    Common_Json_SetAttrValueStr(tmp, "Name", str_tmp);
                }

                if(Common_Json_GetAttrValue(list_get, i, "X", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "OSDX", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "Y", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "OSDY", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "IsUpperThreshold", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "Symbol", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "ThresholdValue", NULL, NULL, &i_num, &f_tmp))
                {
                    f_tmp = i_num?i_num*1.0:f_tmp;
                    Common_Json_SetAttrValueFlt(tmp, "Threshold", f_tmp);
                }

                if(Common_Json_GetAttrValue(list_get, i, "MainSize", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "MainStreamFontSize", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "SubSize", NULL, NULL, &i_num, NULL))
                {
                    Common_Json_SetAttrValueInt(tmp, "SubStreamFontSize", i_num);
                }

                if(Common_Json_GetAttrValue(list_get, i, "TextColor", NULL, NULL, &i_num, NULL))
                {

                    i_num = TRCOLOR_RGB555RGB888(i_num);
                    char tmpstr[16];
                    snprintf(tmpstr, sizeof(tmpstr), "#%06x", i_num);
                    Common_Json_SetAttrValueStr(tmp, "TextColor", tmpstr);
                }
            }
        }
    }

    if(lowerData)
    {
        Common_Json_Delete(lowerData);
        lowerData = NULL;
    }

    return ret;
}

int web_semantic_set_sensoralarm(cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;
    int i = 0;
    int id = -1;
    int i_num = 0;
    int size = 0;
    int mainSize = 0;
    int subSize = 0;
    int textColor = -1;
    double f_tmp = 0.0;
    char *str_tmp = NULL;
    cJSON_Struct *tmp = NULL;
    cJSON_Struct *lowerData = NULL;
    cJSON_Struct *list_get = NULL;

    if(Common_Json_GetAttrValueInt(indata, "SensorId", &i_num))
    {
        LOGD("SensorId GET:[%d]\n",i_num);
        if(i_num<1 || i_num>8)
        {
            return WEB_CODE_InvalidArg;
        }
        else
        {
            id = i_num-1;
            size = 1;
        }
    }
    LOGD("SensorId:[%d]\n",id);

    if(id == -1)
    {
        list_get = Common_Json_GetAttrValueArr(indata, "List");
        if(list_get)
        {
            size = Common_Json_ArraySize(list_get);
        }
        LOGD("size:[%d]\n",size);

        if(size == 0)
        {
            return WEB_CODE_InvalidArg;
        }
    }

    if ((lowerData = Common_Json_New(NULL, Common_Json_Type_Array, NULL, 0, 0)) == NULL)
    {
        return WEB_CODE_LackingMem;
    }

    ret = web_set_linkcfg(header, 10, indata);

    if (0 == ret)
    {
        LOGD("size:[%d]\n",size);
        for(i=0; i<size; i++)
        {
            tmp = Common_Json_SetAttrValueArrObj(lowerData, i);

            cJSON_Struct *tmp_item = indata;
            if(list_get)
            {
                tmp_item = Common_Json_GetAttrValueArrItem(list_get, i);
            }

            if (Common_Json_GetAttrValueInt(tmp_item, "SensorId", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "UartOsdId", i_num-1);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "Enable", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "Enable", i_num);
            }

            if(Common_Json_GetAttrValueStr(tmp_item, "Name", &str_tmp))
            {
                Common_Json_SetAttrValueStr(tmp, "Name", str_tmp);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "OSDX", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "X", i_num);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "OSDY", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "Y", i_num);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "Symbol", &i_num))
            {
                Common_Json_SetAttrValueInt(tmp, "IsUpperThreshold", i_num);
            }

            if(Common_Json_GetAttrValue(tmp_item, -1, "Threshold", NULL, NULL, &i_num, &f_tmp))
            {
                f_tmp = i_num?i_num*1.0:f_tmp;
                Common_Json_SetAttrValueFlt(tmp, "ThresholdValue", f_tmp);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "MainStreamFontSize", &mainSize))
            {
                Common_Json_SetAttrValueInt(tmp, "MainSize", mainSize);
            }

            if(Common_Json_GetAttrValueInt(tmp_item, "SubStreamFontSize", &subSize))
            {
                Common_Json_SetAttrValueInt(tmp, "SubSize", subSize);
            }

            if(Common_Json_GetAttrValueStr(tmp_item, "TextColor", &str_tmp))
            {
                i_num = (int)strtoul(str_tmp+1, NULL, 16);
                textColor = TRCOLOR_RGB888RGB555(i_num);
                if (textColor >= 0 && textColor <= 0x7fff)
                {
                    Common_Json_SetAttrValueInt(tmp, "TextColor", textColor);
                }
            }

            LOGD("Json_Size:[%d]\n",Common_Json_Size(tmp));
            if(Common_Json_Size(tmp) == 0 && size == 1)
            {
                LOGD("RemoveItem\n");
                Common_Json_RemoveItem(lowerData, 0, NULL);
            }
        }
    }

    if (0 == ret)
    {
        ovfs_print_json(lowerData);
        Ovfs_Web_UpdateHeader(header, REST_PUT, "/ptz/uartosd");
        ret = Ovfs_Web_RestMethodA(header, lowerData, NULL, 0);
    }

    Common_Json_Delete(lowerData);
    lowerData = NULL;

    return ret;
}

int frmSensorAlarm(Webs *wp, OVFS_WEB_OPTION_S *opt, cJSON_Struct *header, cJSON_Struct *indata, cJSON_Struct *outdata)
{
    int ret = 0;

    if (0 == ret)
    {
        switch (opt->type)
        {
        case 0:
            ret = web_semantic_get_sensoralarm(header, indata, outdata);
            break;
        case 1:
            ret = web_semantic_set_sensoralarm(header, indata, outdata);
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

