#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef WIN32
#include <Windows.h>
#include <time.h>
#else
#include<sys/time.h>
#endif
#include "rtsp_common.h"
void Ants_WaitTime(int nSec,int MicroSec)
{
#ifdef WIN32

    unsigned long t;

    t = ((unsigned long)nSec)*1000+(((unsigned long)MicroSec)/1000);
    Sleep(t);
#else
    struct timespec req,rem;

    req.tv_sec = (time_t)nSec;
    req.tv_nsec = ((long)MicroSec)*1000;
    nanosleep(&req,&rem);
#endif

}
#ifdef WIN32
static  unsigned __int64 CalculateMicroseconds(unsigned __int64 performancecount,unsigned __int64 performancefrequency)
{
    unsigned __int64 f = performancefrequency;
    unsigned __int64 a = performancecount;
    unsigned __int64 b = a/f;
    unsigned __int64 c = a%f; // a = b*f+c => (a*1000000)/f = b*1000000+(c*1000000)/f

    return b*1000000ui64+(c*1000000ui64)/f;
}
#endif


#define ANTS_RTSPSERVER_PAYLOADTYPE_G711U             (0)
#define ANTS_RTSPSERVER_PAYLOADTYPE_G711A             (8)
#define ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG             (26)
#define ANTS_RTSPSERVER_PAYLOADTYPE_H264             (96)
#define ANTS_RTSPSERVER_PAYLOADTYPE_G726_16             (97)
#define ANTS_RTSPSERVER_PAYLOADTYPE_H265             (98)
#define ANTS_RTSPSERVER_PAYLOADTYPE_AAC             (99)
#define ANTS_RTSPSERVER_PAYLOADTYPE_APP             (106)
#define ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB             (110) // name="AntsComb"
typedef enum
{

    ANTS_RTSP_STREAM_H264 = 0,
    ANTS_RTSP_STREAM_MPEG4,
    ANTS_RTSP_STREAM_JPEG,
    ANTS_RTSP_STREAM_G711A,
    ANTS_RTSP_STREAM_G711U,
    ANTS_RTSP_STREAM_G726,
    ANTS_RTSP_STREAM_AAC,
    ANTS_RTSP_STREAM_H265,
    ANTS_RTSP_STREAM_AntsComb,
    ANTS_RTSP_STREAM_PS,
    ANTS_RTSP_STREAM_APP = 20,
    ANTS_RTSP_STREAM_UNKNOW = 0x10000, //低16位为对应的编码号
}ANTS_RTSP_STREAM_TYPE;

 int Ants_rtsp_Payload2StreamType(int PayloadType)
 {
    if(PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711U)
        return ANTS_RTSP_STREAM_G711U;
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G711A)
    {
        return ANTS_RTSP_STREAM_G711A;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG)
    {
        return ANTS_RTSP_STREAM_JPEG;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H264)
    {
        return ANTS_RTSP_STREAM_H264;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_G726_16)
    {
        return ANTS_RTSP_STREAM_G726;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_AAC)
    {
        return ANTS_RTSP_STREAM_AAC;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_H265)
    {
        return ANTS_RTSP_STREAM_H265;
    }
    else if (PayloadType == ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB)
    {
        return ANTS_RTSP_STREAM_AntsComb;
    }
    else
    {
        return ANTS_RTSP_STREAM_UNKNOW;
    }
 }
 int Ants_rtsp_Stream2PayloadType(int nStreamType)
{
    if(nStreamType == ANTS_RTSP_STREAM_G711U)
        return ANTS_RTSPSERVER_PAYLOADTYPE_G711U;
    else if (nStreamType == ANTS_RTSP_STREAM_G711A)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_G711A;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_JPEG)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_MJPEG;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_H264)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_H264;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_G726)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_G726_16;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_AAC)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_AAC;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_H265)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_H265;
    }
    else if (nStreamType == ANTS_RTSP_STREAM_AntsComb)
    {
        return ANTS_RTSPSERVER_PAYLOADTYPE_ANTSCOMB;
    }
    else
    {
        return -1;
    }
}
int Ants_rtsp_CurrentTime(long *pSec,long *puSec)
{
#ifdef WIN32
#if 0
    static int inited = 0;
    static unsigned __int64 microseconds, initmicroseconds;
    static LARGE_INTEGER performancefrequency;

    unsigned __int64 emulate_microseconds, microdiff;
    SYSTEMTIME systemtime;
    FILETIME filetime;

    LARGE_INTEGER performancecount;

    QueryPerformanceCounter(&performancecount);

    if(!inited){
        inited = 1;
        QueryPerformanceFrequency(&performancefrequency);
        GetSystemTime(&systemtime);
        SystemTimeToFileTime(&systemtime,&filetime);
        microseconds = ( ((unsigned __int64)(filetime.dwHighDateTime) << 32) + (unsigned __int64)(filetime.dwLowDateTime) ) / 10ui64;
        microseconds-= 11644473600000000ui64; // EPOCH
        initmicroseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);
    }

    emulate_microseconds = CalculateMicroseconds(performancecount.QuadPart, performancefrequency.QuadPart);

    microdiff = emulate_microseconds - initmicroseconds;


    if (pSec)
    {
        *pSec = (uint32_t)((microseconds + microdiff) / 1000000ui64);
    }
    if (puSec)
    {
        *puSec = ((uint32_t)((microseconds + microdiff) % 1000000ui64));
    }
