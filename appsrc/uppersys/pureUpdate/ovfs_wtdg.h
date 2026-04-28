#ifndef __WATCH_DOG_H_
#define __WATCH_DOG_H_
#ifdef __cplusplus 
extern "C" {
#endif
int Wtdg_Start(void);
int Wtdg_Stop(void);
int Wtdg_SetTime(int timeOuts);
int Wtdg_GetTime(int* timeOuts);
int Wtdg_Feed(void);

#ifdef __cplusplus 
}
#endif

#endif
                                                  
