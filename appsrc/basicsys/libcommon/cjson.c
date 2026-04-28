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

/* Common_cJSON_T */
/* JSON parser in C. */

#include <string.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <float.h>
#include <limits.h>
#include <ctype.h>
#include "cjson.h"
#include "libcommon_api.h"

#define MODULE_CJSON_ARRAY_MALLOC_COUNT 16
static const char *ep;

const char *Common_cJSON_GetErrorPtr(void) {return ep;}

static S32 Common_cJSON_strcasecmp(const char *s1,const char *s2)
{
	if (!s1) return (s1==s2)?0:1;
	if (!s2) return 1;
	for(; tolower(*s1) == tolower(*s2); ++s1, ++s2)	if(*s1 == 0)	return 0;
	return tolower(*(const unsigned char *)s1) - tolower(*(const unsigned char *)s2);
}

/* static void *(*_cJSON_malloc)(size_t sz) = malloc; */
/* static void (*_cJSON_free)(void *ptr) = free; */

void *Common_cJSON_malloc(size_t sz)
{
	if (sz == 0)
	{
		return NULL;
	}
	return Common_Malloc(sz,0,__FUNCTION__,__LINE__);
	//return _cJSON_malloc(sz);
}
void Common_cJSON_free(void *ptr)
{
	if (ptr == NULL)
	{
		return;
	}
	Common_Free(ptr,__FUNCTION__,__LINE__);
	//_cJSON_free(ptr);
}

char* Common_cJSON_strdup(const char* str)
{
      /* size_t len; */
      /* char* copy; */
      return Common_StrDup((char *)str,__FUNCTION__,__LINE__);
#if 0
      len = strlen(str) + 1;
      if (!(copy = (char*)_cJSON_malloc(len))) return 0;
      memcpy(copy,str,len);
      return copy;
#endif
}

void *Common_cJSON_malloc_ex(size_t sz,const S8 *pDes,S32 nLine)
{
	if (sz == 0)
	{
		return NULL;
	}
	return Common_Malloc(sz,0,pDes,nLine);
}
void Common_cJSON_free_ex(void *ptr,const S8 *pDes,S32 nLine)
{
	if (ptr == NULL)
	{
		return;
	}
	Common_Free(ptr,pDes,nLine);
}
S8* Common_cJSON_strdup_ex(const S8* str,const S8 *pDes,S32 nLine)
{
	/* size_t len; */
	/* char* copy; */
	return Common_StrDup((char *)str,pDes,nLine);
}


void Common_cJSON_InitHooks(Common_cJSON_Hooks* hooks)
{
    //if (!hooks) { /* Reset hooks */
   //     _cJSON_malloc = malloc;
   //     _cJSON_free = free;
   //     return;
   // }
	Common_InitHooks(hooks->malloc_fn,hooks->free_fn);

	//_cJSON_malloc = (hooks->malloc_fn)?hooks->malloc_fn:malloc;
	//_cJSON_free	 = (hooks->free_fn)?hooks->free_fn:free;
}

S32 Common_cJSON_ArrayCheckAndRealloc_ex(Common_cJSON_T *pArray,const S8 *pDes,S32 nLine)
{
	Common_cJSON_T **pNewArray = NULL;
	if (pArray->nArrayNum >= pArray->nArraySize)
	{
		pNewArray = (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T*) * (pArray->nArraySize + MODULE_CJSON_ARRAY_MALLOC_COUNT),pDes,nLine);
		if (pNewArray != NULL)
		{
			memset(&pNewArray[pArray->nArrayNum], 0 ,sizeof(Common_cJSON_T*) * (pArray->nArraySize - pArray->nArrayNum + MODULE_CJSON_ARRAY_MALLOC_COUNT));
			if (pArray->nArrayNum > 0)
			{
				memcpy(pNewArray,pArray->pArrays,sizeof(Common_cJSON_T*) * pArray->nArrayNum);
			}

			pArray->nArraySize = pArray->nArraySize + MODULE_CJSON_ARRAY_MALLOC_COUNT;
			Common_cJSON_free_ex(pArray->pArrays,pDes,nLine);
			pArray->pArrays = pNewArray;
		}
	}
	return 0;
}
S32 Common_cJSON_ArrayCheckAndRealloc(Common_cJSON_T *pArray)
{
	return Common_cJSON_ArrayCheckAndRealloc_ex(pArray,__FUNCTION__,__LINE__);
}

void Common_cJSON_ArrayFree_pArrays_ex(Common_cJSON_T *pArray,const S8 *pDes,S32 nLine)
{
	S32 i;
	if (pArray == NULL)
	{
		return;
	}
	if (pArray->pArrays == NULL || pArray->nArraySize == 0)
	{
		if (pArray->nArraySize != 0 || pArray->pArrays != NULL)
		{
			printf("!!!!!!!!!!!!!!!!!!!*******************************************\n");
		}
		
		return;
	}
	
	
	for (i = 0; i < pArray->nArrayNum && i < pArray->nArraySize;i++)
	{
		pArray->pArrays[i]->pParent = NULL;
		Common_cJSON_Delete_ex(pArray->pArrays[i],pDes,nLine);
		pArray->pArrays[i] = NULL;
	}
	Common_cJSON_free_ex(pArray->pArrays,pDes,nLine);
	pArray->pArrays = NULL;
	pArray->nArrayNum = 0;
	pArray->nArraySize = 0;
}
void Common_cJSON_ArrayFree_pArrays(Common_cJSON_T *pArray)
{
	Common_cJSON_ArrayFree_pArrays_ex(pArray,__FUNCTION__,__LINE__); 
}

/* Internal constructor. */
static Common_cJSON_T *Common_cJSON_New_Item(void)
{
	Common_cJSON_T* node = (Common_cJSON_T*)Common_cJSON_malloc(sizeof(Common_cJSON_T));
	if (node) memset(node,0,sizeof(Common_cJSON_T));
	return node;
}
static Common_cJSON_T *Common_cJSON_New_Item_ex(const S8 *pDes,S32 nLine)
{
	Common_cJSON_T* node = (Common_cJSON_T*)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T),pDes,nLine);
	if (node) memset(node,0,sizeof(Common_cJSON_T));
	return node;
}

