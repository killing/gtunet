/* os.c
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

THREAD global_main_thread;

static LOCK *timelock;



/////////////////////////////////
//  Make the filename valid
/////////////////////////////////
CHAR *os_adjust_filename(CHAR *filename)
{
	CHAR *p;
	if(filename)
	{
		p = filename;
		while(*p)
		{
			if( *p == '\'' ||
				*p == '\"' ||
				*p == '\\' ||
				*p == '\t' ||
				*p == '=' ||
				*p == ',' ||
				*p == ';' ||
				*p == ':' ||
				*p == '*' ||
				*p == '?' ||
				*p == '|' ||
				*p == '/' ||
				*p == '>' ||
				*p == '<' )
			{
				*p = '_';
			}
			p++;
		}
	}
	return filename;
}

/////////////////////////////////
//           TIME
/////////////////////////////////
TIME os_time_convert(time_t t)
{
	TIME ret;

	struct tm *nt;

	os_lock_lock(timelock);
	nt = localtime(&t);
/*
             struct tm {
                      int     tm_sec;          seconds 
                      int     tm_min;          minutes 
                      int     tm_hour;         hours 
                      int     tm_mday;         day of the month 
                      int     tm_mon;          month 
                      int     tm_year;         year 
                      int     tm_wday;         day of the week 
                      int     tm_yday;         day in the year 
                      int     tm_isdst;        daylight saving time 
              };

*/	

	ret.year = nt->tm_year + 1900;
	ret.month = nt->tm_mon + 1;
	ret.day = nt->tm_mday;
	ret.hour = nt->tm_hour;
	ret.minute = nt->tm_min;
	ret.second = nt->tm_sec;
	os_lock_unlock(timelock);

	return ret;

}

TIME os_time_get()
{	
	return os_time_convert(time(NULL));	
}


LONG os_tick_get_now()
{
#ifndef _WIN32
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec % (24 * 60 * 60) ) * 1000 + tv.tv_usec / 1000;
#else
	return GetTickCount() % (24 * 60 * 60 * 1000);
#endif 
	
}


VOID os_tick_active(TICK *tick)
{
	if(!tick) return;
	tick->tick = os_tick_get_now() - tick->delay;
}

VOID os_tick_set_delay(TICK *tick, LONG delay)
{
	if(!tick) return;
	tick->delay = delay;
}

TICK *os_tick_new(LONG delay, BOOL active)
{
	TICK *tick;
	tick = os_new(TICK, 1);
	tick->tick = os_tick_get_now();
	tick->delay = delay;
	
	if(active) tick->tick -= delay;
		
	return tick;
}
TICK *os_tick_free(TICK *tick)
{
	return (TICK *)os_free(tick);
}

VOID os_tick_clear(TICK *tick)
{
	if(!tick) return ;
	tick->tick = os_tick_get_now();
}


BOOL os_tick_is_active(TICK *tick)
{
	LONG ticknew, tickold, delta;
	
	if(!tick) return FALSE;
		
	ticknew = os_tick_get_now();
	tickold = tick->tick;
	delta = tick->delay;
	
	//dprintf("[os_tick_check(] TICKOLD = %d   TICKNOW = %d    DELTA = %d\r\n", tickold, ticknew, delta);
	
	if( ((ticknew > tickold) && (ticknew - delta >= tickold)) ||  
		((ticknew < tickold) && (ticknew - delta >= tickold - TICKS_OF_DAY)) ) 	
	{
		//tick->tick = os_tick_get_now();
		return TRUE;
	}
	return FALSE;
}

BOOL os_tick_check(TICK *tick)
{
	if(os_tick_is_active(tick))
	{
		tick->tick = os_tick_get_now();
		return TRUE;
	}
	return FALSE;		
}


////////////////////////////////////////
//             Buffer
////////////////////////////////////////
BUFFER *buffer_new(UINT size)
{
	BUFFER *buf;
	buf = os_new(BUFFER, 1);
	buf->len = 0;
	buf->allocated_len = size;
	buf->data = (BYTE *)os_new(CHAR, size);
	return buf;
}

BUFFER *buffer_free(BUFFER *buf)
{
	if(!buf)return NULL;

	os_free(buf->data);
	return (BUFFER *)os_free(buf);
}

BYTE *bufchr(BUFFER *buf, BYTE *startpos, BYTE c)
{
	BYTE *p;

	if(startpos == NULL) startpos = buf->data;
	for(p = startpos; p<buf->data+buf->len; p++)
	{
		if(*p == c)
			return p;
	}
	return NULL;
}

