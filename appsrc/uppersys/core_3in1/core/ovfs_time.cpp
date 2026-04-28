#include <string.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>

#ifndef WIN32

#endif
#include "librtc_api.h"
#include "ovfs_time.h"
#include "core_version.h"

#define OVFS_NTP_BAT           "/var/.ntp_bat"
#define OVFS_HWCLOCK_TIME_BAT  "/var/.hwclock_bat"
//#define OVFS_NTPCLIENT_PATH    "/root/bin/ntpclient"
#define OVFS_NTPCLIENT_PATH    "ntpclient"
#define OVFS_CORE_SAVE_TIME_FILE "/usr/etc/savetime.txt"
#define OVFS_CORE_TIME_ZONE_FILE "/tmp/TZ"
#define OVFS_REBOOT_SPEND_TIME  24

static time_t              m_dst_start_time;
static ovfs_ntp_config     m_ntp_cfg;
static ovfs_time_zone_cfg  m_time_zone_cfg;
static ovfs_dst_cfg        m_dst_cfg;
static long long              m_last_ntp_check_time = 0; //记录自动校时上次校时时间
static OVFS_BOOL           m_is_need_ntp_check = OVFS_FALSE;
static Common_InterSleep_T m_sleep_ops;
static Common_Lock_T       m_lock;
static Common_Thread_T     m_thread_handle;
static Common_Lock_T       set_sys_time_lock;

//static pthread_mutex_t    set_sys_time_lock = PTHREAD_MUTEX_INITIALIZER;

static U32 dwDayInMonth[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
const  U64 OVFS_INTERVAL_1970_1900 = (U64)2208988800UL;

extern Core_Version_T g_tVersion;

static ovfs_dst_cap_node s_support_dst[] =
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
    {OVFS_TIME_ZONE_GREENWICH, 0, 0},                   //格林威治标准时间：都柏林，爱丁堡，伦敦，里斯本
    {OVFS_TIME_ZONE_AMSTERDAM, 1, 0},                   //阿姆斯特丹，柏林，伯尔尼，罗马，斯德哥尔摩，维也纳
    {OVFS_TIME_ZONE_ATHENS, 2, 0},                      //雅典，布加勒斯特
    {OVFS_TIME_ZONE_BAGHDAD, 3, 0},                     //巴格达 ,科威特
    {OVFS_TIME_ZONE_TEHERAN, 3, 30},                    //德黑兰
    {OVFS_TIME_ZONE_MOSCOW, 4, 0},                      //莫斯科，圣彼得堡，伏尔加格勒
    {OVFS_TIME_ZONE_KABUL, 4, 30},                      //喀布尔
    {OVFS_TIME_ZONE_ISB, 5, 0},                         //伊斯兰堡, 卡拉奇 ,塔什干
    {OVFS_TIME_ZONE_MADRAS, 5, 30},                     //马德拉斯，加尔各答，孟买，新德里
    {OVFS_TIME_ZONE_KATHMANDU, 5, 45},                  //加德满都
    {OVFS_TIME_ZONE_NOVOSIBIRSK, 6, 0},                 //新西伯利亚
    {OVFS_TIME_ZONE_RANGOON, 6, 30},                    //仰光
    {OVFS_TIME_ZONE_BANGKOK, 7, 0},                     //曼谷，河内，雅加达
    {OVFS_TIME_ZONE_BEIJING, 8, 0},                     //北京，重庆，香港特别行政区，乌鲁木齐
    {OVFS_TIME_ZONE_OSAKA, 9, 0},                       //首尔,大阪，札幌，东京
    {OVFS_TIME_ZONE_ADELAIDE, 9, 30},                   //阿德莱德 ,达尔文
    {OVFS_TIME_ZONE_CANBERRA, 10, 0},                   //堪培拉，墨尔本，悉尼
    {OVFS_TIME_ZONE_SOLOMON_ISLANDS, 11, 0},            //所罗门群岛，新喀里多尼亚
    {OVFS_TIME_ZONE_OSKLAND, 12, 0},                    //奥克兰, 惠林顿 ,斐济 ,马加丹
    {OVFS_TIME_ZONE_NUKUALOFA, 13, 0},                  //努库阿洛法

};

static U32 ovfs_utility_cnt_interval(time_t para1, time_t para2)
{
    return (para1 > para2) ? (para1 - para2) : (para2 - para1);
}

static S32 ovfs_utility_linux_to_ovfs_time(time_t linux_time,
        ovfs_time_struct *ovfs_time)
{
    if (ovfs_time == NULL)
    {
        LOGE("ovfs_time == NULL\n");
        return -1;
    }

    struct tm p;

    Common_LocalTime_r (&linux_time, &p);
    ovfs_time->year = 1900 + p.tm_year;
    ovfs_time->month = 1 + p.tm_mon;
    ovfs_time->day = p.tm_mday;

    ovfs_time->hour = p.tm_hour;
    ovfs_time->min = p.tm_min;
    ovfs_time->sec = p.tm_sec;

    return 0;
}

