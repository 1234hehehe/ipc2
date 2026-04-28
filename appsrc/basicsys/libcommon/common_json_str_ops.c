/*
 * ants_cfg_operator.c
 *
 *  Created on: 2016.7.26
 *  Author: longzhou
 */
#ifndef WIN32
#include "libcommon_api.h"
#include "libcommon_struct.h"
#include "common_json_str_ops.h"
#define EC_CFG_BASE                                               -2000
#define EC_CFG_PARAM_INVALID                            (EC_CFG_BASE-1)
#define EC_CFG_PARAM_INVALID_STR                        "Input param Invalid"
#define EC_CFG_CREATE_DEF_FILE_FAIL                    (EC_CFG_BASE-2)
#define EC_CFG_CREATE_DEF_FILE_FAIL_STR                "Create default configture file fail!"
#define EC_CFG_SAVE_FILE_FAIL                            (EC_CFG_BASE-3)
#define EC_CFG_SAVE_FILE_FAIL_STR                        "Save configture to file fail!"
#define EC_CFG_ITEM_MIS_MATCH                            (EC_CFG_BASE-4)
#define EC_CFG_ITEM_MIS_MATCH_STR                        "Mismatch  item!Can't get/set item."
#define EC_CFG_SUB_SUCCESS                            (EC_CFG_BASE-5)
#define EC_CFG_SUB_SUCCESS_STR                        "A part item success, the others fail."
#define EC_CFG_TOO_MANY_CFG_FILE                        (EC_CFG_BASE-6)
#define EC_CFG_TOO_MANY_CFG_FILE_STR                    "Too many cfg file."
#define EC_CFG_PRASE_FILE_FAIL                            (EC_CFG_BASE-7)
#define EC_CFG_PRASE_FILE_FAIL_STR                        "Prase file to json obj fail."
#define EC_CFG_HAS_INITED                                (EC_CFG_BASE-8)
#define EC_CFG_HAS_INITED_STR                            "Cfg module has alreadly init."
#define EC_CFG_NO_INITE                                   (EC_CFG_BASE-9)
#define EC_CFG_NO_INITE_STR                            "Cfg module must init first."
#define EC_CFG_HAVE_NO_RIGHT                                (EC_CFG_BASE-10)
#define EC_CFG_HAVE_NO_RIGHT_STR                        "Have no right on cfg file."
#define EC_CFG_SET_PARENT_ITEM_FAIL                    (EC_CFG_BASE-11)
#define EC_CFG_SET_PARENT_ITEM_FAIL_STR                    "Can't set parent item."
#define EC_CFG_ARRAY_NOT_SUPPORT                            (EC_CFG_BASE-12)
#define EC_CFG_ARRAY_NOT_SUPPORT_STR                    "Array not support in cfg"
#define EC_CFG_OBJ_WITH_NO_CHILD                            (EC_CFG_BASE-13)
#define EC_CFG_OBJ_WIEH_NO_CHILD_STR                            "Object type with no child"
#define EC_CFG_ARRAY_ONLY_SUPPORT_LEAF_NODE                   (EC_CFG_BASE-14)
#define EC_CFG_ARRAY_ONLY_SUPPORT_LEAF_NODE_STR               "array operation supported only on leaf node when get/del item"

static Common_cJSON_T* CreateRecursionObj(const char* getString,Common_cJSON_T** lastObj)
{
    if (getString == NULL)
    {
        LOGE("%s : string is NULL\n", __FUNCTION__);
        return NULL;
    }
    
    char* savePtr = NULL;
    char* token = NULL;
    char* tmpString = Common_StrDup((char *)getString,__FUNCTION__,__LINE__);
    Common_cJSON_T* ret = NULL;
    Common_cJSON_T* root = NULL;
    Common_cJSON_T* tmp = NULL;
    token = strtok_r(tmpString,OPER_SEPARATOR, &savePtr);
    char strBuffer[128];
    char* lastToken = NULL;
    
    while (token != NULL)
    {
        memset(strBuffer,0,sizeof(strBuffer));
        if( (lastToken = strstr(token,"=")) != NULL)
        {
            memcpy(strBuffer,token, (int)(abs(lastToken-token)));
        }
        else
        {
            strcpy(strBuffer,token);
        }

        if(root == NULL)
        {
            ret = Common_cJSON_CreateObject();
            if(!ret)
                break;
            tmp = ret;
            root = ret;
        }
        ret = Common_cJSON_CreateObject();
        if(!ret)
        {
            Common_cJSON_Delete(root);
            root = NULL;
            break; 
        }
        Common_cJSON_AddItemToObject(tmp,strBuffer,ret);
        tmp = ret;
             
        token = strtok_r(NULL, OPER_SEPARATOR, &savePtr);
    }
    Common_Free(tmpString,__FUNCTION__,__LINE__);
    *lastObj = tmp;
    return root;
}