BYTE *bufstr(BUFFER *buf, BYTE *startpos, CHAR *str)
{
	BYTE *p;
	INT l;
	l = strlen(str);
	if(startpos == NULL) startpos = buf->data;
	for(p = startpos; p<buf->data+buf->len - l + 1; p++)
	{
		if(strncmp((const CHAR *)p, str, l) == 0)
			return p;
	}
	return NULL;
}

BYTE *bufstri(BUFFER *buf, BYTE *startpos, CHAR *str)
{
	BYTE *p;
	INT l;
	l = strlen(str);
	if(startpos == NULL) startpos = buf->data;
	for(p = startpos; p<buf->data+buf->len - l + 1; p++)
	{
		if(strnicmp((const CHAR *)p, str, l) == 0)
			return p;
	}
	return NULL;
}

BUFFER *buffer_clear(BUFFER *buf)
{
	if(!buf)return buffer_new(4096);
	buf->len = 0;
	memset(buf->data, 0, buf->allocated_len);
	return buf;
}

BUFFER *buffer_delete(BUFFER *buf, INT from, INT to)
{
	BUFFER *newbuf;
	if(!buf) return NULL;
	if(from > to) return buf;
	if(from >= buf->len) return buf;
	if(to >= buf->len) to = buf->len - 1;

	newbuf = buffer_new(buf->allocated_len);
	newbuf = buffer_append(newbuf, buf->data, from - 1);
	newbuf = buffer_append(newbuf, buf->data + to + 1, buf->len - to - 1);

	buf = buffer_free(buf);

	return newbuf;
}

BOOL buffer_has_data(BUFFER *buf, BYTE *p, INT len)
{
	return buffer_ptr_in(buf, p) && buffer_ptr_in(buf, p + len);
}

BOOL buffer_ptr_in(BUFFER *buf, BYTE *p)
{
	if(!buf) return FALSE;
	if(p < buf->data || p > buf->data + buf->len) 
		return FALSE;
	return TRUE;
}

BOOL buffer_fetch_DWORD(BUFFER *buf, BYTE **ptr, DWORD *d)
{
	BYTE *p = *ptr;

	if(!buffer_has_data(buf, p, sizeof(DWORD)))
		return FALSE;

	*d = BUF_FETCH_DWORD(p);

	*ptr = p;

	return TRUE;
}

BOOL buffer_fetch_WORD(BUFFER *buf, BYTE **ptr, WORD *w)
{
	BYTE *p = *ptr;

	if(!buffer_has_data(buf, p, sizeof(WORD)))
		return FALSE;

	*w = BUF_FETCH_WORD(p);

	*ptr = p;
	return TRUE;
}

BOOL buffer_fetch_BYTE(BUFFER *buf, BYTE **ptr, BYTE *b)
{
	BYTE *p = *ptr;

	if(!buffer_has_data(buf, p, sizeof(BYTE)))
		return FALSE;

	*b = BUF_FETCH_BYTE(p);

	*ptr = p;
	return TRUE;
}

BOOL buffer_fetch_STRING(BUFFER *buf, BYTE **ptr, STRING **pstr, INT len)
{
	BYTE *p = *ptr;

	if(!buffer_has_data(buf, p, len))
		return FALSE;

	*pstr = string_nappend(NULL, (CHAR *)p, len);

	BUF_ROLL(p, len);

	*ptr = p;
	return TRUE;

}

BOOL buffer_fetch_bytes(BUFFER *buf, BYTE **ptr, BYTE *bytes, INT len)
{
	BYTE *p = *ptr;

	if(!buffer_has_data(buf, p, len))
		return FALSE;

	//pstr = string_nappend(NULL, p, len);
	memcpy(bytes, p, len);

	BUF_ROLL(p, len);

	*ptr = p;
	return TRUE;

}

BUFFER *buffer_rollto(BUFFER *buf, BYTE *p)
{
	int newlen;

	if(!buffer_ptr_in(buf, p))
	{
		if(p >= buf->data + buf->len)
			buf->len = 0;
		else
			dprintf("[buffer_rollto] error when rolling buffer.\n");

		return buf;
	}

	newlen = buf->len - (p - buf->data);

	if(newlen)
		memmove(buf->data, p, buf->len - (p - buf->data));

	buf->len = newlen;

	return buf;
}

BUFFER *buffer_append(BUFFER *buf, BYTE *newdata, INT len)
{
	BYTE *p;
	INT newlen;
	
	if(len <= 0) return buf;
	if(!buf) buf = buffer_new(len);

	if(len + buf->len > buf->allocated_len)
	{
		newlen = (len + buf->len) * 3 / 2 + 1;
		p = (BYTE *)os_new(CHAR, newlen);
		buf->allocated_len = newlen;
		memcpy(p, buf->data, buf->len);
	}
	else
	{
		p = buf->data;
	}

	memcpy(p + buf->len, newdata, len);

	if(buf->data != p)
	{
		os_free(buf->data);
		buf->data = p;
	}

	buf->len += len;
	return buf;
}