static S32 ovfs_utility_ovfs_to_linux_time(const ovfs_time_struct *ovfs_time,
        time_t *linux_time)
{
    if ((ovfs_time == NULL) || (linux_time == NULL))
    {
        LOGE("para is NULL\n");
        return -1;
    }

    struct tm p;
	memset(&p, 0, sizeof(p));

	*linux_time = time(NULL);
    Common_LocalTime_r (linux_time, &p);   //有个is_dst成员，不能乱配置

    p.tm_year = ovfs_time->year - 1900;
    p.tm_mon = ovfs_time->month - 1;
    p.tm_mday = ovfs_time->day;

    p.tm_hour = ovfs_time->hour;
    p.tm_min = ovfs_time->min;
    p.tm_sec = ovfs_time->sec;

	Common_Lock(m_lock);
    *linux_time = mktime(&p);
	Common_UnLock(m_lock);

    return 0;
}


static S32 ovfs_utility_linux_UTC_to_ovfs_time(time_t linux_time, ovfs_time_struct *ovfs_time)
{
    if (ovfs_time == NULL)
    {
        LOGE("ovfs_time == NULL\n");
        return -1;
    }

    struct tm *p;

	Common_Lock(m_lock);
	// memset(&p,0,sizeof(struct tm));
	  p = gmtime(&linux_time);
	Common_UnLock(m_lock);
	// p = &xtp;
    //  Common_LocalTime_r (&linux_time, &xtp);
    ovfs_time->year = 1900 + p->tm_year;
    ovfs_time->month = 1 + p->tm_mon;
    ovfs_time->day = p->tm_mday;

    ovfs_time->hour = p->tm_hour;
    ovfs_time->min = p->tm_min;
    ovfs_time->sec = p->tm_sec;

    return 0;
}

static S32 set_sys_time(const ovfs_time_struct *hw_time,
                        const ovfs_time_struct *sys_time)
{
    if(sys_time == NULL && hw_time == NULL)
    {
        return 0;
    }

    time_t start_time = time(NULL);
    time_t stop_time = time(NULL);
    Common_Lock(set_sys_time_lock);

    stop_time = time(NULL);
    U32 interval = ovfs_utility_cnt_interval(start_time, stop_time);
    if(hw_time != NULL)
    {
        LOGW("HW_CLOCK: %d-%d-%d %d:%d:%d\n",
             hw_time->year,
             hw_time->month,
             hw_time->day,
             hw_time->hour,
             hw_time->min,
             hw_time->sec);

		ovfs_time_struct tHwTime;
		memcpy(&tHwTime, hw_time, sizeof(ovfs_time_struct));

		if(tHwTime.year < 2000)
		{
			tHwTime.year = 2000;
			tHwTime.month = 1;
			tHwTime.day = 1;
			tHwTime.hour = 0;
			tHwTime.min = 0;
			tHwTime.sec = 0;

			LOGW("HW_CLOCK: %d-%d-%d %d:%d:%d\n",
             tHwTime.year,
             tHwTime.month,
             tHwTime.day,
             tHwTime.hour,
             tHwTime.min,
             tHwTime.sec);
		}

        if (access("/dev/hi_rtc", F_OK) == 0)
        {
#ifndef PLATFORM_JZT40
            rtc_time_t tRtcTimeV2;
            tRtcTimeV2.year = tHwTime.year;
            tRtcTimeV2.month = tHwTime.month;
            tRtcTimeV2.date = tHwTime.day;
            tRtcTimeV2.hour = tHwTime.hour;
            tRtcTimeV2.minute = tHwTime.min;
            tRtcTimeV2.second = tHwTime.sec;
            tRtcTimeV2.weekday = 0;
            Rtc_HwClock_SetTime(&tRtcTimeV2);
#endif
        }
        /*if has rtc0 then uses common ioctl interface*/
        else if (access("/dev/rtc0", F_OK) == 0)
        {
            int ret = 0, fd = 0;
            struct rtc_time rtc_tm;
            fd = open("/dev/rtc0", O_WRONLY);
            if (fd < 0)
            {
                LOGE("open rtc0 failed %s\n",strerror(errno));
            }
            else
            {
                rtc_tm.tm_hour = tHwTime.hour;
                rtc_tm.tm_min = tHwTime.min;
                rtc_tm.tm_sec = tHwTime.sec;
                rtc_tm.tm_year = tHwTime.year - 1900;
                rtc_tm.tm_mon = tHwTime.month -1;
                rtc_tm.tm_mday = tHwTime.day;
                LOGW("y %d m %d d %d h %d m %d s %d\n",
                     rtc_tm.tm_year,
                     rtc_tm.tm_mon,
                     rtc_tm.tm_mday,
                     rtc_tm.tm_hour,
                     rtc_tm.tm_min,
                     rtc_tm.tm_sec);
                ret = ioctl(fd, RTC_SET_TIME, &rtc_tm);
                if (ret == -1)
                {
                    LOGE("ioctl rtc0 failed %s\n",strerror(errno));
                }
                close(fd);
            }
        }

        //write ftc time
        {
			time_t offsetTime;

			offsetTime = time(NULL);
		    offsetTime = offsetTime + OVFS_REBOOT_SPEND_TIME;//24s reboot spend time

            FILE *fp = fopen(OVFS_CORE_SAVE_TIME_FILE,"wb");
            if (fp != NULL)
            {
                char buf[32] = { 0 };
                snprintf(buf,sizeof(buf),"%lu",offsetTime);
                fwrite(buf,1,strlen(buf),fp);
                fflush(fp);
                fclose(fp);

				LOGD("write ftcTime %d\n",offsetTime);
            }
        }

        time_t linux_time = 0;

		ovfs_utility_ovfs_to_linux_time(&tHwTime, &linux_time);
		linux_time += (Common_GetTimeDiff() + interval);

        stime(&linux_time);

		tzset();

	    struct tm *tmp_ptr = NULL;
	    tmp_ptr = localtime(&linux_time);
	    printf("stime is:%04d%02d%02d-%02d:%02d:%02d\n", 1900+tmp_ptr->tm_year, 1+tmp_ptr->tm_mon, tmp_ptr->tm_mday,
			tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);
    }

    Common_UnLock(set_sys_time_lock);
    return 0;
}