static int RecursionPrintCjsonObj(Common_cJSON_T* cjonsObj, char** labels)
{
    static int s_curOpsLevel = 0;
    
    if(cjonsObj == NULL)
    {
        
        LOGE("%s : string is NULL\n", __FUNCTION__);
        return -1;
    }
    
    Common_cJSON_T* opsObj = cjonsObj;
    
    do{
        if(opsObj->child != NULL)
        {
            if(opsObj->string != NULL)
            {
                if(labels[s_curOpsLevel])
                {
                    Common_Free(labels[s_curOpsLevel],__FUNCTION__,__LINE__);
                    labels[s_curOpsLevel]  = NULL;
                }

                labels[s_curOpsLevel] = Common_StrDup(opsObj->string,__FUNCTION__,__LINE__);
                s_curOpsLevel++;
            }

            RecursionPrintCjsonObj(opsObj->child,labels);

            if(opsObj->string != NULL)
            {
                if(labels[s_curOpsLevel])
                {
                    Common_Free(labels[s_curOpsLevel],__FUNCTION__,__LINE__);
                    labels[s_curOpsLevel]  = NULL;
                }
                s_curOpsLevel--;
            }
        }
        else
        {
            char printBuffer[256];
            memset(printBuffer,0,sizeof(printBuffer));
            int curLen = 0;
            int i = 0;
            for(i = 0 ; i < MAX_LABEL_LEVEL_SUPPORT; i++)
            {
                if(labels[i] == NULL)
                    break;
                
                if(i == 0)
                {
                    curLen = strlen(printBuffer);
                    snprintf(printBuffer + curLen,sizeof(printBuffer) - curLen,"%s",labels[i]);
                }
                else
                {                           
                   curLen = strlen(printBuffer);
                   snprintf(printBuffer + curLen,sizeof(printBuffer) - curLen,".%s",labels[i]);
                }
            }
            
            curLen = strlen(printBuffer);
            snprintf(printBuffer + curLen,sizeof(printBuffer) - curLen,".%s",opsObj->string);         
            curLen = strlen(printBuffer);
             
            if(opsObj->type == Common_cJSON_Number)
            {
                snprintf(printBuffer + curLen,sizeof(printBuffer) - curLen,"    =   %d",opsObj->valueint);                        
            }
            else if(opsObj->type == Common_cJSON_String)
            {
                snprintf(printBuffer + curLen,sizeof(printBuffer) - curLen,"    =   %s",opsObj->valuestring);                        
            }
            else
            {
                LOGI(" nuknow type=%d  name=%s\n",opsObj->type,opsObj->string );
            }
            
            LOGI("%s\n",printBuffer);
         }
        
        if(opsObj)
            opsObj = opsObj->next;

    }while(opsObj!= NULL);

    return 0;
}


int JsonOper_JsonPrint(Common_cJSON_T* printObj)
{
    if(printObj == NULL)
    {
        LOGE("%s: param is NULL!\n",__FUNCTION__);
        return -1;
    }

    char* labels[MAX_LABEL_LEVEL_SUPPORT];
    memset(labels,0,sizeof(labels));

    RecursionPrintCjsonObj(printObj,(char**)&labels);

    int i = 0;
    for(i = 0 ; i < MAX_LABEL_LEVEL_SUPPORT; i++)
    {
        if(labels[i])
        {
            Common_Free(labels[i],__FUNCTION__,__LINE__);
            labels[i] = NULL;
        }
    }
    return 0;
}

Common_cJSON_T* JsonOper_StringToJsonGet(const char* getString)
{
    if(getString == NULL)
        return NULL;
    
    char* endPtr = strstr(getString, "=");
    if(endPtr != NULL)
    {   
        LOGE("%s: param can't have \"=\"!\n",__FUNCTION__);
        return NULL;
    }

    if(strstr(getString,"["))
    {
        LOGE("Not support array operation\n");
        return NULL;
    }
          
    Common_cJSON_T* lastObj = NULL;
    Common_cJSON_T* root = CreateRecursionObj(getString,&lastObj);
    if(root == NULL)
    {
        LOGE("root is NULL.\n");
        return NULL;
    }
    
    lastObj->type = Common_cJSON_Number;
    lastObj->valueint = 0;
    lastObj->valuedouble = 0;

    return root;
}

