#ifdef __cplusplus
	extern "C"
	{
#endif


#ifndef __TUNET_H__
#define __TUNET_H__

#include "userconfig.h"

enum
{
	TUNET_STATE_NONE,

	TUNET_STATE_LOGIN,
	TUNET_STATE_SEND_LOGON,
	TUNET_STATE_RECV_WELCOME,
	TUNET_STATE_REPLY_WELCOME,
	TUNET_STATE_RECV_REMAINING_DATA,

	TUNET_STATE_KEEPALIVE,
	TUNET_STATE_FAILURE,

	TUNET_STATE_LOGOUT,
	TUNET_STATE_LOGOUT_SEND_LOGOUT,
	TUNET_STATE_LOGOUT_RECV_LOGOUT,
	TUNET_STATE_ERROR
};

VOID tunet_stop();
BOOL tunet_is_keepalive_timeout();
VOID tunet_start(USERCONFIG *uc);
VOID tunet_reset();
INT  tunet_get_state();
VOID tunet_init();
VOID tunet_cleanup();
#endif



#ifdef __cplusplus
	}
#endif
