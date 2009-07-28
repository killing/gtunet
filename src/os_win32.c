/* os_win32.c
 *
 * Copyright (C) 2004-2004 Wang Xiaoguang (Chice) <chice_wxg@hotmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 *
 * Authors: Wang Xiaoguang (Chice) <chice_wxg@hotmail.com>
 */


#ifdef _WIN32

#include "os.h"

static LOCK *debuglock = NULL;
static LOCK *malloclock = NULL;

void DPRINTF(char *fmt, ...)
{
	
    char tmp[1024*10];
	va_list ap;

	

	if(debuglock) os_lock_lock(debuglock);
    
    va_start(ap, fmt);
    _vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

	OutputDebugStringA(tmp);
	//printf("%s", tmp);
	if(debuglock) os_lock_unlock(debuglock);

}
/*
void FDPRINTF(char *fmt, ...)
{
	
    char tmp[1024*10];
	va_list ap;
	FILE *fp;
	

	if(debuglock) os_lock_lock(debuglock);
    
    va_start(ap, fmt);
    _vsnprintf(tmp, sizeof(tmp), fmt, ap);
    va_end(ap);

	fp = fopen("c:\\mytunetsvc.txt", "a+");
	fprintf(fp, "%10d %s", GetTickCount(), tmp);
	fclose(fp);

	if(debuglock) os_lock_unlock(debuglock);

}
*/
///////////////////////////////////////
//     Other:  Make Directory Ex
///////////////////////////////////////
INT os_mkdir(CHAR *dir, BOOL last_is_file)
{
	CHAR *tmp;
	CHAR *p;
	INT ret;

	tmp = os_new(CHAR, strlen(dir) + 3);
	strcpy(tmp, dir);

	for(p = tmp; *p; p++)
	{
		if(*p == '\\')
			*p = '/';
	}

	if(!last_is_file)
		strcat(tmp, "/");

	ret = -1;
	for(p = tmp; *p; p++)
	{
		if(*p == '/')
		{
			*p = 0;
			mkdir(tmp);

			if(access(tmp, 06) == 0) 
				ret = S_OK;
			else
				ret = -1;

			*p = '/';

		}
	}
	return ret;

}


/////////////////////////////////
//         Other: Home Dir
/////////////////////////////////
//TODO
//DEBUG!!!!!!!!!!!!!!!!!!!!
CHAR *os_get_home_dir()
{
#ifdef DEBUG
	return "."
#else  //ndef DEBUG
	CHAR *p;
	if( (p = getenv("USERPROFILE")) != NULL) return p;
	return "C:/My Documents";
#endif
}

/////////////////////////////////
//         Thread
/////////////////////////////////
VOID os_thread_create_tiny(THREADPROC proc, POINTER param)
{
	DWORD id;
	CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, param ,0 ,&id);
}

THREAD *os_thread_create(THREADPROC proc, POINTER param, BOOL wait_for_init, BOOL paused)
{
	THREAD *thr;
	DWORD id;
	thr = os_new(THREAD, 1);

	thr->status = paused ? THREAD_PAUSED : THREAD_RUNNING;
	thr->param = param;
	thr->init_complete = FALSE;
	thr->thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)proc, thr, 0 ,&id);
	thr->threadid = id;

	while(wait_for_init && !thr->init_complete)
	{
		os_sleep(20);
	}
	return thr;
}


BOOL os_thread_is_myself(THREAD *thread)
{
	if(thread)
		return thread->threadid == GetCurrentThreadId();
	else
		return FALSE;
}

THREAD *os_thread_free(THREAD *thread)
{
	if(thread)
	{
		return os_free(thread);
	}
	return NULL;
}

void os_thread_kill(THREAD *thread)
{
	if(thread)
	{
		if(thread->thread)
		{
			if(os_thread_is_myself(thread))
			{
				dprintf("[os_thread_kill] try to kill SELF %d\r\n", thread->thread);
			}
			else
			{
				//we shouldn't force to kill it... 
				//TerminateThread(thread->thread, 0);
				dprintf("[os_thread_kill] finished killing %d\r\n", thread->thread);
			}		
			thread->status = THREAD_KILLED;
			CloseHandle(thread->thread);
		}
		//thread->thread = 0;
		//thread->threadid = 0;
	}
}


VOID os_thread_pause(THREAD *thread)
{
	if(thread) thread->status = THREAD_PAUSED;
	if(os_thread_is_myself(thread)) os_thread_test_paused(thread);
}

VOID os_thread_run(THREAD *thread)
{
	if(thread) thread->status = THREAD_RUNNING;
}

VOID os_thread_init_complete(THREAD *thread)
{
	thread->init_complete = TRUE;
}

BOOL os_thread_is_killed(THREAD *thread)
{
	if(thread)
		return thread->status == THREAD_KILLED;
	else
		return TRUE;
}
BOOL os_thread_is_paused(THREAD *thread)
{
	if(thread)
		return thread->status == THREAD_PAUSED;
	else
		return FALSE;
}
BOOL os_thread_is_running(THREAD *thread)
{
	if(thread)
		return thread->status == THREAD_RUNNING;
	else
		return FALSE;
}

VOID os_thread_test_paused(THREAD *thread)
{
	if(thread)
		while(os_thread_is_paused(thread)) 
			os_sleep(20);
}