#else
	FILETIME tFileTime ;
	unsigned __int64 microseconds = 0;
	GetSystemTimeAsFileTime(&tFileTime);
	microseconds = tFileTime.dwHighDateTime;
	microseconds = (microseconds << 32) + tFileTime.dwLowDateTime;
	microseconds -= 116444736000000000UL;
	microseconds /= 10;

	if (pSec)
	{
		*pSec = (uint32_t)((microseconds) / 1000000ui64);
	}
	if (puSec)
	{
		*puSec = ((uint32_t)((microseconds) % 1000000ui64));
	}

#endif

#else
    struct timeval tv;
    int nRet;

    nRet = gettimeofday(&tv,0);
    if (pSec)
    {
        *pSec = tv.tv_sec;
    }
    if (puSec)
    {
        *puSec = tv.tv_usec;
    }
    return nRet;
#endif
    return 0;
}

int Ants_rtsp_GetSysRunTime(unsigned int *pSec,unsigned int *puSec)
{
#ifdef WIN32
    ULONGLONG  llms = GetTickCount(),llsec;
    llsec = llms / 1000;
    if(pSec)
    {
        *pSec = llsec;

    }
    if (puSec)
    {
        *puSec = (llms - llsec * 1000) * 1000;
    }
    return 0;
#else
struct timespec tsp;
if(0 == clock_gettime(CLOCK_MONOTONIC,&tsp))
{
    if (pSec)
    {
        *pSec = tsp.tv_sec;
    }
    if (puSec)
    {
        *puSec = tsp.tv_nsec / 1000;
    }
    return 0;

}
return -1;

#endif
}
#define  ANTS_RTSP_RTP_NTPTIMEOFFSET 2208988800UL
int Ants_rtsp_GetNTPTime(unsigned int  *pmsw,unsigned int  *plsw)
{
    uint32_t msw,sec;
    uint32_t lsw,usec;
    double x;
    Ants_rtsp_CurrentTime((long *)&sec,(long *)&usec);
    msw = sec+ANTS_RTSP_RTP_NTPTIMEOFFSET;
    x = usec/1000000.0;
    x *= (65536.0*65536.0);
    lsw = (uint32_t)x;
    if (pmsw)
    {
        *pmsw = msw;
    }
    if (plsw)
    {
        *plsw = lsw;
    }

    return 0;
}

struct tm * Ants_rtsp_LocalTime_r(time_t *t,struct tm *pTm)
{
#ifdef WIN32
    int nRet;
    nRet = localtime_s(pTm,t);
    if (nRet == 0)
    {
        return pTm;
    }
    else
    {
        return NULL;
    }
#else
    return localtime_r(t,pTm);
#endif
}

