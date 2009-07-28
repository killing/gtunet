/* os_linux.c
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


#include "os.h"

#ifdef _POSIX

void DPRINTF(char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
}


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

	ret = ERR;
	for(p = tmp; *p; p++)
	{
		if(*p == '/')
		{
			*p = 0;
			mkdir(tmp, 0777);

			if(access(tmp, 06) == 0) 
				ret = OK;
			else
				ret = ERR;

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
	CHAR *p;
	if( (p = getenv("HOME")) != NULL) return p;
	return (CHAR *)"";
}


/////////////////////////////////
//         Other: Extract Filename/Path
/////////////////////////////////
STRING *os_extract_filename(CHAR *full)
{
	char *p;
	if(!full) return NULL;
	p = strrchr(full, '/');
	if(p)return string_new(p + 1);
	return string_new(full);
}

STRING *os_extract_path(CHAR *full)
{
	char *p;
	if(!full) return NULL;
	p = strrchr(full, '/');
	if(p && p!=full) return string_nappend(NULL, full, p - full);
	return string_new("");

}

/////////////////////////////////
//         Thread
/////////////////////////////////
typedef POINTER (* LINUXTHREADPROC) (VOID *);
THREAD *os_thread_create(THREADPROC proc, POINTER param, BOOL wait_for_init, BOOL paused)
{
	THREAD *thr;
	pthread_t thread;
	
	thr = os_new(THREAD, 1);
	if(!thr) return NULL;
	
	thr->status = paused ? THREAD_PAUSED : THREAD_RUNNING;
	thr->param = param;
	thr->init_complete = FALSE;

	pthread_create(&thread, NULL, (LINUXTHREADPROC)proc, thr);
	thr->thread = thread;
	
	while(wait_for_init && !thr->init_complete)
		os_sleep(20);

	return thr;
}

BOOL os_thread_is_myself(THREAD *thread)
{
	return thread->thread == pthread_self();
}


THREAD *os_thread_free(THREAD *thread)
{
	if(thread)
	{
		return (THREAD *)os_free(thread);
	}
	return NULL;
}

void os_thread_kill(THREAD *thread)
{
	if(thread)
	{
		if(thread->thread)
		{
			if(thread->thread == pthread_self())
			{
//				dprintf("[os_thread_kill] try to kill SELF 0x%x\r\n", thread->thread);
				thread->status = THREAD_KILLED;
				//pthread_exit(0);
				//dprintf("[os_thread_kill] ERROR! FAILED TO KILL SELF %d\r\n", thread->thread);
			}
			else
			{
				
				//pthread_cancel(thread->thread);
				thread->status = THREAD_KILLED;
//				dprintf("[os_thread_kill] finished killing 0x%x\r\n", thread->thread);
			}		
		}
		thread->thread = 0;
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

//int pthread_kill (pthread_t THREAD, int SIGNO);
//SIGKILL
//SIGTERM

/////////////////////////////////
//         LOCK
/////////////////////////////////
LOCK *os_lock_create()
{
	LOCK *lock;
	pthread_mutex_t mutex;
	lock = os_new(LOCK, 1);
	if(!lock) return NULL;
	pthread_mutex_init(&mutex, NULL);
	lock->lock = mutex;
	return lock;
}

VOID os_lock_lock(LOCK *lock)
{
	if(lock) pthread_mutex_lock(&lock->lock); 
	//dprintf("In LOCK\r\n");
}

BOOL os_lock_trytolock(LOCK *lock)
{
	//TODO
	//DEBUG!!!!!
	if(lock) return !pthread_mutex_trylock(&lock->lock);
	//return 1;
	return 0;
}

void os_lock_unlock(LOCK *lock)
{
	if(lock) pthread_mutex_unlock(&lock->lock);
	//dprintf("Out LOCK\r\n");
}
LOCK *os_lock_free(LOCK *lock)
{
	if(lock) 
	{
		pthread_mutex_destroy(&lock->lock);
		return (LOCK *)os_free(lock);
	}
	return NULL;
}



/////////////////////////////////
//         SOCKET
/////////////////////////////////
UINT os_socket_init()
{
	//WSADATA wsaData;
	//if(WSAStartup(MAKEWORD(1,0), &wsaData ) !=0) return DLD_ERR_SOCKET_INIT;
	return OK;
}
UINT os_socket_cleanup()
{
	//WSACleanup();
	return OK;
}

/*
STRING *os_socket_getip(CHAR *host)
{
	HOSTENT hent;
	HOSTENT *hostinfo, *result; 

	//CHAR ip[4*5];
	CHAR buf[1024];
	INT lerr;
	STRING *ip;
	
	ip = string_new("0.0.0.0");
	
	dprintf("[os_socket_host2ip]Try to get IP of %s\r\n", host);
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

		
		hostinfo = &hent;
		if(gethostbyname_r(host, hostinfo, buf, sizeof(buf), &result, &lerr) == 0)
		{
			//strcpy(ip, inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list));
			ip = string_assign(ip, inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list));
		}
		else
		{
			ip = string_assign(ip, "0.0.0.0");
		}*
	}
	else
	{
		ip = string_assign(ip, host);
	}
	dprintf("[os_socket_host2ip] Get IP OK %s\r\n", ip->str);
	return ip;
}

SOCKET os_socket_tcp_connect(CHAR *ip, USHORT port, BOOL nonblocking)
{
	SOCKADDR_IN target;
	SOCKET s;
	target.sin_family = AF_INET;
    target.sin_addr.s_addr = inet_addr(ip);
	target.sin_port = htons(port);

	s=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);	
	os_socket_set_nonblocking(s, nonblocking);
	
	if(strchr(ip, '.') == strrchr(ip, '.')) return 0;

	if(connect(s,(struct sockaddr *)&target,sizeof(target)) != 0  && !nonblocking)
	{
		//Has Error!
		dprintf("[os_socket_tcp_connect] FAIL to connect %s:%d", ip, port);
		shutdown(s, SHUT_RDWR);
		close(s);
		return 0;
	}
	else
	{
		//dprintf("[os_socket_tcp_connect] succeed to connect %s:%d", ip, port);
		return s;
	}
	return 0;
}
*/


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
		shutdown(s, SHUT_RDWR);
		close(s);
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
	if(s != 0)
	{
		shutdown(s, SHUT_RDWR);
		close(s);
	}
	return 0;
}

