#ifdef _WIN32

#include "ethcard.h"
#include "os.h"
#include <iphlpapi.h>


static THREAD *thread_ethcard_recv = NULL;


BOOL GetMACAddress(char *name, char *mac)
{
    IP_ADAPTER_INFO AdapterInfo[16]; 
    DWORD dwBufLen = sizeof(AdapterInfo);  
	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; 
	
    DWORD dwStatus = GetAdaptersInfo(AdapterInfo,&dwBufLen);
	if(strchr(name, '{'))
		name = strchr(name, '{');

	strcpy(mac, "");

	strcpy(mac, "00 00 00 00 00 00");
    if(dwStatus == ERROR_SUCCESS)
	{
		while(pAdapterInfo) 
		{
			if(strnicmp(pAdapterInfo->AdapterName, name, strlen(pAdapterInfo->AdapterName)) == 0)
			{
				if(mac)
				{
					sprintf(mac, "%02X %02X %02X %02X %02X %02X",
								pAdapterInfo->Address[0], 
								pAdapterInfo->Address[1], 
								pAdapterInfo->Address[2], 
								pAdapterInfo->Address[3], 
								pAdapterInfo->Address[4], 
								pAdapterInfo->Address[5]);
				}
				return TRUE;
			}
			pAdapterInfo = pAdapterInfo->Next;   
		}	
	}
	return FALSE;
}

//TODO
// use bufsize to decide the size!!
INT get_ethcards_by_pcap(ETHCARD_INFO *devices, INT bufsize)
{
	pcap_if_t *alldevs, *d;
	char errbuf[PCAP_ERRBUF_SIZE];
	struct sockaddr_in *ina;
	int i = 0;
	char ip[255];
	struct pcap_addr *address;

	if (pcap_findalldevs(&alldevs, errbuf) == -1)
	{
		//dprintf("Error in pcap_findalldevs: %s\n", errbuf);
		return ERR;
	}

	for(i=0, d=alldevs; d; d=d->next, i++)
	{
		if( d->description )
			strcpy(devices[i].desc, d->description);
		else
			strcpy(devices[i].desc, "");

		strcpy(devices[i].name, d->name);
		strcpy(devices[i].ip, "0.0.0.0");
		strcpy(devices[i].mac, "00 00 00 00 00 00");
		if(d)
		{
			address = d->addresses;
			while(address)
			{
				if( !address->addr)
					continue;

				ina = (struct sockaddr_in *)address->addr;
				strcpy(ip, inet_ntoa(ina->sin_addr));
				if(strstr(ip, "166.111.") || strstr(ip, "59.66."))
				{
					strcpy(devices[i].ip, ip);
					GetMACAddress(d->name, devices[i].mac);
					break;
				}	
				address = address->next;
			}
		}
		
	}
	
	pcap_freealldevs(alldevs);
	return i;
}


//TODO
// use bufsize to decide the size!!
INT get_ethcards_by_win(ETHCARD_INFO *devices, INT bufsize)
{
	IP_ADAPTER_INFO AdapterInfo[16] = {0}; 
    DWORD dwBufLen = sizeof(AdapterInfo);  
	PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo; 
	PIP_ADDR_STRING address;
    
	int i;
	DWORD dwStatus = GetAdaptersInfo(AdapterInfo,&dwBufLen);

	memset(AdapterInfo, sizeof(AdapterInfo), 0);

    if(dwStatus == ERROR_SUCCESS)
	{
		i = 0;
		for(i = 0; strcmp(AdapterInfo[i].AdapterName, "") != 0 ; i++)
		{
			sprintf(devices[i].mac, "%02X %02X %02X %02X %02X %02X",
				AdapterInfo[i].Address[0], 
				AdapterInfo[i].Address[1], 
				AdapterInfo[i].Address[2], 
				AdapterInfo[i].Address[3], 
				AdapterInfo[i].Address[4], 
				AdapterInfo[i].Address[5]);

			address = &(AdapterInfo[i].IpAddressList);
			strcpy(devices[i].ip, "0.0.0.0");
			strcpy(devices[i].mac, "00 00 00 00 00 00");
			while(address)
			{
				if(strstr(address->IpAddress.String, "166.111.") || 
					strstr(address->IpAddress.String, "59.66."))
				{
					strcpy(devices[i].ip, address->IpAddress.String);
				}
				address = address->Next;
			}

			if(AdapterInfo[i].AdapterName)
				strcpy(devices[i].name, AdapterInfo[i].AdapterName);
			else
				strcpy(devices[i].name, "Unknown Name");

			if(AdapterInfo[i].Description)
				strcpy(devices[i].desc, AdapterInfo[i].Description);
			else
				strcpy(devices[i].desc, "Unknown Device");
		}	
		return i;
	}
	else
	{
		return 0;
	}
	return ERR;
}


