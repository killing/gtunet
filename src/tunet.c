#include <assert.h>

#include "dot1x.h"
#include "tunet.h"
#include "util.h"
#include "md5.h"
#include "des.h"
#include "userconfig.h"
#include "logs.h"

static THREAD *	thread_tunet = NULL;
static int		tunet_state = TUNET_STATE_NONE;
static BYTE		welcome_data[8];
static char		main_server[100] = "166.111.8.10";
static short    main_server_port = 80;
static BYTE		keepalive_key[8];
static char		keepalive_server[100] = "";
static short	keepalive_server_port = 0;
static TICK	   *keepalive_timeout = NULL;
static char		msg_server[100] = "";
static short	msg_server_port = 0;

static SOCKET main_socket = 0, keepalive_socket = 0, logout_socket = 0;

static USERCONFIG userconfig;

static BUFFER *main_socket_buffer = NULL, *keepalive_socket_buffer = NULL, *logout_socket_buffer = NULL;


static int tunet_connect_main_server()
{
	if(main_socket == 0)
		main_socket = os_socket_tcp_connect(main_server, main_server_port, TRUE);

	return OK;
}

static int tunet_connect_logout_server()
{
	if(logout_socket == 0)
		logout_socket = os_socket_tcp_connect(main_server, main_server_port, TRUE);

	return OK;
}

static int tunet_connect_keepalive_server()
{
	if(keepalive_socket == 0 && strlen(keepalive_server) != 0)
		keepalive_socket = os_socket_tcp_connect(keepalive_server, keepalive_server_port, TRUE);

	return OK;
}

static int tunet_logon_send_tunet_user()
{
	BUFFER *buf;
	BYTE tmpbuf[100];
	int len;
	UINT32 lang;

	BOOL sr, sw, se;
	
	if(!main_socket) return OK;
	
	os_socket_tcp_status(main_socket, &sr, &sw, &se);


	if(tunet_state != TUNET_STATE_LOGIN) 
		return OK;

	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "SEND_TUNET_USER", NULL, 0);
		return ERR;
	}

	if(!sw) return OK;


	buf = buffer_new(200);

	//hex2buf("12 54 55 4e 45 54 20 55 53 45 52 00 00 4e ea 00 00 00 01", tmpbuf, &len);
	//															   00 for english

	//buf = buffer_append(buf, tmpbuf, len);

	hex2buf("12 54 55 4e 45 54 20 55 53 45 52 00 00 4e ea", tmpbuf, &len);
	buf = buffer_append(buf, tmpbuf, len);

	lang = htonl(userconfig.language);
	buf = buffer_append(buf, (BYTE *)(&lang), 4);

	os_socket_tcp_send(main_socket, buf->data, buf->len);

	

	tunet_state = TUNET_STATE_RECV_WELCOME;

	logs_append(g_logs, "TUNET_LOGON_SEND_TUNET_USER", NULL, buf->data, buf->len);
	//dprintf("已经向主服务器发出登陆请求...\n");

	buf = buffer_free(buf);
	return OK;
}

