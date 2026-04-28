#include <stdlib.h>
#include <string.h>
#include "libcommon_api.h"
#include "libmodule_api.h"
#include "cjson.h"
#include "core_time.h"
#include "ovfs_time.h"

static ModuleHandle_T m_hModuleHandle;

static S32 FillNTPInfo(cJSON_Struct *parentItem, void *cfgData)
{
    char *pStringValue;
    S32 nIntValue;
    ovfs_ntp_config *p_ntp_cfg = NULL;

    if (NULL == cfgData)
    {
        return -1;
    }

    if (parentItem != NULL)
    {
        p_ntp_cfg = (ovfs_ntp_config *)cfgData;

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "NTP/Enable", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_ntp_cfg->enable = (OVFS_BOOL)nIntValue;

            if(p_ntp_cfg->enable == 1)
            {
                ovfs_change_ntp_check_status(OVFS_TRUE);
            }
        }

        pStringValue = NULL;
        Common_Json_GetAttrValue(parentItem, -1, "NTP/Server", NULL, &pStringValue,
                                 NULL, NULL);
        if (NULL != pStringValue)
        {
            Common_Strncpy(p_ntp_cfg->server, pStringValue, sizeof(p_ntp_cfg->server));
        }

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "NTP/Interval", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_ntp_cfg->auto_set_time_interval = nIntValue;
        }
    }
    else
    {
        LOGE("%s: Can't find %s\n", __FUNCTION__, "NTP");
    }

    return 0;
}

static S32 FillTimeZoneInfo(cJSON_Struct *parentItem, void *cfgData)
{
    S32 nIntValue;
    ovfs_time_zone_cfg *p_time_zone_cfg = NULL;

    if (NULL == cfgData)
    {
        return -1;
    }

    p_time_zone_cfg = (ovfs_time_zone_cfg *)cfgData;
    p_time_zone_cfg->zone = OVFS_TIME_ZONE_BEIJING;

    if (parentItem != NULL)
    {
        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "TimeZone/Zone", NULL, NULL,
                                 &nIntValue, NULL);
        if (-1 != nIntValue)
        {
            p_time_zone_cfg->zone = (ovfs_time_zone)nIntValue;
        }

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "TimeZone/EnableBias", NULL, NULL,
                                 &nIntValue, NULL);
        if (-1 != nIntValue)
        {
            p_time_zone_cfg->enable_bias = (OVFS_BOOL)nIntValue;
        }

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "TimeZone/ZoneBias", NULL, NULL,
                                 &nIntValue, NULL);
        if (-1 != nIntValue)
        {
            p_time_zone_cfg->zone_bias = nIntValue;
        }
    }
    else
    {
        LOGE("%s: Can't find %s\n", __FUNCTION__, "TimeZone");
    }

    return 0;
}

static S32 FillDSTInfo(cJSON_Struct *parentItem, void *cfgData)
{
    S32 nIntValue;
    ovfs_dst_cfg *p_dst_cfg = NULL;

    if (NULL == cfgData)
    {
        return -1;
    }

    if (parentItem != NULL)
    {
        p_dst_cfg = (ovfs_dst_cfg *)cfgData;

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "DST/Enable", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_dst_cfg->enable = (OVFS_BOOL)nIntValue;
        }

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "DST/Bias", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_dst_cfg->dst_bias = nIntValue;
        }

        nIntValue = -1;
        Common_Json_GetAttrValue(parentItem, -1, "DST/Mode", NULL, NULL, &nIntValue,
                                 NULL);
        if (-1 != nIntValue)
        {
            p_dst_cfg->mode = (ovfs_dst_mode)nIntValue;
            if (OVFS_DST_WEEK == p_dst_cfg->mode)
            {
                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Month", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.start_time.month = (ovfs_month_enum)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/WeekIdx", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.start_time.week_idx = (ovfs_week_per_month)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/WeekDay", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.start_time.week_day = (ovfs_week_day)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Hour", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.start_time.hour = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Min", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.start_time.min = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Month", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.stop_time.month = (ovfs_month_enum)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/WeekIdx", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.stop_time.week_idx = (ovfs_week_per_month)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/WeekDay", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.stop_time.week_day = (ovfs_week_day)nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Hour", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.stop_time.hour = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Min", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.week.stop_time.min = nIntValue;
                }
            }
            else if (OVFS_DST_DATE == p_dst_cfg->mode)
            {
                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Year", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.year = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Month", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.month = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Day", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.day = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Hour", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.hour = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Min", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.min = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StartTime/Sec", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.start_time.sec = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Year", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.year = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Month", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.month = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Day", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.day = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Hour", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.hour = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Min", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.min = nIntValue;
                }

                nIntValue = -1;
                Common_Json_GetAttrValue(parentItem, -1, "DST/StopTime/Sec", NULL, NULL,
                                         &nIntValue, NULL);
                if (-1 != nIntValue)
                {
                    p_dst_cfg->cfg.date.stop_time.sec = nIntValue;
                }
            }
        }
    }
    else
    {
        LOGE("%s: Can't find %s\n", __FUNCTION__, "DST");
    }

    return 0;
}