int Ants_rtsp_GetRTP2NTPTime(unsigned int sec,unsigned int usec,unsigned int  *pmsw,unsigned int  *plsw)
{
    uint32_t msw;
    uint32_t lsw;
    double x;
    msw = sec+ANTS_RTSP_RTP_NTPTIMEOFFSET;
    x = usec/1000000.0;
    x *= (65536.0*65536.0);
    lsw = (uint32_t)x;
    if (pmsw)
    {
        *pmsw = msw;
    }
    if (plsw)
    {
        *plsw = lsw;
    }

    return 0;
}

int Ants_rtsp_GetNTP2RTPTime(unsigned int  msw,unsigned int  lsw,unsigned int *psec,unsigned int *pusec)
{
    uint32_t sec;
    uint32_t usec;
    double x;


    sec = msw - ANTS_RTSP_RTP_NTPTIMEOFFSET;
    x = lsw;
    x /= (65536.0*65536.0);
    x *= 1000000.0;
    usec  = (uint32_t)x;

    if (psec)
    {
        *psec = sec;
    }
    if (pusec)
    {
        *pusec = usec;
    }

    return 0;
}
char *Ants_strndup(char *pstr,unsigned int nMaxLen)
{
    int olen;
    char *pret;
    if (pstr == NULL)
    {
        return NULL;
    }
    olen = strlen(pstr);
    if (olen == 0)
    {
        return NULL;
    }
    if (olen < nMaxLen)
    {
        nMaxLen = olen;
    }
    pret = (char *)new char[(nMaxLen + 1)];
    if (pret == NULL)
    {
        return NULL;
    }
    strncpy(pret,pstr,nMaxLen);
    pret[nMaxLen] = 0;
    return pret;

}
void Ants_strFree(char *pstr)
{
    if (pstr != NULL)
    {
        delete [] pstr;
    }
}

const char* strstri(const char* str, const char* subStr)
{
    int len = strlen(subStr);
    if(len == 0)
    {
        return NULL;
    }

    while(*str)
    {
        if(strnicmp(str, subStr, len) == 0)
        {
            return str;
        }
        ++str;
    }
    return NULL;
}