static int tunet_logon_recv_welcome()
{
	BYTE tmpbuf[1024 * 8];
	CHAR  tmp[1024];

	BYTE  btag;
	UINT32  unknowntag;
	UINT32  datalen;

	BYTE *p;

	int len;
	
	const CHAR *WELCOME = "WELCOME TO TUNET";
	//int msglen = 0;

	STRING *str = NULL;

	BOOL sr, sw, se;
	
	if(!main_socket) return OK;
	
	os_socket_tcp_status(main_socket, &sr, &sw, &se);

	if(tunet_state != TUNET_STATE_RECV_WELCOME) 
		return OK;


	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_WELCOME", NULL, 0);
		return ERR;
	}

	if(!sr) return OK;


	len = os_socket_tcp_recv(main_socket, tmpbuf, sizeof(tmpbuf));
	if(len == -1)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_WELCOME", NULL, 0);
		return ERR;
	}
	if(len > 0)
	{
		main_socket_buffer = buffer_append(main_socket_buffer, tmpbuf, len);

		logs_append(g_logs, "TUNET_LOGON_RECV", "WELCOME", tmpbuf, len);

		buf2output(tmpbuf, len, tmp, 16);
		//dprintf("data received(recv welcome):\n%s\n", tmp);

		p = main_socket_buffer->data;
		while(buffer_fetch_BYTE(main_socket_buffer, &p, &btag))
		{
			switch(btag)
			{
				case 0x01:
					if(!buffer_fetch_STRING(main_socket_buffer, &p, &str, strlen(WELCOME)))
						return OK;

					if(strncmp(str->str, WELCOME, strlen(WELCOME)) != 0)
					{
						str = string_free(str);

						//TODO
						//process such error!!!!!!!!!
						logs_append(g_logs, "TUNET_LOGON_WELCOME", str->str, NULL, 0);
						tunet_state = TUNET_STATE_ERROR;
						return OK;
					}
					str = string_free(str);

					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &unknowntag))
						return OK;

					unknowntag = htonl(unknowntag);

					if(!buffer_fetch_bytes(main_socket_buffer, &p, welcome_data, 8))
						return OK;

					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;
					
					datalen = htonl(datalen);
					//dprintf("欢迎消息长 %d\n", datalen);

					if(!buffer_fetch_STRING(main_socket_buffer, &p, &str, datalen))
						return OK;

					logs_append(g_logs, "TUNET_LOGON_WELCOME", str->str, NULL, 0);

					//dprintf("%s\n", str->str);
					str = string_free(str);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					tunet_state = TUNET_STATE_REPLY_WELCOME;			

					break;

				case 0x02: 
				case 0x05:
					datalen = htonl(BUF_FETCH_DWORD(p));
					//dprintf("出错消息长 %d\n", datalen);

					str = string_new("");
					str = string_nappend(str, (CHAR *)p, datalen);
					//dprintf("%s\n", str->str);

					tunet_state = TUNET_STATE_ERROR;

					logs_append(g_logs, "TUNET_LOGON_ERROR", str->str, NULL, 0);

					str = string_free(str);

					BUF_ROLL(p, datalen);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;
				
					break;

			}
		}
		
	}

	return OK;
}



static int tunet_logon_reply_welcome()
{
	BUFFER *buf;

	BYTE des3data[12];
	des3_context ctx3;

	UINT32 limitation;


	BOOL sr, sw, se;
	
	if(!main_socket) return OK;
	
	os_socket_tcp_status(main_socket, &sr, &sw, &se);

	if(tunet_state != TUNET_STATE_REPLY_WELCOME) 
		return OK;


	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "REPLY_WELCOME", NULL, 0);
		return ERR;
	}

	if(!sw) return OK;


	memset(des3data, 0, sizeof(des3data));

    des3_set_3keys( &ctx3, userconfig.md5Password,
                           userconfig.md5Password + 8,
                           userconfig.md5Password );


	des3_encrypt( &ctx3, (uint8 *)welcome_data, (uint8 *)des3data);



	buf = buffer_new(100);
	buf = buffer_append(buf, (BYTE *)"\x03", 1);
	buf = buffer_append(buf, userconfig.md5Username, 16);
	buf = buffer_append(buf, des3data, 8);


	limitation = htonl(userconfig.limitation);
	buf = buffer_append(buf, (BYTE *)&limitation, 4);

	os_socket_tcp_send(main_socket, buf->data, buf->len);

	

	tunet_state = TUNET_STATE_RECV_REMAINING_DATA;

	logs_append(g_logs, "TUNET_LOGON_REPLY_WELCOME", NULL, buf->data, buf->len);

	//dprintf("%s\n", "已经应答welcome的数据包。准备接受剩余数据...");

	buf = buffer_free(buf);

	return OK;

}



static float tunet_imoney_to_fmoney(INT32 imoney)
{
	return ((float)imoney) / 100;
}