/* Delete a Common_cJSON_T structure. */
void Common_cJSON_Delete(Common_cJSON_T *c)
{
	Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
}
void Common_cJSON_Delete_ex(Common_cJSON_T *c,const S8 *pDes,S32 nLine)
{
	Common_cJSON_T *next,*child;
	Common_cJSON_T *pParent = NULL;
	if (c == NULL)
	{
		return;
	}
	
	// 从父节点摘除自己
	
		pParent = c->pParent;
		if (pParent != NULL)
		{
			if (c->prev == NULL && c->next == NULL)
			{
				if (pParent->child != c || pParent->nChildNum > 1)
				{
					printf("[%s.%d] json node error !***********************************\n",__FUNCTION__,__LINE__);
				}
			
				
			}
			else if (pParent->nChildNum == 1 && pParent->child != c)
			{
				printf("[%s.%d] json node error !***********************************1\n",__FUNCTION__,__LINE__);
				
			}
			if (c->pParent != NULL)
			{
				if (c->prev == NULL)
				{
					pParent->child = c->next;
					if (pParent->child != NULL)
					{
						pParent->child->prev = NULL;
					}
					else
					{
						pParent->LastChild = NULL; 
					}
				}
				else if (c->next == NULL)
				{
					c->prev->next = NULL;
					pParent->LastChild = c->prev;
				}
				else
				{
					c->prev->next = c->next;
					c->next->prev = c->prev;
				}
				pParent->nChildNum--;
				c->next = NULL;
				c->prev = NULL;
				c->pParent = NULL;
			}
			
			
			
		}
		
		
	// 子节点
	child = c->child;
	while (child)
	{
		next=child->next;
		// 摘除
		Common_cJSON_Delete_ex(child,pDes,nLine);
		child = next;
	}
	if (c->child != NULL || c->LastChild != NULL || c->nChildNum != 0)
	{
		printf("[%s.%d] json child node error !***********************************1\n",__FUNCTION__,__LINE__);
	}
	
	c->child = NULL;
	c->LastChild = NULL;
	c->nChildNum = 0;
	if (!(c->type&Common_cJSON_IsReference) && c->child)
	{
		Common_cJSON_Delete_ex(c->child,pDes,nLine);
		c->child = NULL;
		c->LastChild = NULL;
	}
	if (!(c->type&Common_cJSON_IsReference) && c->valuestring)
	{
		S8 szTmp[128];
		if (c->string != NULL)
		{
			snprintf(szTmp,127,"%s.value<%s>",pDes?pDes:"",c->string);
		}
		else
		{
			snprintf(szTmp,127,"%s.value[%s]",pDes?pDes:"",c->valuestring);
		}
		
		
		szTmp[127] = 0;

		Common_cJSON_free_ex(c->valuestring,szTmp,nLine);
		c->valuestring = NULL;
	}
	Common_cJSON_ArrayFree_pArrays(c);
	if (c->pExtData != NULL)
	{
		S8 szTmp[128];
		
		snprintf(szTmp,127,"%s.ExtData<%s>",pDes?pDes:"",c->string?c->string:"");
		


		szTmp[127] = 0;

		Common_cJSON_free_ex(c->pExtData,pDes,nLine);
		c->pExtData = NULL;
		c->nExtDataSize = 0;
	}
	if (c->string)
	{
		S8 szTmp[128];
		snprintf(szTmp,127,"%s.name<%s>",pDes?pDes:"",c->string);
		szTmp[127] = 0;
		Common_cJSON_free_ex(c->string,szTmp,nLine);
		c->string = NULL;
	}
	
	Common_cJSON_free_ex(c,pDes,nLine);

}

/* Parse the input text to generate a number, and populate the result into item. */
static const char *parse_number(Common_cJSON_T *item,const char *num)
{
	double n=0,sign=1,scale=0;S32 subscale=0,signsubscale=1;
	S32 bDouble = 0;

	/* Could use sscanf for this? */
	if (*num=='-') sign=-1,num++;	/* Has sign? */
	if (*num=='0') num++;			/* is zero */
	if (*num>='1' && *num<='9')	do	n=(n*10.0)+(*num++ -'0');	while (*num>='0' && *num<='9');	/* Number? */
	if (*num=='.')
	{
		bDouble = 1;
	}
	
	if (*num=='.' && num[1]>='0' && num[1]<='9') {num++;		do	n=(n*10.0)+(*num++ -'0'),scale--; while (*num>='0' && *num<='9');}	/* Fractional part? */
	if (*num=='e' || *num=='E')		/* Exponent? */
	{	num++;if (*num=='+') num++;	else if (*num=='-') signsubscale=-1,num++;		/* With sign? */
		while (*num>='0' && *num<='9') subscale=(subscale*10)+(*num++ - '0');	/* Number? */
	}

	n=sign*n*pow(10.0,(scale+subscale*signsubscale));	/* number = +/- number.fraction * 10^+/- exponent */
	if (bDouble)
	{
		item->valuedouble=n;
		item->type=Common_cJSON_Double;
	}
	else
	{
		item->valueint=(S32)n;
		item->type=Common_cJSON_Number;
	}
	
	
	
	return num;
}

/* Render the number nicely from the given item into a string. */
static char *print_number(Common_cJSON_T *item,S32 *lpStrLen)
{
	char *str = NULL;
	S32 nLen = 0;
#if 1
	if (item->type == Common_cJSON_Number)
	{
		str=(char*)Common_cJSON_malloc_ex(21,__FUNCTION__,__LINE__);	/* 2^64+1 can be represented in 21 chars. */
		if (str) 
		{
			nLen = sprintf(str,"%d",item->valueint);
		}
	}
	else if (item->type==Common_cJSON_Double)
	{
		double d=item->valuedouble;
		str=(char*)Common_cJSON_malloc_ex(64,__FUNCTION__,__LINE__);	/* This is a nice tradeoff. */
		if (str)
		{
			if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)
			{
				nLen = sprintf(str,"%.0f",d);
			}
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			
			{
				nLen = sprintf(str,"%e",d);
			}
			else												
			{
				nLen = sprintf(str,"%f",d);
			}
		}
	}
	if (lpStrLen)
	{
		*lpStrLen = nLen;
	}
	
#else	
	
	double d=item->valuedouble;
	if (fabs(((double)item->valueint)-d)<=DBL_EPSILON && d<=INT_MAX && d>=INT_MIN)
	{
		str=(char*)Common_cJSON_malloc(21);	/* 2^64+1 can be represented in 21 chars. */
		if (str) sprintf(str,"%d",item->valueint);
	}
	else
	{
		str=(char*)Common_cJSON_malloc(64);	/* This is a nice tradeoff. */
		if (str)
		{
			if (fabs(floor(d)-d)<=DBL_EPSILON && fabs(d)<1.0e60)sprintf(str,"%.0f",d);
			else if (fabs(d)<1.0e-6 || fabs(d)>1.0e9)			sprintf(str,"%e",d);
			else												sprintf(str,"%f",d);
		}
	}
#endif
	return str;
}

static unsigned parse_hex4(const char *str)
{
	unsigned h=0;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	h=h<<4;str++;
	if (*str>='0' && *str<='9') h+=(*str)-'0'; else if (*str>='A' && *str<='F') h+=10+(*str)-'A'; else if (*str>='a' && *str<='f') h+=10+(*str)-'a'; else return 0;
	return h;
}