/*note rtc was used to store UTC time*/
static S32 get_hwclock_time(ovfs_time_struct *gmt_time)
{
    time_t ftcTime = 0;
    time_t rtcTime = 0;
    struct tm rTime, *pTime = NULL;

    /*if has hi_rtc then uses librtc interface*/
    if (access("/dev/hi_rtc", F_OK) == 0)
    {
#ifndef PLATFORM_JZT40
        rtc_time_t tRtcTimeV2;
        memset(&tRtcTimeV2, 0, sizeof(rtc_time_t));
        Rtc_HwClock_GetTime(&tRtcTimeV2);
        LOGD("hi_rtc year %d mon %d min %d sec %d\n",
             tRtcTimeV2.year,tRtcTimeV2.month,tRtcTimeV2.minute,tRtcTimeV2.second);
        memset(&rTime,0,sizeof(struct tm));
        rTime.tm_mday = tRtcTimeV2.date;
        rTime.tm_hour = tRtcTimeV2.hour;
        rTime.tm_min = tRtcTimeV2.minute;
        rTime.tm_sec = tRtcTimeV2.second;
        rTime.tm_year = tRtcTimeV2.year - 1900;
        rTime.tm_mon = tRtcTimeV2.month - 1;
        rTime.tm_isdst = 0;
        rtcTime = timegm(&rTime);//mktime(&rTime)
        rTime.tm_year += 1900;
        rTime.tm_mon += 1;
#endif
    }
    /*if has rtc0 then uses common ioctl interface*/
    else if (access("/dev/rtc0", F_OK) == 0)
    {
        int ret = 0, fd = 0;
        struct rtc_time rtc_tm;
        memset(&rtc_tm, 0 , sizeof(struct rtc_time));

        fd = open("/dev/rtc0", O_RDONLY);
        if (fd < 0)
        {
            LOGE("open rtc0 failed %s\n",strerror(errno));
        }
        else
        {
            ret = ioctl(fd, RTC_RD_TIME, &rtc_tm);
            if (ret == -1)
            {
                LOGE("ioctl rtc0 failed %s\n",strerror(errno));
            }
            close(fd);
        }

        rTime.tm_mday = rtc_tm.tm_mday;
        rTime.tm_hour = rtc_tm.tm_hour;
        rTime.tm_min = rtc_tm.tm_min;
        rTime.tm_sec = rtc_tm.tm_sec;
        rTime.tm_year = rtc_tm.tm_year;
        rTime.tm_mon = rtc_tm.tm_mon;
        rTime.tm_isdst = 0;
        rtcTime = timegm(&rTime);//mktime(&rTime)
        rTime.tm_year += 1900;
        rTime.tm_mon += 1;
        LOGD("rtc_tm.year %d mon %d\n",rtc_tm.tm_year,rtc_tm.tm_mon);
    }

    // read ftc time
    {
        FILE *fp = fopen(OVFS_CORE_SAVE_TIME_FILE,"rb");
        if (fp != NULL)
        {
            char buf[32] = { 0 };
            fread(buf,1,sizeof(buf)-1,fp);
            fclose(fp);
            ftcTime = atoi(buf);

			LOGD("read ftcTime %d\n",ftcTime);
        }
    }

    /*use rtc time*/
    /*系统调用重启时，会在写ftc时间时多加24秒，做为系统启动消耗的时间*/
    LOGW("rtc %ld ftcTime %ld\n",rtcTime,ftcTime);
    if ((int)rtcTime > 0 && rtcTime + OVFS_REBOOT_SPEND_TIME > ftcTime)
    {
        pTime = &rTime;
        LOGW("use rtc time \n");
    }
    /*use ftc time*/
    else
    {
        pTime = gmtime(&ftcTime);
        pTime->tm_year += 1900;
        pTime->tm_mon += 1;
        LOGW("use ftc time \n");


    }

    gmt_time->day = pTime->tm_mday;
    gmt_time->hour = pTime->tm_hour;
    gmt_time->min = pTime->tm_min;
    gmt_time->sec = pTime->tm_sec;
    gmt_time->year = pTime->tm_year;
    gmt_time->month = pTime->tm_mon;

	printf("pTime is:%04d%02d%02d-%02d:%02d:%02d\n", pTime->tm_year, pTime->tm_mon, pTime->tm_mday,
		pTime->tm_hour, pTime->tm_min, pTime->tm_sec);

    return 0;
}