static int tunet_logon_recv_remaining_data()
{
	BYTE tmpbuf[1024 * 8];
	CHAR tmp[1024];

	CHAR sztmp[255];

	BYTE *p;
	INT len;
	BYTE key77[8];
	
	STRING *str = NULL;
	char des3data[12];
	des3_context ctx3;
	TIME tm;


	UINT32 datalen, port;
	UINT32 uint_money;
	BYTE btag;


	BOOL sr, sw, se;
	
	if(!main_socket) return OK;
	
	os_socket_tcp_status(main_socket, &sr, &sw, &se);


	if(tunet_state != TUNET_STATE_RECV_REMAINING_DATA && tunet_state != TUNET_STATE_KEEPALIVE)
		return OK;



	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_REMAINING_DATA", NULL, 0);
		return ERR;
	}

	if(!sr) return OK;



	len = os_socket_tcp_recv(main_socket, tmpbuf, sizeof(tmpbuf));
	if(len == -1)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_REMAINING_DATA", NULL, 0);
		return ERR;
	}


	if(len > 0)
	{
		main_socket_buffer = buffer_append(main_socket_buffer, tmpbuf, len);
		logs_append(g_logs, "TUNET_LOGON_RECV", "REMAINING", tmpbuf, len);

		buf2output(tmpbuf, len, tmp, 16);
		//dprintf("data received(recv remaining):\n%s\n", tmp);

		

		p = main_socket_buffer->data;
		while(buffer_fetch_BYTE(main_socket_buffer, &p, &btag))
		{
			switch(btag)
			{
				case 0x01:
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;

					datalen = htonl(datalen);

					memset(keepalive_server, 0, sizeof(keepalive_server));
					if(!buffer_fetch_bytes(main_socket_buffer, &p, (BYTE *)keepalive_server, datalen))
						return OK;

					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &port))
						return OK;

					port = htonl(port);
					keepalive_server_port = (short)port;

					snprintf(sztmp, sizeof(sztmp), "%s:%d", keepalive_server, keepalive_server_port);
					//dprintf("保持活动服务器：%s\n", sztmp);



					//we got the KEEPALIVE server, try to keep alive
					os_tick_clear(keepalive_timeout);
					tunet_state = TUNET_STATE_KEEPALIVE;

					logs_append(g_logs, "TUNET_LOGON_KEEPALIVE_SERVER", sztmp, NULL, 0);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					break;
				case 0x02:
					//出错消息
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;
					datalen = htonl(datalen);

					//dprintf("出错消息长 %d\n", datalen);

					if(!buffer_fetch_STRING(main_socket_buffer, &p, &str, datalen))
						return OK;

					//dprintf("%s\n", str->str);


					tunet_state = TUNET_STATE_ERROR;

					logs_append(g_logs, "TUNET_LOGON_ERROR", str->str, NULL, 0);

					str = string_free(str);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					

					break;
				case 0x05:
					if(buffer_fetch_BYTE(main_socket_buffer, &p, &btag))
					{
						if(btag != 0)
						{
							//dprintf("与消息中介服务器通信结束。\n");
							main_socket = os_socket_tcp_close(main_socket);

							logs_append(g_logs, "TUNET_LOGON_FINISH_MSGSERVER", NULL, NULL, 0);

							main_socket_buffer = buffer_rollto(main_socket_buffer, p);
							p = main_socket_buffer->data;
							
							break;
						}
					}

					BUF_ROLL(p, -1);  //restore the point for further use


					//出错消息
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;
					datalen = htonl(datalen);

					//dprintf("出错消息长 %d\n", datalen);

					if(!buffer_fetch_STRING(main_socket_buffer, &p, &str, datalen))
						return OK;

					//dprintf("%s\n", str->str);
					
					tunet_state = TUNET_STATE_ERROR;

					logs_append(g_logs, "TUNET_LOGON_ERROR", str->str, NULL, 0);

					str = string_free(str);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					
					break;
					
				case 0x04:
				case 0x08:
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;

					datalen = htonl(datalen);

					memset(msg_server, 0, sizeof(msg_server));
					if(!buffer_fetch_bytes(main_socket_buffer, &p, (BYTE *)msg_server, datalen))
						return OK;

					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &port))
						return OK;

					port = htonl(port);

					msg_server_port = (short)port;

					if(!buffer_fetch_bytes(main_socket_buffer, &p, key77, 8))
						return OK;

					//登陆消息
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &datalen))
						return OK;
					datalen = htonl(datalen);

					//dprintf("登陆消息长 %d\n", datalen);

					if(!buffer_fetch_STRING(main_socket_buffer, &p, &str, datalen))
						return OK;
					//dprintf("%s\n", str->str);

					logs_append(g_logs, "TUNET_LOGON_MSG", str->str, NULL, 0);

					str = string_free(str);


					//make a new key for keep-alive
					//dprintf("key77 == %s\n", buf2hex(key77, 8, tmp));
					memset(des3data, 0, sizeof(des3data));
					des3_set_3keys( &ctx3, userconfig.md5Password,
										   userconfig.md5Password + 8,
										   userconfig.md5Password );

					des3_encrypt( &ctx3, (uint8 *)key77, (uint8 *)des3data);

					memcpy(keepalive_key, des3data, 8);


					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					//--------------------------------
					snprintf(sztmp, sizeof(sztmp), "%s:%d", msg_server, msg_server_port);

					//dprintf("准备连接到消息中介服务器 %s 获得活动服务器地址....\n", sztmp);

					logs_append(g_logs, "TUNET_LOGON_MSGSERVER", sztmp, NULL, 0);

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					//switch to another server to get alive-server ip
					strcpy(keepalive_server, "");
					main_socket = os_socket_tcp_close(main_socket);
					main_socket = os_socket_tcp_connect(msg_server, msg_server_port, TRUE);

					break;
				case 0x1b:
					if(!buffer_fetch_DWORD(main_socket_buffer, &p, &uint_money))
						return OK;

					uint_money = htonl(uint_money);
					snprintf(sztmp, sizeof(sztmp), "%0.2f", tunet_imoney_to_fmoney(uint_money));
					//dprintf("您在登陆前的余额是：%s\n", sztmp);

					logs_append(g_logs, "TUNET_LOGON_MONEY", sztmp, NULL, 0);

					if(buffer_has_data(main_socket_buffer, p, 16))
					{
						snprintf(sztmp, sizeof(sztmp), "%d.%d.%d.%d/%d.%d.%d.%d", p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]);
						logs_append(g_logs, "TUNET_LOGON_IPs", sztmp, NULL, 0);
						//dprintf("登陆IP状况: %s\n", sztmp);
						BUF_ROLL(p, 8);

						
						tm = os_time_convert(htonl(BUFDWORD(p)));
						snprintf(sztmp, sizeof(sztmp), "%d-%d-%d %d:%d:%d", tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second );
						logs_append(g_logs, "TUNET_LOGON_SERVERTIME", sztmp, NULL, 0);
						//dprintf("当前服务器时间： %s\n", sztmp);
						BUF_ROLL(p, 4);

						tm = os_time_convert(htonl(BUFDWORD(p)));
						snprintf(sztmp, sizeof(sztmp), "%d-%d-%d %d:%d:%d", tm.year, tm.month, tm.day, tm.hour, tm.minute, tm.second );
						logs_append(g_logs, "TUNET_LOGON_LASTTIME", sztmp, NULL, 0);
						//dprintf("上次登陆时间： %s\n", sztmp);
						BUF_ROLL(p, 4);
					}
					else
					{
						return OK;
					}

					main_socket_buffer = buffer_rollto(main_socket_buffer, p);
					p = main_socket_buffer->data;

					break;
			}
		}

	}

	return OK;

}