/* Parse the input text into an unescaped cstring, and populate item. */
static const unsigned char firstByteMark[7] = { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };
static const char *parse_string(Common_cJSON_T *item,const char *str,const char **pszErrorString)
{
	const char *ptr=str+1;char *ptr2;char *out;S32 len=0;unsigned uc,uc2;
	if (*str!='\"') 
	{
		if (pszErrorString)
		{
			*pszErrorString = str;
		}
		
		//ep=str;
		return 0;
	}	/* not a string! */
	
	while (*ptr!='\"' && *ptr && ++len) if (*ptr++ == '\\') ptr++;	/* Skip escaped quotes. */
	
	out=(char*)Common_cJSON_malloc_ex(len+1,__FUNCTION__,__LINE__);	/* This is how long we need for the string, roughly. */
	if (!out) return 0;
	
	ptr=str+1;ptr2=out;
	while (*ptr!='\"' && *ptr)
	{
		if (*ptr!='\\') *ptr2++=*ptr++;
		else
		{
			ptr++;
			switch (*ptr)
			{
				case 'b': *ptr2++='\b';	break;
				case 'f': *ptr2++='\f';	break;
				case 'n': *ptr2++='\n';	break;
				case 'r': *ptr2++='\r';	break;
				case 't': *ptr2++='\t';	break;
				case 'u':	 /* transcode utf16 to utf8. */
					uc=parse_hex4(ptr+1);ptr+=4;	/* get the unicode char. */

					if ((uc>=0xDC00 && uc<=0xDFFF) || uc==0)	break;	/* check for invalid.	*/

					if (uc>=0xD800 && uc<=0xDBFF)	/* UTF16 surrogate pairs.	*/
					{
						if (ptr[1]!='\\' || ptr[2]!='u')	break;	/* missing second-half of surrogate.	*/
						uc2=parse_hex4(ptr+3);ptr+=6;
						if (uc2<0xDC00 || uc2>0xDFFF)		break;	/* invalid second-half of surrogate.	*/
						uc=0x10000 + (((uc&0x3FF)<<10) | (uc2&0x3FF));
					}

					len=4;if (uc<0x80) len=1;else if (uc<0x800) len=2;else if (uc<0x10000) len=3; ptr2+=len;
					
					switch (len) {
						case 4: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 3: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 2: *--ptr2 =((uc | 0x80) & 0xBF); uc >>= 6;
						case 1: *--ptr2 =(uc | firstByteMark[len]);
					}
					ptr2+=len;
					break;
				default:  *ptr2++=*ptr; break;
			}
			ptr++;
		}
	}
	*ptr2=0;
	if (*ptr=='\"') ptr++;
	item->valuestring=out;
	item->type=Common_cJSON_String;
	return ptr;
}

/* Render the cstring provided to an escaped version that can be printed. */
static char *print_string_ptr(const char *str,S32 *lpStrLen)
{
	const char *ptr;char *ptr2,*out;S32 len=0;unsigned char token;
	
	if (!str)
	{
		if (lpStrLen)
		{
			*lpStrLen = 0;
		}
		return Common_cJSON_strdup_ex("",__FUNCTION__,__LINE__);
	}
	ptr=str;
	while ((token=*ptr) && ++len) 
	{
		if (strchr("\"\\\b\f\n\r\t",token)) 
		{
			len++;
		}
		else if (token<32)
		{
			len+=5;
		}
		ptr++;
	}
	
	out=(char*)Common_cJSON_malloc_ex(len+3,__FUNCTION__,__LINE__);
	if (!out)
	{
		if (lpStrLen)
		{
			*lpStrLen = 0;
		}
		return 0;
	}

	ptr2=out;ptr=str;
	*ptr2++='\"';
	while (*ptr)
	{
		if ((unsigned char)*ptr>31 && *ptr!='\"' && *ptr!='\\') *ptr2++=*ptr++;
		else
		{
			*ptr2++='\\';
			switch (token=*ptr++)
			{
				case '\\':	*ptr2++='\\';	break;
				case '\"':	*ptr2++='\"';	break;
				case '\b':	*ptr2++='b';	break;
				case '\f':	*ptr2++='f';	break;
				case '\n':	*ptr2++='n';	break;
				case '\r':	*ptr2++='r';	break;
				case '\t':	*ptr2++='t';	break;
				default: sprintf(ptr2,"u%04x",token);ptr2+=5;	break;	/* escape and print */
			}
		}
	}
	*ptr2++='\"';*ptr2++=0;
	if (lpStrLen)
	{
		*lpStrLen = ptr2 - out - 1;
	}
	return out;
}
/* Invote print_string_ptr (which is useful) on an item. */
static char *print_string(Common_cJSON_T *item,S32 *lpStrLen)	{return print_string_ptr(item->valuestring,lpStrLen);}

/* Predeclare these prototypes. */
static const char *parse_value(Common_cJSON_T *item,const char *value,const char **pszErrorString);
static char *print_value(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen);
static const char *parse_array(Common_cJSON_T *item,const char *value,const char **pszErrorString);
static char *print_array(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen);
static const char *parse_object(Common_cJSON_T *item,const char *value,const char **pszErrorString);
static char *print_object(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen);

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in) {while (in && *in && (unsigned char)*in<=32) in++; return in;}

/* Parse an object - create a new root, and populate. */
Common_cJSON_T *Common_cJSON_ParseWithOpts(const char *value,const char **return_parse_end,const char **return_parse_error,S32 require_null_terminated)
{
	const char *end=0;
	Common_cJSON_T *c=Common_cJSON_New_Item();
	if (return_parse_error)
	{
		*return_parse_error = 0;
	}
	
	//ep=0;
	if (!c) return 0;       /* memory fail */

	end=parse_value(c,skip(value),return_parse_error);
	if (!end)	
	{
		Common_cJSON_Delete(c);
		return 0;
	}	/* parse failure. ep is set. */

	/* if we require null-terminated JSON without appended garbage, skip and then check for a null terminator */
	if (require_null_terminated) 
	{
		end=skip(end);
		if (*end) 
		{
			Common_cJSON_Delete(c);
			if (return_parse_error)
			{
				*return_parse_error = end;
			}
			//ep=end;
			return 0;
		}
	}
	if (return_parse_end) 
	{
		*return_parse_end=end;
	}
	return c;
}
/* Default options for Common_cJSON_Parse */
Common_cJSON_T *Common_cJSON_Parse(const char *value,const char **pEndString,const char **pszErrorString) {return Common_cJSON_ParseWithOpts(value,pEndString,pszErrorString,0);}

/* Render a Common_cJSON_T item/entity/structure to text. */
char *Common_cJSON_Print(Common_cJSON_T *item,S32 *lpStrLen)				{return print_value(item,0,1,lpStrLen);}
char *Common_cJSON_PrintUnformatted(Common_cJSON_T *item,S32 *lpStrLen)	{return print_value(item,0,0,lpStrLen);}

/* Parser core - when encountering text, process appropriately. */
static const char *parse_value(Common_cJSON_T *item,const char *value,const char **pszErrorString)
{
	if (!value)						return 0;	/* Fail on null. */
	if (!strncmp(value,"null",4))	{ item->type=Common_cJSON_NULL;  return value+4; }
	if (!strncmp(value,"false",5))	{ item->type=Common_cJSON_False; return value+5; }
	if (!strncmp(value,"true",4))	{ item->type=Common_cJSON_True; item->valueint=1;	return value+4; }
	if (*value=='\"')				{ return parse_string(item,value,pszErrorString); }
	if (*value=='-' || (*value>='0' && *value<='9'))	{ return parse_number(item,value); }
	if (*value=='[')				{ return parse_array(item,value,pszErrorString); }
	if (*value=='{')				{ return parse_object(item,value,pszErrorString); }
	if (pszErrorString)
	{
		*pszErrorString = value;
	}
	
	//ep=value;
	return 0;	/* failure. */
}

