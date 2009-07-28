#define DOT1XMAC    ((CHAR *)("01 80 c2 00 00 03"))


#include "ethcard.h"
#include "os.h"
#include "md5.h"
#include "dot1x.h"

#include "util.h"

#include "userconfig.h"
#include "logs.h"

#include <assert.h>

#define MAX_PRINT 80
#define MAX_LINE 16


/* Request and Response datagrams have form CODE ID LEN TYPE */
#define EAP_TYPE_HDR          0x5       /* EAP with type 5 bytes */
/** EAP CODE FIELD - RFC 2284 Sec 2.1**/
#define EAP_REQUEST           0x1       /* Request code for EAP */
#define EAP_RESPONSE          0x2       /* Response code for EAP */
#define EAP_SUCCESS           0x3       /* Success Code */
#define EAP_FAILURE           0x4       /* Failure Code */
/** EAP Type Field - RFC 2284 Sec 3*/
#define EAP_TYPE_ID           0x1       /* Identity Type *//* REQUIRED */
#define EAP_TYPE_NOTIFY       0x2       /* Notification Type *//* REQUIRED*/
#define EAP_TYPE_NAK          0x3       /* Nak Response *//* REQUIRED */
#define EAP_TYPE_MD5          0x4       /* MD5 Challenge *//*REQUIRED*/

#define EAP_PAYLOAD_OFFSET    23        /* How many bytes in to the frame */
                                        /* before we get to the data section*/

// Different Types of EAPOL Messages we might need.
#define EAPOL_PACKET    0
#define EAPOL_START     1
#define EAPOL_LOGOFF    2
#define EAPOL_KEY       3
#define EAPOL_ASF_ALERT 4

// Different EAPOL codes
#define CODE_REQUEST    1
#define CODE_RESPONSE   2
#define CODE_SUCCESS    3
#define CODE_FAILURE    4


/*** EAPOL over 802.11 ***/
#define EAP_PACK_TYPE    0x0        /* EAP-Packet EAPOL type */
                                    /* this is what is the MAX MTU as defined for CISCO Aironet in PCMCIA-CS */
#define EAPOL_MAX_PACKET 2400
#define EAPOL_KEY_UNICAST    0x80  /* first bit */
#define EAPOL_KEY_INDEX      0x7F  /* last 7 bits */

//typedef unsigned short UINT16;
//typedef unsigned char UINT8;



static int dot1x_state = DOT1X_STATE_NONE;
static ETHCARD *ethcard = NULL;
//static THREAD *thread_dot1x = NULL;
static USERCONFIG userconfig;
static TICK *tick_timeout = NULL;

static BYTE null_buffer_60[61] = {0};

static void dot1x_logon_request()
{
    BUFFER *buf;
    BYTE tmpbuf[100];

    buf = buffer_new(200);

    buf = buffer_append(buf, hex2buf(DOT1XMAC, tmpbuf, NULL), 6);
    buf = buffer_append(buf, hex2buf(userconfig.szMac, tmpbuf, NULL), 6);
    buf = buffer_append(buf, hex2buf((CHAR *)"88 8e", tmpbuf, NULL), 2);
    buf = buffer_append(buf, hex2buf((CHAR *)"01 01 00 00", tmpbuf, NULL), 4);

    if(buf->len < 60)
        buf = buffer_append(buf, null_buffer_60, 60 - buf->len);

    ethcard_send_packet(ethcard, buf->data, buf->len);

    logs_append(g_logs, "DOT1X_LOGON_REQUEST", NULL, buf->data, buf->len);

    buf = buffer_free(buf);
}

static void dot1x_logout()
{
    BUFFER *buf;
    BYTE tmpbuf[100];

	if (!ethcard)
		return;

    buf = buffer_new(200);

    buf = buffer_append(buf, hex2buf(DOT1XMAC, tmpbuf, NULL), 6);
    buf = buffer_append(buf, hex2buf(userconfig.szMac, tmpbuf, NULL), 6);
    buf = buffer_append(buf, hex2buf((CHAR *)"88 8e", tmpbuf, NULL), 2);
    buf = buffer_append(buf, hex2buf((CHAR *)"01 02 00 00", tmpbuf, NULL), 4);

    if(buf->len < 60)
        buf = buffer_append(buf, null_buffer_60, 60 - buf->len);

    ethcard_send_packet(ethcard, buf->data, buf->len);

    logs_append(g_logs, "DOT1X_LOGOUT", NULL, buf->data, buf->len);

    buf = buffer_free(buf);
}


