#ifndef __LIBRTC_API_H__
#define __LIBRTC_API_H__


#ifdef WIN32
#define LIBRTC_API __declspec(dllexport)
#else
#define LIBRTC_API
#endif



#ifdef __cplusplus
extern "C"{
#endif
	
typedef struct {
        unsigned int  year;//年，是多少就多少，不需要1900做处理
        unsigned int  month;//月，是多少就多少，不需要加1减1做处理
        unsigned int  date;
        unsigned int  hour;
        unsigned int  minute;
        unsigned int  second;
        unsigned int  weekday;
} rtc_time_t;	
	
	LIBRTC_API int Rtc_HwClock_SetTime(rtc_time_t *p_hw_time);
	LIBRTC_API int Rtc_HwClock_GetTime(rtc_time_t *p_hw_time);



#ifdef __cplusplus
};
#endif
#endif


