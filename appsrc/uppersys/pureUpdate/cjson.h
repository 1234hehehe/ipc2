/*
  Copyright (c) 2009 Dave Gamble
 
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:
 
  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.
 
  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/

#ifndef __CJSON_H__
#define __CJSON_H__
#include "libcommon_api.h"
#ifdef __cplusplus
extern "C"
{
#endif

/* Common_cJSON_T Types: */
#define Common_cJSON_False 0
#define Common_cJSON_True 1
#define Common_cJSON_NULL 2
#define Common_cJSON_Number 3
#define Common_cJSON_Double 4
#define Common_cJSON_String 5
#define Common_cJSON_Array 6
#define Common_cJSON_Object 7
	
#define Common_cJSON_IsReference 256

/* The Common_cJSON_T structure: */
typedef struct Common_cJSON_T {
	struct Common_cJSON_T *next,*prev;	/* next/prev allow you to walk array/object chains. Alternatively, use GetArraySize/GetArrayItem/GetObjectItem */
	struct Common_cJSON_T *child;		/* An array or object item will have a child pointer pointing to a chain of the items in the array/object. */
	struct Common_cJSON_T *LastChild;
	struct Common_cJSON_T *pParent;
	S32 nChildNum;
	S32 type;					/* The type of the item, as above. */

	S8 *valuestring;			/* The item's string, if type==Common_cJSON_String */
	S32 valueint;				/* The item's number, if type==Common_cJSON_Number */
	double valuedouble;			/* The item's number, if type==Common_cJSON_Number */
    struct Common_cJSON_T **pArrays; // pArrays[nArrayNum],每次分128个元数
	S32 nArrayNum;
	S32 nArraySize;
	void *pExtData;
	S32 nExtDataSize;

	S8 *string;				/* The item's name string, if this item is the child of, or is in the list of subitems of an object. */
} Common_cJSON_T;

typedef struct Common_cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} Common_cJSON_Hooks;

/* Supply malloc, realloc and free functions to Common_cJSON_T */
LIBCOMMON_API void Common_cJSON_InitHooks(Common_cJSON_Hooks* hooks);
LIBCOMMON_API void *Common_cJSON_malloc(size_t sz);
LIBCOMMON_API void Common_cJSON_free(void *ptr);

LIBCOMMON_API void *Common_cJSON_malloc_ex(size_t sz,const S8 *pDes,S32 nLine);
LIBCOMMON_API void Common_cJSON_free_ex(void *ptr,const S8 *pDes,S32 nLine);
LIBCOMMON_API S8* Common_cJSON_strdup_ex(const S8* str,const S8 *pDes,S32 nLine);

/* Supply a block of JSON, and this returns a Common_cJSON_T object you can interrogate. Call Common_cJSON_Delete when finished. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_Parse(const S8 *value,const S8 **pEndString,const S8 **pszErrorString);
/* Render a Common_cJSON_T entity to text for transfer/storage. Free the S8* when finished. */
LIBCOMMON_API S8  *Common_cJSON_Print(Common_cJSON_T *item,S32 *lpStrLen);
/* Render a Common_cJSON_T entity to text for transfer/storage without any formatting. Free the S8* when finished. */
LIBCOMMON_API S8  *Common_cJSON_PrintUnformatted(Common_cJSON_T *item,S32 *lpStrLen);
/* Delete a Common_cJSON_T entity and all subentities. */
LIBCOMMON_API void   Common_cJSON_Delete(Common_cJSON_T *c);
LIBCOMMON_API void Common_cJSON_Delete_ex(Common_cJSON_T *c,const S8 *pDes,S32 nLine);

/* Returns the number of items in an array (or object). */
LIBCOMMON_API S32	  Common_cJSON_GetArraySize(Common_cJSON_T *array);
/* Retrieve item number "item" from array "array". Returns NULL if unsuccessful. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_GetArrayItem(Common_cJSON_T *array,S32 item);
/* Get item "string" from object. Case insensitive. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_GetObjectItem(Common_cJSON_T *object,const S8 *string);

/* For analysing failed parses. This returns a pointer to the parse error. You'll probably need to look a few chars back to make sense of it. Defined when Common_cJSON_Parse() returns 0. 0 when Common_cJSON_Parse() succeeds. */
LIBCOMMON_API const S8 *Common_cJSON_GetErrorPtr(void);
	
