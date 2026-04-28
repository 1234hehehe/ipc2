#include "libcommon_api.h"

static char static_Common_base64_alphabet[]= {
	'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P', 
	'Q','R','S','T','U','V','W','X','Y','Z','a','b','c','d','e','f', 
	'g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v', 
	'w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/','='};
/*
static const char b64_table[] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
    'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
    'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
    'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
    'w', 'x', 'y', 'z', '0', '1', '2', '3',
    '4', '5', '6', '7', '8', '9', '+', '/'
};
*/
// S8* Common_Base64_Decode(S8 *pBase64code, U32 Base64length,U32 *ResultSize)
// S8 *Common_Base64_Decode (S8 *src, U32 len, U32 *decsize) {
//   int i = 0;
//   int j = 0;
//   int l = 0;
//   size_t size = 0;
//   unsigned char *dec = NULL;
//   unsigned char buf[3];
//   unsigned char tmp[4];

//   // alloc
//   dec = (unsigned char *) malloc(1);
//   if (NULL == dec) { return NULL; }

//   // parse until end of source
//   while (len--) {
//     // break if char is `=' or not base64 char
//     if ('=' == src[j]) { break; }
//     if (!(isalnum(src[j]) || '+' == src[j] || '/' == src[j])) { break; }

//     // read up to 4 bytes at a time into `tmp'
//     tmp[i++] = src[j++];

//     // if 4 bytes read then decode into `buf'
//     if (4 == i) {
//       // translate values in `tmp' from table
//       for (i = 0; i < 4; ++i) {
//         // find translation char in `b64_table'
//         for (l = 0; l < 64; ++l) {
//           if (tmp[i] == b64_table[l]) {
//             tmp[i] = l;
//             break;
//           }
//         }
//       }

//       // decode
//       buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
//       buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
//       buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

//       // write decoded buffer to `dec'
//       dec = (unsigned char *) realloc(dec, size + 3);
//       if (dec != NULL){
//         for (i = 0; i < 3; ++i) {
//           dec[size++] = buf[i];
//         }
//       } else {
//         return NULL;
//       }

//       // reset
//       i = 0;
//     }
//   }

//   // remainder
//   if (i > 0) {
//     // fill `tmp' with `\0' at most 4 times
//     for (j = i; j < 4; ++j) {
//       tmp[j] = '\0';
//     }

//     // translate remainder
//     for (j = 0; j < 4; ++j) {
//         // find translation char in `b64_table'
//         for (l = 0; l < 64; ++l) {
//           if (tmp[j] == b64_table[l]) {
//             tmp[j] = l;
//             break;
//           }
//         }
//     }

//     // decode remainder
//     buf[0] = (tmp[0] << 2) + ((tmp[1] & 0x30) >> 4);
//     buf[1] = ((tmp[1] & 0xf) << 4) + ((tmp[2] & 0x3c) >> 2);
//     buf[2] = ((tmp[2] & 0x3) << 6) + tmp[3];

//     // write remainer decoded buffer to `dec'
//     dec = (unsigned char *) realloc(dec, size + (i - 1));
//     if (dec != NULL){
//       for (j = 0; (j < i - 1); ++j) {
//         dec[size++] = buf[j];
//       }
//     } else {
//       return NULL;
//     }
//   }

//   // Make sure we have enough space to add '\0' character at end.
//   dec = (unsigned char *) realloc(dec, size + 1);
//   if (dec != NULL){
//     dec[size] = '\0';
//   } else {
//     return NULL;
//   }

//   // Return back the size of decoded string if demanded.
//   if (decsize != NULL) {
//     *decsize = size;
//   }

//   return (S8 *)dec;
// }

