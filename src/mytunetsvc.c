#include "ethcard.h"
#include "os.h"
#include "mytunet.h"
#include "des.h"
#include "md5.h"
#include "util.h"
#include "setting.h"
#include "logs.h"
#include "userconfig.h"

#include "dot1x.h"
#include "tunet.h"

#include "mytunetsvc.h"

//TCHAR                 g_szSvcConfigFile[MAX_PATH];
USERCONFIG              g_UserConfig;
enum
{
    GOAL_NONE,
    GOAL_LOGIN,
    GOAL_RELOGIN,
    GOAL_LOGOUT
};

enum
{
    DELAY_RETRY_NONE,
    DELAY_RETRY_DOT1X,
    DELAY_RETRY_TUNET
};

INT                 g_Goal = GOAL_NONE;
INT                 g_DelayRetry = DELAY_RETRY_NONE;
static INT          mytunet_default_language = LANGUAGE_ENGLISH;

int mytunetsvc_login()
{

    if(g_Goal == GOAL_LOGIN)
    {
        g_Goal = GOAL_RELOGIN;
        g_DelayRetry = DELAY_RETRY_NONE;
    }
    else
        g_Goal = GOAL_LOGIN;


    return OK;
}

int mytunetsvc_logout()
{
    //os_lock_lock(g_GoalLock);

    g_Goal = GOAL_LOGOUT;

    //os_lock_unlock(g_GoalLock);

    return OK;
}



VOID mytunetsvc_transmit_log_win32(LOG *log)
{

    BUFFER *buf = NULL;
    INT32 len;

    if(log)
    {
        buf = buffer_clear(buf);

        len = strlen(log->tag);
        buf = buffer_append(buf, (BYTE *)&len, sizeof(len));
        buf = buffer_append(buf, (BYTE *)log->tag, len);

        if(log->str)
        {
            len = strlen(log->str);
            buf = buffer_append(buf, (BYTE *)&len, sizeof(len));
            buf = buffer_append(buf, (BYTE *)log->str, len);
        }
        else
        {
            len = 0;
            buf = buffer_append(buf, (BYTE *)&len, sizeof(len));
        }

        len = log->datalen;
        if(len)
        {
            buf = buffer_append(buf, (BYTE *)&len, sizeof(len));
            buf = buffer_append(buf, (BYTE *)log->data, len);
        }
        else
        {
            buf = buffer_append(buf, (BYTE *)&len, sizeof(len));
        }

        #ifdef _WIN32
        {
            HANDLE hSlot;
            DWORD cbWritten = 0;

            hSlot = CreateFile(MYTUNET_LOGS_MAILSLOT_NAME,
                            GENERIC_WRITE, FILE_SHARE_READ, (LPSECURITY_ATTRIBUTES) NULL,
                            OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, (HANDLE) NULL);

            if(hSlot != INVALID_HANDLE_VALUE)
            {
                WriteFile(  hSlot, buf->data, buf->len,&cbWritten, NULL);
                CloseHandle(hSlot);
            }
        }
        #endif

    }

    buf = buffer_free(buf);
}

VOID mytunetsvc_transmit_log_posix(LOG *log)
{
        #ifdef _POSIX
            //...
            if(strcmp(log->tag, "MYTUNETSVC_LIMITATION") != 0
            && strcmp(log->tag, "MYTUNETSVC_STATE") != 0 )
                printf("mytunet: %s %s \n", log->tag, log->str);

        #endif
}

extern VOID mytunetsvc_transmit_log_qt(LOG *log);
#ifdef _WIN32
VOID mytunetsvc_transmit_log_qt(LOG *log){}
#endif

typedef VOID (  *MYTUNETSVC_TRANSMIT_FUNC)(LOG *log);

static INT mytunetsvc_stop_flag = 0;
MYTUNETSVC_TRANSMIT_FUNC mytunetsvc_transmit_log = NULL;