static void dot1x_logon_send_username(int id)
{
    BUFFER *buf;
    BYTE tmpbuf[100];
    EAPOL_PACKET_HEADER *pkt;
    STRING *strUserAtIp = NULL;

    buf = buffer_new(sizeof(EAPOL_PACKET_HEADER));

        pkt = (EAPOL_PACKET_HEADER *)buf->data;

        memcpy(pkt->dest, hex2buf(DOT1XMAC, tmpbuf, NULL), 6);
        memcpy(pkt->src, hex2buf(userconfig.szMac, tmpbuf, NULL), 6);

        pkt->tags[0] = 0x88;
        pkt->tags[1] = 0x8e;


        pkt->eapol_version = 1;
        pkt->eapol_type = EAPOL_PACKET;

        strUserAtIp = string_append(strUserAtIp, userconfig.szUsername);
        strUserAtIp = string_append(strUserAtIp, "@");
        strUserAtIp = string_append(strUserAtIp, userconfig.szIp);

        pkt->eapol_length = htons((u_short)(strlen(strUserAtIp->str) + 5));
        pkt->code = EAP_RESPONSE;
        pkt->identifier = id;
        pkt->length = pkt->eapol_length;
        pkt->type = EAP_TYPE_ID;

    buf->len = sizeof(EAPOL_PACKET_HEADER);

    buf = buffer_append(buf, (BYTE *)strUserAtIp->str, strlen(strUserAtIp->str));



    if(buf->len < 60)
        buf = buffer_append(buf, null_buffer_60, 60 - buf->len);

    ethcard_send_packet(ethcard, buf->data, buf->len);

    logs_append(g_logs, "DOT1X_LOGON_SEND_USERNAME", NULL, buf->data, buf->len);

    //if(ethcard_send_packet(ethcard, buf->data, buf->len) != 0)
    //{
    //  dprintf("Error with WinPCap\n");
    //}

    strUserAtIp = string_free(strUserAtIp);
    buf = buffer_free(buf);
}


static void dot1x_logon_auth(int id, BYTE *stream, int streamlen)
{
    BUFFER *buf = buffer_new(1024);
    EAPOL_PACKET_HEADER *pkt;
    BYTE md5_result[16];
    BYTE tmpbuf[100];
    STRING *strUserAtIp = NULL;

    buf = buffer_append(buf, (BYTE *)"?", 1);
    buf->data[0] = id;

    buf = buffer_append(buf, (BYTE *)userconfig.szMD5Password, strlen(userconfig.szMD5Password));
    buf = buffer_append(buf, stream, streamlen);

    MD5Buffer(buf->data, buf->len, md5_result);

    buf = buffer_free(buf);


    buf = buffer_new(sizeof(EAPOL_PACKET_HEADER));
        pkt = (EAPOL_PACKET_HEADER *)buf->data;

        memcpy(pkt->dest, hex2buf(DOT1XMAC, tmpbuf, NULL), 6);
        memcpy(pkt->src, hex2buf(userconfig.szMac, tmpbuf, NULL), 6);

        pkt->tags[0] = 0x88;
        pkt->tags[1] = 0x8e;

        pkt->eapol_version = 1;
        pkt->eapol_type = EAPOL_PACKET;


        strUserAtIp = string_append(strUserAtIp, userconfig.szUsername);
        strUserAtIp = string_append(strUserAtIp, "@");
        strUserAtIp = string_append(strUserAtIp, userconfig.szIp);


        pkt->eapol_length = htons((u_short)(strlen(strUserAtIp->str) + 16/*md5*/ + 6));
        pkt->code = EAP_RESPONSE;
        pkt->identifier = id;
        pkt->length = pkt->eapol_length;
        pkt->type = EAP_TYPE_MD5;

    buf->len = sizeof(EAPOL_PACKET_HEADER);

    buf = buffer_append(buf, (BYTE *)"\x10", 1);
    buf = buffer_append(buf, md5_result, 16);
    buf = buffer_append(buf, (BYTE *)strUserAtIp->str, strlen(strUserAtIp->str));

    if(buf->len < 60)
        buf = buffer_append(buf, null_buffer_60, 60 - buf->len);

    ethcard_send_packet(ethcard, buf->data, buf->len);

    logs_append(g_logs, "DOT1X_LOGON_AUTH", NULL, buf->data, buf->len);
    /*
    if(ethcard_send_packet(ethcard, buf->data, buf->len) != 0)
    {
        dprintf("Error with WinPCap\n");
    }
*/

    strUserAtIp = string_free(strUserAtIp);
    buf = buffer_free(buf);

}


