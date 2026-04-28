#include <signal.h>
#include <unistd.h>

#include <sys/types.h>
#include <AntsWebCommon.h>


#if LINUX
/** * 
    utf-8
    1 bytes 0xxxxxxx
    2 bytes 110xxxxx 10xxxxxx
    3 bytes 1110xxxx 10xxxxxx 10xxxxxx
    4 bytes 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    5 bytes 111110xx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx
    6 bytes 1111110x 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx 10xxxxxx */
BOOL isutf8(const char* data, int32_t len){
    if(NULL == data){
        return TRUE;    
    }    
    const char* p = data;
    const char* end = data + len;
    while (p < end){
       if (0 == (*p & 0x80)){
            ++p;
       }else{
          uint8_t i;
          for (i = 2; i < sizeof(utf8Head) / sizeof(UTF8_HEAD) - 1; ++i){
                if (utf8Head[i].head == (*p & utf8Head[i+1].head)){
                    uint8_t j;
                    for(j = 1; j < i; ++j){
                        if (p + j > end){
                            return FALSE;
                        }else if (0x80 != (*(p+j) & 0xC0)){
                            return FALSE;
                        }                    
                    }
                    break;
               }else if (i >= 6){
                //not matched even over 6 bytes                    
                return FALSE;                
                }            
           }            

           if (p + i > end){
                return FALSE;
           }

           p += i; 
       }
   }    
   return TRUE;
}

void Print_buf(char* buf,int len){
     int k = 0;
    printf("\n\n Source Buf:{");
    for(k = 0;k < len;++k){
        
        printf("0x%x,",buf[k]);
        if(k%10 == 0)printf("\n");
    }
    printf("}\n\n");
}

void PrintBuffer(void *buffer, int len)
{
    unsigned char* p = (unsigned char*)buffer;
    int i;
    
    printf("[yul][%s:%d] buffer=%p len=0x%x\n", __FUNCTION__, __LINE__, buffer, len);
    for (i = 0; i < len; i += 16)
    {
        int j;
        printf("0x%04x:", i);
        for (j = 0; j < 16 && i + j < len; j++)
        {
            printf(" %02x", p[i + j]);
        }
        if (j < 16)
        {
            for ( ; j < 16; j++)
            {
                printf("   ");
            }
        }
        printf(" | ");
        for (j = 0; j < 16 && i + j < len; j++)
        {
            char c = p[i + j];
            printf("%c", c >= 0x20 && c <= 0x7f ? c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

void PrintBufferWrap(void *buffer, int len, int wrapbyte)
{
    unsigned char* p = (unsigned char*)buffer;
    int i;
    if (0 == wrapbyte) wrapbyte = 16;
    printf("[%s:%d] buffer=%p len=0x%x\n", __FUNCTION__, __LINE__, buffer, len);
    for (i = 0; i < len; i += wrapbyte)
    {
        int j;
        printf("0x%04x:", i);
        for (j = 0; j < wrapbyte && i + j < len; j++)
        {
            printf(" %02x", p[i + j]);
        }
        if (j < wrapbyte)
        {
            for ( ; j < wrapbyte; j++)
            {
                printf("   ");
            }
        }
        printf(" | ");
        for (j = 0; j < wrapbyte && i + j < len; j++)
        {
            char c = p[i + j];
            printf("%c", c >= 0x20 && c <= 0x7f ? c : '.');
        }
        printf("\n");
    }
    printf("\n");
}

uint32_t check_reparse_utf8( char* data, int32_t len,char* buf){
	uint32_t bufSize = 0;
   
   // Print_buf(data,len);

    
    if(NULL == data){
        return bufSize;    
    }    
     char* p = data;
     char* end = data + len;
	
    while (p < end){
       if (0 == (*p & 0x80)){//判断最高位是否为0  为0的一定是ascii字符，占一个字节
		   buf[bufSize++] = *p;
       }else{//多字节
          uint8_t i;
          for (i = 2; i < sizeof(utf8Head) / sizeof(UTF8_HEAD) - 1; ++i){
                if (utf8Head[i].head == (*p & utf8Head[i+1].head)){//根据utf8 head判断是几字节编码
                    uint8_t j;
					BOOL chk_flg = TRUE;
                    for(j = 1; j < i; ++j){//判断剩下的字节是否正确
                        if (p + j > end){
							chk_flg = FALSE;
							break;
                        }else if (0x80 != (*(p+j) & 0xC0)){
							p = p + j;
							chk_flg = FALSE;
							break;
                        }                    
                    }

					//
					if(chk_flg){
							for(j = 0; j < i; ++j){//copy
								buf[bufSize++] = *(p+j);     
							}
					}
				
                    break;
               }else if (i >= 6){
                //not matched even over 6 bytes  
				//try next char
                       break;       
                }            
           }               
       }
	   p++;
   }

   DEBUGV3("[src:%d  dst:%d]\n",len,bufSize);
   return bufSize;
}



#endif