VOID mytunetsvc_set_transmit_log_func(INT i)
{
    switch(i)
    {
        case MYTUNETSVC_TRANSMIT_LOG_WIN32:
            mytunetsvc_transmit_log = mytunetsvc_transmit_log_win32;
            break;
        case MYTUNETSVC_TRANSMIT_LOG_POSIX:
            mytunetsvc_transmit_log = mytunetsvc_transmit_log_posix;
            break;
        case MYTUNETSVC_TRANSMIT_LOG_QT:
            mytunetsvc_transmit_log = mytunetsvc_transmit_log_qt;
            break;
    }
}

void mytunetsvc_set_stop_flag()
{
    mytunetsvc_stop_flag = 1;
}

void mytunetsvc_clear_stop_flag()
{
    mytunetsvc_stop_flag = 0;
}

void mytunetsvc_main()
{
    BYTE tmpbuf[1024];

    BOOL bDot1xSuccess = FALSE;
    BOOL bDot1xFailure = FALSE;
    BOOL bNetworkError = FALSE;
    BOOL bTunetFailure = FALSE;
    BOOL bTunetSuccess = FALSE;
    BOOL bKeepAliveError = FALSE;

    LOG *log = NULL;

    TICK *retry_timer = os_tick_new(10000, TRUE);
    TICK *state_timer = os_tick_new(1000, TRUE);
    INT nDot1xRetryCount = 0, nTunetRetryCount = 0;
    INT nRetryDelay = 5000;


    mytunetsvc_login();


    while(!mytunetsvc_stop_flag)
    {
        /***************************************************************/
        // Main loop for service.
        /***************************************************************/

        //如果要重新登陆，则清除原来的登陆状态
        if(g_Goal == GOAL_RELOGIN)
        {

            tunet_stop();

            g_DelayRetry = DELAY_RETRY_NONE;
            nDot1xRetryCount = 0;
            nTunetRetryCount = 0;
            //userconfig = g_UserConfig;

            tunet_reset();
            dot1x_reset();

            g_Goal = GOAL_LOGIN;
        }


        if(os_tick_check(state_timer))
        {
            g_logs = logs_append(g_logs, "MYTUNETSVC_LIMITATION", NULL, (BYTE *)&g_UserConfig.limitation,  sizeof(g_UserConfig.limitation));

            tmpbuf[0] = dot1x_get_state();
            tmpbuf[1] = tunet_get_state();
            g_logs = logs_append(g_logs, "MYTUNETSVC_STATE", NULL, tmpbuf, 2);

        }


        // 如果需要重试，则等待这个重试，而不再进行其他操作。
        if(g_DelayRetry != DELAY_RETRY_NONE)
        {

            if(os_tick_check(retry_timer))
            {
                switch(g_DelayRetry)
                {
                    case DELAY_RETRY_DOT1X:

                        if( dot1x_get_state() == DOT1X_STATE_NONE
                            || dot1x_get_state() == DOT1X_STATE_FAILURE)
                        {
                            nDot1xRetryCount ++;
                            dot1x_start(&g_UserConfig);
                        }
                        break;
                    case DELAY_RETRY_TUNET:
                        if( tunet_get_state() == TUNET_STATE_NONE
                            || tunet_get_state() == TUNET_STATE_FAILURE
                            || tunet_get_state() == TUNET_STATE_ERROR )
                        {
                            nTunetRetryCount ++;
                            tunet_start(&g_UserConfig);
                        }
                        break;
                }
                g_DelayRetry = DELAY_RETRY_NONE;
            }
            else
            {
                os_sleep(100);
                continue;
            }
        }



        if(g_Goal == GOAL_LOGIN)
        {
            //dprintf("bUseDot1x %d\n",  g_UserConfig.bUseDot1x);

            if(g_UserConfig.bUseDot1x)
            {
                if( dot1x_get_state() == DOT1X_STATE_NONE )
                {
                    dot1x_start(&g_UserConfig);
                }

                if( dot1x_get_state() == DOT1X_STATE_SUCCESS )
                {
                    if( tunet_get_state() == TUNET_STATE_NONE )
                    {
                        tunet_start(&g_UserConfig);
                    }
                }
                else
                {
                    if(dot1x_is_timeout())
                    {
                        dot1x_start(&g_UserConfig);
                    }
                }
            }
            else
            {
                if( tunet_get_state() == TUNET_STATE_NONE )
                {
                    tunet_start(&g_UserConfig);
                }
            }


            if(tunet_is_keepalive_timeout())
            {
                tunet_reset();
                if(g_UserConfig.bUseDot1x)
                    dot1x_start(&g_UserConfig);
                else
                    tunet_start(&g_UserConfig);
            }
        }
        else if(g_Goal == GOAL_LOGOUT)
        {
            tunet_stop();
            g_Goal = GOAL_NONE;
        }


RefetchLog:
        log = logs_fetch(g_logs, 0);

        if(log)
        {
            if(mytunetsvc_transmit_log)
                mytunetsvc_transmit_log(log);


            #define TagIs(s) (strcmp(log->tag, s) == 0)
            #define StrIs(s) (strcmp(log->str, s) == 0)

            bDot1xSuccess = FALSE;
            bDot1xFailure = FALSE;
            bNetworkError = FALSE;
            bTunetFailure = FALSE;
            bTunetSuccess = FALSE;
            bKeepAliveError = FALSE;

            if (TagIs("DOT1X_RECV_PACK" ))
            {
                if (StrIs("EAP_SUCCESS" ))  bDot1xSuccess = TRUE;
                if (StrIs("EAP_FAILURE" ))  bDot1xFailure = TRUE;
            }

            if (TagIs("TUNET_NETWORK_ERROR" ))  bNetworkError = TRUE;
            if (TagIs("TUNET_LOGON_ERROR" ))  bTunetFailure = TRUE;
            if (TagIs("TUNET_LOGON_KEEPALIVE_SERVER" ))  bTunetSuccess = TRUE;
            //if (TagIs("TUNET_KEEPALIVE_MONEY" ))  AccountMoney = str
            //if (TagIs("TUNET_LOGON_MONEY" ))  AccountMoney = str
            if (TagIs("TUNET_KEEPALIVE_ERROR" ))  bKeepAliveError = TRUE;

            #undef TagIs
            #undef StrIs

            if(bDot1xSuccess)
            {
                nDot1xRetryCount = 0;
            }

            if(bTunetSuccess)
            {
                nTunetRetryCount = 0;
            }

            if(bDot1xFailure && g_UserConfig.bUseDot1x)
            {
                dot1x_reset();
                g_DelayRetry = DELAY_RETRY_DOT1X;
                nRetryDelay = 5 + nDot1xRetryCount * 2;
                if(nRetryDelay >= 30) nRetryDelay = 30;
                nRetryDelay *= 1000;
                os_tick_set_delay(retry_timer, nRetryDelay);
                os_tick_clear(retry_timer);
            }

            if(bTunetFailure)
            {
                dot1x_reset();
                g_DelayRetry = DELAY_RETRY_TUNET;
                nRetryDelay = 120 + nTunetRetryCount * 60;
                if(nRetryDelay >= 5 * 60) nRetryDelay = 5 * 60;
                nRetryDelay *= 1000;
                os_tick_set_delay(retry_timer, nRetryDelay);
                os_tick_clear(retry_timer);

                if(nTunetRetryCount >= 10)
                {
                    tunet_reset();
                    g_Goal =  GOAL_NONE;
                }
            }

            if( (bNetworkError || bKeepAliveError) && g_Goal == GOAL_LOGIN)
            {
                tunet_reset();
                dot1x_reset();

                if(g_UserConfig.bUseDot1x)
                {
                    //网络错误要重新连接 802.1x
                    nDot1xRetryCount = 0;
                    g_DelayRetry = DELAY_RETRY_DOT1X;
                    nRetryDelay = 5;
                }
                else
                {
                    //网络错误要重新连接 Tunet

                    //网络错误并不增加tunet的重试次数
                    nTunetRetryCount = 0;
                    g_DelayRetry = DELAY_RETRY_TUNET;
                    nRetryDelay = 1;
                }

                nRetryDelay *= 1000;
                os_tick_set_delay(retry_timer, nRetryDelay);
                os_tick_clear(retry_timer);
            }

            log = log_free(log);
            goto RefetchLog;
        }
        os_sleep(20);
    }

    tunet_stop();
    if(g_UserConfig.bUseDot1x) dot1x_stop();

    tmpbuf[0] = DOT1X_STATE_NONE;
    tmpbuf[1] = DOT1X_STATE_NONE;

    g_logs = logs_append(g_logs, "MYTUNETSVC_STATE", NULL, tmpbuf, 2);

    while(1)
    {
        log = logs_fetch(g_logs, 0);

        if(log)
        {
            if(mytunetsvc_transmit_log)
                mytunetsvc_transmit_log(log);
            log = log_free(log);
        }
        else
        {
            break;
        }
    }

    retry_timer = os_tick_free(retry_timer);
    state_timer = os_tick_free(state_timer);

}


