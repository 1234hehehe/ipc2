/*
 * ants_cfg_operator.h
 *
 *  Created on: 2016.7.21
 *  Author: longzhou
 */

#ifndef COMMON_JSON_STR_H_
#define COMMON_JSON_STR_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include"cjson.h"

#define OPER_SEPARATOR                  "."
#define MAX_LABEL_LEVEL_SUPPORT       16

/*
*    func:       json object with string type. e.g.==>  aa.bb.cc=1 dd.ee="ABC"
*    param:    printObj-> which cjson object do you want to print.
*    return:     0-> success     <0 fail
*/
int JsonOper_JsonPrint(Common_cJSON_T* printObj);

/*
*   func:        Generate a cjson object from a string which without "=".
*   param:    getString->  source string. e.g ==> aa.bb.cc
*   return:     Success: cjson object   Fail: NULL
*   Note:       return object must be free maunally.
*/
Common_cJSON_T* JsonOper_StringToJsonGet(const char* getString);

 /*
*   func:        Generate a cjson object from a string which with "=".
*   param:    getString->  source string. e.g ==> aa.bb.cc=1
*                   valueType-> o: int 1:string others: no support now.     
*   return:     Success: cjson object   Fail: NULL
*   Note:       return object must be free maunally.
*/
Common_cJSON_T* JsonOper_StringToJsonSet( const char* getString,int valueType);

/*
*   func:        Get a sub cjson  by path string, which cjson name separate by "."
*   param:    object->  source cjson
*                   path-> cjson  path string, separate by ".". e.g path = "aa.bb" , this function will search sub cjson aa from object,
*                   if get , it will search sub cjson bb from aa
*   return:     Success: cjson object   Fail: NULL
*/
Common_cJSON_T *JsonOper_GetObjectItemByPath(Common_cJSON_T *object, const char *path);

/*
*   func:        Get a sub cjson  by string, it will check object whether is NULL , if object is NULL , this function will return NULL
*   param:    object->  cjson object
*                   string-> cjson  object name string
*   return:     Success: cjson object   Fail: NULL
*/
Common_cJSON_T *JsonOper_GetObjectItemWithCheck(Common_cJSON_T *object, const char *string);

/*
*   func:        merge src json obj to dst.
*   param:    dst->  target ojbect item
*                   src-> source object item
			type-> 0: cover the same item  1: return when have the same item
*   return:     Success: 0   Fail:<0
*/
int		JsonOper_MergeObj(Common_cJSON_T* dst,Common_cJSON_T* src,int type);

#ifdef __cplusplus
}
#endif

#endif /* COMMON_JSON_STR_H_ */