////////////////////////////////////////
//             String
////////////////////////////////////////

char *strrstr(char *main, char *sub)
{
	char *p;
	int sublen;
	
	if(!main) return NULL;
	if(!sub) return NULL;
		
	sublen = strlen(sub);
	
	p = main + strlen(main);
	while(p!=main)
	{
		if(strncmp(p, sub, sublen) == 0) return p;
		p--;
	}
	if(strncmp(p, sub, sublen) == 0) return p;
	return NULL;
}

STRING *string_new(CHAR *init)
{
	CHAR *p;
	STRING *str;
	
	if(!init) init="";
	
	if(strlen(init) == 0)
		p = os_new(CHAR, 512);
	else
		p = os_new(CHAR, strlen(init) + 1);
	
	if(!p)return NULL;
	strcpy(p, init);

	str = os_new(STRING, 1);

	if(!str)
		return (STRING *)os_free(p);

	str->str = p;
	str->len = strlen(p);
	str->allocated_len = str->len;

	return str;
}

STRING *string_set_size(STRING *str, INT size)
{
	CHAR *p;
	if(str) 
	{
		if(size <= str->allocated_len) return str;
			
		p = os_new(CHAR, size + 1);
		strcpy(p, str->str);
		os_free(str->str);
		str->str = p;
		
		str->allocated_len = size + 1;
		str->len = 0;
	}
	else
	{
		str = os_new(STRING ,1);
		str->str = os_new(CHAR, size + 1);
		str->allocated_len = size + 1;
		str->len = 0;
	}
	return str;
}
STRING *string_free(STRING *str)
{
	if(!str)return NULL;
	str->str = (CHAR *)os_free(str->str);
	return (STRING *)os_free(str);
}

STRING *string_assign(STRING *str, CHAR *init)
{
	if(str)
	{
		if(str->str != init)
		{
			os_free(str->str);
			str->str = os_new(CHAR, strlen(init) + 1);
			strcpy(str->str, init);
			str->len = strlen(init);
			str->allocated_len = str->len + 1;
		}
	}
	else
	{
		str = string_new(init);
	}

	return str;
}

STRING *string_truncate(STRING *str, INT len)
{
	if(str)
	{
		if(len<=str->len)
		{
			str->len = len;
			str->str[len] = 0;
		}
		return str;
	}
	else	
	{
		return string_new((CHAR *)"");
	}
	
}

STRING *string_append(STRING *str, CHAR *newstr)
{
	if(newstr)
	{
		return string_nappend(str, newstr, strlen(newstr));
	}
	return str;
}

STRING *string_nappend(STRING *str, CHAR *newstr, INT len)
{
	CHAR *p;
	INT newlen;
	
	if(!newstr) return NULL;

	if(!str) 
	{
		str = string_new(newstr);
		str = string_truncate(str, len);
		return str;
	}

	if (len + str->len >= str->allocated_len )
	{
		newlen = (len + str->len) * 3 / 2 + 1;
		p = os_new(CHAR, newlen);
		str->allocated_len = newlen;
		strcpy(p, str->str);
	}
	else
	{ 
		p = str->str;
	}
	//strncpy(p + str->len, newstr, len);
	strncat(p, newstr, len);

	if(p != str->str)
	{
		os_free(str->str);
		str->str = p;
	}
	str->len += len;

	str->str[str->len] = 0;
	return str;
}


STRING *string_replace(STRING *str, CHAR *oldstr, CHAR *newstr, BOOL incase)
{
	CHAR *pbackup, *pstr;
	INT oldlen, newlen;
	BOOL found;
	STRING *backup;
	if(!str) return NULL;
	if(!str->str || !oldstr || !newstr) return str;
		
	oldlen = strlen(oldstr);
	newlen = strlen(newstr);	
	
	backup = string_new(str->str);
	
	str = string_set_size(str, str->len * (1 + newlen / oldlen) );
	
	pbackup = backup->str;
	pstr = str->str;
	
	while(*pbackup)
	{
		found = FALSE;
		if(incase)
		{
			if(strnicmp(pbackup, oldstr, oldlen) == 0)
				found = TRUE;
		}
		else
		{
			if(strncmp(pbackup, oldstr, oldlen) == 0)
				found = TRUE;			
		}
		if(found)
		{
			//str = string_truncate(str, 
			strncpy(pstr, newstr, newlen);
			pstr += newlen;
			pbackup += oldlen;
		}
		else
		{
			*pstr = *pbackup;
			pstr++;
			pbackup++;
		}
	}
	string_free(backup);
	*pstr = 0;
	return str;
}


