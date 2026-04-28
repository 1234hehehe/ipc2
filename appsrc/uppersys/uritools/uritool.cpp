#include <stdio.h>
#include<string.h>
#include<stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include"cjson.h"
#include"libcommon_api.h"
#include"libmodule_api.h"

static S32 TOOLS_Module_CallFunctions(ModuleHandle_T hModuleHandle, cJSON_Struct *pInParams,
                                      cJSON_Struct **pOutParams, void *pUserData)
{
    return 0;
}

static int Usage(char *name)
{
    printf("%s [put/get/post/delete]  [URI] [IN JSON]\n", name);
    printf("%s tty on/off\n", name);
    exit(-1);
    return 0;
}

static int GetInfo(int argc, char **argv, char **method, char **uri, Common_cJSON_T **inputParam)
{
    if(argc < 3)
        Usage(argv[0]);

    if(strcmp(argv[1], "put") == 0)
        *method = strdup("put");
    else if(strcmp(argv[1], "get") == 0)
        *method = strdup("get");
    else if(strcmp(argv[1], "post") == 0)
        *method = strdup("post");
    else if(strcmp(argv[1], "delete") == 0)
        *method = strdup("delete");
    else if (strcmp(argv[1], "tty") == 0)
        *method = strdup("tty");
    else
        Usage(argv[0]);

    *uri = strdup(argv[2]);

    if(argv[3])
    {
        const char *errStr = NULL;
        *inputParam = Common_cJSON_Parse(argv[3], NULL, &errStr);
        if(*inputParam == NULL)
        {
            printf("JSON prase fail. errStr=%s\n", errStr);
            Usage(argv[0]);
        }
    }
    return 0;
}

static void tty_redirect(int enable)
{
    char *tty_name = NULL;
    int tty = -1, ret = 0;
    tty_name = ttyname(STDOUT_FILENO);
    if (enable > 0)
    {
        tty = open(tty_name, O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
    }
    else
    {
        tty = open("/dev/console", O_RDONLY | O_WRONLY);
        ret = ioctl(tty, TIOCCONS);
    }

    if (ret != 0)
        perror("ioctl failed ");
    close(tty);
}

int main(int argc, char **argv)
{
    char *uri = NULL;
    char *method = NULL;
    Common_cJSON_T *inData = NULL;
    GetInfo(argc, argv, &method, &uri, &inData);

    if (method != NULL && uri != NULL && strcmp(method, "tty") == 0)
    {
        if (strcmp(uri, "on") == 0)
            tty_redirect(1);
        else
            tty_redirect(0);

        free(method);
        free(uri);
        return 0;
    }

    Common_cJSON_T *inParam = Common_cJSON_CreateObject();
    Common_cJSON_T *outParam = NULL;

    Common_cJSON_T *header = Common_cJSON_CreateObject();
    Common_cJSON_AddStringToObject(header, "Method", method);
    Common_cJSON_AddStringToObject(header, "Uri", uri);
    Common_cJSON_AddItemToObject(inParam, "Header", header);
    Common_cJSON_AddItemToObject(inParam, "Data", inData);

    ModuleHandle_T moduleHdl;
    Common_cJSON_T *param = Common_cJSON_CreateObject();
    Common_cJSON_AddStringToObject(param, "SystemName", "ovfs");
    Common_cJSON_AddStringToObject(param, "ModuleName", "Uritool");

    printf("inparam=%s\n", Common_cJSON_Print(inParam, NULL));

    int ret = Module_Init(&moduleHdl, param, NULL, TOOLS_Module_CallFunctions, NULL);
    if(ret != 0)
        printf("module init fail!\n");
    else
        printf("module init ok!\n");

    Common_cJSON_Delete(param);

    ret = Module_CallFunctions(moduleHdl, (cJSON_Struct *)inParam, (cJSON_Struct **)(&outParam), 3000);
    if(ret != 0)
    {
        printf("module call fail! ret=%d\n", ret);
        return -1;
    }

    Common_cJSON_Delete(inParam);

    printf("module call finish!\n");
    if(outParam)
        printf("%s\n", Common_cJSON_Print(outParam, NULL));


    return 0;
}