static S32 Time_Put_NTPInfo(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    char *pStringValue;
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;
    ovfs_ntp_config ntp_cfg;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    COMMON_CLR_ARG(ntp_cfg);
    ovfs_get_time_cfg(&ntp_cfg, NULL, NULL);

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Enable", NULL, NULL, &nIntValue,
                             NULL);
    if (-1 != nIntValue)
    {
        ntp_cfg.enable = (OVFS_BOOL)nIntValue;
    }

    pStringValue = NULL;
    Common_Json_GetAttrValue(parentItem, -1, "Server", NULL, &pStringValue, NULL,
                             NULL);
    if (NULL != pStringValue)
    {
        Common_Strncpy(ntp_cfg.server, pStringValue, sizeof(ntp_cfg.server));
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Interval", NULL, NULL, &nIntValue,
                             NULL);
    if (-1 != nIntValue)
    {
        ntp_cfg.auto_set_time_interval = nIntValue;
    }

    ovfs_set_ntp_cfg(&ntp_cfg);

    return 0;
}

static S32 Time_Get_NTPInfo(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_ntp_config ntp_cfg;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    COMMON_CLR_ARG(ntp_cfg);
    ovfs_get_time_cfg(&ntp_cfg, NULL, NULL);

    Common_cJSON_AddNumberToObject(out, "Enable", ntp_cfg.enable);
    Common_cJSON_AddStringToObject(out, "Server", ntp_cfg.server);
    Common_cJSON_AddNumberToObject(out, "Interval", ntp_cfg.auto_set_time_interval);

    return 0;
}

static S32 Time_Put_TimeZoneInfo(const char *uriString, const char *condition,
                                 Common_cJSON_T *in, Common_cJSON_T *out)
{
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;
    ovfs_time_zone_cfg time_zone_cfg;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    COMMON_CLR_ARG(time_zone_cfg);
    ovfs_get_time_cfg(NULL, &time_zone_cfg, NULL);

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Zone", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time_zone_cfg.zone = (ovfs_time_zone)nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "EnableBias", NULL, NULL, &nIntValue,
                             NULL);
    if (-1 != nIntValue)
    {
        time_zone_cfg.enable_bias = (OVFS_BOOL)nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "ZoneBias", NULL, NULL, &nIntValue,
                             NULL);
    if (-1 != nIntValue)
    {
        time_zone_cfg.zone_bias = nIntValue;
    }

    ovfs_set_time_zone_cfg(&time_zone_cfg);

    return 0;
}

static S32 Time_Get_TimeZoneInfo(const char *uriString, const char *condition,
                                 Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_time_zone_cfg time_zone_cfg;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    COMMON_CLR_ARG(time_zone_cfg);
    ovfs_get_time_cfg(NULL, &time_zone_cfg, NULL);

    Common_cJSON_AddNumberToObject(out, "Zone", time_zone_cfg.zone);
    Common_cJSON_AddNumberToObject(out, "EnableBias", time_zone_cfg.enable_bias);
    Common_cJSON_AddNumberToObject(out, "ZoneBias", time_zone_cfg.zone_bias);

    return 0;
}