static VOID dot1x_loop_recv_proc(ETHCARD *ethcard, BYTE *pkt_data, INT pkt_len)
{
    BYTE tmpbuf[100];
    EAPOL_RANDSTREAM_PACKET * stmpkt = (EAPOL_RANDSTREAM_PACKET *)pkt_data;
    EAPOL_PACKET_HEADER *pkthdr = (EAPOL_PACKET_HEADER *)pkt_data;

/*    INT i,j;
	dprintf("dot1x_loop_recv_proc\n");
	for (i=0; i<pkt_len; i++)
		dprintf("%.2x ", pkt_data[i]);

        dprintf("Recv: \n DEST: ");
        for(i = 1, j = 0; j < 6; i++,j++)
        {
            dprintf("%.2x ", pkt_data[i-1]);
        }


        dprintf("\nSRC : ");
        for(j = 0; j < 6; i++,j++)
        {
            dprintf("%.2x ", pkt_data[i-1]);
        }

        i += 2;  //0x888E

        dprintf("\nDATA:\n");
        for (j =1 ; (i < pkt_len ) ; i++, j++)
        {
            dprintf("%.2x ", pkt_data[i-1]);
            if ( (j % 16) == 0) dprintf("\n");
        }

        dprintf("\n");
		*/



#define FNAME "dot1x_loop_recv_proc"
	if (!pkt_data)
		dprintf(FNAME": !pkt_data error\n");
	if (!pkt_len)
		dprintf(FNAME": !pkt_len error\n");
	if (memcmp(pkthdr->dest, hex2buf(userconfig.szMac, tmpbuf, NULL), 6) != 0)
	{
		hex2buf(DOT1XMAC, tmpbuf, NULL);
		if (memcmp(pkthdr->dest, tmpbuf, 6) != 0)
		{
			char tempbuf[100];
			dprintf(FNAME": Not our packet\n");
			buf2hex(pkthdr->dest, 6, tempbuf);
			dprintf(FNAME": packet dest: %s\n", tempbuf);
			dprintf(FNAME": our addr:    %s\n", userconfig.szMac);
		}
	}
	if (pkthdr->tag != 0x8e88)
		dprintf(FNAME": proto error : %x\n", pkthdr->tag);
#undef FNAME
    if(pkt_data && pkt_len && memcmp(pkthdr->dest, hex2buf(userconfig.szMac, tmpbuf, NULL), 6) == 0 && (pkthdr->tag == 0x8e88))
    {

        logs_append(g_logs, "DOT1X_RECV", NULL, pkt_data, pkt_len);

        os_tick_clear(tick_timeout);

        switch (pkthdr->eapol_type)
        {
            case EAP_PACK_TYPE:
                //dprintf("EAP_PACK_TYPE  receiveID = %d  code = %d\n", pkthdr->identifier, pkthdr->code);
                switch (pkthdr->code)
                {
                    case EAP_REQUEST:
                        switch(pkthdr->type)
                        {
                            case EAP_TYPE_ID:
                                logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_REQUEST", NULL, 0);

                                dot1x_state = DOT1X_STATE_RESPONSE;

                                dot1x_logon_send_username(pkthdr->identifier);

                                break;
                            case EAP_TYPE_MD5:
                                if(sizeof(EAPOL_PACKET_HEADER) + sizeof(stmpkt->streamlen) + stmpkt->streamlen  <= (UINT32)pkt_len)
                                {
                                    logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_REQUEST(AUTH)", NULL, 0);

                                    dot1x_state = DOT1X_STATE_AUTH;

                                    dot1x_logon_auth(pkthdr->identifier, stmpkt->stream, stmpkt->streamlen);
                                }
                                else
                                {
                                    dot1x_state = DOT1X_STATE_LOGIN;
                                    dot1x_logon_request();
                                }
                                break;
                        }
                        //dprintf("   EAP_REQUEST\n");

                        /*
                        if(     dot1x_state == DOT1X_STATE_LOGIN
                            ||  dot1x_state == DOT1X_STATE_SUCCESS
                            ||  dot1x_state == DOT1X_STATE_FAILURE)
                        {
                            logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_REQUEST", NULL, 0);

                            dot1x_state = DOT1X_STATE_RESPONSE;
                            dot1x_logon_send_username(pkthdr->identifier);
                        }
                        else if(dot1x_state == DOT1X_STATE_RESPONSE)
                        {
                            //dprintf("     streamlen = %d\n", stmpkt->streamlen);
                            if(sizeof(EAPOL_PACKET_HEADER) + sizeof(stmpkt->streamlen) + stmpkt->streamlen  <= (UINT32)pkt_len)
                            {
                                logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_REQUEST(AUTH)", NULL, 0);

                                dot1x_state = DOT1X_STATE_AUTH;

                                dot1x_logon_auth(pkthdr->identifier, stmpkt->stream, stmpkt->streamlen);
                            }
                            else
                            {
                                //dprintf("   ERROR WHEN RECV STREAM !! Retry ...\n");
                                dot1x_state = DOT1X_STATE_LOGIN;
                                dot1x_logon_request();
                            }
                        }
                        */
                        break;
                    case EAP_RESPONSE:
                        logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_RESPONSE", NULL, 0);
                        //dprintf("   EAP_RESPONSE\n");
                        break;
                    case EAP_SUCCESS:
                        logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_SUCCESS", NULL, 0);
                        //dprintf("   EAP_SUCCESS\n");
                        dot1x_state = DOT1X_STATE_SUCCESS;

                        break;
                    case EAP_FAILURE:


                        //TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                        if(dot1x_state == DOT1X_STATE_LOGOUT)
                        {
                            logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_FAILURE(LOGOUT)", NULL, 0);
                            //dprintf("   EAP_FAILURE(LOGOUT)\n");


                            dot1x_state = DOT1X_STATE_NONE;


                            //Here, we finished stopping DOT1X !!!!
                            logs_append(g_logs, "DOT1X_STOP", "END", NULL, 0);

                            break;
                        }
                        else
                        {
                            logs_append(g_logs, "DOT1X_RECV_PACK", "EAP_FAILURE", NULL, 0);

                            //dprintf("   EAP_FAILURE\n");

                            dot1x_state = DOT1X_STATE_FAILURE;

                            if(userconfig.bRetryDot1x)
                            {
                                // keep retrying ...
                                dot1x_logon_request();
                            }
                        }
                        break;
                    default:
                        logs_append(g_logs, "DOT1X_RECV_PACK", "UNKNOWN", NULL, 0);
                        //dprintf("   Unknown EAP_TYPE\n");
                }

                break;
            case EAPOL_START:
                logs_append(g_logs, "DOT1X_RECV_START", NULL, NULL, 0);
                //dprintf("EAPOL_START\n");
                break;
            case EAPOL_LOGOFF:
                logs_append(g_logs, "DOT1X_RECV_LOGOFF", NULL, NULL, 0);
                //dprintf("EAPOL_LOGOFF\n");
                break;
            case EAPOL_KEY:
                logs_append(g_logs, "DOT1X_RECV_KEY", NULL, NULL, 0);
                //dprintf("EAPOL_KEY\n");
                break;
            case EAPOL_ASF_ALERT:
                logs_append(g_logs, "DOT1X_RECV_ASF_ALERT", NULL, NULL, 0);
                //dprintf("EAPOL_ASF_ALERT\n");
                break;
            default:
                logs_append(g_logs, "DOT1X_RECV_UNKNOWN", NULL, NULL, 0);
                return;
        }
        //dprintf("\n\n");
    }
    else
    {
        //it's not the packet for us
    }

}