static ovfs_dst_cap_node *proc_get_dst_node(ovfs_time_zone zone)
{
    ovfs_dst_cap_node *dst_node = NULL;
    U32 i = 0;

    for(i = 0; i < COMMON_ARRAY_ELEMENT_COUNT(s_support_dst); ++i)
    {
        if(s_support_dst[i].zone_type == zone)
        {
            dst_node = &s_support_dst[i];
            break;
        }
    }

    return dst_node;
}

static OVFS_VOID proc_cnt_sys_time(const ovfs_time_struct *hw_time,
                                   ovfs_time_struct *sys_time,
                                   ovfs_time_zone_cfg *zone_cfg,
                                   ovfs_dst_cfg *dst_cfg)
{
    time_t hw_utc_time;
	if((sys_time == NULL) || (hw_time == NULL))
    {
		return ;
    }

	LOGD("[org]hw_time: %d-%d-%d %d:%d:%d\n",
		   hw_time->year,
		   hw_time->month,
		   hw_time->day,
		   hw_time->hour,
		   hw_time->min,
		   hw_time->sec);

	ovfs_utility_ovfs_to_linux_time(hw_time, &hw_utc_time);

	ovfs_utility_linux_to_ovfs_time(hw_utc_time, sys_time);

	LOGD("[out]sys_time: %d-%d-%d %d:%d:%d\n",
		   sys_time->year,
		   sys_time->month,
		   sys_time->day,
		   sys_time->hour,
		   sys_time->min,
		   sys_time->sec);

    return;
}

static OVFS_VOID proc_cnt_hw_time(ovfs_time_struct *sys_time,
                                  ovfs_time_struct *hw_time,
                                  ovfs_time_zone_cfg *zone_cfg,
                                  ovfs_dst_cfg *dst_cfg)
{
    time_t hw_utc_time ;
	if((sys_time == NULL) || (hw_time == NULL))
    //同步硬件时间的时候夏令时的比较时间应该以系统时间为准
    {
			return ;
    }

	LOGD("[org]sys_time: %d-%d-%d %d:%d:%d\n",
		   sys_time->year,
		   sys_time->month,
		   sys_time->day,
		   sys_time->hour,
		   sys_time->min,
		   sys_time->sec);
	ovfs_utility_ovfs_to_linux_time(sys_time, &hw_utc_time);

	ovfs_utility_linux_UTC_to_ovfs_time(hw_utc_time, hw_time);

	LOGD("[out]hw_time: %d-%d-%d %d:%d:%d\n",
         hw_time->year,
         hw_time->month,
         hw_time->day,
         hw_time->hour,
         hw_time->min,
         hw_time->sec);

    return;
}

static OVFS_BOOL is_need_ntp_check()
{

    if(m_ntp_cfg.enable != OVFS_TRUE)
    {
        //MODEL_CLASS_PW("ntp disable\n");
        return OVFS_FALSE;
    }

    if(m_is_need_ntp_check == OVFS_TRUE)
    {
        return OVFS_TRUE;
    }


	long long cur_time;
	cur_time = Common_GetSystemCount64() / 1000;
	long long llInterval = cur_time - m_last_ntp_check_time;
	long long llSetInterval = m_ntp_cfg.auto_set_time_interval;
	if (llSetInterval <= 0 )
    {


		if(llInterval < llSetInterval * 24 * 3600)
    {
        //LOGE("auto_set_time_interval = %d interval = %d!\n", m_ntp_cfg.auto_set_time_interval * 3600, interval);
        return OVFS_FALSE;
    }

	}
	else if (llInterval < llSetInterval * 3600)
    {
        //LOGE("ovfs_utility_cnt_interval %d!\n", ovfs_utility_cnt_interval(hw_utc_time , m_last_ntp_check_time));
        return OVFS_FALSE;
    }

    return OVFS_TRUE;
}

static OVFS_BOOL is_need_set_time(const ovfs_time_struct *hw_time,
                                  const ovfs_time_struct *sys_time,
                                  U32 interval)
{

    if(hw_time != NULL)
    {
        ovfs_time_struct local_hw_time;
        get_hwclock_time(&local_hw_time);
        time_t local_hw_utc_time = 0;
		ovfs_utility_ovfs_to_linux_time(&local_hw_time, &local_hw_utc_time);
        time_t local_hw_set_utc_time = 0;
		ovfs_utility_ovfs_to_linux_time(hw_time, &local_hw_set_utc_time);
        // LOGE("hw %d %d\n",local_hw_set_utc_time,local_hw_utc_time);
        if(ovfs_utility_cnt_interval(local_hw_set_utc_time,
                                     local_hw_utc_time) > interval)
        {
            return OVFS_TRUE;
        }
    }

    if(sys_time != NULL)
    {
        time_t local_sys_utc_time = time(NULL);
        time_t local_sys_set_utc_time = 0;
        ovfs_utility_ovfs_to_linux_time(sys_time, &local_sys_set_utc_time);
        // LOGE("sys %lu %lu\n",local_sys_set_utc_time,local_sys_utc_time);
        if(ovfs_utility_cnt_interval(local_sys_set_utc_time,
                                     local_sys_utc_time) > interval)
        {
            return OVFS_TRUE;
        }
    }

    return OVFS_FALSE;
}