static S32 Time_Put_DSTInfo(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;
    ovfs_dst_cfg dst_cfg;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    COMMON_CLR_ARG(dst_cfg);
    ovfs_get_time_cfg(NULL, NULL, &dst_cfg);

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Enable", NULL, NULL, &nIntValue,
                             NULL);
    if (-1 != nIntValue)
    {
        dst_cfg.enable = (OVFS_BOOL)nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Bias", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        dst_cfg.dst_bias = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Mode", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        dst_cfg.mode = (ovfs_dst_mode)nIntValue;
        if (OVFS_DST_WEEK == dst_cfg.mode)
        {
            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Month", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.start_time.month = (ovfs_month_enum)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/WeekIdx", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.start_time.week_idx = (ovfs_week_per_month)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/WeekDay", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.start_time.week_day = (ovfs_week_day)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Hour", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.start_time.hour = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Min", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.start_time.min = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Month", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.stop_time.month = (ovfs_month_enum)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/WeekIdx", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.stop_time.week_idx = (ovfs_week_per_month)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/WeekDay", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.stop_time.week_day = (ovfs_week_day)nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Hour", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.stop_time.hour = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Min", NULL, NULL, &nIntValue,
                                     NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.week.stop_time.min = nIntValue;
            }
        }
        else if (OVFS_DST_DATE == dst_cfg.mode)
        {
            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Year", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.year = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Month", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.month = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Day", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.day = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Hour", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.hour = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Min", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.min = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StartTime/Sec", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.start_time.sec = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Year", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.year = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Month", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.month = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Day", NULL, NULL, &nIntValue,
                                     NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.day = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Hour", NULL, NULL,
                                     &nIntValue, NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.hour = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Min", NULL, NULL, &nIntValue,
                                     NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.min = nIntValue;
            }

            nIntValue = -1;
            Common_Json_GetAttrValue(parentItem, -1, "StopTime/Sec", NULL, NULL, &nIntValue,
                                     NULL);
            if (-1 != nIntValue)
            {
                dst_cfg.cfg.date.stop_time.sec = nIntValue;
            }
        }
    }

    ovfs_set_dst_cfg(&dst_cfg);

    return 0;
}