/////////////////////////////////
//         LOCK
/////////////////////////////////
LOCK *os_lock_create()
{
	LOCK *lock;
	lock = os_new(LOCK, 1);
	if(!lock) return NULL;
	InitializeCriticalSection(&lock->lock); 
	return lock;
}

void os_lock_lock(LOCK *lock)
{
	if(lock) EnterCriticalSection(&lock->lock); 
}

BOOL os_lock_trytolock(LOCK *lock)
{
	//if(lock) return (TryEnterCriticalSection(&lock->lock) != 0);
	return FALSE;
}

VOID os_lock_unlock(LOCK *lock)
{
	//if(debuglock != NULL && lock != debuglock) dprintf("UNLOCK %d\r\n", lock);
	if(lock) LeaveCriticalSection(&lock->lock);
}
LOCK *os_lock_free(LOCK *lock)
{
	if(lock)
	{
		DeleteCriticalSection(&lock->lock);
		return os_free(lock);
	}
	return NULL;
}


/////////////////////////////////
//         SOCKET
/////////////////////////////////
UINT os_socket_init()
{
	WSADATA wsaData;
	if(WSAStartup(MAKEWORD(1,0), &wsaData ) !=0) return -1;
	return S_OK;
}
UINT os_socket_cleanup()
{
	WSACleanup();
	return S_OK;
}

SOCKET os_socket_tcp_connect(CHAR *host, USHORT port, BOOL nonblocking)
{
	SOCKADDR_IN target;
	SOCKET s;
	HOSTENT *hostinfo;
	CHAR *ip;
	
	s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);

	if(inet_addr(host) == INADDR_NONE)
	{
		hostinfo = gethostbyname(host);
		if(hostinfo)
		{
			ip = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
		}
		else
		{
			ip = "0.0.0.0";
		}
	}
	else
	{
		ip = host;
	}

	target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(ip);
	target.sin_port = htons(port);

	os_socket_set_nonblocking(s, nonblocking);

	if(connect(s,(struct sockaddr *)&target,sizeof(target)) != 0 && !nonblocking)
	{
		//Has Error!
		dprintf("[os_socket_tcp_connect] FAIL to connect %s:%d\r\n", ip, port);
		closesocket(s);
		return 0;
	}
	else
	{
		//dprintf("[os_socket_tcp_connect] succeed to connect %s:%d\r\n", ip, port);
		return s;
	}
}

UINT os_socket_tcp_close(SOCKET s)
{
	if(s != 0) closesocket(s);
	return 0;
}

INT os_socket_tcp_send(SOCKET s, BYTE *buf, UINT len)
{
	return send(s, buf, len, 0);
}

INT os_socket_tcp_recv(SOCKET s, BYTE *buf, UINT size)
{
	//setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)); 
	return recv(s, buf, size, 0);
}

VOID os_socket_set_nonblocking(SOCKET s, BOOL nonblocking)
{
	ioctlsocket(s, FIONBIO, &nonblocking);
}

VOID os_socket_tcp_status(SOCKET s, BOOL *readable, BOOL *writeable, BOOL *error)
{
	fd_set set;
	TIMEVAL tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	if(!s)
	{
		if(*readable) *readable = FALSE;
		if(*writeable) *writeable = FALSE;
		if(*error) *error = FALSE;
		return;
	}

	if(readable)
	{
		FD_ZERO(&set);
		FD_SET(s, &set);
		select(s + 1, &set, NULL, NULL, &tv);
		*readable = FD_ISSET(s, &set);
	}

	if(writeable)
	{
		FD_ZERO(&set);
		FD_SET(s, &set);
		select(s + 1, NULL, &set, NULL, &tv);
		*writeable = FD_ISSET(s, &set);
	}
	if(error)
	{
		FD_ZERO(&set);
		FD_SET(s, &set);
		select(s + 1,  NULL, NULL, &set, &tv);
		*error = FD_ISSET(s, &set);
	}
}






/////////////////////////////////
//           OTHER
/////////////////////////////////
static int malloc_count = 0;

VOID *os_malloc(INT size)
{
	VOID *p;

	if(malloclock)
		os_lock_lock(malloclock);

	malloc_count ++;
	p = malloc(size);
	memset(p, 0, size);

	if(malloclock)
		os_lock_unlock(malloclock);

	//dprintf("malloc    count = %d\n", malloc_count);

	return p;
}

VOID *os_free_ex(VOID *p)
{
	if(!p) return NULL;

	if(p == malloclock) malloclock = NULL;

	if(malloclock)
		os_lock_lock(malloclock);

	malloc_count --;
	free(p);


	if(malloclock)
		os_lock_unlock(malloclock);

	//dprintf("free      count = %d\n", malloc_count);

	return NULL;
}

INT os_init()
{
	malloclock = NULL;
	malloclock = os_lock_create();
	//debuglock = os_lock_create();
	//dprintf("You shouldn't use global_main_thread !!!!!!!!!!\r\n");
	//TODO
	//Get thread ID !!!! not a fake handle
	global_main_thread.threadid = GetCurrentThreadId();
	os_socket_init();
	return S_OK;
}

INT os_cleanup()
{
	os_socket_cleanup();
	//os_lock_free(debuglock);
	os_lock_free(malloclock);
	malloclock = NULL;
	debuglock = NULL;
	return S_OK;
}

INT os_sleep(INT microseconds)
{
	Sleep(microseconds);
	return 0;
}

#endif