/* Render a value to text. */
static char *print_value(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen)
{
	char *out=0;
	if (!item) return 0;
	switch ((item->type)&255)
	{
		case Common_cJSON_NULL:	
			{
				out=Common_cJSON_strdup_ex("null",__FUNCTION__,__LINE__);	
				if (lpStrLen)
				{
					*lpStrLen = 4;
				}
				break;
			}
		case Common_cJSON_False:	
			{
				out=Common_cJSON_strdup_ex("false",__FUNCTION__,__LINE__);
				if (lpStrLen)
				{
					*lpStrLen = 5;
				}
				break;
			}
		case Common_cJSON_True:	
			{
				out=Common_cJSON_strdup_ex("true",__FUNCTION__,__LINE__);
				if (lpStrLen)
				{
					*lpStrLen = 4;
				}
				break;
			}
		case Common_cJSON_Number:	
			{
				out=print_number(item,lpStrLen);
				break;
			}
		case Common_cJSON_Double:	out=print_number(item,lpStrLen);break;
		case Common_cJSON_String:	out=print_string(item,lpStrLen);break;
		case Common_cJSON_Array:	out=print_array(item,depth,fmt,lpStrLen);break;
		case Common_cJSON_Object:	out=print_object(item,depth,fmt,lpStrLen);break;
	}
	return out;
}

/* Build an pArray from input text. */
static const char *parse_array(Common_cJSON_T *item,const char *value,const char **pszErrorString)
{
	Common_cJSON_T *c;
	if (*value!='[')	
	{
		if (pszErrorString)
		{
			*pszErrorString = value;
		}
		
		//ep=value;
		return 0;
	}	/* not an pArray! */

	item->type=Common_cJSON_Array;
	value=skip(value+1);
	if (*value==']') return value+1;	/* empty pArray. */

	item->pArrays = Common_cJSON_malloc_ex(sizeof(Common_cJSON_T *) * MODULE_CJSON_ARRAY_MALLOC_COUNT,__FUNCTION__,__LINE__);
	if (item->pArrays == NULL)
	{
		return 0;
	}
	memset(item->pArrays,0,sizeof(Common_cJSON_T *) * MODULE_CJSON_ARRAY_MALLOC_COUNT);
	item->nArrayNum = 0;
	item->nArraySize = MODULE_CJSON_ARRAY_MALLOC_COUNT;
	c = Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);
	value=skip(parse_value(c,skip(value),pszErrorString));	/* skip any spacing, get the value. */
	if (!value)
	{
		Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
		Common_cJSON_ArrayFree_pArrays_ex(item,__FUNCTION__,__LINE__);
		return 0;
	}
	c->pParent = item;
	item->pArrays[item->nArrayNum] = c;
	item->nArrayNum++;
	while (*value==',')
	{
		Common_cJSON_ArrayCheckAndRealloc_ex(item,__FUNCTION__,__LINE__);
		if (item->nArrayNum >= item->nArraySize)
		{
			return 0;
		}
		c = Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);
		value=skip(parse_value(c,skip(value+1),pszErrorString));
		if (!value)
		{
			Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
			return 0;	/* memory fail */
		}
		item->pArrays[item->nArrayNum] = c;
		c->pParent = item;

		item->nArrayNum++;
	
		
	}


	if (*value==']') return value+1;	/* end of pArray */
	if (pszErrorString)
	{
		*pszErrorString = value;
	}
	
	//ep=value;
	return 0;	/* malformed. */
}

/* Render an pArray to text */
static char *print_array(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen)
{
	char **entries;
	char *out=0,*ptr,*ret;S32 len=5;
	S32 numentries=0,i=0,fail=0;
	
	/* How many entries in the pArray? */
	numentries = item->nArrayNum;
	/* Explicitly handle numentries==0 */
	if (!numentries)
	{
		out=(char*)Common_cJSON_malloc_ex(3,__FUNCTION__,__LINE__);
		if (out)
		{
			strcpy(out,"[]");
			if (lpStrLen)
			{
				*lpStrLen = 2;
			}
			
		}
		return out;
	}
	/* Allocate an pArray to hold the values for each */
	entries=(char**)Common_cJSON_malloc_ex(numentries*sizeof(char*),__FUNCTION__,__LINE__);
	if (!entries)
	{
		if (lpStrLen)
		{
			*lpStrLen = 0;
		}
		return 0;
	}
	memset(entries,0,numentries*sizeof(char*));
	/* Retrieve all the results: */
	for (i = 0; i < item->nArrayNum && (!fail);i++)
	{
		ret=print_value(item->pArrays[i],depth+1,fmt,NULL);
		entries[i]=ret;
		if (ret) 
		{
			len+=strlen(ret)+2+(fmt?1:0);
		}
		else
		{
			fail=1;
		}
	}

	
	/* If we didn't fail, try to malloc the output string */
	if (!fail) out=(char*)Common_cJSON_malloc_ex(len,__FUNCTION__,__LINE__);
	/* If that fails, we fail. */
	if (!out) fail=1;

	/* Handle failure. */
	if (fail)
	{
		for (i=0;i<numentries;i++) if (entries[i]) Common_cJSON_free_ex(entries[i],__FUNCTION__,__LINE__);
		Common_cJSON_free_ex(entries,__FUNCTION__,__LINE__);
		return 0;
	}
	
	/* Compose the output pArray. */
	*out='[';
	ptr=out+1;*ptr=0;
	for (i=0;i<numentries;i++)
	{
		strcpy(ptr,entries[i]);ptr+=strlen(entries[i]);
		if (i!=numentries-1) {*ptr++=',';if(fmt)*ptr++=' ';*ptr=0;}
		Common_cJSON_free_ex(entries[i],__FUNCTION__,__LINE__);
	}
	Common_cJSON_free_ex(entries,__FUNCTION__,__LINE__);
	*ptr++=']';*ptr++=0;
	if (lpStrLen)
	{
		*lpStrLen = ptr - out - 1;
	}
	
	return out;	
}

/* Build an object from the text. */
static const char *parse_object(Common_cJSON_T *item,const char *value,const char **pszErrorString)
{
	Common_cJSON_T *child;
	if (*value!='{')	
	{
		if (pszErrorString)
		{
			*pszErrorString = value;
		}
		
		//ep=value;
		return 0;
	}	/* not an object! */
	
	item->type=Common_cJSON_Object;
	value=skip(value+1);
	if (*value=='}') return value+1;	/* empty pArray. */
	
	item->child=child=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);
	item->LastChild = child;
	
	if (!item->child) return 0;
	item->nChildNum++;
	child->pParent = item;
	value=skip(parse_string(child,skip(value),pszErrorString));
	if (!value) return 0;
	child->string=child->valuestring;child->valuestring=0;
	if (*value!=':') 
	{
		if (pszErrorString)
		{
			*pszErrorString = value;
		}
		
		//ep=value;
		return 0;
	}	/* fail! */
	value=skip(parse_value(child,skip(value+1),pszErrorString));	/* skip any spacing, get the value. */
	if (!value) return 0;
	
	
	while (*value==',')
	{
		Common_cJSON_T *new_item;
		if (!(new_item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__)))	return 0; /* memory fail */
		item->nChildNum++;
		new_item->pParent = item;
		child->next=new_item;new_item->prev=child;child=new_item;
		item->LastChild = child;
		value=skip(parse_string(child,skip(value+1),pszErrorString));
		if (!value) return 0;
		child->string=child->valuestring;child->valuestring=0;
		if (*value!=':') 
		{
			if (pszErrorString)
			{
				*pszErrorString = value;
			}
			
			//ep=value;
			return 0;
		}	/* fail! */
		value=skip(parse_value(child,skip(value+1),pszErrorString));	/* skip any spacing, get the value. */
		if (!value) return 0;
		
	}
	
	if (*value=='}') return value+1;	/* end of pArray */
	if (pszErrorString)
	{
		*pszErrorString = value;
	}
	
	//ep=value;
	return 0;	/* malformed. */
}