int Ants_Rtsp_Com_ReleaseUrl(RTSP_COMMON_URL_T *pResult)
{
    int i;
    if (pResult == NULL)
    {
        return 0;
    }
    if (pResult->pProtoType != NULL)
    {
        Ants_strFree(pResult->pProtoType);
        pResult->pProtoType = NULL;
    }
    if (pResult->pPort != NULL)
    {
        Ants_strFree(pResult->pPort);
        pResult->pPort = NULL;
    }
    if (pResult->pPath != NULL)
    {
        Ants_strFree(pResult->pPath);
        pResult->pPath = NULL;
    }
    if (pResult->pUserName != NULL)
    {
        Ants_strFree(pResult->pUserName);
        pResult->pUserName = NULL;
    }
    if (pResult->pPassword != NULL)
    {
        Ants_strFree(pResult->pPassword);
        pResult->pPassword = NULL;
    }
    if (pResult->pAddress != NULL)
    {
        Ants_strFree(pResult->pAddress);
        pResult->pAddress = NULL;
    }
    if (pResult->pPathName != NULL)
    {
        Ants_strFree(pResult->pPathName);
        pResult->pPathName = NULL;
    }
    if (pResult->pFileName != NULL)
    {
        Ants_strFree(pResult->pFileName);
        pResult->pFileName = NULL;
    }
    if (pResult->pParameters != NULL)
    {
        Ants_strFree(pResult->pParameters);
        pResult->pParameters = NULL;
    }
    if (pResult->pIgnore != NULL)
    {
        Ants_strFree(pResult->pIgnore);
        pResult->pIgnore = NULL;
    }
    for (i = 0; i < ANTS_RTSP_MAX_QUERY_NUM;i++)
    {
        if (pResult->pQueryName[i] != NULL)
        {
            Ants_strFree(pResult->pQueryName[i]);
            pResult->pQueryName[i] = NULL;
        }
        if (pResult->pQueryValue[i] != NULL)
        {
            Ants_strFree(pResult->pQueryValue[i]);
            pResult->pQueryValue[i] = NULL;
        }
    }
    return 0;
}
int Ants_Rtsp_Com_ParseUrl(char *pUrl,RTSP_COMMON_URL_T *pResult)
{//rtsp://admin:admin@192.168.1.199/ipcamera.live
    char *pstart,*pstop,*pstr,*pstr1,*pTail;
    int len;
    if (pUrl == NULL || pResult == NULL)
    {
        return -1;
    }
    memset(pResult,0,sizeof(RTSP_COMMON_URL_T));
    //
    pTail = pUrl + strlen(pUrl);
    pstart = pUrl;
    pstop = strstr(pUrl,"://");
    if(pstop == NULL)
        return -1;

    pResult->pProtoType = Ants_strndup(pstart,pstop - pstart);
    pstart = pstop + 3;

    //IP 及dnsname
    pstr = strchr(pstart,'/');
    if (pstr != NULL)
    {
        pstop = pstr;
    }
    else
    {
        pstop = pTail;
    }

    pstr = strchr(pstart,'@');
    if (pstr != NULL && pstr <= pstop)
    {// 用户名及密码
        //处理用户及密码
        pstr1 = strchr(pstart,':');
        if (pstr1 != NULL)
        {
            pResult->pUserName = Ants_strndup(pstart,pstr1 - pstart);
            pResult->pPassword = Ants_strndup(pstr1 + 1,pstr - pstr1 - 1);
        }
        else
        {
             pResult->pUserName = Ants_strndup(pstart,pstr - pstart);
        }


        pstart = pstr + 1;
    }
	if (pstart[0] == '[')
	{
		pResult->IsIPv6 = 1;
		pstr = strchr(pstart,']');
		if (pstr != NULL)
		{
			pResult->pAddress = Ants_strndup(pstart + 1,pstr - pstart - 1);
			pstart = pstr + 1;
		}
		else
		{
			return -1;
		}
	}
    pstr = strchr(pstart,':');
    if (pstr != NULL && pstr <= pstop)
    {
        // 带端口
		if(!pResult->IsIPv6)
		{
			pResult->pAddress = Ants_strndup(pstart,pstr - pstart);
		}
        pResult->pPort = Ants_strndup(pstr + 1,pstop - (pstr + 1));
    }
    else
    {
        // 不带端口
		if(!pResult->IsIPv6)
		{
			pResult->pAddress = Ants_strndup(pstart,pstop - pstart);
		}
        pResult->pPort = Ants_strndup("554",-1);
    }
    pstart = pstop;
    // path
    if (pstart < pTail)
    {
        pResult->pPath = Ants_strndup(pstart,-1);
    }
    if (pResult->pAddress == NULL)
    {
        return -1;
    }
    if (pResult->pProtoType == NULL)
    {
        return -1;
    }
    if (pResult->pProtoType == NULL)
    {
        return -1;
    }

    return 0;
}
#if 0
int Ants_Rtsp_Com_ParseUrl1(char *pUrl,RTSP_COMMON_URL_T *pResult)
{//rtsp://admin:admin@192.168.1.199/ipcamera.live
   // char tembuf[256];
    int step,idx,pos,firstpos,pathEndPos;
    int pathflag, nQueryIdx;
    int ndot;
    char c;
    int bAt = 0;
    if (pUrl == NULL || pResult == NULL)
    {
        return -1;
    }
    memset(pResult,0,sizeof(RTSP_COMMON_URL_T));
    step = 0;
    idx = 0;
    pos = 0;
    firstpos = 0;
    pathflag = 0;
    nQueryIdx = 0;
    pathEndPos = 0;
    ndot = 0;
    bAt = NULL != strchr(pUrl,'@');
    while(1)
    {
        c = pUrl[idx];
        if (c == 0)
        {
            break;
        }
        if (step == 0)
        {//proto
            if (c == ':')
            {
                if (pUrl[idx + 1] != '/' ||
                    pUrl[idx + 2] != '/')
                {

                    return -1;
                }
                pos = idx;
                if (pos <= firstpos)
                {//协议名不存在
                    return -2;
                }
                if (pos - firstpos >= 10)
                {
                    //协议名太长
                    return -3;
                }
                strncpy(pResult->sProtoType,&pUrl[firstpos],pos - firstpos);
                idx += 3;
                firstpos = idx;
                step = bAt ? 10001:1;//转到处理IP
                continue;
            }
            idx++;
            continue;
        }
        else if(step == 10001)
        {
            if (c == ':')
            {
                pos = idx;

                pResult->pUserName = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                if (pResult->pUserName == NULL)
                {
                    //IP地址分配失败
                    return -11;
                }


                 step = 10002;//转到密码

                idx++;
                firstpos = idx;

                continue;
            }
            if (c == '@')
            {
                pResult->pUserName = Ants_strndup(&pUrl[firstpos],idx - firstpos);
                idx++;
                firstpos = idx;
                continue;
            }

            idx++;
            continue;
        }
        else if(step == 10002)
        {
            if (c == '@')
            {
                pos = idx;

                pResult->pPassword = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                if (pResult->pPassword == NULL)
                {
                    //IP地址分配失败
                    return -11;
                }


                step = 1;//转到IP

                idx++;
                firstpos = idx;

                continue;
            }


            idx++;
            continue;
        }
        else if (step == 1)
        {//处理IP/dnsname
            if (c == ':' || c== '/')
            {
                pos = idx;
                if (pos - firstpos <= 0)
                {
                    //未找到地址
                    return -10;
                }
                pResult->pAddress = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                if (pResult->pAddress == NULL)
                {
                    //IP地址分配失败
                    return -11;
                }
                if (!pResult->bDnsName)
                {
                    if(ndot!=3)
                    {
                        pResult->bDnsName = 1;
                    }
                }
                if (c == ':')
                {//
                    step = 2;//转到处理端口
                }
                else
                {
                    step = 3;//转到处理文件路径
                }
                idx++;
                firstpos = idx;

                continue;
            }
            if (c == '.')
            {
                ndot++;
            }
            else if (c < '0' || c > '9')
            {
                pResult->bDnsName = 1;
            }
            idx++;
            continue;
        }
        else if (step == 2)
        {//处理端口
            if (c == '/')
            {
                pos = idx;
                if (pos - firstpos <= 0)
                {
                    //未找到端口
                    return -20;
                }
                if (pos - firstpos > 5)
                {
                    return -21;
                }
                strncpy(pResult->sPort,&pUrl[firstpos],pos - firstpos);
                idx++;
                firstpos = idx;
                step = 3;//转到处理路径
                continue;
            }
            if (c < '0' || c > '9')
            {
                //非法端口
                return -21;
            }
            idx++;
            continue;
        }
        else if (step == 3)
        {//处理路径
            if (c == '/'|| c == '?' || c== '#'|| c == 0)
            {

                if (c == '/')
                {
                    pathflag = 1;
                    pathEndPos = idx;
                    idx++;
                    continue;
                }
                pos = idx;
                if (pathflag)
                {//检测到路径
                    if (pathEndPos -firstpos <= 0)
                    {
                        return -30;
                    }
                    pResult->pPathName = Ants_strndup(&pUrl[firstpos],pathEndPos - firstpos);
                    if (pResult->pPathName == NULL)
                    {
                        return -31;
                    }
                    firstpos = pathEndPos + 1;
                }

                if (pos -firstpos > 0)
                {
                    pResult->pFileName = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                    if (pResult->pFileName == NULL)
                    {
                        return -33;
                    }
                }


                if(c == '?')
                {
                    step = 4;//处理后面查询query，
                }
                else if (c == '#')
                {
                    step = 6;
                }
                idx++;
                firstpos = idx;

                continue;
            }
            idx++;
            continue;
        }
        else if (step == 4)
        {// ?query
            if (c == '=')
            {
               pos = idx;
               if (pos - firstpos <= 0)
               {
                   return -40;
               }
               pResult->pQueryName[nQueryIdx] = Ants_strndup(&pUrl[firstpos],pos - firstpos);
               if (pResult->pQueryName[nQueryIdx] == NULL)
               {
                   return -41;
               }
               idx++;
               firstpos = idx;
               step = 5;//处理值
               continue;
            }
            idx++;
            continue;

        }
        else if (step == 5)
        {
            if (c == '&' || c == '#')
            {
                pos = idx;
                if (pos - firstpos <= 0)
                {
                    return -50;
                }
                pResult->pQueryValue[nQueryIdx] = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                if (pResult->pQueryValue[nQueryIdx] == NULL)
                {
                    return -51;
                }
                nQueryIdx++;
                idx++;
                firstpos = idx;
                if(c == '#')
                {
                    step = 6;//处理#fragment
                }
                else if (c == '&')
                {
                    step = 4;
                }
                else
                {
                    step = 200;
                }
                continue;
            }
            idx++;
            continue;
        }
        else if (step == 6)
        {// #fragment
        }
        idx++;

    }
    pos = idx;
    if (pos - firstpos > 0)
    {
        if (step == 0)
        {
            if (pos - firstpos >= 10)
            {
                //协议名太长
                return -3;
            }
            strncpy(pResult->sProtoType,&pUrl[firstpos],pos - firstpos);
        }
        else if (step == 1)
        {
            pResult->pAddress = Ants_strndup(&pUrl[firstpos],pos - firstpos);
            if (pResult->pAddress == NULL)
            {
                //IP地址分配失败
                return -11;
            }
        }
        else if (step == 2)
        {
            if (pos - firstpos > 5)
            {
                return -21;
            }
            strncpy(pResult->sPort,&pUrl[firstpos],pos - firstpos);
        }
        else if (step == 3)
        {
            if (pathflag)
            {//检测到路径
                if (pathEndPos -firstpos <= 0)
                {
                    return -30;
                }
                pResult->pPathName = Ants_strndup(&pUrl[firstpos],pathEndPos - firstpos);
                if (pResult->pPathName == NULL)
                {
                    return -31;
                }
                firstpos = pathEndPos + 1;
            }

            if (pos -firstpos > 0)
            {
                pResult->pFileName = Ants_strndup(&pUrl[firstpos],pos - firstpos);
                if (pResult->pFileName == NULL)
                {
                    return -33;
                }
            }

        }
        else if (step == 4)
        {
            pResult->pQueryName[nQueryIdx] = Ants_strndup(&pUrl[firstpos],pos - firstpos);
            pResult->pQueryValue[nQueryIdx] = NULL;
            if(pResult->pQueryName[nQueryIdx] == NULL)
                return - 45;
            return 0;
        }
        else if (step == 5)
        {
            pResult->pQueryValue[nQueryIdx] = Ants_strndup(&pUrl[firstpos],pos - firstpos);
            if (pResult->pQueryValue[nQueryIdx] == NULL)
            {
                return -51;
            }
            nQueryIdx++;
        }
        else if (step == 6)
        {
            pResult->pFragment = Ants_strndup(&pUrl[firstpos],pos - firstpos);
            if (pResult->pFragment == NULL)
            {
                return -61;
            }
        }
        else
        {
            pResult->pIgnore = Ants_strndup(&pUrl[firstpos],pos - firstpos);
            if (pResult->pIgnore == NULL)
            {
                return -301;
            }
        }
    }

    return 0;
}
#endif
//base64编码表
static char base64_alphabet[]= {
    'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P',
    'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f',
    'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v',
    'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};
    /**********************************************************
    *函数名：Base64Decode
    *功能：对base64编码进行解码
    *入口参数：base64code:base64编码序列指针 base64length:序列长度
    *出口参数：解码后的字节序列指针
    *说明：用户调用此函数以后必须将返回指针所对内存释放,用delete
    *     方法,base64length参数必须为4的倍数
    **********************************************************/
  int Ants_Rtsp_IsBase64(char *base64code, unsigned int base64length)
  {
      int i;
      unsigned int len;
      char c;
      if (base64code == NULL)
      {
          return 0;
      }
      len = strlen(base64code);
      if (base64length < len)
      {
          len = base64length;
      }
      for (i = 0; i < len;i++)
      {
          c = base64code[i];
          if ((c >= 'A' && c <= 'Z') ||
              (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') ||
              c == '+' ||
              c == '/' ||
              c == '=')
          {
          }
          else
          {
              return 0;
          }
      }
      return 1;

  }
char* Ants_Rtsp_Base64Decode(char *base64code, unsigned int base64length,unsigned int *ResultSize)
{
	char *buffertemp;
	char *buffer;
	char num=0;
	buffer=new char[base64length/4*3+1];
	if (buffer == NULL)
	{
		*ResultSize = 0;
		return NULL;
	}
	buffertemp=buffer;

	char temp[4];
	int *ptemp = (int*)temp;
	unsigned int i=0;
	unsigned int m=0;
	for(i=0; i<base64length;){
		*ptemp = 0;
		char *chtemp=temp;
		for(m=0;m<4&&i<base64length;i++){

			if(((*base64code)>='a')&&((*base64code)<='z')){
				*chtemp=(*base64code)-'a'+26;
			}else if(((*base64code)>='A')&&((*base64code)<='Z')){
				*chtemp=(*base64code)-'A';
			}else if(((*base64code)>='0')&&((*base64code)<='9')){
				*chtemp=(*base64code)-'0'+52;
			}else if(*base64code=='/'){
				*chtemp=63;
			}else if(*base64code=='+'){
				*chtemp=62;
			}else if (*base64code=='='){
				*chtemp='=';
			}else{
				base64code++;
				continue;
			}
			m++;
			chtemp++;
			base64code++;
		}
		*buffertemp++=(temp[0]<<2)+((temp[1]>>4));
		*buffertemp++=(temp[1]<<4)+((temp[2]>>2));
		*buffertemp++=(temp[2]<<6)+((temp[3]));
	}

    if(temp[2]=='=')
        num++;
    if(temp[3]=='=')
        num++;
    *(buffertemp-num)=0;
	if (ResultSize)
	{
		 *ResultSize = buffertemp - buffer - num;
	}

	return buffer;
}
    /**********************************************************
    *函数名：Base64Encode
    *功能：对给定字节序列进行base64编码
    *入口参数：base64code:要编码的字节序列指针 base64length:要编码
    *      字节序列的长度
    *出口参数：编码后的字节序列指针
    *说明：用户调用此函数以后必须将返回指针所对内存释放,用delete
    *     方法
    **********************************************************/
char* Ants_Rtsp_Base64Encode(char *base64code, unsigned int base64length)
{
     int i;
     unsigned long m;
     char *p;
     unsigned char *s;
    int n = base64length;
    char *t = NULL;
    s = (unsigned char *)base64code;

    if (!t)
        t = (char*)new char[(n + 2) / 3 * 4 + 1];
    if (!t)
        return NULL;
    if (t == NULL)
    {
        return NULL;
    }

    p = t;
    t[0] = '\0';
    if (!s)
        return p;
    for (; n > 2; n -= 3, s += 3)
    { m = s[0];
    m = (m << 8) | s[1];
    m = (m << 8) | s[2];
    for (i = 4; i > 0; m >>= 6)
        t[--i] = base64_alphabet[m & 0x3F];
    t += 4;
    }
    t[0] = '\0';
    if (n > 0)
    { m = 0;
    for (i = 0; i < n; i++)
        m = (m << 8) | *s++;
    for (; i < 3; i++)
        m <<= 8;
    for (i++; i > 0; m >>= 6)
        t[--i] = base64_alphabet[m & 0x3F];
    for (i = 3; i > n; i--)
        t[i] = '=';
    t[4] = '\0';
    }
    return p;
}

char *Ants_Rtsp_GetPayloadTypeName(int nPayloadType)
{
    int nCnt,i;
    nCnt = sizeof(m_tPayloadTypes)/sizeof(PAYLOADTYPEMAP_T);
    for (i = 0; i < nCnt; i++)
    {
        if (m_tPayloadTypes[i].nPayloadType == nPayloadType)
        {
            return m_tPayloadTypes[i].Name;
        }
    }
    return NULL;
}
int Ants_Rtsp_GetPayloadClockRate(int nPayloadType)
{
    int nCnt,i;
    nCnt = sizeof(m_tPayloadTypes)/sizeof(PAYLOADTYPEMAP_T);
    for (i = 0; i < nCnt; i++)
    {
        if (m_tPayloadTypes[i].nPayloadType == nPayloadType)
        {
            return m_tPayloadTypes[i].clockRate;
        }
    }
    return 90000;

}