static S32 Time_Get_DSTInfo(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_dst_cfg dst_cfg;
    Common_cJSON_T *tmp = NULL;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    COMMON_CLR_ARG(dst_cfg);
    ovfs_get_time_cfg(NULL, NULL, &dst_cfg);

    Common_cJSON_AddNumberToObject(out, "Enable", dst_cfg.enable);
    Common_cJSON_AddNumberToObject(out, "Mode", dst_cfg.mode);
    Common_cJSON_AddNumberToObject(out, "Bias", dst_cfg.dst_bias);
    if (OVFS_DST_WEEK == dst_cfg.mode)
    {
        tmp = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "StartTime", tmp);

        Common_cJSON_AddNumberToObject(tmp, "Month", dst_cfg.cfg.week.start_time.month);
        Common_cJSON_AddNumberToObject(tmp, "WeekIdx",
                                       dst_cfg.cfg.week.start_time.week_idx);
        Common_cJSON_AddNumberToObject(tmp, "WeekDay",
                                       dst_cfg.cfg.week.start_time.week_day);
        Common_cJSON_AddNumberToObject(tmp, "Hour", dst_cfg.cfg.week.start_time.hour);
        Common_cJSON_AddNumberToObject(tmp, "Min", dst_cfg.cfg.week.start_time.min);

        tmp = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "StopTime", tmp);

        Common_cJSON_AddNumberToObject(tmp, "Month", dst_cfg.cfg.week.stop_time.month);
        Common_cJSON_AddNumberToObject(tmp, "WeekIdx",
                                       dst_cfg.cfg.week.stop_time.week_idx);
        Common_cJSON_AddNumberToObject(tmp, "WeekDay",
                                       dst_cfg.cfg.week.stop_time.week_day);
        Common_cJSON_AddNumberToObject(tmp, "Hour", dst_cfg.cfg.week.stop_time.hour);
        Common_cJSON_AddNumberToObject(tmp, "Min", dst_cfg.cfg.week.stop_time.min);
    }
    else if (OVFS_DST_DATE == dst_cfg.mode)
    {
        tmp = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "StartTime", tmp);

        Common_cJSON_AddNumberToObject(tmp, "Year", dst_cfg.cfg.date.start_time.year);
        Common_cJSON_AddNumberToObject(tmp, "Month", dst_cfg.cfg.date.start_time.month);
        Common_cJSON_AddNumberToObject(tmp, "Day", dst_cfg.cfg.date.start_time.day);
        Common_cJSON_AddNumberToObject(tmp, "Hour", dst_cfg.cfg.date.start_time.hour);
        Common_cJSON_AddNumberToObject(tmp, "Min", dst_cfg.cfg.date.start_time.min);
        Common_cJSON_AddNumberToObject(tmp, "Sec", dst_cfg.cfg.date.start_time.sec);

        tmp = Common_cJSON_CreateObject();
        Common_cJSON_AddItemToObject(out, "StopTime", tmp);

        Common_cJSON_AddNumberToObject(tmp, "Year", dst_cfg.cfg.date.stop_time.year);
        Common_cJSON_AddNumberToObject(tmp, "Month", dst_cfg.cfg.date.stop_time.month);
        Common_cJSON_AddNumberToObject(tmp, "Day", dst_cfg.cfg.date.stop_time.day);
        Common_cJSON_AddNumberToObject(tmp, "Hour", dst_cfg.cfg.date.stop_time.hour);
        Common_cJSON_AddNumberToObject(tmp, "Min", dst_cfg.cfg.date.stop_time.min);
        Common_cJSON_AddNumberToObject(tmp, "Sec", dst_cfg.cfg.date.stop_time.sec);
    }

    return 0;
}

static S32 Time_Put_SysTime(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_time_struct time;
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    COMMON_CLR_ARG(time);

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Year", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.year = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Month", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.month = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Day", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.day = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Hour", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.hour = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Min", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.min = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Sec", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.sec = nIntValue;
    }

    ovfs_set_sys_time(&time);

    return 0;
}

static S32 Time_Get_SysTime(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_time_struct time;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    COMMON_CLR_ARG(time);
    ovfs_get_sys_time(&time);

    Common_cJSON_AddNumberToObject(out, "Year", time.year);
    Common_cJSON_AddNumberToObject(out, "Month", time.month);
    Common_cJSON_AddNumberToObject(out, "Day", time.day);
    Common_cJSON_AddNumberToObject(out, "Hour", time.hour);
    Common_cJSON_AddNumberToObject(out, "Min", time.min);
    Common_cJSON_AddNumberToObject(out, "Sec", time.sec);

    return 0;
}

static S32 Time_Put_UTCTime(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_time_struct time;
    S32 nIntValue;
    cJSON_Struct *parentItem = NULL;

    if (NULL == in || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    parentItem = (cJSON_Struct *)in;

    COMMON_CLR_ARG(time);

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Year", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.year = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Month", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.month = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Day", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.day = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Hour", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.hour = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Min", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.min = nIntValue;
    }

    nIntValue = -1;
    Common_Json_GetAttrValue(parentItem, -1, "Sec", NULL, NULL, &nIntValue, NULL);
    if (-1 != nIntValue)
    {
        time.sec = nIntValue;
    }

    ovfs_set_hw_time(&time, 1);

    return 0;
}