/* Render an object to text. */
static char *print_object(Common_cJSON_T *item,S32 depth,S32 fmt,S32 *lpStrLen)
{
	char **entries=0,**names=0;
	char *out=0,*ptr,*ret,*str;S32 len=7,i=0,j;
	Common_cJSON_T *child=item->child;
	S32 numentries=0,fail=0;
	/* Count the number of entries. */
	// while (child) numentries++,child=child->next;

	numentries = item->nChildNum;
	
	/* Explicitly handle empty object case */
	if (!numentries)
	{
		out=(char*)Common_cJSON_malloc_ex(fmt?(depth+4):3,__FUNCTION__,__LINE__);
		if (!out)
		{
			return 0;
		}
		ptr=out;
		*ptr++='{';
		if (fmt) 
		{
			*ptr++='\n';
			for (i=0;i<depth-1;i++)
			{
				*ptr++='\t';
			}
		}
		*ptr++='}';
		*ptr++=0;
		if (lpStrLen)
		{
			*lpStrLen = ptr-out -1;
		}
		
		return out;
	}
	/* Allocate space for the names and the objects */
	entries=(char**)Common_cJSON_malloc_ex(numentries*sizeof(char*),__FUNCTION__,__LINE__);
	if (!entries) 
	{
		return 0;
	}
	names=(char**)Common_cJSON_malloc_ex(numentries*sizeof(char*),__FUNCTION__,__LINE__);
	if (!names) 
	{
		Common_cJSON_free_ex(entries,__FUNCTION__,__LINE__);
		return 0;
	}
	memset(entries,0,sizeof(char*)*numentries);
	memset(names,0,sizeof(char*)*numentries);

	/* Collect all the results into our arrays: */
	
		child=item->child;
		depth++;
		if (fmt)
		{
			len+=depth;
		}
		while (child)
		{
			names[i]=str=print_string_ptr(child->string,NULL);
			entries[i++]=ret=print_value(child,depth,fmt,NULL);
			if (str && ret)
			{
				len+=strlen(ret)+strlen(str)+2+(fmt?2+depth:0);
			}
			else
			{
				fail=1;
			}
			child=child->next;
		}
	
	
	
	/* Try to allocate the output string */
	if (!fail)
	{
		out=(char*)Common_cJSON_malloc_ex(len,__FUNCTION__,__LINE__);
	}
	if (!out)
	{
		fail=1;
	}

	/* Handle failure */
	if (fail)
	{
		for (i=0;i<numentries;i++) 
		{
			if (names[i]) 
			{
				Common_cJSON_free_ex(names[i],__FUNCTION__,__LINE__);
				names[i] = NULL;
			}
			if (entries[i])
			{
				Common_cJSON_free_ex(entries[i],__FUNCTION__,__LINE__);
				entries[i] = NULL;
			}
		}
		Common_cJSON_free_ex(out,__FUNCTION__,__LINE__);
		Common_cJSON_free_ex(names,__FUNCTION__,__LINE__);
		Common_cJSON_free_ex(entries,__FUNCTION__,__LINE__);
		return 0;
	}
	
	/* Compose the output: */
	*out='{';ptr=out+1;if (fmt)*ptr++='\n';*ptr=0;
	for (i=0;i<numentries;i++)
	{
		if (fmt)
		{
			for (j=0;j<depth;j++)
			{
				*ptr++='\t';
			}
		}
		strcpy(ptr,names[i]);
		ptr+=strlen(names[i]);
		*ptr++=':';
		if (fmt)
		{
			*ptr++='\t';
		}
		strcpy(ptr,entries[i]);
		ptr+=strlen(entries[i]);
		if (i!=numentries-1)
		{
			*ptr++=',';
		}
		if (fmt)
		{
			*ptr++='\n';
		}
		*ptr=0;
		Common_cJSON_free_ex(names[i],__FUNCTION__,__LINE__);
		names[i] = NULL;
		Common_cJSON_free_ex(entries[i],__FUNCTION__,__LINE__);
		entries[i] = NULL;
	}
	
	Common_cJSON_free_ex(names,__FUNCTION__,__LINE__);
	Common_cJSON_free_ex(entries,__FUNCTION__,__LINE__);
	if (fmt) 
	{
		for (i=0;i<depth-1;i++)
		{
			*ptr++='\t';
		}
	}

	*ptr++='}';
	*ptr++=0;
	if (lpStrLen)
	{
		*lpStrLen = ptr-out -1;
	}
	return out;	
}

/* Get Array size/item / object item. */
S32    Common_cJSON_GetArraySize(Common_cJSON_T *pArray)	
{
	if (pArray == NULL)
	{
		return 0;
	}
	return pArray->nArrayNum;
	
}
Common_cJSON_T *Common_cJSON_GetArrayItem(Common_cJSON_T *pArray,S32 item)				
{
	if (pArray == NULL || item < 0)
	{
		return NULL;
	}
	if (pArray->nArrayNum <= item)
	{
		return NULL;
	}
	return pArray->pArrays[item];

}
Common_cJSON_T *Common_cJSON_GetObjectItem(Common_cJSON_T *object,const char *string)	
{
	Common_cJSON_T *c=object->child; 
	while (c && Common_cJSON_strcasecmp(c->string,string)) 
		c=c->next;
	return c;
}

/* Utility for pArray list handling. */
/* static void suffix_object(Common_cJSON_T *prev,Common_cJSON_T *item) {prev->next=item;item->prev=prev;} */
/* Utility for handling references. */
static Common_cJSON_T *create_reference(Common_cJSON_T *item) 
{
	Common_cJSON_T *ref=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);
	if (!ref) 
		return 0;
	memcpy(ref,item,sizeof(Common_cJSON_T));
	ref->string=0;
	ref->type|=Common_cJSON_IsReference;
	ref->next=ref->prev=0;
	return ref;
}


/* Add item to pArray/object. */
void   Common_cJSON_AddItemToArray(Common_cJSON_T *pArray, Common_cJSON_T *item)	
{
	if (item == NULL || pArray == NULL)
	{
		return;
	}
	Common_cJSON_ArrayCheckAndRealloc_ex(pArray,__FUNCTION__,__LINE__);
	
	if (pArray->nArrayNum < pArray->nArraySize)
	{
		pArray->pArrays[pArray->nArrayNum] = item;
		item->pParent = pArray;
		pArray->nArrayNum++;
	}

}