static S32 get_ntp_time(ovfs_time_struct *gmt_time, const S8 *server)
{
    S8 cmd_string[256];
    snprintf(cmd_string, sizeof(cmd_string), "rm %s", OVFS_NTP_BAT);

    LOGW("sys_cmd:%s\n", cmd_string);
    Common_System(cmd_string);
#if 0
    if(0 != Common_System(cmd_string))
    {
        LOGE("cmd %s fail\n", cmd_string);
        return -1;
    }
#endif

    snprintf(cmd_string, sizeof(cmd_string), "%s -t -h %s -c 3 -i 5 -g 30 > %s",
             OVFS_NTPCLIENT_PATH, server, OVFS_NTP_BAT);

    LOGW("sys_cmd:%s\n", cmd_string);
    Common_System(cmd_string);
#if 0
    char buf[64] = {0};
    if(0 != Common_Exe_Cmd(cmd_string,30*1000, buf, sizeof(buf)))
    {
        LOGE("cmd %s fail\n", cmd_string);
        return -1;
    }
#endif

    FILE *fp = fopen(OVFS_NTP_BAT, "r");
    if(!fp)
    {
        LOGE("open file fail\n");
        return -1;
    }
    S32 ret = -1;
    U32 day = 0;
    U32 second = 0;
    S64 skew = 0;

    while(!feof(fp))
    {
        if(fgets(cmd_string, 256, fp) == NULL)
        {
            break;
        }

        S8 *str_day = strstr(cmd_string, "day=");
        S8 *str_second = strstr(cmd_string, "second=");
        S8 *str_skew = strstr(cmd_string, "skew=");

        if(NULL != str_day
                && NULL != str_second
                && NULL != str_skew)
        {
            ret = 0;
            sscanf(str_day + strlen("day="), "%u", &day);
            sscanf(str_second + strlen("second="), "%u", &second);
            sscanf(str_skew + strlen("skew="), "%llu", &skew);

        }

    }
    fclose(fp);
    if (ret != 0)
    {
        LOGE("get time err \n");
        return -1;
    }

    skew = skew / 1000000;
    U64 gmt_second = day * 24 * 3600 + skew + (S32)second;
    if(gmt_second < OVFS_INTERVAL_1970_1900)
    {
        return -1;
    }

    time_t utc_time = gmt_second - OVFS_INTERVAL_1970_1900;
	ovfs_utility_linux_UTC_to_ovfs_time(utc_time, gmt_time);
    LOGD("NTP time: %d-%d-%d %d:%d:%d\n",
         gmt_time->year,
         gmt_time->month,
         gmt_time->day,
         gmt_time->hour,
         gmt_time->min,
         gmt_time->sec);

    return 0;
}

static void writeTmpTz()
{
	int diff = Common_GetTimeDiff();

    /*writing the time zone info to the file /tmp/TZ */
    FILE *fp = fopen(OVFS_CORE_TIME_ZONE_FILE,"wb");
    if (fp != NULL)
    {
        char tz[8] = {};
        int hour = s_support_dst[m_time_zone_cfg.zone].zone_hour,
            min = s_support_dst[m_time_zone_cfg.zone].zone_min;

        if (hour > 0)
        {
            if (min != 0)
                snprintf(tz, sizeof(tz), "+%02d%02d",hour, abs(min));
            else
                snprintf(tz, sizeof(tz), "+%02d",hour);
        }
        else if (hour == 0)
        {
            if (min != 0)
                snprintf(tz, sizeof(tz), "+%02d%02d",hour, abs(min));
            else
                snprintf(tz, sizeof(tz), "Z");
        }
        else
        {
            if (min != 0)
                snprintf(tz, sizeof(tz), "-%02d%02d",abs(hour), abs(min));
            else
                snprintf(tz, sizeof(tz), "-%02d",abs(hour));
        }
        /*第一行当前时区信息*/
        fwrite(tz,1,strlen(tz),fp);
        fwrite("\n",1,1,fp);

        /*第二行本地时间与UTC时间的插值，秒数，包括夏令时*/
        char tmp[32] = {};
        snprintf(tmp, sizeof(tmp), "%+d\n", diff);
        fwrite(tmp,1,strlen(tmp),fp);

        fflush(fp);
        fclose(fp);
    }

}


static void systime_sync_to_hwtime()
{
	Common_Lock(set_sys_time_lock);
    time_t hw_utc_time;

	hw_utc_time = time(NULL);
    hw_utc_time = hw_utc_time + OVFS_REBOOT_SPEND_TIME;//24s reboot spend time

    FILE *fp = fopen(OVFS_CORE_SAVE_TIME_FILE,"wb");
    if (fp != NULL)
    {
        char buf[32] = { 0 };
        snprintf(buf,sizeof(buf),"%lu",hw_utc_time);
        fwrite(buf,1,strlen(buf),fp);
        fflush(fp);
        fclose(fp);

		//LOGD("write ftcTime %d\n",hw_utc_time);
    }
	Common_UnLock(set_sys_time_lock);
}

 /*schedule check ntp and save time file*/