////////////////////////////////////////
//             List
////////////////////////////////////////
LIST *list_free(LIST *list)
{
	while(list)
	{
		list = list_remove(list, list->data, TRUE);
	}

	return NULL;
}

LIST *list_dup(LIST *list)
{
	LIST *l = NULL, *newlist = NULL;
	if(!list) return NULL;

	//os_lock_lock(list->lock);
	for(l = list; l != NULL; l = l->next)
	{
		newlist = list_append(newlist, l->data);
	}
	//os_lock_unlock(list->lock);
	return newlist;
}

INT list_length(LIST *list)
{
	LIST *l;
	UINT count = 1;

	if(!list)return 0;

	//os_lock_lock(list->lock);
	l = list;
	while(l->next)
	{
		l = l->next;
		count ++;
	}
	l = list;
	while(l->prev)
	{
		l = l->prev;
		count ++;
	}
	//os_lock_unlock(list->lock);
	return count;

}

LIST *list_next(LIST *list)
{
	LIST *l;

	if(!list) return NULL;

	//os_lock_lock(list->lock);
	l = list->next;
	//os_lock_unlock(list->lock);

	return l;
}

LIST *list_nth(LIST *list, UINT n)
{
	UINT i;
	LIST *l;

	if(!list) return NULL;

	//os_lock_lock(list->lock);
	l = list;
	for(i=0;i<n;i++)
	{
		if(!l) return NULL;
		l = l->next;
	}
	//os_lock_unlock(list->lock);
	return l;
}

POINTER list_nth_data(LIST *list, UINT n)
{
	LIST *l;

	l = list_nth(list, n);
	if(l) 
		return l->data;
	else
		return NULL;
}


LIST *list_find(LIST *list, POINTER data)
{
	LIST *l;
	if(!list) return NULL;
	
	//os_lock_lock(list->lock);
	l = list;
	while(l)
	{
		if(l->data == data) break;
		l = l->next;
	}
	//os_lock_unlock(list->lock);
	return l;
}

LIST *list_append(LIST *list, POINTER data)
{
	LIST *l, *ll;

	if(!list)
	{
		ll = os_new(LIST, 1);
		ll->prev = NULL;
		ll->next = NULL;
		ll->data = data;
		//ll->lock = os_lock_create();
		return ll;
	}

	//os_lock_lock(list->lock);

	l = list;
	while(l->next) l = l->next;

	ll = os_new(LIST, 1);
	ll->prev=l;
	ll->next = NULL;
	ll->data = data;
	//ll->lock = l->lock;

	l->next = ll;

	//os_lock_unlock(list->lock);

	return list;
}

LIST *list_insert_after(LIST *list, POINTER data)
{
	LIST *l1 = NULL, *l2 = NULL;
	LIST *l;

	if(!list) return list_append(NULL, data);

	//os_lock_lock(list->lock);

	l1 = list;
	l2 = list->next;

	l = os_new(LIST, 1);
	l->prev=l1;
	l->next = l2;
	l->data = data;
//	l->lock = list->lock;

	if(l1) l1->next = l;
	if(l2) l2->prev = l;

	//os_lock_unlock(list->lock);

	while(list->prev) list=list->prev;

	return list;
}

LIST *list_remove(LIST *list, POINTER data, BOOL once)
{
	LIST *l;
	LIST *l1,*l2;

	l = list;
	//if(list) os_lock_lock(list->lock);
	while(l)
	{
		if(l->data == data)
		{
			l1 = l->prev;
			l2 = l->next;

			if(l1)
				l1->next=l2;
			if(l2)
				l2->prev=l1;

			if(l1 == NULL)
			{
//				if(l2 == NULL)
//					list->lock = os_lock_free(list->lock);

				list = l2;
			}
			os_free(l);
			l = l2;
			
			if(once) return list;
		}
		else
		{
			l = l->next;
		}
	}
	
	dprintf("NOT FIND\n");
	
	//if(list) os_lock_unlock(list->lock);
	return list;
}

LIST *list_remove_nth(LIST *list, UINT n)
{
	return list_remove(list, list_nth_data(list, n), TRUE);
}


//if the list can be locked(not NULL), return TRUE;
//BOOL list_lock(LIST *list)
//{
//	if(!list) return FALSE;
//	os_lock_lock(list->lock);
	//dprintf("[list_lock] LOCK list %d\r\n", list);
//	return TRUE;
//}

//VOID list_unlock(LIST *list)
//{
//	if(!list) return;
//	os_lock_unlock(list->lock);
	//dprintf("[list_unlock] UNLOCK list %d\r\n", list);
//}