VOID dot1x_start(USERCONFIG *uc)
{

    assert(uc != NULL);

    ethcard_stop_loop_recv();

    if(ethcard)
        ethcard = ethcard_close(ethcard);


    dot1x_state = DOT1X_STATE_NONE;

    os_tick_clear(tick_timeout);
    logs_append(g_logs, "DOT1X_START", NULL, NULL, 0);

    memcpy(&userconfig, uc, sizeof(USERCONFIG));

    if((ethcard = ethcard_open(userconfig.szAdaptor)) == NULL)
    {
        logs_append(g_logs, "DOT1X_START_FAIL", "ETHCARD_OPEN", NULL, 0);
		dot1x_state = DOT1X_STATE_ERROR;
        //dprintf("failed to open the ethcard!\n");
        return;
    }


    ethcard_start_loop_recv(ethcard, dot1x_loop_recv_proc);

    dot1x_state = DOT1X_STATE_LOGIN;
    dot1x_logon_request();
}

VOID dot1x_stop()
{
    if(dot1x_state != DOT1X_STATE_NONE)
    {
        dot1x_state = DOT1X_STATE_LOGOUT;
        logs_append(g_logs, "DOT1X_STOP", NULL, NULL, 0);
        dot1x_logout();
    }

}

INT dot1x_get_state()
{
    return dot1x_state;
}

VOID dot1x_reset()
{
    dot1x_state = DOT1X_STATE_NONE;
    logs_append(g_logs, "DOT1X_RESET", NULL, NULL, 0);
}

/*
VOID dot1x_set_timeout(INT delay)
{
    os_tick_set_delay(tick_timeout, delay);
    os_tick_clear(tick_timeout);
}
*/

BOOL dot1x_is_timeout()
{
    return  dot1x_get_state() != DOT1X_STATE_NONE &&
            dot1x_get_state() != DOT1X_STATE_SUCCESS &&
            os_tick_is_active(tick_timeout);
}
/*
BOOL dot1x_check_timeout()
{
    return os_tick_check(tick_timeout);
}
*/


VOID dot1x_init()
{
    dot1x_state = DOT1X_STATE_NONE;
    tick_timeout = os_tick_new(30000, FALSE);
}

VOID dot1x_cleanup()
{
    tick_timeout = os_tick_free(tick_timeout);
    ethcard_stop_loop_recv();
    ethcard = ethcard_close(ethcard);
}