int tunet_keepalive()
{
	BYTE tmpbuf[1024];
	BYTE repbuf[9];
	CHAR tmp[1024];
	BYTE *p;
	BYTE btag;
	BYTE data[16];

	CHAR smoney[255];

	des_context ctx;
	int len;
	UINT32 uint_used_money, uint_money;
	STRING *str = NULL;
	
	BOOL sr, sw, se;
	
	if(!keepalive_socket) return OK;
	
	os_socket_tcp_status(keepalive_socket, &sr, &sw, &se);

	if(tunet_state != TUNET_STATE_KEEPALIVE)
		return OK;

	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "KEEPALIVE", NULL, 0);
		return ERR;
	}

	if(!sr) return OK;


	len = os_socket_tcp_recv(keepalive_socket, tmpbuf, sizeof(tmpbuf));
	if(len == -1)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "KEEPALIVE", NULL, 0);
		return ERR;
	}

	if(len > 0)
	{
		keepalive_socket_buffer = buffer_append(keepalive_socket_buffer, tmpbuf, len);
		buf2output(tmpbuf, len, tmp, 16);
		//dprintf("data received(keepalive):\n%s\n", tmp);


		logs_append(g_logs, "TUNET_KEEPALIVE_RECV", NULL, tmpbuf, len);

		p = keepalive_socket_buffer->data;
		while(buffer_fetch_BYTE(keepalive_socket_buffer, &p, &btag))
		{
			switch(btag)
			{
				case 0x03:

					if(!buffer_fetch_bytes(keepalive_socket_buffer, &p, data, 16))
						return OK;

					logs_append(g_logs, "TUNET_KEEPALIVE_CONFIRM", NULL, NULL, 0);

					uint_used_money = htonl(BUFDWORD( (data + 8) ));
					

					uint_money = htonl(BUFDWORD( (data + 12) ));
					

					des_set_key(&ctx, (uint8 *)keepalive_key);
					des_encrypt(&ctx, (uint8 *)data, (uint8 *)(repbuf + 1));
					repbuf[0] = 0x02;
					os_socket_tcp_send(keepalive_socket, repbuf, sizeof(repbuf));



					keepalive_socket_buffer = buffer_rollto(keepalive_socket_buffer, p);
					p = keepalive_socket_buffer->data;

					os_tick_clear(keepalive_timeout);

					snprintf(smoney, sizeof(smoney), "%0.2f", tunet_imoney_to_fmoney(uint_money));					
					logs_append(g_logs, "TUNET_KEEPALIVE_MONEY", smoney, NULL, 0);

					snprintf(smoney, sizeof(smoney), "%0.2f", tunet_imoney_to_fmoney(uint_used_money));					
					logs_append(g_logs, "TUNET_KEEPALIVE_USED_MONEY", smoney, NULL, 0);

					break; 


				case 0xff://ff 53 65 72 76 69 63 65 20 54 65 72 6d 69 6e 61 74 65 64 21 0d 0a
					tunet_state = TUNET_STATE_ERROR;

					str = string_nappend(str, (CHAR *)(keepalive_socket_buffer->data + 1), keepalive_socket_buffer->len - 1);
					logs_append(g_logs, "TUNET_KEEPALIVE_ERROR", str->str, NULL, 0);
					str = string_free(str);
			
					

					keepalive_socket_buffer = buffer_clear(keepalive_socket_buffer);

					break;
				default:
					tunet_state = TUNET_STATE_ERROR;

					logs_append(g_logs, "TUNET_KEEPALIVE_RECV_UNKNOWN", NULL, NULL, 0);

					//dprintf("%s\n", "意外的标记");

					
					break;
			}
		}
	}
	return OK;
}		


