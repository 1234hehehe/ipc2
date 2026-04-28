#ifndef __DES_H__
#define __DES_H__

/*************************************************************************************************************
- 函数名称 : void ENThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out)
- 函数说明 : 3DES算法函数
- 输入参数 :
-           DoubleKeyStr: 24BYTE的加密密钥
-           Data:          8BYTE的明文数据
-           Out :          8BYTE的加密结果
- 输出参数 : 无
*************************************************************************************************************/
void ENThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out);

/**************************************************************************************************************
- 函数名称 : void DEThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out)
- 函数说明 : 3DES算法函数
- 输入参数 :
-           DoubleKeyStr: 24BYTE的加密密钥
-           Data:          8BYTE的加密数据
-           Out :          8BYTE的解密结果
- 输出参数 : 无
**************************************************************************************************************/
void DEThreeDES(unsigned char *DoubleKeyStr,unsigned char *Data,unsigned char *Out);


#endif
