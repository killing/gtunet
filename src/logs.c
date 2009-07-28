
#include "logs.h"

LOGS *g_logs = NULL;

LOG *log_new(CHAR *tag, CHAR *str, BYTE *data, INT datalen)
{
	LOG *log;
	log = os_new(LOG, 1);
	
	if(strlen(tag) >= sizeof(log->tag))
	{
		dprintf("[os_log_new] tag is too long!\n");
		strcpy(log->tag, "TAG_TOO_LONG");
	}
	else
	{
		strcpy(log->tag, tag);
	}

	if(str)
	{
		log->str = os_new(CHAR, strlen(str) + 1);
		strcpy(log->str, str);
	}

	if(data && datalen > 0)
	{
		log->data = os_new(BYTE, datalen);
		memcpy(log->data, data, datalen);

		log->datalen = datalen;
	}

	return log;
}

LOG *log_free(LOG *log)
{
	if(log->str) log->str = (CHAR *)os_free(log->str);
	if(log->data) log->data = (BYTE *)os_free(log->data);

	return (LOG *)os_free(log);
}

LOGS *logs_new()
{
	LOGS *logs = os_new(LOGS, 1);
	logs->lock = os_lock_create();
	logs->logs = NULL;

	return logs;
}

LOGS *logs_free(LOGS *logs)
{
	LIST *l;

	os_lock_lock(logs->lock);

	for(l = logs->logs; l ; l = l->next)
	{
		log_free((LOG *)l->data);
	}
	logs->logs = list_free(logs->logs);
	logs->lock = os_lock_free(logs->lock);

	return (LOGS *)os_free(logs);
}

LOGS *logs_append(LOGS *logs, CHAR *tag, CHAR *str, BYTE *data, INT datalen)
{
	LOG *log = log_new(tag, str, data, datalen);
	
	if(!logs) logs = logs_new();
	os_lock_lock(logs->lock);
	logs->logs = list_append(logs->logs, log);
	os_lock_unlock(logs->lock);

	return logs;
}

LOGS *logs_remove(LOGS *logs, LOG *log)
{
	if(!logs) return NULL;
	os_lock_lock(logs->lock);
	logs->logs = list_remove(logs->logs, log, TRUE);
	os_lock_unlock(logs->lock);

	log = log_free(log);

	return logs;
}

 
LOG *logs_fetch(LOGS *logs, INT n)
{
	LOG *curlog = NULL;
	LOG *outlog = NULL;

	os_lock_lock(logs->lock);
	curlog = (LOG *)list_nth_data(logs->logs, n);

	if(curlog)
	{
		outlog = log_new(curlog->tag, curlog->str, curlog->data, curlog->datalen);

		logs->logs = list_remove(logs->logs, curlog, TRUE);
		
		log_free(curlog);
	}
	os_lock_unlock(logs->lock);
	return outlog;
}

INT	 logs_count(LOGS *logs)
{
	INT count;
	if(!logs) return 0;
	os_lock_lock(logs->lock);
	count = list_length(logs->logs);
	os_lock_unlock(logs->lock);

	return count;
}


VOID logs_init()
{
	g_logs = logs_new();
}

VOID logs_cleanup()
{
	g_logs = logs_free(g_logs);
}