void Common_cJSON_AddItemToArrayByIndex(Common_cJSON_T *pArray,S32 which, Common_cJSON_T *item)
{
	if (pArray == NULL || item == NULL)
	{
		return;
	}
	if (pArray->nArrayNum > which)
	{// 代替
		Common_cJSON_ReplaceItemInArray(pArray,which,item);
	}
	else
	{
		//补齐
		while (pArray->nArrayNum < which)
		{
			Common_cJSON_AddItemToArray(pArray,Common_cJSON_Duplicate(item,1));
		}
		Common_cJSON_AddItemToArray(pArray,item);
	}
	
	

}
void   Common_cJSON_AddItemToObject(Common_cJSON_T *object,const char *string,Common_cJSON_T *item)
{
	Common_cJSON_T *c = object->child;
	if (!item)
		return; 
	if (object->type == Common_cJSON_Array)
	{
		Common_cJSON_AddItemToArray(object,item);
		return ;
	}
	if (string)
	{
		if (item->string) 
			Common_cJSON_free_ex(item->string,__FUNCTION__,__LINE__);
		item->string=Common_cJSON_strdup_ex(string,__FUNCTION__,__LINE__);
	}
	while (c)
	{
		if (0 == Common_cJSON_strcasecmp(item->string,c->string))
		{

			item->next = c->next;
			item->prev = c->prev;
			if (item->prev == NULL)
			{
				object->child = item;
			}
			else
			{
				item->prev->next = item;
			}
			if (item->next == NULL)
			{
				object->LastChild = item;
			}
			else
			{
				item->next->prev = item;
			}
			item->pParent = object;
			c->next = NULL;
			c->prev = NULL;
			c->pParent = NULL;
		    Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
			return;
		}
		
		c = c->next;
	}
	
	
	
	
	if(object->child == NULL)
	{
		object->child = object->LastChild = item;
	}
	else
	{
		object->LastChild->next = item;
		item->prev = object->LastChild;
		object->LastChild = item;
	}
	item->pParent = object;
	object->nChildNum++;
	
}
void	Common_cJSON_AddItemReferenceToArray(Common_cJSON_T *pArray, Common_cJSON_T *item)						
{
	Common_cJSON_AddItemToArray(pArray,create_reference(item));
}
void	Common_cJSON_AddItemReferenceToObject(Common_cJSON_T *object,const char *string,Common_cJSON_T *item)	{Common_cJSON_AddItemToObject(object,string,create_reference(item));}

Common_cJSON_T *Common_cJSON_DetachItemFromArray(Common_cJSON_T *pArray,S32 which)
{
	Common_cJSON_T *c = NULL;
	S32 i;
	if (pArray == NULL)
	{
		return NULL;
	}
	if(pArray->pArrays != NULL && which >= 0 && which < pArray->nArrayNum)
	{
		c = pArray->pArrays[which];
		for (i = which + 1; i < pArray->nArrayNum;i++)
		{
			pArray->pArrays[i - 1] = pArray->pArrays[i];
		}
		pArray->nArrayNum--;
		pArray->pArrays[pArray->nArrayNum] = NULL;
		if (pArray->nArrayNum <= 0)
		{
			Common_cJSON_free_ex(pArray->pArrays,__FUNCTION__,__LINE__);
			pArray->pArrays = NULL;
			pArray->nArrayNum = pArray->nArraySize = 0;
		}		
	}
	if (pArray->nArrayNum == 0)
	{
		Common_cJSON_free_ex(pArray->pArrays,__FUNCTION__,__LINE__);
		pArray->pArrays  = NULL;
		pArray->nArraySize = 0;
	}
	if (c)
	{
		c->pParent = NULL;
		c->prev = NULL;
		c->next = NULL;
	}
	
	return c;
}
void   Common_cJSON_DeleteItemFromArray(Common_cJSON_T *pArray,S32 which)
{
	Common_cJSON_Delete_ex(Common_cJSON_DetachItemFromArray(pArray,which),__FUNCTION__,__LINE__);
}
Common_cJSON_T *Common_cJSON_DetachItemFromObject(Common_cJSON_T *object,const char *string)
{
	Common_cJSON_T *c=object->child;
	while (c && Common_cJSON_strcasecmp(c->string,string))
	{
		c=c->next;
	}
	if (c) 
	{
		if (c == object->LastChild)
		{
			if (c == object->child)
			{
				object->child = object->LastChild = NULL;
			}
			else
			{
				object->LastChild = c->prev;
				c->prev->next = NULL;
				c->prev = NULL;
			}
			
		}
		else
		{
			if (c == object->child)
			{
				object->child = c->next;
				c->next->prev = NULL;
				
			}
			else
			{
				c->prev->next = c->next;
				c->next->prev = c->prev;
				c->prev = c->next = NULL;
			}
			 
		}
		object->nChildNum--;
		c->next = NULL;
		c->prev = NULL;
		c->pParent = NULL;
		
	}
	return c;
}
void   Common_cJSON_DeleteItemFromObject(Common_cJSON_T *object,const char *string) {Common_cJSON_Delete_ex(Common_cJSON_DetachItemFromObject(object,string),__FUNCTION__,__LINE__);}

/* Replace pArray/object items with new ones. */
void   Common_cJSON_ReplaceItemInArray(Common_cJSON_T *pArray,S32 which,Common_cJSON_T *newitem)		
{
	Common_cJSON_T *c = NULL;
	if (pArray == NULL)
	{
		return;
	}
	if(pArray->pArrays != NULL && which >= 0 && which < pArray->nArrayNum)
	{
		c = pArray->pArrays[which];
		pArray->pArrays[which] = newitem;
		if (newitem)
		{
			newitem->pParent = c->pParent;
		}
		
		
	}
	if (c != NULL)
	{
		c->pParent = NULL;
		c->prev = NULL;
		c->next = NULL;
		Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
	}
	
   
	
	
}
void   Common_cJSON_ReplaceItemInObject(Common_cJSON_T *object,const char *string,Common_cJSON_T *newitem)
{
	S32 i=0;Common_cJSON_T *c=object->child;
	while(c && Common_cJSON_strcasecmp(c->string,string))i++,c=c->next;
	if(c)
	{
		if (newitem->string)
		{
			Common_cJSON_free_ex(newitem->string,__FUNCTION__,__LINE__);
			newitem->string = NULL;
		}
		
		newitem->string=Common_cJSON_strdup_ex(string,__FUNCTION__,__LINE__);
		newitem->next = c->next;
		newitem->prev = c->prev;
		newitem->pParent = c->pParent;
			if (newitem->prev == NULL)
			{
				object->child = newitem;
			}
			else
			{
				newitem->prev->next = newitem;
			}
			if (newitem->next == NULL)
			{
				object->LastChild = newitem;
			}
			else
			{
				newitem->next->prev = newitem;
			}
		c->prev = NULL;
		c->next = NULL;
		c->pParent = NULL;
			
		
		
		Common_cJSON_Delete_ex(c,__FUNCTION__,__LINE__);
	}
}

/* Create basic types: */
Common_cJSON_T *Common_cJSON_CreateNull(void)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item)item->type=Common_cJSON_NULL;return item;}
Common_cJSON_T *Common_cJSON_CreateTrue(void)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item)item->type=Common_cJSON_True;return item;}
Common_cJSON_T *Common_cJSON_CreateFalse(void)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item)item->type=Common_cJSON_False;return item;}
Common_cJSON_T *Common_cJSON_CreateBool(S32 b)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item)item->type=b?Common_cJSON_True:Common_cJSON_False;return item;}
Common_cJSON_T *Common_cJSON_CreateNumber(S32 num)			    {Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item){item->type=Common_cJSON_Number;item->valueint=(S32)num;}return item;}
Common_cJSON_T *Common_cJSON_CreateDouble(double num)			{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item){item->type=Common_cJSON_Double;item->valuedouble=num;}return item;}
Common_cJSON_T *Common_cJSON_CreateString(const char *string)	{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item){item->type=Common_cJSON_String;item->valuestring=Common_cJSON_strdup_ex(string,__FUNCTION__,__LINE__);}return item;}
Common_cJSON_T *Common_cJSON_CreateArray(void)					
{
	Common_cJSON_T *item=Common_cJSON_New_Item();
	if(item)item->type=Common_cJSON_Array;
	return item;
}
Common_cJSON_T *Common_cJSON_CreateObject(void)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(__FUNCTION__,__LINE__);if(item)item->type=Common_cJSON_Object;return item;}


