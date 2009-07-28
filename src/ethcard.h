#ifdef __cplusplus
	extern "C"
	{
#endif

#ifndef __ETHCARD_H__
#define __ETHCARD_H__



#ifdef _WIN32

	#include <pcap.h>

	#pragma comment(lib, "wpcap.lib")
	#pragma comment (lib, "iphlpapi.lib")

	typedef struct _ETHCARD
	{
		pcap_t *pcapHandle;
	}ETHCARD;

#endif

#ifdef _LINUX

	#include <netpacket/packet.h>
	#include <net/ethernet.h>
	#include <sys/ioctl.h>
	#include <sys/types.h>
	#include <net/if.h>
	#include <net/if_packet.h>
	#include <net/if_arp.h>
	
	typedef struct _ETHCARD
	{
		int fd;
		int iface;
	}ETHCARD;
	
#endif

#ifdef _BSD

	#include <stdio.h>
	#include <fcntl.h>
	#include <sys/ioctl.h>
	#include <sys/time.h>
	#include <sys/types.h>
	#include <stdlib.h>
	#include <sys/uio.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <ifaddrs.h>
	#include <net/bpf.h>
	#include <net/if.h>
	#include <string.h>
	#include <sys/select.h>
	#include <signal.h>
	#include <net/if.h>
	#include <sys/queue.h>
	#include <ctype.h>
	#include <net/ethernet.h>
	#include <net/if_dl.h>
	#include <errno.h>

	typedef struct _ETHCARD
	{
		int fd;
		int blen;
	}ETHCARD;
#endif

#include "os.h"

#define MAX_ETHCARDS 16

#pragma pack(push, 1)
	typedef struct _ETHCARD_INFO
	{
		CHAR name[255];
		CHAR desc[255];
		CHAR mac[255];
		CHAR ip[255];
		BOOL live;
	}ETHCARD_INFO;



	typedef struct frame_data {
		BYTE dest[6];
		BYTE src[6];
			union
			{
				UINT16 type;
				BYTE types[2];
			};
	}ETH_FRAME;
#pragma pack(pop)
	
typedef VOID ( *ETHCARD_LOOP_RECV_PROC ) (ETHCARD *ethcard, BYTE *pkt_data, INT pkt_len);


typedef struct _ETHCARD_LOOP_RECV_PROC_PARAM
{
	ETHCARD *ethcard;
	ETHCARD_LOOP_RECV_PROC proc;
}ETHCARD_LOOP_RECV_PROC_PARAM;


INT		get_ethcards(ETHCARD_INFO *devices, INT bufsize);
INT		ethcard_send_packet(ETHCARD *ethcard, BYTE *buf, INT len);
VOID	ethcard_start_loop_recv(ETHCARD *ethcard, ETHCARD_LOOP_RECV_PROC proc);
VOID	ethcard_stop_loop_recv();
ETHCARD *ethcard_open(CHAR *name);
ETHCARD *ethcard_close(ETHCARD *ethcard);

VOID	ethcard_init();
VOID	ethcard_cleanup();
#endif





#ifdef __cplusplus
	}
#endif