Common_cJSON_T* JsonOper_StringToJsonSet( const char* getString,int valueType)
{
    char* endPtr = strstr(getString, "=");
    if (endPtr == NULL)
    {
        LOGE("%s: param must have \"=\"!\n",__FUNCTION__);
        return NULL;
    }

    if (strstr(getString,"["))
    {
        LOGE("Not support array operation\n");
        return NULL;
    }
     
    Common_cJSON_T* lastObj = NULL;
    Common_cJSON_T* root = CreateRecursionObj(getString,&lastObj);

    if (root == NULL)
    {
        LOGE("root is NULL.\n");
        return NULL;
    }

    if (valueType == 0)
    {
        if (lastObj->valuestring)
        {
             Common_Free(lastObj->valuestring,__FUNCTION__,__LINE__);
             lastObj->valuestring = NULL;
        }

        lastObj->type = Common_cJSON_Number;
        lastObj->valueint = atoi(endPtr+1);
         lastObj->valuedouble = atoi(endPtr+1);
    }
    else if (valueType == 1)
    {
        lastObj->type = Common_cJSON_String;
        lastObj->valuestring = Common_StrDup(endPtr+1,__FUNCTION__,__LINE__);
    }
    else
    {   
        Common_cJSON_Delete(root);
        return NULL;
    }

    return root;
}

Common_cJSON_T *JsonOper_GetObjectItemByPath(Common_cJSON_T *object, const char *path)
{
    if (object == NULL || path == NULL)
        return NULL;

    char *tppath = Common_StrDup((char *)path,__FUNCTION__,__LINE__), *p = NULL;
    char *tmp = tppath;
    Common_cJSON_T *c = object;
    do
    {
        p = strsep(&tmp, ".");
        if (p)
        {
            c = Common_cJSON_GetObjectItem(c, p);
        }
        else
        {
            c = Common_cJSON_GetObjectItem(c, tmp);
        }
    } while (c && p && tmp);

    if (tppath)
        Common_Free(tppath,__FUNCTION__,__LINE__);
    return c;
}

Common_cJSON_T *JsonOper_GetObjectItemWithCheck(Common_cJSON_T *object, const char *string)
{
    if (object)
        return Common_cJSON_GetObjectItem(object,string);

    return NULL;
}

static int MgrSetCfgItemValue(Common_cJSON_T*dest, Common_cJSON_T* src)
{
    if(dest == NULL || src == NULL )
    {
        return -1;
    }
    
    Common_cJSON_T* destObj = dest;
    Common_cJSON_T* srcObj = src;
  
    if(destObj->type == Common_cJSON_Object)                   // Cant't set a parent item;
        return -1;  
        
    if(destObj->type == srcObj->type)
    { 
        if(srcObj->type == Common_cJSON_String)
        {
            if(strcmp(destObj->valuestring,srcObj->valuestring) != 0)
            {
                Common_Free(destObj->valuestring,__FUNCTION__,__LINE__);
                destObj->valuestring = Common_StrDup(srcObj->valuestring,__FUNCTION__,__LINE__);
            }
            else
                return -111;
        }
        else if(srcObj->type == Common_cJSON_Number)
        {  
            if( destObj->valueint != srcObj->valueint)
            {
                destObj->valueint = srcObj->valueint;
                destObj->valuedouble = srcObj->valuedouble;
            }
            else
                return -111;

        }
        else if (srcObj->type == Common_cJSON_Double)
        {
            if( destObj->valuedouble != srcObj->valuedouble)
            {
                destObj->valuedouble = srcObj->valuedouble;
            }
            else
                return -111;
        }
        else if (srcObj->type == Common_cJSON_Array)
        {
            Common_cJSON_T *parent = destObj->pParent;
            Common_cJSON_DeleteItemFromObject(parent,destObj->string);
            Common_cJSON_AddItemToObject(parent,srcObj->string,Common_cJSON_Duplicate(srcObj,1));
        }
        else        // True False NULL type are the same;
            return -111;
    }
    else
    {       
        if( (destObj->type == Common_cJSON_String) && destObj->valuestring)
        {   
            Common_Free(destObj->valuestring,__FUNCTION__,__LINE__); 
            destObj->valuestring = NULL;
        }
        
        if(srcObj->type==Common_cJSON_String)
        {
            destObj->valuestring = Common_StrDup(srcObj->valuestring,__FUNCTION__,__LINE__);
        }
        
        destObj->valueint = srcObj->valueint;
        
        destObj->valuedouble = srcObj->valuedouble;
        destObj->type = srcObj->type;
    }

    return 0;
}