Common_cJSON_T *Common_cJSON_CreateNull_ex(const S8 *pDes,S32 nLine)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item)item->type=Common_cJSON_NULL;return item;}
Common_cJSON_T *Common_cJSON_CreateTrue_ex(const S8 *pDes,S32 nLine)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item)item->type=Common_cJSON_True;return item;}
Common_cJSON_T *Common_cJSON_CreateFalse_ex(const S8 *pDes,S32 nLine)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item)item->type=Common_cJSON_False;return item;}
Common_cJSON_T *Common_cJSON_CreateBool_ex(S32 b,const S8 *pDes,S32 nLine)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item)item->type=b?Common_cJSON_True:Common_cJSON_False;return item;}
Common_cJSON_T *Common_cJSON_CreateNumber_ex(S32 num,const S8 *pDes,S32 nLine)			    {Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item){item->type=Common_cJSON_Number;item->valueint=(S32)num;}return item;}
Common_cJSON_T *Common_cJSON_CreateDouble_ex(double num,const S8 *pDes,S32 nLine)			{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item){item->type=Common_cJSON_Double;item->valuedouble=num;}return item;}
Common_cJSON_T *Common_cJSON_CreateString_ex(const char *string,const S8 *pDes,S32 nLine)	{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item){item->type=Common_cJSON_String;item->valuestring=Common_cJSON_strdup_ex(string,pDes,nLine);}return item;}
Common_cJSON_T *Common_cJSON_CreateArray_ex(const S8 *pDes,S32 nLine)					
{
	Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);
	if(item)item->type=Common_cJSON_Array;
	return item;
}
Common_cJSON_T *Common_cJSON_CreateObject_ex(const S8 *pDes,S32 nLine)					{Common_cJSON_T *item=Common_cJSON_New_Item_ex(pDes,nLine);if(item)item->type=Common_cJSON_Object;return item;}

/* Create Arrays: */
Common_cJSON_T *Common_cJSON_CreateIntArray(S32 *numbers,S32 count)
{
	return Common_cJSON_CreateIntArray_ex(numbers,count,__FUNCTION__,__LINE__);
}
Common_cJSON_T *Common_cJSON_CreateIntArray_ex(S32 *numbers,S32 count,const S8 *pDes,S32 nLine)				
{
	S32 i;
	Common_cJSON_T *a=0;
	if (count < 0 || numbers == NULL)
	{
		return NULL;
	}
	a = Common_cJSON_CreateArray_ex(pDes,nLine);
	if (a == NULL)
	{
		return NULL;
	}
	a->pArrays =  (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T*) * count,pDes,nLine);
	if (a->pArrays == NULL)
	{
		Common_cJSON_Delete_ex(a,pDes,nLine);
		return NULL;
	}
	memset(a->pArrays, 0 ,sizeof(Common_cJSON_T*) * count);
	a->nArrayNum = 0;
	a->nArraySize = count;
	for (i = 0; i < count; i++)
	{
		a->pArrays[a->nArrayNum] = Common_cJSON_CreateNumber_ex(numbers[i],pDes,nLine);
		if (a->pArrays[a->nArrayNum] != NULL)
		{
			a->pArrays[a->nArrayNum]->pParent = a;
			a->nArrayNum++;
		}
		
	}
	if (a->nArrayNum == 0)
	{
		Common_cJSON_free_ex(a->pArrays,pDes,nLine);
		a->pArrays  = NULL;
		a->nArraySize = 0;
	}
	
	return a;
}
Common_cJSON_T *Common_cJSON_CreateFloatArray(float *numbers,S32 count)
{
	return Common_cJSON_CreateFloatArray_ex(numbers,count,__FUNCTION__,__LINE__);
}
Common_cJSON_T *Common_cJSON_CreateFloatArray_ex(float *numbers,S32 count,const S8 *pDes,S32 nLine)				
{
	S32 i;
	Common_cJSON_T *a=0;
	if (count < 0 || numbers == NULL)
	{
		return NULL;
	}
	a = Common_cJSON_CreateArray_ex(pDes,nLine);
	if (a == NULL)
	{
		return NULL;
	}
	a->pArrays =  (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T*) * count,pDes,nLine);
	if (a->pArrays == NULL)
	{
		Common_cJSON_Delete_ex(a,pDes,nLine);
		return NULL;
	}
	memset(a->pArrays, 0 ,sizeof(Common_cJSON_T*) * count);
	a->nArrayNum = 0;
	a->nArraySize = count;
	for (i = 0; i < count; i++)
	{
		a->pArrays[a->nArrayNum] = Common_cJSON_CreateDouble_ex(numbers[i],pDes,nLine);
		if (a->pArrays[a->nArrayNum] != NULL)
		{
			a->pArrays[a->nArrayNum]->pParent = a;
			a->nArrayNum++;
		}

	}
	if (a->nArrayNum == 0)
	{
		Common_cJSON_free_ex(a->pArrays,pDes,nLine);
		a->pArrays  = NULL;
		a->nArraySize = 0;
	}
	
	return a;
}
Common_cJSON_T *Common_cJSON_CreateDoubleArray(double *numbers,S32 count)	
{
	return Common_cJSON_CreateDoubleArray_ex(numbers,count,__FUNCTION__,__LINE__);
}
Common_cJSON_T *Common_cJSON_CreateDoubleArray_ex(double *numbers,S32 count,const S8 *pDes,S32 nLine)				
{
	S32 i;
	Common_cJSON_T *a=0;
	if (count < 0 || numbers == NULL)
	{
		return NULL;
	}
	a = Common_cJSON_CreateArray_ex(pDes,nLine);
	if (a == NULL)
	{
		return NULL;
	}
	a->pArrays =  (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T*) * count,pDes,nLine);
	if (a->pArrays == NULL)
	{
		Common_cJSON_Delete_ex(a,pDes,nLine);
		return NULL;
	}
	memset(a->pArrays, 0 ,sizeof(Common_cJSON_T*) * count);
	a->nArrayNum = 0;
	a->nArraySize = count;
	for (i = 0; i < count; i++)
	{
		a->pArrays[a->nArrayNum] = Common_cJSON_CreateDouble_ex(numbers[i],pDes,nLine);
		if (a->pArrays[a->nArrayNum] != NULL)
		{
			a->pArrays[a->nArrayNum]->pParent = a;
			a->nArrayNum++;
		}

	}
	if (a->nArrayNum == 0)
	{
		Common_cJSON_free_ex(a->pArrays,pDes,nLine);
		a->pArrays  = NULL;
		a->nArraySize = 0;
	}
	
	return a;
}
Common_cJSON_T *Common_cJSON_CreateStringArray(const char **strings,S32 count)	
{
	return Common_cJSON_CreateStringArray_ex(strings,count,__FUNCTION__,__LINE__);
}
Common_cJSON_T *Common_cJSON_CreateStringArray_ex(const char **strings,S32 count,const S8 *pDes,S32 nLine)				
{
	S32 i;
	Common_cJSON_T *a=0;
	if (count < 0 || strings == NULL)
	{
		return NULL;
	}
	a = Common_cJSON_CreateArray_ex(pDes,nLine);
	if (a == NULL)
	{
		return NULL;
	}
	a->pArrays =  (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T *) * count,pDes,nLine);
	if (a->pArrays == NULL)
	{
		Common_cJSON_Delete_ex(a,pDes,nLine);
		return NULL;
	}
	memset(a->pArrays, 0 ,sizeof(Common_cJSON_T*) * count);
	a->nArrayNum = 0;
	a->nArraySize = count;
	for (i = 0; i < count; i++)
	{
		a->pArrays[a->nArrayNum] = Common_cJSON_CreateString_ex(strings[i],pDes,nLine);
		if (a->pArrays[a->nArrayNum] != NULL)
		{
			a->pArrays[a->nArrayNum]->pParent = a;
			a->nArrayNum++;
		}

	}
	if (a->nArrayNum == 0)
	{
		Common_cJSON_free_ex(a->pArrays,pDes,nLine);
		a->pArrays  = NULL;
		a->nArraySize = 0;
	}
	
	
	return a;
}