static int tunet_logout_send_logout()
{
	BOOL sr, sw, se;
	BYTE tmpbuf[1024];
	BUFFER *buf = NULL;
	INT len;

	if(!logout_socket) return OK;
	
	os_socket_tcp_status(logout_socket, &sr, &sw, &se);

	if(tunet_state != TUNET_STATE_LOGOUT)
		return OK;

	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "SEND_LOGOUT", NULL, 0);
		return ERR;
	}

	if(!sw) return OK;

	hex2buf("0f 54 55 4e 45 54 20 55 53 45 52", tmpbuf, &len);

	buf = buffer_append(buf, tmpbuf, len);

	os_socket_tcp_send(logout_socket, buf->data, buf->len);

	

	tunet_state = TUNET_STATE_LOGOUT_RECV_LOGOUT;

	logs_append(g_logs, "TUNET_LOGON_SEND_LOGOUT", NULL, buf->data, buf->len);
	//dprintf("向主服务器发出注销请求...\n");

	buf = buffer_free(buf);
	return OK;
}

static int  tunet_logout_recv_logout()
{
	BYTE tmpbuf[1024 * 8];
	CHAR  tmp[1024];

	BYTE  btag;
	UINT32  unknowntag;

	BYTE *p;

	int len;
	DWORD datalen;
	
	const CHAR *LOGOUT_TUNET = "LOGOUT TUNET";

	STRING *str = NULL;

	BOOL sr, sw, se;
	
	if(!logout_socket) return OK;
	os_socket_tcp_status(logout_socket, &sr, &sw, &se);

	
	if(tunet_state != TUNET_STATE_LOGOUT_RECV_LOGOUT) 
		return OK;


	if(se)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_LOGOUT", NULL, 0);
		return ERR;
	}

	if(!sr) return OK;

	len = os_socket_tcp_recv(logout_socket, tmpbuf, sizeof(tmpbuf));
	if(len == -1)
	{
		logs_append(g_logs, "TUNET_NETWORK_ERROR", "RECV_LOGOUT", NULL, 0);
		return ERR;
	}

	if(len > 0)
	{
		logout_socket_buffer = buffer_append(logout_socket_buffer, tmpbuf, len);
		buf2output(tmpbuf, len, tmp, len);
		//dprintf("data received(logout):\n%s\n", tmp);


		logs_append(g_logs, "TUNET_LOGOUT_RECV", NULL, tmpbuf, len);

		p = logout_socket_buffer->data;
		while(buffer_fetch_BYTE(logout_socket_buffer, &p, &btag))
		{
			switch(btag)
			{
				case 0x10:

					if(!buffer_has_data(logout_socket_buffer, p, strlen(LOGOUT_TUNET) + 4))
						return OK;

					buffer_fetch_STRING(logout_socket_buffer, &p, &str, strlen(LOGOUT_TUNET));
					if(strcmp(str->str, LOGOUT_TUNET) != 0)
					{
						//dprintf("[tunet_logout_recv] unknown string\n");

						tunet_state = TUNET_STATE_ERROR;
						return OK;
					}
					str = string_free(str);
					buffer_fetch_DWORD(logout_socket_buffer, &p, &unknowntag);


					//say "we will logout"
					os_socket_tcp_send(logout_socket, (BYTE *)"\x0cLOGOUT", 7);
					logout_socket_buffer = buffer_rollto(logout_socket_buffer, p);
					p = logout_socket_buffer->data;


					break;

				//
				case 0x0d:
				case 0x0e:
					if(!buffer_fetch_DWORD(logout_socket_buffer, &p, &datalen))
						return OK;
					datalen = htonl(datalen);

					//dprintf("注销反馈消息长 %d\n", datalen);

					if(!buffer_fetch_STRING(logout_socket_buffer, &p, &str, datalen))
						return OK;

					//dprintf("%s\n", str->str);

					logs_append(g_logs, "TUNET_LOGOUT_MSG", str->str, NULL, 0);

					logs_append(g_logs, "TUNET_LOGOUT", NULL, NULL, 0);


					str = string_free(str);

					

					tunet_state = TUNET_STATE_NONE;

					logout_socket_buffer = buffer_rollto(logout_socket_buffer, p);
					p = logout_socket_buffer->data;

					break;

			}
		}

	}
	return OK;
}