static int MgrMatchSubItem(Common_cJSON_T* cfgFileObj, Common_cJSON_T* cfgParam,int checkNext,Common_cJSON_T* parent_note,int type)
{
    if(cfgParam == NULL || cfgFileObj == NULL )
    {   
        LOGE("param invaild cfgPararm=%d cfgFileObj=%d\n",cfgParam!=NULL,cfgFileObj!=NULL);
        return -1;
    }

    Common_cJSON_T* opsParamObj = cfgParam;
    Common_cJSON_T* fileObj = cfgFileObj;
    Common_cJSON_T* tmp = NULL;
    int ret = 0;

    if((opsParamObj->type == Common_cJSON_Array) && Common_cJSON_GetObjectItem(fileObj,opsParamObj->string))
    {
        Common_cJSON_DeleteItemFromObject(fileObj,opsParamObj->string);
        Common_cJSON_AddItemToObject(fileObj,opsParamObj->string,Common_cJSON_Duplicate(opsParamObj,1));    
        return 0;
    }
    
   //printf("dst=%s   src=%s  dstType=%d srcType=%d src.child=%d dst.next=%d\n",cfgFileObj->string,cfgParam->string,cfgFileObj->type,cfgParam->type,opsParamObj->child!=NULL,opsParamObj->next!=NULL);
    while(opsParamObj)
    {
        tmp = Common_cJSON_GetObjectItem(fileObj,opsParamObj->string);
        if(tmp == NULL)
        {  
            if(type == 2)       //only merge the same item.
            {
                opsParamObj = opsParamObj->next;
                continue;
            }
            
            if(fileObj->type != Common_cJSON_Object)
            {
                //if(fileObj->type == cJSON_Array)
                //    return EC_CFG_ARRAY_NOT_SUPPORT;
                
                if(fileObj->type == Common_cJSON_String && fileObj->valuestring)
                {
                    Common_Free(fileObj->valuestring,__FUNCTION__,__LINE__);
                    fileObj->valuestring = NULL;
                }
                fileObj->type = Common_cJSON_Object;
            }
            Common_cJSON_AddItemToObject(fileObj,opsParamObj->string,Common_cJSON_Duplicate(opsParamObj,1));
        }
        else
        {
            if(opsParamObj->child != NULL)
            {
                if(opsParamObj->type == Common_cJSON_Array)              // array operations not support;
                {
                    if(tmp->type != Common_cJSON_Array)
                    {
                        LOGE("Have the same name item. But type=%d not the array.\n",tmp->type);
                        return EC_CFG_ITEM_MIS_MATCH;
                    }

                    Common_cJSON_DeleteItemFromObject(fileObj,opsParamObj->string);
                    Common_cJSON_AddItemToObject(fileObj,opsParamObj->string,Common_cJSON_Duplicate(opsParamObj,1));        
                    opsParamObj = opsParamObj->next;
                    continue;
                }
                                
                ret = MgrMatchSubItem(tmp,opsParamObj->child,1,opsParamObj,type);
                if(ret != 0)
                    return ret;                                      // if any item set fail, return immediately;
                
                if( opsParamObj->child == NULL && parent_note)                         //if all child item are the same and were remove, then remove the parent item.
                {
                    opsParamObj = opsParamObj->next;
                    if(checkNext)                                     
                        continue;
                }
            }
            else
            {    
                if(type != 1)
                {
                    ret = MgrSetCfgItemValue(tmp,opsParamObj);  // -111 -> have the same value
                    if((ret != 0) && (ret != -111)) 
                    {
                        LOGE("set cfg item value! ret=%d\n",ret);
                        return ret;
                    }
                }
                else
                {
                    LOGE("Can't set value in type=1 mode\n");
                    return -1;
                }
            }
        }

        if(checkNext == 0)
            break;

        opsParamObj = opsParamObj->next;
    };

    return 0;
}

int JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type)//0: cover the same item  1: return when have the same item 2:only merge the dst same item
{
    if(dst == NULL || type <0 )
    {
        LOGE("param invail: dst=%d type=%d\n",dst!=NULL,type);
        return -1;
    }

    if(src == NULL)
        return 0;

    return MgrMatchSubItem(dst,src->child,1,src,type);
}


#endif
