#ifndef __LIB_ENCRY_H__
#define __LIB_ENCRY_H__

#ifdef __cplusplus
extern "C" {
#endif
int GetUserZoneInf(unsigned char ucUZId, unsigned char *ucpData, unsigned char ucDataCount);
int GetLot(unsigned char *ucpData);
int GetPortByMac(unsigned char *macaddr, unsigned int *port);
#ifdef __cplusplus
}
#endif
#endif