static S32 Time_Get_UTCTime(const char *uriString, const char *condition,
                            Common_cJSON_T *in, Common_cJSON_T *out)
{
    ovfs_time_struct time;

    if (NULL == out || NULL == uriString)
    {
        LOGE("param is NULL.\n");
        return -1;
    }

    COMMON_CLR_ARG(time);
    ovfs_get_hw_time(&time);

    Common_cJSON_AddNumberToObject(out, "Year", time.year);
    Common_cJSON_AddNumberToObject(out, "Month", time.month);
    Common_cJSON_AddNumberToObject(out, "Day", time.day);
    Common_cJSON_AddNumberToObject(out, "Hour", time.hour);
    Common_cJSON_AddNumberToObject(out, "Min", time.min);
    Common_cJSON_AddNumberToObject(out, "Sec", time.sec);

    return 0;
}

static S32 Core_Time_SaveCfg()
{
    Common_cJSON_T *saveObj = NULL;
    Common_cJSON_T *tmp = NULL;

    saveObj = Common_cJSON_CreateObject();

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(saveObj, "NTP", tmp);
    Time_Get_NTPInfo("NTP", NULL, NULL, tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(saveObj, "TimeZone", tmp);
    Time_Get_TimeZoneInfo("TimeZone", NULL, NULL, tmp);

    tmp = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(saveObj, "DST", tmp);
    Time_Get_DSTInfo("DST", NULL, NULL, tmp);

    Core_Time_SaveConfig(m_hModuleHandle, (cJSON_Struct *)saveObj);

    return 0;
}

static S32 Core_Time_PraseInputJson(Common_cJSON_T *inputData, S8 **method,
                                    S8 **uri, Common_cJSON_T **inData)
{
    Common_cJSON_T *header = Common_cJSON_GetObjectItem(inputData, "Header");
    if(header == NULL)
    {
        LOGE("Get header fail!\n");
        return -1;
    }

    Common_cJSON_T *tmp = NULL;
    tmp = Common_cJSON_GetObjectItem(header, "Method");
    if(tmp == NULL)
    {
        LOGE("Can't found method!\n");
        return -1;
    }
    if(method)
    {
        *method = tmp->valuestring;
    }

    tmp = Common_cJSON_GetObjectItem(header, "Uri");
    if(tmp == NULL)
    {
        LOGE("Can't found Uri.\n");
        return -1;
    }
    if(uri)
    {
        *uri = tmp->valuestring;
    }


    tmp = Common_cJSON_GetObjectItem(inputData, "Data");
    if(inData)
    {
        *inData = tmp;
    }

    return 0;
}

static S32 Core_Time_GetUriAndQue(const S8 *srcUri, S8 **uriStr,
                                  S8 **conditionStr)
{
    S8 *tmp = (S8 *)strstr(srcUri, "?");
    if(tmp)
    {
        char buff[256];
        memset(buff, 0, sizeof(buff));
        memcpy(buff, srcUri, (int)(tmp - srcUri));
        *uriStr         = Common_StrDup(buff, __FUNCTION__, __LINE__);
        *conditionStr   = Common_StrDup(tmp + 1, __FUNCTION__, __LINE__);
    }
    else
    {
        *uriStr         = Common_StrDup((S8 *)srcUri, __FUNCTION__, __LINE__);
        *conditionStr   = NULL;
    }

    return 0;
}

static Common_cJSON_T *Core_Time_GenerateOutParam(S32 retCode,
        Common_cJSON_T *outData)
{
    Common_cJSON_T *root = Common_cJSON_CreateObject();
    Common_cJSON_T *header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root, "Header", header);

    Common_cJSON_AddNumberToObject(header, "Code", retCode);
    if(retCode != 0)
    {
        Common_cJSON_AddStringToObject(header, "Decribe", "Operation fail.");
    }

    if(outData)
    {
        Common_cJSON_AddItemToObject(root, "Data", outData);
    }

    return root;
}
/*
static Common_cJSON_T* Core_Time_GenerateInParam(const char* method,const char* uriPath,const char* condition,Common_cJSON_T* inData)
{
    if(uriPath == NULL)
    {
        return NULL;
    }

    Common_cJSON_T* root = Common_cJSON_CreateObject();
    Common_cJSON_T* header = Common_cJSON_CreateObject();
    Common_cJSON_AddItemToObject(root,"Header",header);

    Common_cJSON_AddStringToObject(header,"Method",method);
    char uriBuff[256];
    memset(uriBuff,0,sizeof(uriBuff));
    int uriPathLen = strlen(uriPath);
    memcpy(uriBuff,uriPath,uriPathLen);
    if(condition)
    {
        uriBuff[uriPathLen] = '?';
        memcpy(uriBuff+uriPathLen+1,condition,strlen(condition));
    }

    Common_cJSON_AddStringToObject(header,"Uri",uriBuff);
    Common_cJSON_AddItemToObject(root,"Data",inData);

    return root;
}
*/

