#ifdef __cplusplus
	extern "C"
	{
#endif

#ifndef __USERCONFIG_H__
#define __USERCONFIG_H__

#include "os.h"

#define LIMITATION_CAMPUS		0xffffffff
#define LIMITATION_NONE			0x7fffffff
#define LIMITATION_DOMESTIC		0x00000000

#define LANGUAGE_ENGLISH        0
#define LANGUAGE_CHINESE        1


typedef struct _USERCONFIG
{
	CHAR szUsername[255];
	CHAR szPassword[255];
	BYTE md5Password[32];
	BYTE md5Username[32];
	CHAR szMD5Password[255];
	CHAR szMD5Username[255];
	CHAR szIp[255];
	CHAR szAdaptor[255];
	CHAR szMac[255];

	INT  limitation;
	INT  language;
	BOOL bUseDot1x;
	BOOL bRetryDot1x;

}USERCONFIG;

VOID userconfig_init();
VOID userconfig_cleanup();

VOID userconfig_set_username(USERCONFIG *userconfig, CHAR *username);
VOID userconfig_set_password(USERCONFIG *userconfig, CHAR *password);
VOID userconfig_set_password_by_md5(USERCONFIG *userconfig, CHAR *szMD5password);
VOID userconfig_set_adapter(USERCONFIG *userconfig, CHAR *name);
VOID userconfig_set_limitation(USERCONFIG *userconfig, INT limition);
VOID userconfig_set_language(USERCONFIG *userconfig, INT language);
VOID userconfig_set_dot1x(USERCONFIG *userconfig, BOOL usedot1, BOOL retrydot1x);

extern USERCONFIG g_UserConfig;

#endif



#ifdef __cplusplus
	}
#endif
