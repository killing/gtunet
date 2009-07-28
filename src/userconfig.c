#include "ethcard.h"
#include "dot1x.h"
#include "tunet.h"
#include "userconfig.h"
#include "util.h"
#include "md5.h"

VOID userconfig_set_dot1x(USERCONFIG *userconfig, BOOL usedot1x, BOOL retrydot1x)
{
	userconfig->bUseDot1x = usedot1x;
	userconfig->bRetryDot1x = retrydot1x;
}

VOID userconfig_set_username(USERCONFIG *userconfig, CHAR *username)
{
	strcpy(userconfig->szUsername, "");

	if(username)
		strcpy(userconfig->szUsername, username);

	MD5Buffer(userconfig->szUsername, strlen(userconfig->szUsername), userconfig->md5Username);
	buf2hex(userconfig->md5Username, 16, userconfig->szMD5Username);
}

VOID userconfig_set_password(USERCONFIG *userconfig, CHAR *password)
{
	strcpy(userconfig->szPassword, "");

	if(password)
		strcpy(userconfig->szPassword, password);

	MD5Buffer(userconfig->szPassword, strlen(userconfig->szPassword), userconfig->md5Password);
	buf2hex(userconfig->md5Password, 16, userconfig->szMD5Password);

	//we will never use the real password, clean it for security.
	strcpy(userconfig->szPassword, "");
}

VOID userconfig_set_password_by_md5(USERCONFIG *userconfig, CHAR *szMD5password)
{
	BYTE tmp[16];

	strcpy(userconfig->szMD5Password, "");
	if(szMD5password)
		strcpy(userconfig->szMD5Password, szMD5password);
	memcpy(userconfig->md5Password, hex2buf(szMD5password, tmp, NULL), 16);
}

VOID userconfig_set_adapter(USERCONFIG *userconfig, CHAR *name)
{
	int i, count;
	ETHCARD_INFO ethcards[16];
	count = get_ethcards(ethcards, sizeof(ethcards));

	strcpy(userconfig->szAdaptor, name);
	for(i = 0;i < count;i ++)
	{
		if(strcmp(ethcards[i].name, name) == 0)
		{
			strcpy(userconfig->szMac, ethcards[i].mac);
			strcpy(userconfig->szIp, ethcards[i].ip);
			break;
		}
	}	
}

VOID userconfig_set_limitation(USERCONFIG *userconfig, INT limitation)
{
	userconfig->limitation = limitation;
}

VOID userconfig_set_language(USERCONFIG *userconfig, INT language)
{
	userconfig->language = language;
}

VOID userconfig_init()
{

}

VOID userconfig_cleanup()
{
}