/* These calls create a Common_cJSON_T item of the appropriate type. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateNull(void);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateTrue(void);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateFalse(void);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateBool(S32 b);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateNumber(S32 num);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateDouble(double num);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateString(const S8 *string);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateArray(void);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateObject(void);

LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateNull_ex(const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateTrue_ex(const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateFalse_ex(const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateBool_ex(S32 b,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateNumber_ex(S32 num,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateDouble_ex(double num,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateString_ex(const S8 *string,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateArray_ex(const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateObject_ex(const S8 *pDes,S32 nLine);

/* These utilities create an Array of count items. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateIntArray(S32 *numbers,S32 count);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateFloatArray(float *numbers,S32 count);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateDoubleArray(double *numbers,S32 count);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateStringArray(const S8 **strings,S32 count);

LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateIntArray_ex(S32 *numbers,S32 count,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateFloatArray_ex(float *numbers,S32 count,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateDoubleArray_ex(double *numbers,S32 count,const S8 *pDes,S32 nLine);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_CreateStringArray_ex(const S8 **strings,S32 count,const S8 *pDes,S32 nLine);
/* Append item to the specified array/object. */
LIBCOMMON_API void Common_cJSON_AddItemToArray(Common_cJSON_T *array, Common_cJSON_T *item);
LIBCOMMON_API void Common_cJSON_AddItemToArrayByIndex(Common_cJSON_T *array,S32 which, Common_cJSON_T *item);
LIBCOMMON_API void	Common_cJSON_AddItemToObject(Common_cJSON_T *object,const S8 *string,Common_cJSON_T *item);
/* Append reference to item to the specified array/object. Use this when you want to add an existing Common_cJSON_T to a new Common_cJSON_T, but don't want to corrupt your existing Common_cJSON_T. */
LIBCOMMON_API void Common_cJSON_AddItemReferenceToArray(Common_cJSON_T *array, Common_cJSON_T *item);
LIBCOMMON_API void	Common_cJSON_AddItemReferenceToObject(Common_cJSON_T *object,const S8 *string,Common_cJSON_T *item);

/* Remove/Detatch items from Arrays/Objects. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_DetachItemFromArray(Common_cJSON_T *array,S32 which);
LIBCOMMON_API void   Common_cJSON_DeleteItemFromArray(Common_cJSON_T *array,S32 which);
LIBCOMMON_API Common_cJSON_T *Common_cJSON_DetachItemFromObject(Common_cJSON_T *object,const S8 *string);
LIBCOMMON_API void   Common_cJSON_DeleteItemFromObject(Common_cJSON_T *object,const S8 *string);
	
/* Update array items. */
LIBCOMMON_API void Common_cJSON_ReplaceItemInArray(Common_cJSON_T *array,S32 which,Common_cJSON_T *newitem);
LIBCOMMON_API void Common_cJSON_ReplaceItemInObject(Common_cJSON_T *object,const S8 *string,Common_cJSON_T *newitem);

/* Duplicate a Common_cJSON_T item */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_Duplicate(Common_cJSON_T *item,S32 recurse);
/* Duplicate will create a new, identical Common_cJSON_T item to the one you pass, in new memory that will
need to be released. With recurse!=0, it will duplicate any children connected to the item.
The item->next and ->prev pointers are always zero on return from Duplicate. */

/* ParseWithOpts allows you to require (and check) that the JSON is null terminated, and to retrieve the pointer to the final byte parsed. */
LIBCOMMON_API Common_cJSON_T *Common_cJSON_ParseWithOpts(const S8 *value,const S8 **return_parse_end,const S8 **return_parse_error,S32 require_null_terminated);

LIBCOMMON_API void Common_cJSON_Minify(S8 *json);

/* Macros for creating things quickly. */
#define Common_cJSON_AddNullToObject(object,name)		Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateNull())
#define Common_cJSON_AddTrueToObject(object,name)		Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateTrue())
#define Common_cJSON_AddFalseToObject(object,name)		Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateFalse())
#define Common_cJSON_AddBoolToObject(object,name,b)	Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateBool(b))
#define Common_cJSON_AddNumberToObject(object,name,n)	Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateNumber(n))
#define Common_cJSON_AddStringToObject(object,name,s)	Common_cJSON_AddItemToObject(object, name, Common_cJSON_CreateString(s))

/* When assigning an integer value, it needs to be propagated to valuedouble too. */
#define Common_cJSON_SetIntValue(object,val)			((object)?(object)->valueint=(object)->valuedouble=(val):(val))

LIBCOMMON_API S32 Common_cJSON_SetItemExtData(Common_cJSON_T *pJson,void *pData,S32 nDataSize);
LIBCOMMON_API void * Common_cJSON_GetItemExtData(Common_cJSON_T *pJson,S32 *lpDataSize);

#ifdef __cplusplus
}
#endif

#endif
