#ifndef _ANTS_TOOL_FONTS_H_
#define _ANTS_TOOL_FONTS_H_
#ifdef __cplusplus
extern "C" {
#endif

int Ants_font_UniCode_to_utf8(unsigned short unicode,unsigned char *pUtf8);
unsigned short Ants_font_utf8_toUniCode2(unsigned char *pUtf8,int *pnRetBytes);
unsigned short Ants_font_UniCode_to_gb2312(unsigned short uniCode);
unsigned short Ants_font_gb2312_toUniCode(unsigned short gb2312);
int Ants_font_utf8_to_gb2312_string(unsigned char *pUTF8,unsigned char *pGB2312,int nBufsize);
int Ants_font_gb2312_to_utf8_string(unsigned char *pGB2312,unsigned char *pUTF8,int nBufsize);

char* Ants_tool_Base64Encode(char *base64code, unsigned int base64length,char *pencOut /*[(base64length + 2) / 3 * 4 + 1]*/);






#ifdef __cplusplus
}
#endif

#endif

