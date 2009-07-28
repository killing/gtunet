/* os.h
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


#ifdef __cplusplus
	extern "C"
	{
#endif


#ifndef _OS_H_
#define _OS_H_

#ifdef _WIN32
#	pragma warning(disable : 4115)  //named type definition in parentheses
#	pragma warning(disable : 4100)  //unreferenced formal parameter, some parameter is not used!!!!!
#	pragma warning(disable : 4206)  //translation unit is empty, some source file is empty!!!!!!!
#endif


#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if defined(_LINUX) && !defined(_POSIX)
#	define _POSIX
#endif

#ifdef _WIN32
#	include <direct.h>
#	include <io.h>
#	include <windows.h>
	typedef unsigned short USHORT;
#	define F_OK 		00 //Existence only 
#	define W_OK			02 // Write permission 
#	define R_OK			04 // Read permission 
#	define WR_OK		06 // Read and write permission 
#	define snprintf	_snprintf
#	define vsnprintf _vsnprintf

#	define THREADRET POINTER WINAPI

	typedef unsigned short UINT16;
	typedef unsigned char UINT8;
	
	
#	pragma comment(lib, "ws2_32.lib")
#endif


#ifdef _POSIX
#	include <sys/ioctl.h>
#	include <sys/types.h>
#	include <sys/time.h>
#	include <sys/socket.h>
#	include <sys/stat.h>
#	include <arpa/inet.h>
#	include <unistd.h>
#	include <netdb.h>
#	include <pthread.h>
#	include <semaphore.h>
#	include <signal.h>
#	include <dirent.h>

#	ifndef strnicmp
#		define strnicmp(a,b,c) strncasecmp(a,b,c)
#	endif

#	ifndef strincmp
#		define strincmp(a,b,c) strnicmp(a,b,c)
#	endif

#	ifndef strcmpi
#		define strcmpi strcasecmp
#	endif

	typedef int SOCKET;
	typedef unsigned int UINT;
	typedef long LONG;
	typedef unsigned int BOOL;
	typedef char CHAR;
	typedef unsigned char BYTE;
	typedef unsigned short USHORT;
	typedef int INT;
	typedef int INT32;
	typedef void VOID;
	typedef unsigned long DWORD;
	typedef unsigned short WORD;
	
	typedef unsigned long UINT32;
	typedef unsigned short UINT16;
	typedef unsigned char UINT8;

	typedef struct sockaddr_in SOCKADDR_IN;
	typedef struct hostent HOSTENT;

#	define THREADRET POINTER
	
#	ifndef FALSE
#		define FALSE 0
#	endif

#	ifndef TRUE
#		define TRUE (!FALSE)
#	endif


#endif

#ifdef _BSD
	#include <netinet/in.h>
#endif

#	ifndef OK
#		define OK 0
#	endif

#	ifndef ERR
#		define ERR (-1)
#	endif

#	ifndef WINAPI
#		define WINAPI
#	endif

#define os_new(type, count) ((type *)os_malloc(sizeof(type) * (count)))
#define os_free(p) os_free_ex(p) //{if(p) os_free_ex(p); }

#define os_zero(t) memset(&t, sizeof(t), 0)

typedef void * POINTER;

#define dprintf DPRINTF
void DPRINTF(char *fmt, ...);
//void FDPRINTF(char *fmt, ...);


/////////////////////////////////
//        String
/////////////////////////////////
typedef struct _STRING
{
	CHAR *str;
	INT len;    
	INT allocated_len;
}STRING;

char *strrstr(char *main, char *sub);

STRING *string_new(CHAR *init);
STRING *string_free(STRING *str);
STRING *string_assign(STRING *str, CHAR *init);
STRING *string_truncate(STRING *str, INT len);
STRING *string_append(STRING *str, CHAR *newstr);
STRING *string_nappend(STRING *str, CHAR *newstr, INT len);
STRING *string_replace(STRING *str, CHAR *oldstr, CHAR *newstr, BOOL incase);
STRING *string_set_size(STRING *str, INT size);


/////////////////////////////////
//        Buffer
/////////////////////////////////
typedef struct _BUFFER
{
	BYTE *data;
	INT len;    
	INT allocated_len;
}BUFFER;

#define BUFDWORD(p)		(*(DWORD *)(p))
#define BUFWORD(p)		(*(WORD *)(p))
#define	BUFBYTE(p)		(*(BYTE *)(p))

#define BUF_FETCH_BYTE(p)	(p+=1, *((BYTE *)(p-1)))
#define BUF_FETCH_WORD(p)	(p+=2, *((WORD *)(p-2)))
#define BUF_FETCH_DWORD(p)	(p+=4, *((DWORD *)(p-4)))

#define BUF_ROLLBACK_BYTE(p)	(p-=1)
#define BUF_ROLLBACK_WORD(p)	(p-=2)
#define BUF_ROLLBACK_DWORD(p)	(p-=4)

#define BUF_ROLL(p, len)		(p += (len))

BUFFER *buffer_new(UINT len);
BUFFER *buffer_free(BUFFER *buf);
BUFFER *buffer_clear(BUFFER *buf);
BOOL buffer_ptr_in(BUFFER *buf, BYTE *p);
BUFFER *buffer_append(BUFFER *buf, BYTE *newdata, INT len);
BUFFER *buffer_rollto(BUFFER *buf, BYTE *p);
BOOL buffer_fetch_DWORD(BUFFER *buf, BYTE **ptr, DWORD *d);
BOOL buffer_fetch_WORD(BUFFER *buf, BYTE **ptr, WORD *w);
BOOL buffer_fetch_BYTE(BUFFER *buf, BYTE **ptr, BYTE *b);
BOOL buffer_fetch_STRING(BUFFER *buf, BYTE **ptr, STRING **pstr, INT len);
BOOL buffer_fetch_bytes(BUFFER *buf, BYTE **ptr, BYTE *bytes, INT len);
BUFFER *buffer_delete(BUFFER *buf, INT from, INT to);
BOOL buffer_has_data(BUFFER *buf, BYTE *p, INT len);
BYTE *bufchr(BUFFER *buf, BYTE *startpos, BYTE c);
BYTE *bufstr(BUFFER *buf, BYTE *startpos, CHAR *str);
BYTE *bufstri(BUFFER *buf, BYTE *startpos, CHAR *str);



/////////////////////////////////
//         LOCK
/////////////////////////////////
typedef struct _LOCK
{
#ifdef _WIN32
	CRITICAL_SECTION lock;
#endif

#ifdef _LINUX
	pthread_mutex_t lock;
#endif
#ifdef _BSD
	pthread_mutex_t lock;
#endif
}LOCK;

LOCK *os_lock_create();
VOID os_lock_lock(LOCK *lock);
BOOL os_lock_trytolock(LOCK *lock);
VOID os_lock_unlock(LOCK *lock);
LOCK *os_lock_free(LOCK *lock);

/////////////////////////////////
//           List
/////////////////////////////////
typedef struct _LIST
{
	POINTER data;
	//LOCK *lock;       //only used by the first node
	struct _LIST *prev, *next;
}LIST;
INT list_length(LIST *list);
LIST *list_dup(LIST *list);
LIST *list_next(LIST *list);
LIST *list_nth(LIST *list, UINT n);
POINTER list_nth_data(LIST *list, UINT n);
LIST *list_find(LIST *list, POINTER data);
LIST *list_new(LIST *list);
LIST *list_free(LIST *list);
LIST *list_append(LIST *list, POINTER data);
LIST *list_insert_after(LIST *list, POINTER data);
LIST *list_remove(LIST *list, POINTER data, BOOL once);
LIST *list_remove_nth(LIST *list, UINT n);
//BOOL list_lock(LIST *list);      //if the list can be locked(not NULL), return TRUE;
//VOID list_unlock(LIST *list);



/////////////////////////////////
//         Thread
/////////////////////////////////
enum
{
	THREAD_RUNNING,
	THREAD_PAUSED,
	THREAD_KILLED
};


typedef struct _THREAD
{
#ifdef _WIN32
	HANDLE thread;
	DWORD  threadid;
#endif

#ifdef _LINUX
	pthread_t thread;
#endif
#ifdef _BSD
	pthread_t thread;
#endif
	BOOL init_complete;
	UINT status;
	POINTER param;
}THREAD;


#ifdef _WIN32
	typedef POINTER ( WINAPI * THREADPROC) (THREAD *self);
#endif

#ifdef _LINUX
	typedef POINTER (* THREADPROC) (THREAD *self);
#endif

#ifdef _BSD
	typedef POINTER (* THREADPROC) (THREAD *self);
#endif

extern THREAD global_main_thread;

THREAD *os_thread_create(THREADPROC proc, POINTER param, BOOL wait_for_init, BOOL paused);
VOID os_thread_create_tiny(THREADPROC proc, POINTER param);
VOID os_thread_pause(THREAD *thread);
VOID os_thread_init_complete(THREAD *thread);
VOID os_thread_run(THREAD *thread);
VOID os_thread_test_paused(THREAD *thread);
THREAD *os_thread_free(THREAD *thread);
VOID os_thread_kill(THREAD *thread);

BOOL os_thread_is_killed(THREAD *thread);
BOOL os_thread_is_paused(THREAD *thread);
BOOL os_thread_is_running(THREAD *thread);

BOOL os_thread_is_myself(THREAD *thread);



/////////////////////////////////
//           SOCKET
/////////////////////////////////
#define SOCKET_BUF_MAX 8192*2

UINT os_socket_init();
UINT os_socket_cleanup();

//STRING *os_socket_getip(CHAR *host);

VOID os_socket_set_nonblocking(SOCKET s, BOOL nonblocking);
SOCKET os_socket_tcp_connect(CHAR *host, USHORT port, BOOL nonblocking);
UINT os_socket_tcp_close(SOCKET s);

INT os_socket_tcp_send(SOCKET s, BYTE *buf, UINT len);
INT os_socket_tcp_recv(SOCKET s, BYTE *buf, UINT size);

VOID os_socket_tcp_status(SOCKET s, BOOL *readable, BOOL *writeable, BOOL *error);

/////////////////////////////////
//           TIME
/////////////////////////////////
typedef struct _TIME
{
	INT year, month, day, hour, minute, second;
}TIME;

//extern LOCK *timelock;

TIME os_time_get();
TIME os_time_convert(time_t t);


/////////////////////////////////
//           TIME
/////////////////////////////////
typedef struct _TICK
{
	LONG tick, delay;
}TICK;
#define TICKS_OF_DAY (24 * 60 * 60 * 1000)

LONG os_tick_get_now();
TICK *os_tick_new(LONG delay, BOOL active);
TICK *os_tick_free(TICK *tick);
BOOL os_tick_check(TICK *tick);
VOID os_tick_clear(TICK *tick);
VOID os_tick_set_delay(TICK *tick, LONG delay);
VOID os_tick_active(TICK *tick);
BOOL os_tick_is_active(TICK *tick);

/////////////////////////////////
//           OTHER
/////////////////////////////////
extern LOCK *malloclock;
VOID *os_malloc(INT size);
VOID *os_free_ex(VOID *p);

INT os_init();
INT os_cleanup();
INT os_sleep(INT microseconds);

INT os_mkdir(CHAR *dir, BOOL last_is_file);
CHAR *os_get_home_dir();

CHAR *os_adjust_filename(CHAR *filename);

STRING *os_extract_filename(CHAR *full);
STRING *os_extract_path(CHAR *full);

BOOL os_file_has_ext(CHAR *filename, CHAR *ext);
BOOL os_file_installable(CHAR *filename);


CHAR *os_convert(CHAR *str);
CHAR *os_convert_free(CHAR *str);

#endif //_OS_H_




#ifdef __cplusplus
	}
#endif

