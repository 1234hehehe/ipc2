#ifdef WIN32
#include <Windows.h>
#else
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#endif

#include "librtc_api.h"

int Rtc_HwClock_SetTime(rtc_time_t *p_hw_time)
{
	int rtc = -1;
	
	if (NULL == p_hw_time)
	{
		return rtc;
	}	
	
	rtc = open("/dev/hi_rtc",O_WRONLY);
	if(rtc >= 0)
	{// UTC
		ioctl(rtc, _IOW('p', 0x0a, rtc_time_t), p_hw_time);
		close(rtc);				
	}
    
    return rtc;	
}

int Rtc_HwClock_GetTime(rtc_time_t *p_hw_time)
{
	int rtc = -1;
	
	if (NULL == p_hw_time)
	{
		return rtc;
	}	
	
	rtc = open("/dev/hi_rtc",O_WRONLY);
	if(rtc >= 0)
	{// UTC
		ioctl(rtc, _IOR('p', 0x09, rtc_time_t), p_hw_time);
		close(rtc);				
	}
    
    return rtc;	
}