static S32 time_check_thread(Common_Thread_T hThreadHandle, void *param)
{
	U32 count = 0,check_count = 0;

    while(1)
    {
        if(count == 10)
        {
            systime_sync_to_hwtime();

            count = 0;
            ovfs_time_struct hw_time;
            ovfs_time_struct sys_time;
            ovfs_time_struct ntp_time;
            COMMON_CLR_ARG(hw_time);
            COMMON_CLR_ARG(sys_time);
            COMMON_CLR_ARG(ntp_time);

            Common_Lock(m_lock);
            if(is_need_ntp_check())
            {
                S8 ntp_server[1024];
                snprintf(ntp_server, sizeof(ntp_server), "%s", m_ntp_cfg.server);
                Common_UnLock(m_lock);
                //获取ntp校时时耗时操作，所以要解锁
                S32 ret = get_ntp_time(&ntp_time, ntp_server);
                Common_Lock(m_lock);
                //获取成功之后再检查一下是否需要校时
                if(ret == 0 && is_need_ntp_check())
                {
                    hw_time = ntp_time;
					m_last_ntp_check_time = Common_GetSystemCount64() / 1000;
                    //if(is_need_set_time(&hw_time, &sys_time, 10) == OVFS_TRUE)
                    {
                        if(0 == set_sys_time(&hw_time, NULL))
                        {
                            m_is_need_ntp_check = OVFS_FALSE;
                        }
                    }
                }
                else
                {
				}

			}

            Common_UnLock(m_lock);
        }

        Common_InterSleep_Sleep(m_sleep_ops, 2, 0);
        count = count + 2;
		check_count+=2;
    }
    return 0;
}

S32 ovfs_get_time_cfg(ovfs_ntp_config *p_ntp_cfg,
                      ovfs_time_zone_cfg *p_time_zone_cfg, ovfs_dst_cfg *p_dst_cfg)
{
    Common_Lock(m_lock);

    if (NULL != p_ntp_cfg)
    {
        Common_Copy(p_ntp_cfg, &m_ntp_cfg, sizeof(m_ntp_cfg));
    }

    if (NULL != p_time_zone_cfg)
    {
        Common_Copy(p_time_zone_cfg, &m_time_zone_cfg, sizeof(m_time_zone_cfg));
    }

    if (NULL != p_dst_cfg)
    {
        Common_Copy(p_dst_cfg, &m_dst_cfg, sizeof(m_dst_cfg));
    }

    Common_UnLock(m_lock);

    return 0;
}