static CHAR szConfigFile[1024] = {0};

VOID mytunetsvc_set_config_file(CHAR *s)
{
    if(!s)
    {
        strcpy(szConfigFile, "NULL");
        return;
    }

    if(strlen(s) > sizeof(szConfigFile) - 1)
    {
        strcpy(szConfigFile, "NULL");
        return;
    }
    strcpy(szConfigFile, s);
}

CHAR *mytunetsvc_config_file()
{
    if(strcmp(szConfigFile, "NULL") == 0)
        return NULL;

    #ifdef _WIN32
        if(szConfigFile[0] == 0)
        {
            GetWindowsDirectory(szConfigFile, sizeof(szConfigFile));
            lstrcat(szConfigFile, "\\mytunetsvc.ini");
        }
    #endif

    #ifdef _POSIX
        if(szConfigFile[0] == 0)
        {
            strcpy(szConfigFile, "/etc/mytunet.conf");
        }
    #endif
    return szConfigFile;
}


INT EncodePasswordByUsername(CHAR *username, CHAR *passwordmd5, CHAR *encoded)
{
    BYTE md5[16];
    des3_context ctx3;
    BYTE output[16];
    BYTE pwdmd5[16];

    CHAR vn[200];

    #ifdef _WIN32
        DWORD sn;
        GetVolumeInformation("C:\\", vn, sizeof(vn), &sn, NULL, NULL, NULL, 0);
        snprintf(vn, sizeof(vn), "%s%x", username, sn);
    #else
        snprintf(vn, sizeof(vn), "%s", username);
    #endif

    //MD5Buffer(username, strlen(username), md5);
    MD5Buffer(vn, strlen(vn), md5);

    hex2buf(passwordmd5, pwdmd5, NULL);

    des3_set_3keys( &ctx3, md5, md5 + 8, md5);
    des3_encrypt( &ctx3, (BYTE *)pwdmd5, output);

    des3_set_3keys( &ctx3, md5, md5 + 8, md5);
    des3_encrypt( &ctx3, (BYTE *)pwdmd5 + 8, output + 8);

    buf2hex(output, 16, encoded);

    return OK;
}