THREADRET tunet_thread(THREAD *self)
{
	BOOL hasErr = FALSE;

	logs_append(g_logs, "TUNET_THREAD_STARTING", NULL, NULL, 0);
	while(os_thread_is_running(self) || os_thread_is_paused(self))
	{
		if(tunet_state == TUNET_STATE_LOGIN)
			tunet_connect_main_server();

		hasErr = FALSE;

		hasErr |= (tunet_logon_send_tunet_user() == ERR);
		hasErr |= (tunet_logon_recv_welcome() == ERR);
		hasErr |= (tunet_logon_reply_welcome() == ERR);
		hasErr |= (tunet_logon_recv_remaining_data() == ERR);

		hasErr |= (tunet_connect_keepalive_server() == ERR);
		hasErr |= (tunet_keepalive() == ERR);

		if(hasErr)
		{
			
			/*
			COMMENT: We won't help the user to retry for some network error shoud be
			         dealt by the users.

				//a network error occurs when try to LOGIN
				main_socket = os_socket_tcp_close(main_socket);
				keepalive_socket = os_socket_tcp_close(keepalive_socket);

				tunet_state = TUNET_STATE_LOGIN;
				continue;
			*/

			break;
		}

		hasErr = FALSE;
		if(tunet_state == TUNET_STATE_LOGOUT)
			tunet_connect_logout_server();
		hasErr |= (tunet_logout_send_logout() == ERR);
		hasErr |= (tunet_logout_recv_logout() == ERR);

		if(hasErr)
		{
			//if an error occurs when LOGOUT, we can just omit it, and exit the thread.
			break;
		}

		if(tunet_get_state() == TUNET_STATE_NONE)
		{
			//nothing to do , exit the thread
			break;
		}

		if(tunet_get_state() == TUNET_STATE_ERROR)
		{
			//an server-side error occurs when trying to login/logout
			logs_append(g_logs, "TUNET_ERROR", NULL, NULL, 0);
			break;
		}
		os_thread_test_paused(self);
		os_sleep(20);
	}

	main_socket = os_socket_tcp_close(main_socket);
	keepalive_socket = os_socket_tcp_close(keepalive_socket);
	logout_socket = os_socket_tcp_close(logout_socket);

	main_socket_buffer = buffer_clear(main_socket_buffer);
	keepalive_socket_buffer = buffer_clear(keepalive_socket_buffer);
	logout_socket_buffer = buffer_clear(logout_socket_buffer);

	tunet_state = TUNET_STATE_NONE;

	logs_append(g_logs, "TUNET_THREAD_EXITING", NULL, NULL, 0);

	

	thread_tunet = os_thread_free(thread_tunet);
	return 0;
}

