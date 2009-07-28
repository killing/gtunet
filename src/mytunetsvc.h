#ifdef __cplusplus
    extern "C"
    {
#endif

#ifndef _MYTUNETSVC_H_
#define _MYTUNETSVC_H_

#include "os.h"
#include "userconfig.h"

void mytunetsvc_init();
void mytunetsvc_cleanup();
void mytunetsvc_set_stop_flag();
void mytunetsvc_clear_stop_flag();
int mytunetsvc_login();
int mytunetsvc_logout();
void mytunetsvc_main();

INT WINAPI mytunetsvc_set_user_config(CHAR *username, CHAR *password, BOOL isMD5Pwd, CHAR *adapter, INT limitation, INT language);
INT WINAPI mytunetsvc_set_global_config_from(USERCONFIG *uc);
INT WINAPI mytunetsvc_set_user_config_dot1x(BOOL usedot1x, BOOL retrydot1x);
INT WINAPI mytunetsvc_get_user_config(USERCONFIG *uc);
INT WINAPI mytunetsvc_set_default_language(INT language);
VOID mytunetsvc_set_config_file(CHAR *s);
enum
{
    MYTUNETSVC_TRANSMIT_LOG_WIN32,
    MYTUNETSVC_TRANSMIT_LOG_POSIX,
    MYTUNETSVC_TRANSMIT_LOG_QT
};

VOID mytunetsvc_set_transmit_log_func(INT i);

#define MYTUNET_SERVICE_NAME    "MyTunetSvc"
#define MYTUNET_SERVICE_DESC    "MyTunet, an unoffical Tsinghua Network Logon Program"

#define MYTUNET_SERVICE_LOGIN   (128 + 1)
#define MYTUNET_SERVICE_LOGOUT  (128 + 2)

#define MYTUNET_LOGS_MAILSLOT_NAME      "\\\\.\\mailslot\\MyTunet_LogsMailSlot"

#endif



#ifdef __cplusplus
    }
#endif