/* Duplication */
Common_cJSON_T *Common_cJSON_Duplicate(Common_cJSON_T *item,S32 recurse)
{
	Common_cJSON_T *newitem,*cptr,*nptr=0,*newchild;
	/* Bail on bad ptr */
	if (!item) return 0;
	/* Create new item */
	newitem=Common_cJSON_New_Item();
	if (!newitem) return 0;
	/* Copy over all vars */
	newitem->type=item->type&(~Common_cJSON_IsReference),newitem->valueint=item->valueint,newitem->valuedouble=item->valuedouble;
	if (item->valuestring)	{newitem->valuestring=Common_cJSON_strdup_ex(item->valuestring,__FUNCTION__,__LINE__);	if (!newitem->valuestring)	{Common_cJSON_Delete_ex(newitem,__FUNCTION__,__LINE__);return 0;}}
	if (item->string)		{newitem->string=Common_cJSON_strdup_ex(item->string,__FUNCTION__,__LINE__);			if (!newitem->string)		{Common_cJSON_Delete_ex(newitem,__FUNCTION__,__LINE__);return 0;}}
	
	
	/* If non-recursive, then we're done! */
	if (!recurse) return newitem;
	/* Walk the ->next chain for the child. */
	if (item->pArrays)
	{
		newitem->pArrays = (Common_cJSON_T**)Common_cJSON_malloc_ex(sizeof(Common_cJSON_T *) * item->nArraySize,__FUNCTION__,__LINE__);
		if (newitem->pArrays)
		{
			S32 i;
			memset(newitem->pArrays,0,sizeof(Common_cJSON_T *) * item->nArraySize);
			newitem->nArraySize = item->nArraySize;
			newitem->nArrayNum = 0;
			for (i = 0; i < item->nArrayNum;i++)
			{
				newchild=Common_cJSON_Duplicate(item->pArrays[i],1);
				newitem->pArrays[newitem->nArrayNum] = newchild;

				newchild->pParent = newitem;
				newitem->nArrayNum++;
			}
			if (newitem->nArrayNum <= 0)
			{
				Common_cJSON_free_ex(newitem->pArrays,__FUNCTION__,__LINE__);
				newitem->pArrays = NULL;
				newitem->nArraySize = 0;
				newitem->nArrayNum = 0;
			}

		}

	}
	if (item->pExtData != NULL && item->nExtDataSize > 0)
	{
		void *pNewExtData;
		pNewExtData = Common_cJSON_malloc_ex(item->nExtDataSize,__FUNCTION__,__LINE__);
		if (pNewExtData != NULL)
		{
			memcpy(pNewExtData,item->pExtData,item->nExtDataSize);
			newitem->pExtData = pNewExtData;
			newitem->nExtDataSize = item->nExtDataSize;
		}
		
	}
	

	cptr=item->child;
	while (cptr)
	{
		newchild=Common_cJSON_Duplicate(cptr,1);		/* Duplicate (with recurse) each item in the ->next chain */
		if (!newchild)
		{
			Common_cJSON_Delete_ex(newitem,__FUNCTION__,__LINE__);
			return 0;
		}
		newchild->pParent = newitem;
		if (nptr)	
		{
			nptr->next=newchild,newchild->prev=nptr;
			newitem->LastChild = newchild;
			nptr=newchild;
		}	/* If newitem->child already set, then crosswire ->prev and ->next and move on */
		else		
		{
			newitem->child=newchild;
			newitem->LastChild = newchild;
			nptr=newchild;
		}					/* Set newitem->child and move to it */
		cptr=cptr->next;
		newitem->nChildNum++;
	}
	return newitem;
}

void Common_cJSON_Minify(char *json)
{
	char *into=json;
	while (*json)
	{
		if (*json==' ') json++;
		else if (*json=='\t') json++;	// Whitespace characters.
		else if (*json=='\r') json++;
		else if (*json=='\n') json++;
		else if (*json=='/' && json[1]=='/')  while (*json && *json!='\n') json++;	// double-slash comments, to end of line.
		else if (*json=='/' && json[1]=='*') {while (*json && !(*json=='*' && json[1]=='/')) json++;json+=2;}	// multiline comments.
		else if (*json=='\"'){*into++=*json++;while (*json && *json!='\"'){if (*json=='\\') *into++=*json++;*into++=*json++;}*into++=*json++;} // string literals, which are \" sensitive.
		else *into++=*json++;			// All other characters.
	}
	*into=0;	// and null-terminate.
}

S32 Common_cJSON_SetItemExtData(Common_cJSON_T *pJson,void *pData,S32 nDataSize)
{
	Common_cJSON_T *pItem = NULL;
	void *pNewData = NULL;
	if (pJson == NULL)
	{
		return -1;
	}
	pItem = (Common_cJSON_T *)pJson;
	if (pItem->pExtData != NULL)
	{
		Common_Free(pItem->pExtData,__FUNCTION__,__LINE__);
		pItem->pExtData = NULL;
		pItem->nExtDataSize = 0;
	}
	if (nDataSize > 0 && pData != NULL)
	{
		pNewData = Common_cJSON_malloc_ex(nDataSize,__FUNCTION__,__LINE__);
		if (pNewData == NULL)
		{
			return -1;
		}
		memcpy(pNewData,pData,nDataSize);
		pItem->pExtData = pNewData;
		pItem->nExtDataSize = nDataSize;
	}
	return 0;


}
void * Common_cJSON_GetItemExtData(Common_cJSON_T *pJson,S32 *lpDataSize)
{
	Common_cJSON_T *pItem = NULL;
	if (pJson == NULL)
	{
		return NULL;
	}
	pItem = (Common_cJSON_T *)pJson;
	if (lpDataSize)
	{
		*lpDataSize = pItem->nExtDataSize;
	}
	return pItem->pExtData;
}