static void print_time()
{
	tzset();
	time_t tmpcal_ptr = time(NULL);
    struct tm *tmp_ptr = NULL;

    tmp_ptr = gmtime(&tmpcal_ptr);
    printf("gmtime is:%04d%02d%02d-%02d:%02d:%02d\n", 1900+tmp_ptr->tm_year, 1+tmp_ptr->tm_mon, tmp_ptr->tm_mday,
		tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);

    tmp_ptr = localtime(&tmpcal_ptr);
    printf("localtime is:%04d%02d%02d-%02d:%02d:%02d\n", 1900+tmp_ptr->tm_year, 1+tmp_ptr->tm_mon, tmp_ptr->tm_mday,
		tmp_ptr->tm_hour, tmp_ptr->tm_min, tmp_ptr->tm_sec);
}
static U32 ovfs_write_utc_file(ovfs_time_zone_cfg *zone_cfg, ovfs_dst_cfg *dst_cfg)
{
#if (defined(__GLIBC__) && !defined(__UCLIBC__))
	static char s_tzfile[] = {0x54, 0x5a, 0x69, 0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x55, 0x54, 0x43, 0x00, 0x00, 0x00, 0x54, 0x5a, 0x69,
	0x66, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x0b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x4c, 0x4d, 0x54, 0x00, 0x47, 0x4d, 0x54, 0x2d,
	0x30, 0x30, 0x00, 0x00, 0x00, 0x0a};
#endif

	char chZone[32];
	char chDst[64];
	char chTemp[512];
	int iWriteLen = 0;

	memset(chZone, 0, sizeof(chZone));
	memset(chDst, 0, sizeof(chDst));
	memset(chTemp, 0, sizeof(chTemp));

	ovfs_dst_cap_node *zone_node = proc_get_dst_node(zone_cfg->zone);
	if(zone_node == NULL)
	{
	    LOGE("dst node is null\n");
	    return -1;
	}

	int bDir = 1;
	if(zone_node->zone_hour < 0 || zone_node->zone_min < 0)
	{
		bDir = -1;
	}
	else
	{
		bDir = 1;
	}

	snprintf(chZone, sizeof(chZone), "CST%c%d:%d", bDir==-1?'+':'-', abs(zone_node->zone_hour), abs(zone_node->zone_min));

	int min = zone_node->zone_hour*60 + zone_node->zone_min + dst_cfg->dst_bias;
	if(min < 0)
	{
		bDir = -1;
	}
	else
	{
		bDir = 1;
	}

	if(dst_cfg->enable && OVFS_DST_WEEK == dst_cfg->mode)
	{
		snprintf(chDst, sizeof(chDst), "DST%c%d:%d,M%d.%d.%d/%d:%d,M%d.%d.%d/%d:%d",
			bDir==-1?'+':'-', abs(min)/60, abs(min)%60,
			dst_cfg->cfg.week.start_time.month+1, dst_cfg->cfg.week.start_time.week_idx+1,
			dst_cfg->cfg.week.start_time.week_day, dst_cfg->cfg.week.start_time.hour,
			dst_cfg->cfg.week.start_time.min,
			dst_cfg->cfg.week.stop_time.month+1, dst_cfg->cfg.week.stop_time.week_idx+1,
			dst_cfg->cfg.week.stop_time.week_day, dst_cfg->cfg.week.stop_time.hour,
			dst_cfg->cfg.week.stop_time.min);
        LOGD("stop min:[%d]\n",dst_cfg->cfg.week.stop_time.min);
	}
	else if(dst_cfg->enable && OVFS_DST_DATE == dst_cfg->mode)
	{

	}

	if(dst_cfg->enable)
	{
	#if (defined(__GLIBC__) && !defined(__UCLIBC__))
		memcpy(chTemp, s_tzfile, sizeof(s_tzfile));
		iWriteLen += sizeof(s_tzfile);

		iWriteLen += snprintf(chTemp+iWriteLen, sizeof(chTemp)-iWriteLen, "%s", chZone);
		iWriteLen += snprintf(chTemp+iWriteLen, sizeof(chTemp)-iWriteLen, "%s", chDst);
	#else
		iWriteLen += snprintf(chTemp, sizeof(chTemp), "%s", chZone);
		iWriteLen += snprintf(chTemp+iWriteLen, sizeof(chTemp)-iWriteLen, "%s", chDst);
	#endif
	}
	else
	{
	#if (defined(__GLIBC__) && !defined(__UCLIBC__))
		memcpy(chTemp, s_tzfile, sizeof(s_tzfile));
		iWriteLen += sizeof(s_tzfile);

		iWriteLen += snprintf(chTemp+iWriteLen, sizeof(chTemp)-iWriteLen, "%s", chZone);;
	#else
		iWriteLen += snprintf(chTemp, sizeof(chTemp), "%s", chZone);
	#endif
	}

	iWriteLen += snprintf(chTemp+iWriteLen, sizeof(chTemp), "\n");

	printf("chTemp=  %s  \n", chTemp);

	print_time();


	FILE *fp = NULL;
	S8 szPath[64] = {0};

#if (defined(__GLIBC__) && !defined(__UCLIBC__))
	snprintf(szPath, sizeof(szPath),"/etc/localtime");
#else
	snprintf(szPath, sizeof(szPath),"/etc/TZ");
#endif

	fp = fopen(szPath,"wb+");
	if (fp == NULL)
	{
		LOGE("open file %s %s\n",szPath, strerror(errno));
		return -1;
	}
	fwrite(chTemp,iWriteLen,1,fp);
	fflush(fp);
	fclose(fp);

#if (defined(__GLIBC__) && !defined(__UCLIBC__))
	usleep(1000000);
#endif

	print_time();

	return 0;

}

S32 ovfs_set_dst_cfg(const ovfs_dst_cfg *dst_cfg)
{
    if(dst_cfg == NULL)
    {
        return -1;
    }

    Common_Lock(m_lock);

	m_dst_cfg = *dst_cfg;
	ovfs_write_utc_file(&m_time_zone_cfg, &m_dst_cfg);

	//writeTmpTz();

    Common_UnLock(m_lock);
    Common_InterSleep_WakeUp(m_sleep_ops);

    return 0;
}

S32 ovfs_set_time_zone_cfg(const ovfs_time_zone_cfg *zone_cfg)
{

    ovfs_time_struct hw_time;
    ovfs_time_struct sys_time;
    COMMON_CLR_ARG(hw_time);
    COMMON_CLR_ARG(sys_time);

    Common_Lock(m_lock);

	m_time_zone_cfg = *zone_cfg;
	ovfs_write_utc_file(&m_time_zone_cfg, &m_dst_cfg);

	//writeTmpTz();

    Common_UnLock(m_lock);
    Common_InterSleep_WakeUp(m_sleep_ops);

    return 0;
}

S32 ovfs_change_ntp_check_status(OVFS_BOOL status)
{
    m_is_need_ntp_check = status;
    return 0;
}

S32 ovfs_set_ntp_cfg(const ovfs_ntp_config *ntp_cfg)
{
    if(ntp_cfg == NULL)
    {
        return -1;
    }

    Common_Lock(m_lock);
    m_ntp_cfg = *ntp_cfg;

    if(ntp_cfg->enable)
    {
        m_is_need_ntp_check = OVFS_TRUE;
    }

    Common_UnLock(m_lock);
    Common_InterSleep_WakeUp(m_sleep_ops);

    return 0;
}