S8* Common_Base64_Decode(S8 *pBase64code, U32 Base64length,U32 *ResultSize)
{
    char *buffertemp;
    char *buffer;
    char num=0;
    buffer=(char *)Common_Malloc(Base64length/4*3+1,0,__FUNCTION__,__LINE__);
    if (buffer == NULL)
    {
        *ResultSize = 0;
        return NULL;
    }
    buffertemp=buffer;

    char temp[4] = { 0 };
    int *ptemp = (int*)temp;
    unsigned int i=0;
    unsigned int m=0;
    for(i=0; i<Base64length;){
        *ptemp = 0;
        char *chtemp=temp;
        for(m=0;m<4&&i<Base64length;i++){

            if(((*pBase64code)>='a')&&((*pBase64code)<='z')){
                *chtemp=(*pBase64code)-'a'+26;
            }else if(((*pBase64code)>='A')&&((*pBase64code)<='Z')){
                *chtemp=(*pBase64code)-'A';
            }else if(((*pBase64code)>='0')&&((*pBase64code)<='9')){
                *chtemp=(*pBase64code)-'0'+52;
            }else if(*pBase64code=='/'){
                *chtemp=63;
            }else if(*pBase64code=='+'){
                *chtemp=62;
            }else if (*pBase64code=='='){
                // *chtemp='=';
                *chtemp = 0;
                num++;
            }else{
                pBase64code++;
                continue;
            }
            m++;
            chtemp++;
            pBase64code++;
        }
        *buffertemp++=(temp[0]<<2)+((temp[1]>>4));
        *buffertemp++=(temp[1]<<4)+((temp[2]>>2));
        *buffertemp++=(temp[2]<<6)+((temp[3]));
    }

    // if(temp[2]=='=')
    // 	num++;
    // if(temp[3]=='=')
    // 	num++;
    // printf("number is %d\n",num);
    *(buffertemp-num)=0;
    if (ResultSize)
    {
        *ResultSize = buffertemp - buffer - num;
    }

    return buffer;
}
S8* Common_Base64_Encode(S8 *pBase64code, U32 base64length,U32 *pResultSize)
{
	int i;     
	unsigned long m;    
	char *p = NULL;    
	unsigned char *s = NULL;
	int n = base64length;    
	char *t = NULL;    
	if (pResultSize)
	{
		*pResultSize = 0;
	}
	s = (unsigned char *)pBase64code;    
	if (!t)        
		t = (char *)Common_Malloc((n + 2) / 3 * 4 + 1,0,__FUNCTION__,__LINE__);//(char*)new char[(n + 2) / 3 * 4 + 1];    
	if (!t)        
		return NULL;    
	if (t == NULL)
	{
		return NULL;
	}
	p = t;    
	t[0] = '\0';    
	if (!s)        
		return p;    
	for (; n > 2; n -= 3, s += 3)    
	{ 
		m = s[0];    
		m = (m << 8) | s[1];    
		m = (m << 8) | s[2];    
		for (i = 4; i > 0; m >>= 6)        
			t[--i] = static_Common_base64_alphabet[m & 0x3F];    
		t += 4;    
	}    
	t[0] = '\0';    
	if (n > 0)    
	{ 
		m = 0;    
		for (i = 0; i < n; i++)        
			m = (m << 8) | *s++;    
		for (; i < 3; i++)        
			m <<= 8;    
		for (i++; i > 0; m >>= 6)        
			t[--i] = static_Common_base64_alphabet[m & 0x3F];    
		for (i = 3; i > n; i--)        
			t[i] = '=';    
		t[4] = '\0';    
		t += 4;
	} 
	if (pResultSize)
	{
		*pResultSize = t - p;
	}
	return p;

}

S32 Common_Base64_IsValid(S8 *base64code, U32 base64length)
{
	unsigned int i;
	//unsigned int len;
	char c;
	if (base64code == NULL)
	{
		return 0;
	}
	for (i = 0; i < base64length;i++)
	{
		c = base64code[i];
		if ((c >= 'A' && c <= 'Z') || 
			(c >= 'a' && c <= 'z') || 
			(c >= '0' && c <= '9') || 
			c == '+' ||
			c == '/' ||
			c == '=')
		{
		}
		else
		{
			return 0;
		}
	}
	return 1;

}
