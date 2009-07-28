#ifdef __cplusplus
	extern "C"
	{
#endif


#ifndef __UTIL_H__
#define __UTIL_H__

#include "os.h"

CHAR *buf2hex(BYTE *bytes, INT len , CHAR *szStr);
BYTE *hex2buf(CHAR *szStr, BYTE *bytes, INT *len);
CHAR *buf2output(BYTE *bytes, INT len , CHAR *szStr, int linelen);



#endif



#ifdef __cplusplus
	}
#endif