INT get_ethcards(ETHCARD_INFO *devices, INT bufsize)
{
	INT r;
	if( ( r = get_ethcards_by_pcap(devices, bufsize)) == ERR)
		if( ( r = get_ethcards_by_win(devices, bufsize)) == ERR)
			return ERR;

	return r;
}

INT		ethcard_send_packet(ETHCARD *ethcard, BYTE *buf, INT len)
{
	if(ethcard != NULL && pcap_sendpacket(ethcard->pcapHandle, buf, len) == 0)
	{
		return len;
	}
	else
	{
		return ERR;
	}
}

static THREADRET pcap_loop_thread(THREAD *self)
{
	INT res;
	struct pcap_pkthdr *header;
	BYTE *pkt_data;

	ETHCARD_LOOP_RECV_PROC_PARAM *pp, p;
	pp = (ETHCARD_LOOP_RECV_PROC_PARAM *)self->param;
	p.ethcard = pp->ethcard;
	p.proc = pp->proc;

	os_thread_init_complete(self);


	/* Read the packets */
	while(os_thread_is_running(self) || os_thread_is_paused(self))
	{

		res = pcap_next_ex(p.ethcard->pcapHandle, &header, &pkt_data);
		
		if(res < 0 )
		{
			//ERROR!!!!!
			os_thread_kill(self);
		}
		if(res == 0)
		{
			os_sleep(10);
			continue;   //TIMEOUT
		}

		p.proc(p.ethcard, (BYTE *)pkt_data, header->len);

		os_thread_test_paused(self);
	}

	thread_ethcard_recv = os_thread_free(thread_ethcard_recv);
	return 0;
}

VOID ethcard_start_loop_recv(ETHCARD *ethcard, ETHCARD_LOOP_RECV_PROC proc)
{
	ETHCARD_LOOP_RECV_PROC_PARAM p;

	p.ethcard = ethcard;
	p.proc = proc;

	if(!thread_ethcard_recv)
		thread_ethcard_recv = os_thread_create(pcap_loop_thread, (POINTER)&p, TRUE, FALSE);
}

VOID ethcard_stop_loop_recv()
{
	os_thread_kill(thread_ethcard_recv);
	while(thread_ethcard_recv) os_sleep(20);
}

ETHCARD *ethcard_open(char *name)
{
	char *filter="ether proto 0x888e";
	u_int i=0;
	char errbuf[PCAP_ERRBUF_SIZE];
	bpf_u_int32 pcap_maskp;               /* subnet mask */
	bpf_u_int32 pcap_netp;                /* ip (not really sure) */
	
	struct bpf_program fcode;
	char fullname[1024];

	ETHCARD *ec;
	pcap_t *pcapHandle;

	if(strstr(name, "\\") == NULL)
	{
		strcpy(fullname, "\\Device\\NPF_");
		strcat(fullname, name);
	}
	else
	{
		strcpy(fullname, name);
	}

	
	if ( (pcapHandle = pcap_open_live(fullname, 1024, 0, 20, errbuf) ) == NULL)
		return NULL;
	
	pcap_lookupnet(fullname, &pcap_netp, &pcap_maskp, errbuf );

	if(pcap_compile(pcapHandle, &fcode, filter, 0, pcap_netp)<0)
		return NULL;

	if(pcap_setfilter(pcapHandle, &fcode)<0)
		return NULL;
	
	ec = os_new(ETHCARD, 1);
	ec->pcapHandle = pcapHandle;
	return ec;
}

ETHCARD *ethcard_close(ETHCARD *ethcard)
{
	if(ethcard)
	{
		pcap_close(ethcard->pcapHandle);
		os_free(ethcard);
	}
	return NULL;
}

VOID	ethcard_init()
{
	thread_ethcard_recv = NULL;
}

VOID	ethcard_cleanup()
{
	os_thread_kill(thread_ethcard_recv);
	while(thread_ethcard_recv) os_sleep(20);
}


#endif //_WIN32