INT DecodePasswordByUsername(CHAR *username, CHAR *encodedstr, CHAR *decoded)
{
    BYTE md5[16];
    des3_context ctx3;
    BYTE output[16];
    BYTE encoded[16];

    CHAR vn[200];

    #ifdef _WIN32
        DWORD sn;
        GetVolumeInformation("C:\\", vn, sizeof(vn), &sn, NULL, NULL, NULL, 0);
        snprintf(vn, sizeof(vn), "%s%x", username, sn);
    #else
        snprintf(vn, sizeof(vn), "%s", username);
    #endif

    //MD5Buffer(username, strlen(username), md5);
    MD5Buffer(vn, strlen(vn), md5);


    hex2buf(encodedstr, encoded, NULL);

    des3_set_3keys( &ctx3, md5, md5 + 8, md5);
    des3_decrypt( &ctx3, (BYTE *)encoded, output);

    des3_set_3keys( &ctx3, md5, md5 + 8, md5);
    des3_decrypt( &ctx3, (BYTE *)encoded + 8, output + 8);

    buf2hex(output, 16, decoded);

    return OK;
}


INT WINAPI mytunetsvc_set_user_config(CHAR *username, CHAR *password, BOOL isMD5Pwd, CHAR *adapter, INT limitation, INT language)
{
    BYTE md5[16];
    CHAR pwd[200];
    CHAR encoded[100];
    CHAR *szConfigFile = mytunetsvc_config_file();

    if(!isMD5Pwd)
    {
        MD5Buffer(password, strlen(password), md5);
        buf2hex(md5, 16, pwd);
        //puts("Not MD5");
    }
    else
    {
        if(strlen(password) != 32 && strlen(password) != 0)
            return ERR;

        strcpy(pwd, password);
    }

    EncodePasswordByUsername(username, pwd, encoded);

    setting_write(szConfigFile, "username", username);

    if(isMD5Pwd && strlen(password) == 0)
        setting_write(szConfigFile, "password", "");
    else
        setting_write(szConfigFile, "password", encoded);

    setting_write(szConfigFile, "adapter", adapter);
    setting_write_int(szConfigFile, "limitation", limitation);
    setting_write_int(szConfigFile, "language", language);



    return OK;
}



