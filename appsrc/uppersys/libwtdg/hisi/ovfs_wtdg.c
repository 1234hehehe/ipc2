#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include <sys/ioctl.h>
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
#include <linux/types.h>
#include <linux/watchdog.h>
#else//ifdef PLATFORM_JZT30
#include"watchdog.h"
#endif//ifdef PLATFORM_JZT30

#include"ovfs_wtdg.h"

//#include"libcommon_api.h"
//#include"watchdog.h"

#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
static int wdt_keep_alive(int fd,int dummy)
{
	int ret = -1;
    ret = ioctl(fd, WDIOC_KEEPALIVE, &dummy);
	if (0 != ret) {
		printf("err(%s,%d): %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;
}

static int wdt_enable(int fd)
{
	int ret = -1;
	int flags = 0;
	flags = WDIOS_ENABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flags);
	if (0 != ret) {
		printf("err(%s,%d): %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;

}

static int wdt_disable(int fd)
{
	int ret = -1;
	int flags = 0;
	flags = WDIOS_DISABLECARD;
	ret = ioctl(fd, WDIOC_SETOPTIONS, &flags);
	if (0 != ret) {
		printf("err(%s,%d): %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;

}

static int wdt_set_timeout(int fd,int to)
{
	int ret = -1;
    int timeout = to;
    ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
	if (0 != ret) {
		printf("err(%s,%d): %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return 0;

}

static int wdt_get_timeout(int fd)
{
	int ret = -1;
    int timeout = 0;
    ret = ioctl(fd, WDIOC_GETTIMEOUT, &timeout);
	if (0 != ret) {
		printf("err(%s,%d): %s\n", __func__, __LINE__, strerror(errno));
		return -1;
	}
	return timeout;
}
#endif

static int WtdgOps(int opsType,int times,int* outTime) // 0: start 1:stop 2:set 3:get 4:feed
{
    int fd = open("/dev/watchdog",O_RDWR);
    if(fd < 0 )
    {
        printf("open wtdg ko fail:%s\n","/dev/watchdog");
        return -1;
    }

	int value=0;
    
    //int ret = 0;
    if(opsType == 0)
    {
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
		 wdt_enable(fd);
#else
         ioctl(fd, WDIOC_START, &value);
#endif
    }
    else if(opsType == 1)
    {
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
         wdt_disable(fd);
#else
         ioctl(fd, WDIOC_STOP, &value);   
#endif
    }
    else if(opsType == 2)
    {
         int setVaild = times/2;
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
         wdt_set_timeout(fd,setVaild);
#else
         ioctl(fd, WDIOC_SETTIMEOUT, &setVaild);
#endif
    }
    else if(opsType == 3)
    {
        if(outTime)
        {
            int getVaild = 0;
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
            getVaild = wdt_get_timeout(fd);
#else
            ioctl(fd, WDIOC_GETTIMEOUT, &getVaild);
#endif
            *outTime = getVaild*2;
        }
    }
    else if(opsType == 4)
    {
#if defined PLATFORM_JZT30 || defined PLATFORM_JZT32 || defined PLATFORM_JZT33 || PLATFORM_JZT40 || PLATFORM_JZT41
         wdt_keep_alive(fd,value);
#else
         ioctl(fd, WDIOC_KEEPALIVE, &value);
#endif
    }
			
    close(fd);
    return 0;
}

int Wtdg_Start(void)
{
    return WtdgOps(0,0,NULL);
}

int Wtdg_Stop(void)
{
    return WtdgOps(1,0,NULL);    
}

int Wtdg_SetTime(int timeOuts)
{
    return WtdgOps(2,timeOuts,NULL);
}

int Wtdg_GetTime(int* timeOuts)
{
    return WtdgOps(3,0,timeOuts);
}

int Wtdg_Feed(void)
{
    
    return WtdgOps(4,0,NULL);
}