INT os_socket_tcp_send(SOCKET s, BYTE *buf, UINT len)
{
	return send(s, buf, len, 0);
}

INT os_socket_tcp_recv(SOCKET s, BYTE *buf, UINT size)
{
	return recv(s, buf, size, 0);
}


VOID os_socket_set_nonblocking(SOCKET s, BOOL nonblocking)
{
	ioctl(s, FIONBIO, &nonblocking);
}

VOID os_socket_tcp_status(SOCKET s, BOOL *readable, BOOL *writeable, BOOL *error)
{
	fd_set set;
	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;
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
VOID *os_malloc(INT size)
{
	VOID *p;
//	printf("os_malloc: %d bytes\n", size);

	p = malloc(size);
	if (p == 0)
	{
		perror("malloc failed!");
		exit(1);
	}
    memset(p, 0, size);
    
	if(!p) dprintf("[os_malloc] ERROR! failed to malloc %d\r\n", size);

	return p;
}

VOID *os_free_ex(VOID *p)
{
	if(!p) return NULL;
	free(p);
	return NULL;
}

INT os_init()
{
	global_main_thread.thread = pthread_self();
	os_socket_init();
	return OK;
}

INT os_cleanup()
{
	os_socket_cleanup();
	return OK;
}

INT os_sleep(INT microseconds)
{
	return usleep(microseconds * 1000);
}



#endif //_POSIX
