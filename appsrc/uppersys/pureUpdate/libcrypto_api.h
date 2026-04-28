#ifndef __LIBCRYPTO_API_H__
#define __LIBCRYPTO_API_H__
#ifdef WIN32
#define LIBCRYPTO_API __declspec(dllexport)
#elif defined(__cplusplus)
#define LIBCRYPTO_API extern "C"
#else
#define LIBCRYPTO_API 
#endif
LIBCRYPTO_API char * CryptoMgr_Dec(char *pOrgString,char *pNewString,int nNewSize);
LIBCRYPTO_API char* ovfs_auth_EncryptString(int nMethod,char *pOrgString,char *pNewString,int nNewSize);
LIBCRYPTO_API char* ovfs_auth_DecryptString(char *pOrgString,int *lpEncryptType,char *pNewString,int nNewSize);
LIBCRYPTO_API char * ovfs_cert_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,char *szTime,int *lpOutLen);
LIBCRYPTO_API char * ovfs_Licence_Enc(char *szSN/*[20 + 1]*/,char *szHardware,char *szUUID,char *szTime,char **szOtherNames,char **szOtherValues,int nOtherCnt,int *lpOutLen);

LIBCRYPTO_API int ovfs_Licence_dec(char *szLicence,char **szHardware,char **szUUID,char **szSerialNumber,char **szCertificate,char **szTime,void/*cJSON_Struct*/ **pOutJson);




#endif