/*
Uri:/Core/Time
Uri:/Core/UtcTime
Method:Get/Put
Data:{Year=,Month=,Day=,Hour=,Min=,Sec=,Zone=,IsDst=,DstOffset=, WDay=,MDay=,YDay=}
*/
S32 Core_Time_CallFunctions(ModuleHandle_T hModuleHandle,
                            cJSON_Struct *pInParams, cJSON_Struct **pOutParams)
{
    // 返回 0表示是time相关功能，否则 不是
#if 0
    char *out = NULL;
    LOGI("recv call input:%s \n",
         out = Common_cJSON_PrintUnformatted((Common_cJSON_T *)pInParams, NULL));
    if(out)
    {
        Common_Free(out, __FUNCTION__, __LINE__);
    }
#endif
    char *method            = NULL;
    char *uri               = NULL;
    Common_cJSON_T *inData  = NULL;
    int ret = Core_Time_PraseInputJson((Common_cJSON_T *)pInParams, &method, &uri,
                                       &inData);
    if(ret != 0)
    {
        LOGE("inparam parse fail!\n");
        return -1;
    }

    if(method == NULL)
    {
        LOGE("method is NULL!\n");
        return -1;
    }

    char *uriString     = NULL;
    char *uriCondition  = NULL;
    Core_Time_GetUriAndQue(uri, &uriString, &uriCondition);

    if (Common_StrniCmp(uriString, (char *)"/Core/Time", strlen("/Core/Time")) != 0)
    {
        if(uriString)
        {
            Common_Free(uriString, __FUNCTION__, __LINE__);
        }

        if(uriCondition)
        {
            Common_Free(uriCondition, __FUNCTION__, __LINE__);
        }

        return -1;
    }

    if (0 == Common_StriCmp((char *)"/Core/Time", uriString)
            && 0 == Common_StriCmp((char *)"get", method))
    {
        cJSON_Struct *pOutJson = NULL;
        cJSON_Struct *pNode, *pNode1;

        pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
        if (pOutJson != NULL)
        {
            Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                     0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                     NULL, 0, 0);
            Common_Json_SetAttrValue(pOutJson, -1, "Data", Common_Json_Type_Object, NULL, 0,
                                     0);

            pNode = Common_Json_SetAttrValue(pOutJson, -1, "Data/ResList",
                                             Common_Json_Type_Array, NULL, 0, 0);

            Common_Json_SetAttrValue(pNode, 0, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/NTP", 0, 0);
            Common_Json_SetAttrValue(pNode, 0, "/Label", Common_Json_Type_String, "NTP", 0,
                                     0);
            Common_Json_SetAttrValue(pNode, 0, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 0, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

            Common_Json_SetAttrValue(pNode, 1, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/TimeZone", 0, 0);
            Common_Json_SetAttrValue(pNode, 1, "/Label", Common_Json_Type_String,
                                     "TimeZone", 0, 0);
            Common_Json_SetAttrValue(pNode, 1, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 1, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

            Common_Json_SetAttrValue(pNode, 2, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/DST", 0, 0);
            Common_Json_SetAttrValue(pNode, 2, "/Label", Common_Json_Type_String, "DST", 0,
                                     0);
            Common_Json_SetAttrValue(pNode, 2, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 2, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

            Common_Json_SetAttrValue(pNode, 3, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/SysTime", 0, 0);
            Common_Json_SetAttrValue(pNode, 3, "/Label", Common_Json_Type_String, "SysTime",
                                     0, 0);
            Common_Json_SetAttrValue(pNode, 3, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 3, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

            Common_Json_SetAttrValue(pNode, 4, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/UTCTime", 0, 0);
            Common_Json_SetAttrValue(pNode, 4, "/Label", Common_Json_Type_String, "UTCTime",
                                     0, 0);
            Common_Json_SetAttrValue(pNode, 4, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 4, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

            Common_Json_SetAttrValue(pNode, 4, "/Uri", Common_Json_Type_String,
                                     "/Core/Time/AllTime", 0, 0);
            Common_Json_SetAttrValue(pNode, 4, "/Label", Common_Json_Type_String, "AllTime",
                                     0, 0);
            Common_Json_SetAttrValue(pNode, 4, "/Describe", Common_Json_Type_String, "None",
                                     0, 0);
            pNode1 = Common_Json_SetAttrValue(pNode, 4, "/Method", Common_Json_Type_Array,
                                              NULL, 0, 0);
            Common_Json_SetAttrValue(pNode1, 0, NULL, Common_Json_Type_String, "Get", 0, 0);
            Common_Json_SetAttrValue(pNode1, 1, NULL, Common_Json_Type_String, "Put", 0, 0);

        }

        if (pOutJson != NULL)
        {
            if (pOutParams != NULL)
            {
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }
        }
        else if(pOutParams != NULL)
        {
            pOutJson = Common_Json_New(NULL, Common_Json_Type_Object, NULL, 0, 0);
            if (pOutJson != NULL)
            {
                Common_Json_SetAttrValue(pOutJson, -1, "Header", Common_Json_Type_Object, NULL,
                                         0, 0);
                Common_Json_SetAttrValue(pOutJson, -1, "Header/Code", Common_Json_Type_Number,
                                         NULL, -1, 0);
                *pOutParams = pOutJson;
                pOutJson = NULL;
            }

        }
        Common_Json_Delete(pOutJson);
        pOutJson = NULL;

        if(uriString)
        {
            Common_Free(uriString, __FUNCTION__, __LINE__);
        }

        if(uriCondition)
        {
            Common_Free(uriCondition, __FUNCTION__, __LINE__);
        }

        return 0;
    }

    ret = 0;

    Common_cJSON_T *outData = Common_cJSON_CreateObject();
    if(Common_StriCmp(method, (char *)"get") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/Time/NTP") == 0)
        {
            Time_Get_NTPInfo(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/TimeZone") == 0)
        {
            Time_Get_TimeZoneInfo(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/DST") == 0)
        {
            Time_Get_DSTInfo(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/SysTime") == 0)
        {
            Time_Get_SysTime(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/UTCTime") == 0)
        {
            Time_Get_UTCTime(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/AllTime") == 0)
        {
            Common_cJSON_T *ntp = Common_cJSON_CreateObject();
            Time_Get_NTPInfo(uriString, uriCondition, inData, ntp);
            Common_cJSON_T *tz = Common_cJSON_CreateObject();
            Time_Get_TimeZoneInfo(uriString, uriCondition, inData, tz);
            Common_cJSON_T *dst = Common_cJSON_CreateObject();
            Time_Get_DSTInfo(uriString, uriCondition, inData, dst);
            Common_cJSON_T *st = Common_cJSON_CreateObject();
            Time_Get_SysTime(uriString, uriCondition, inData, st);
            Common_cJSON_T *utc = Common_cJSON_CreateObject();
            Time_Get_UTCTime(uriString, uriCondition, inData, utc);
            Common_cJSON_AddItemToObject(outData, "NTP", ntp);
            Common_cJSON_AddItemToObject(outData, "TimeZone", tz);
            Common_cJSON_AddItemToObject(outData, "DST", dst);
            Common_cJSON_AddItemToObject(outData, "SysTime", st);
            Common_cJSON_AddItemToObject(outData, "UTCTime", utc);
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"put") == 0)
    {
        if (Common_StriCmp(uriString, (char *)"/Core/Time/NTP") == 0)
        {
            Time_Put_NTPInfo(uriString, uriCondition, inData, outData);
            Core_Time_SaveCfg();
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/TimeZone") == 0)
        {
            Time_Put_TimeZoneInfo(uriString, uriCondition, inData, outData);
            Core_Time_SaveCfg();
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/DST") == 0)
        {
            Time_Put_DSTInfo(uriString, uriCondition, inData, outData);
            Core_Time_SaveCfg();
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/SysTime") == 0)
        {
            Time_Put_SysTime(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/UTCTime") == 0)
        {
            Time_Put_UTCTime(uriString, uriCondition, inData, outData);
        }
        else if (Common_StriCmp(uriString, (char *)"/Core/Time/AllTime") == 0)
        {

            Common_cJSON_T *item = NULL;
            item = (Common_cJSON_T *)Common_Json_GetAttrValue(
                inData, -1, "NTP", NULL, NULL, NULL, NULL);

            if (item != NULL)
                Time_Put_NTPInfo(uriString, uriCondition, item, outData);

            item = (Common_cJSON_T *)Common_Json_GetAttrValue(
                inData, -1, "TimeZone", NULL, NULL, NULL, NULL);
            if (item != NULL)
                Time_Put_TimeZoneInfo(uriString, uriCondition, item, outData);

            item = (Common_cJSON_T *)Common_Json_GetAttrValue(
                inData, -1, "DST", NULL, NULL, NULL, NULL);
            if (item != NULL)
                Time_Put_DSTInfo(uriString, uriCondition, item, outData);

            item = (Common_cJSON_T *)Common_Json_GetAttrValue(
                inData, -1, "SysTime", NULL, NULL, NULL, NULL);
            if (item != NULL)
                Time_Put_SysTime(uriString, uriCondition, item, outData);

            item = (Common_cJSON_T *)Common_Json_GetAttrValue(
                inData, -1, "UTCTime", NULL, NULL, NULL, NULL);
            if (item != NULL)
                Time_Put_UTCTime(uriString, uriCondition, item, outData);

            Core_Time_SaveCfg();
        }
        else
        {
            ret = -1;
        }
    }
    else if(Common_StriCmp(method, (char *)"post") == 0)
    {
        //ret = NetWork_Rest_Post(uriString,uriCondition,inData,outData);
        ret = -1;
    }
    else if(Common_StriCmp(method, (char *)"delete") == 0)
    {
        //ret = NetWork_Rest_Delete(uriString,uriCondition,inData,outData);
        ret = -1;
    }
    else
    {
        LOGE("Unknow method=%s\n", method);
        ret = -1;
    }

    if(outData->child == NULL)
    {
        Common_cJSON_Delete(outData);
        outData = NULL;
    }

    *pOutParams = Core_Time_GenerateOutParam(ret, outData);

    if(uriString)
    {
        Common_Free(uriString, __FUNCTION__, __LINE__);
    }

    if(uriCondition)
    {
        Common_Free(uriCondition, __FUNCTION__, __LINE__);
    }

    return ret;
}

S32 Core_Time_Init(ModuleHandle_T hModuleHandle, cJSON_Struct *pConfig)
{
    ovfs_ntp_config     m_ntp_cfg;
    ovfs_time_zone_cfg  m_time_zone_cfg;
    ovfs_dst_cfg        m_dst_cfg;

    COMMON_CLR_ARG(m_dst_cfg);
    COMMON_CLR_ARG(m_ntp_cfg);
    COMMON_CLR_ARG(m_time_zone_cfg);
    COMMON_CLR_ARG(m_hModuleHandle);

    m_hModuleHandle = hModuleHandle;
    FillNTPInfo(pConfig, (void *)(&m_ntp_cfg));
    FillTimeZoneInfo(pConfig, (void *)(&m_time_zone_cfg));
    FillDSTInfo(pConfig, (void *)(&m_dst_cfg));

    ovfs_time_init(&m_ntp_cfg, &m_time_zone_cfg, &m_dst_cfg);

    Core_Time_SaveCfg();

    return 0;
}



