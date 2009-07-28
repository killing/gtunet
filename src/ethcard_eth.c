#ifdef _LINUX

#include "ethcard.h"


static THREAD *thread_ethcard_recv = NULL;


INT   get_ethcard_iface_byname(int sd, CHAR *name)
{
	int ret;
	struct ifreq req;
	strncpy(req.ifr_name,name,IFNAMSIZ);
	ret=ioctl(sd,SIOCGIFINDEX,&req);
	if (ret==-1) return -1;
	return req.ifr_ifindex;
}


INT get_ethcards(ETHCARD_INFO *devices, INT bufsize)
{
	int fd, i, count = 0;
	struct ifreq buf[MAX_ETHCARDS];
	struct ifconf ifc;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) 
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc)) 
		{
			count = ifc.ifc_len / sizeof(struct ifreq);
			i = count;
			while (i-- > 0)
			{
				strcpy(devices[i].name, buf[i].ifr_name);
				strcpy(devices[i].desc, buf[i].ifr_name);
				
				/*Jugde whether the net card status is up*/
				if (!(ioctl(fd, SIOCGIFFLAGS, (char *) &buf[i])))
				{
					devices[i].live = (buf[i].ifr_flags & IFF_UP);
				}
				
				/*Get IP of the net card */
				if (!(ioctl(fd, SIOCGIFADDR, (char *) &buf[i]))) 
				{
					strcpy(devices[i].ip, inet_ntoa(((struct sockaddr_in *) (&buf[i].ifr_addr))->sin_addr));
				}

				
				/*Get HW ADDRESS of the net card */
				if (!(ioctl(fd, SIOCGIFHWADDR, (char *) &buf[i])))
				{
					snprintf(devices[i].mac, sizeof(devices[i].mac),
					    "%02x %02x %02x %02x %02x %02x",
						(unsigned char) buf[i].ifr_hwaddr.sa_data[0],
						(unsigned char) buf[i].ifr_hwaddr.sa_data[1],
						(unsigned char) buf[i].ifr_hwaddr.sa_data[2],
						(unsigned char) buf[i].ifr_hwaddr.sa_data[3],
						(unsigned char) buf[i].ifr_hwaddr.sa_data[4],
						(unsigned char) buf[i].ifr_hwaddr.sa_data[5]
						);
				}
			}
		} 
	}
	close(fd);

	return count;
}




INT		ethcard_send_packet(ETHCARD *ethcard, BYTE *buf, INT len)
{
  	ETH_FRAME *epkt = (ETH_FRAME *)buf;

	struct sockaddr_ll to;
	to.sll_family = AF_PACKET;
	to.sll_protocol = epkt->type;

	//interface number
	//to.sll_ifindex = 2;
	to.sll_ifindex = ethcard->iface;
		
	//to.sll_hatype = ARPHRD_ETHER;
	//ARP tag, only useful when recv
	to.sll_hatype = 0;
	
	//to.sll_pkttype = 4;
	//Packet type, only useful when recv
	to.sll_pkttype = 0;

	//MAC address length
	to.sll_halen = 6;
	memset(to.sll_addr, 0, sizeof(to.sll_addr));
	memcpy(to.sll_addr, epkt->dest, 6);

	/*
	dprintf("Send:\n");
		dprintf("DEST: ");
		for(i = 1, j = 0; j < 6; i++,j++)
		{
			dprintf("%.2x ", buf[i-1]);
		}
		dprintf("\n");
		
		dprintf("SRC : ");
		for(j = 0; j < 6; i++,j++)
		{
			dprintf("%.2x ", buf[i-1]);
		}
		dprintf("\n");
	
		i += 2;  //0x888E
		
		dprintf("DATA:\n");
		for (j =1 ; (i < len ) ; i++, j++)
		{
			dprintf("%.2x ", buf[i-1]);
			if ( (j % 16) == 0) dprintf("\n");
		}
	
		dprintf("\n");
*/

	if( sendto(ethcard->fd, buf, len, 0, (const struct sockaddr *) &to, sizeof(to)) > 0)
	{
		return OK;
	}
	else
	{
		return ERR;
	}
}


static THREADRET raw_socket_loop_thread(THREAD *self)
{
	CHAR buf[2*32767];
	
	ETHCARD_LOOP_RECV_PROC_PARAM *pp, p;
	size_t len = 0, recvlen = 0;
	struct sockaddr_ll addr;

	fd_set set;
	struct timeval tv;

	
	pp = (ETHCARD_LOOP_RECV_PROC_PARAM *)self->param;
	p.ethcard = pp->ethcard;
	p.proc = pp->proc;

	BYTE *pkt_data;

	os_thread_init_complete(self);	

	//pcap_loop(p.ethcard, 0, dispatcher_handler, (BYTE *)&p);
	while(os_thread_is_running(self) || os_thread_is_paused(self))
	{
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(p.ethcard->fd, &set);
		select(p.ethcard->fd + 1, &set, NULL, NULL, &tv);
		if( FD_ISSET(p.ethcard->fd, &set) )
		{
			len = sizeof(addr);  
			recvlen = recvfrom(p.ethcard->fd,(char *)buf, sizeof(buf), 0, (struct sockaddr *)&addr, &len);
	
			if(recvlen > 0)
			{
				pkt_data = (BYTE *)buf;
		
				if(addr.sll_protocol == 0x8e88)
					p.proc(p.ethcard, pkt_data, recvlen);
			}
			else
			{
				//must be something wrong....
			}			
		}
		else
		{
			//dprintf("ETH Sleep...\n");
			os_sleep(20);
		}
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


	//dprintf("ethcard_start_loop_recv");
	thread_ethcard_recv = os_thread_create(raw_socket_loop_thread, (POINTER)&p, TRUE, FALSE);

}


VOID ethcard_stop_loop_recv()
{
	//dprintf("ethcard_stop_loop_recv");
	os_thread_kill(thread_ethcard_recv);
	while(thread_ethcard_recv) os_sleep(20);
}

ETHCARD *ethcard_open(char *name)
{
	ETHCARD *ec = NULL;
	int sock;
	
	if((sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL))) == -1)
	{
		//dprintf("Open Raw Socket ERROR\n");
		return NULL;
	}
	ec = os_new(ETHCARD, 1);
	
	ec->fd = sock;
	ec->iface = get_ethcard_iface_byname(ec->fd, name);

	//dprintf(" adaptor = %s   iface = %d\n", name, ethcard->iface);

	
	return ec;
}

ETHCARD *ethcard_close(ETHCARD *ethcard)
{
	if(ethcard)
	{
		close(ethcard->fd);
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


#endif //_LINUX