VOID tunet_stop()
{
	logs_append(g_logs, "TUNET_STOP", NULL, NULL, 0);

	if(thread_tunet)
	{
		logout_socket_buffer = buffer_clear(logout_socket_buffer);

		tunet_state = TUNET_STATE_LOGOUT;
		while(tunet_state != TUNET_STATE_NONE && tunet_state != TUNET_STATE_ERROR)
			os_sleep(20);

		os_thread_kill(thread_tunet);
		while(thread_tunet)	os_sleep(20);

		main_socket_buffer = buffer_clear(main_socket_buffer);
		keepalive_socket_buffer = buffer_clear(keepalive_socket_buffer);
		logout_socket_buffer = buffer_clear(logout_socket_buffer);

		strcpy(keepalive_server, "");
		keepalive_server_port = 0;

		strcpy(msg_server, "");
		msg_server_port = 0;
	}

	logs_append(g_logs, "TUNET_STOP", "END", NULL, 0);
}


VOID tunet_start(USERCONFIG *uc)
{
	assert(uc != NULL);

	if(thread_tunet)
	{
		os_thread_kill(thread_tunet);
		while(thread_tunet) os_sleep(20);
	}


	logs_append(g_logs, "TUNET_START", NULL, NULL, 0);

	memcpy(&userconfig, uc, sizeof(USERCONFIG));
	
	strcpy(keepalive_server, "");
	keepalive_server_port = 0;

	strcpy(msg_server, "");
	msg_server_port = 0;

	main_socket = os_socket_tcp_close(main_socket);
	keepalive_socket = os_socket_tcp_close(keepalive_socket);
	logout_socket = os_socket_tcp_close(logout_socket);


	tunet_state = TUNET_STATE_LOGIN;

	main_socket_buffer = buffer_clear(main_socket_buffer);
	keepalive_socket_buffer = buffer_clear(keepalive_socket_buffer);
	logout_socket_buffer = buffer_clear(logout_socket_buffer);

	thread_tunet = os_thread_create(tunet_thread, NULL, FALSE, FALSE);
}


INT tunet_get_state()
{
	return tunet_state;
}

BOOL tunet_is_keepalive_timeout()
{
	return (tunet_get_state() == TUNET_STATE_KEEPALIVE && os_tick_is_active(keepalive_timeout));
}

VOID tunet_reset()
{
	logs_append(g_logs, "TUNET_RESET", NULL, NULL, 0);
	if(thread_tunet)
	{
		os_thread_kill(thread_tunet);
		while(thread_tunet) os_sleep(20);
	}

	tunet_state = TUNET_STATE_NONE;
}

VOID tunet_init()
{
	main_socket_buffer = buffer_new(1024);
	keepalive_socket_buffer = buffer_new(1024);
	logout_socket_buffer = buffer_new(1024);

	thread_tunet = NULL;
	keepalive_timeout = os_tick_new(5 * 60 * 1000, FALSE);

	tunet_state = TUNET_STATE_NONE;
}

VOID tunet_cleanup()
{
	os_thread_kill(thread_tunet);
	while(thread_tunet) os_sleep(20);

	main_socket_buffer = buffer_free(main_socket_buffer);
	keepalive_socket_buffer = buffer_free(keepalive_socket_buffer);
	logout_socket_buffer = buffer_free(logout_socket_buffer);

	keepalive_timeout = os_tick_free(keepalive_timeout);
}