S32 ovfs_set_sys_time(const ovfs_time_struct *sys_time)
{
    if(NULL == sys_time)
    {
        return -1;
    }

    ovfs_time_struct hw_time;
    COMMON_CLR_ARG(hw_time);

    Common_Lock(m_lock);

    ovfs_time_struct set_systime = *sys_time;
    proc_cnt_hw_time(&set_systime, &hw_time, &m_time_zone_cfg, &m_dst_cfg);
    //设置时间时需要设置硬件时钟
    if(is_need_set_time(&hw_time, &set_systime, 1) == OVFS_TRUE)
    {
        S32 ret = set_sys_time(&hw_time, NULL);
        Common_UnLock(m_lock);
        return ret;
    }

    Common_UnLock(m_lock);

    return 0;
}

S32 ovfs_get_sys_time(ovfs_time_struct *sys_time)
{
    time_t tNow = time(NULL);

    return ovfs_utility_linux_to_ovfs_time(tNow, sys_time);
}

S32 ovfs_set_hw_time(const ovfs_time_struct *hw_time, int check)
{
    if(NULL == hw_time)
    {
        return -1;
    }

    ovfs_time_struct sys_time;
    COMMON_CLR_ARG(sys_time);

    Common_Lock(m_lock);

    proc_cnt_sys_time(hw_time, &sys_time, &m_time_zone_cfg, &m_dst_cfg);
    //设置时间时需要设置硬件时钟
    if(check == 1)
    {
        if (is_need_set_time(hw_time, &sys_time, 1) == OVFS_TRUE)
        {
            S32 ret = set_sys_time(hw_time, NULL);
            Common_UnLock(m_lock);
            return ret;
        }
    }
    else
    {
        set_sys_time(hw_time, NULL);
    }

    Common_UnLock(m_lock);

    return 0;
}

S32 ovfs_get_hw_time(ovfs_time_struct *hw_time)
{
    //return get_hwclock_time(hw_time);
    if(NULL == hw_time)
    {
        return -1;
    }

    ovfs_time_struct sys_time;
    COMMON_CLR_ARG(sys_time);

    Common_Lock(m_lock);

    time_t tNow = time(NULL);
    ovfs_utility_linux_to_ovfs_time(tNow, &sys_time);
    proc_cnt_hw_time(&sys_time, hw_time, &m_time_zone_cfg, &m_dst_cfg);

    Common_UnLock(m_lock);

    return 0;
}

static OVFS_VOID load_cfg(ovfs_ntp_config *p_ntp_cfg,
                          ovfs_time_zone_cfg *p_time_zone_cfg, ovfs_dst_cfg *p_dst_cfg)
{
    COMMON_CLR_ARG(m_dst_cfg);
    COMMON_CLR_ARG(m_ntp_cfg);
    COMMON_CLR_ARG(m_time_zone_cfg);

    if (NULL != p_ntp_cfg)
    {
        Common_Copy(&m_ntp_cfg, p_ntp_cfg, sizeof(m_ntp_cfg));
    }

    if (NULL != p_time_zone_cfg)
    {
        Common_Copy(&m_time_zone_cfg, p_time_zone_cfg, sizeof(m_time_zone_cfg));
    }

    if (NULL != p_dst_cfg)
    {
        Common_Copy(&m_dst_cfg, p_dst_cfg, sizeof(m_dst_cfg));
    }

    return;
}

static OVFS_VOID init_system_time()
{
    ovfs_time_struct hw_time;
    ovfs_time_struct sys_time;
    COMMON_CLR_ARG(hw_time);
    COMMON_CLR_ARG(sys_time);

	ovfs_write_utc_file(&m_time_zone_cfg, &m_dst_cfg);

    get_hwclock_time(&hw_time);

	printf("hw_time is:%04d%02d%02d-%02d:%02d:%02d\n", hw_time.year, hw_time.month,hw_time.day,
		hw_time.hour,hw_time.min,hw_time.sec);

    proc_cnt_sys_time(&hw_time, &sys_time, &m_time_zone_cfg, &m_dst_cfg);

    if(is_need_set_time(&hw_time, &sys_time, 5) == OVFS_TRUE)
    {
        set_sys_time(&hw_time, NULL);
    }

    return;
}

S32 ovfs_time_init(ovfs_ntp_config *p_ntp_cfg,
                   ovfs_time_zone_cfg *p_time_zone_cfg, ovfs_dst_cfg *p_dst_cfg)
{
    m_dst_start_time = 0;

    load_cfg(p_ntp_cfg, p_time_zone_cfg, p_dst_cfg);
    init_system_time();
    //m_is_need_ntp_check = OVFS_FALSE;
    m_last_ntp_check_time = 0;

    COMMON_CLR_ARG(m_sleep_ops);
    COMMON_CLR_ARG(m_lock);
    COMMON_CLR_ARG(set_sys_time_lock);
    Common_Lock_Create(&m_lock, NULL);
    Common_InterSleep_Create(&m_sleep_ops);
    Common_Lock_Create(&set_sys_time_lock, NULL);

    m_thread_handle = NULL;
    Common_Thread_Create(&m_thread_handle, "time_check_thread", 1024 * 64,
                         COMMON_THREAD_CREATEFLAG_NORMAL, time_check_thread, NULL);
    return 0;

}

