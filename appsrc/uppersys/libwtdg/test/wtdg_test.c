

#include"stdio.h"
#include"stdlib.h"
#include"string.h"
//#include"libcommon_api.h"
#include"ovfs_wtdg.h"

void UsagePrint(const char* name)
{
    printf("%s [start/stop/set/get/feed] [times]\n",name);
    return;
}

int main(int argc,char* argv[])
{
    if(argc < 2)
    {
        UsagePrint(argv[0]);
        return -1;
    }

    if(strcmp(argv[1],"start") ==0)
        return Wtdg_Start();
    else if(strcmp(argv[1],"stop") ==0)
        return Wtdg_Stop();
    else if(strcmp(argv[1],"set") ==0)
    {
        if(argc < 3)
        {
              UsagePrint(argv[0]);
              return -1;
        }
        return Wtdg_SetTime(atoi(argv[2]));
    }
    else if(strcmp(argv[1],"get") ==0)
    {
        int retTime = 0;
        int ret =Wtdg_GetTime(&retTime);
        printf("%d\n",retTime);
        return ret;
    }
    else if(strcmp(argv[1],"feed") ==0)
    {
        return Wtdg_Feed();
    }
    else
    {
        printf("cmd unknow.\n");
        return -1;
    }
    return 0;
}
                                                  