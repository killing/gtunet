
#ifdef __cplusplus
	extern "C"
	{
#endif


#ifndef __LOGS_H__
#define __LOGS_H__


/////////////////////////////////
//         Log System
/////////////////////////////////
#include "os.h"

typedef struct _LOG
{
	CHAR tag[100];
	CHAR *str;
	BYTE *data;
	INT  datalen;
}LOG;

typedef struct _LOGS
{
	LOCK *lock;
	LIST *logs;
}LOGS;

LOG *log_new(CHAR *tag, CHAR *str, BYTE *data, INT datalen);
LOG *log_free(LOG *log);
LOGS *logs_new();
LOGS *logs_free(LOGS *logs);
LOGS *logs_append(LOGS *logs, CHAR *tag, CHAR *str, BYTE *data, INT datalen);
LOGS *logs_remove(LOGS *logs, LOG *log);
LOG *logs_fetch(LOGS *logs, INT n);
INT	 logs_count(LOGS *logs);

VOID logs_init();
VOID logs_cleanup();

extern LOGS *g_logs;
#endif





#ifdef __cplusplus
	}
#endif