INT WINAPI mytunetsvc_set_user_config_dot1x(BOOL usedot1x, BOOL retrydot1x)
{
    CHAR *szConfigFile = mytunetsvc_config_file();

    setting_write_int(szConfigFile, "usedot1x", usedot1x);


    setting_write_int(szConfigFile, "retrydot1x", retrydot1x);
    //由服务来处理，所以不采取连续重试的方法。
    setting_write_int(szConfigFile, "retrydot1x", 0);

    return OK;
}

INT WINAPI mytunetsvc_set_default_language(INT language)
{
    mytunet_default_language = language;

    return OK;
}

INT WINAPI mytunetsvc_set_global_config_from(USERCONFIG *uc)
{
    memcpy(&g_UserConfig, uc, sizeof(USERCONFIG));
    return 0;
}

INT WINAPI mytunetsvc_get_user_config(USERCONFIG *uc)
{
    STRING *str = NULL;
    CHAR username[200], password[200];
    CHAR *szConfigFile = mytunetsvc_config_file();

    memset(uc, 0, sizeof(USERCONFIG));

    str = setting_read(szConfigFile, "username", "");
    if(strlen(str->str) >= sizeof(username))
        strcpy(username, "");
    else
        strcpy(username, str->str);
    str = string_free(str);


    str = setting_read(szConfigFile, "password", "");
    DecodePasswordByUsername(username, str->str, password);
    str = string_free(str);


    userconfig_set_username(uc, username);
    userconfig_set_password_by_md5(uc, password);


    str = setting_read(szConfigFile, "adapter", "");
    userconfig_set_adapter(uc, str->str);
    str = string_free(str);

    userconfig_set_language(uc, setting_read_int(szConfigFile, "language", mytunet_default_language));
    userconfig_set_limitation(uc, setting_read_int(szConfigFile, "limitation", 0));

    userconfig_set_dot1x(uc, setting_read_int(szConfigFile, "usedot1x", 1), 0);

    return OK;
}



void mytunetsvc_init()
{
    mytunet_init();
}

void mytunetsvc_cleanup()
{
    mytunet_cleanup();
}


