#ifdef _BSD
#define _BPF
#endif
#ifdef _BPF

#include "ethcard.h"

#define ETHERTYPE_8021X 0x888e

static struct bpf_insn insns[] = {
	BPF_STMT(BPF_LD+BPF_H+BPF_ABS, 12),//加载halfword以太网链路层的type
	BPF_JUMP(BPF_JMP+BPF_JEQ+BPF_K, ETHERTYPE_8021X, 0, 1),//判断是否是802.1X数据包，是则返回给本程序
	BPF_STMT(BPF_RET+BPF_K, (u_int)-1),
	BPF_STMT(BPF_RET+BPF_K, 0),
};



static THREAD *thread_ethcard_recv = NULL;


#ifdef _MACOSX
INT   get_ethcard_iface_byname(int sd, CHAR *name)
{
    struct ifreq req;
    strncpy(req.ifr_name,name,IFNAMSIZ);
    return if_nametoindex(req.ifr_name);
}
#else
INT   get_ethcard_iface_byname(int sd, CHAR *name)
{
	int ret;
	struct ifreq req;
	strncpy(req.ifr_name,name,IFNAMSIZ);
	ret=ioctl(sd,SIOCGIFINDEX,&req);
	if (ret==-1) return -1;
	return req.ifr_index;
}
#endif


INT get_ethcards(ETHCARD_INFO *devices, INT bufsize)
{
	int fd, i = 0;
	char* buf[MAX_ETHCARDS * sizeof(struct ifreq)];
	struct ifconf ifc;
	struct ifreq *ifrp, *ifend, *ifnext;
	int n;

	if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) >= 0) 
	{
		ifc.ifc_len = sizeof(buf);
		ifc.ifc_buf = (caddr_t)buf;
		if (!ioctl(fd, SIOCGIFCONF, (char *) &ifc)) 
		{
//			count = ifc.ifc_len / sizeof(struct ifreq);
//			i = count;
//			while (i-- > 0)
			ifrp = (struct ifreq *)buf;
			ifend = (struct ifreq *)(buf + ifc.ifc_len);
			for (; ifrp < ifend; ifrp = ifnext)
			{
				n = ifrp->ifr_addr.sa_len + sizeof(ifrp->ifr_name);
				if (n < sizeof(*ifrp))
					ifnext = ifrp + 1;
				else
					ifnext = (struct ifreq *)((char *)ifrp + n);
				if (!(*ifrp->ifr_name))
					break;
				if (ifrp->ifr_addr.sa_family != AF_INET)
					continue;
				if (bufsize / sizeof(ETHCARD_INFO) == i)
					break;
				strcpy(devices[i].name, ifrp->ifr_name);
				strcpy(devices[i].desc, ifrp->ifr_name);
				
				/*Jugde whether the net card status is up*/
				if (!(ioctl(fd, SIOCGIFFLAGS, (char *) ifrp)))
				{
					devices[i].live = (ifrp->ifr_flags & IFF_UP);
				}
				
				/*Get IP of the net card */
				if (!(ioctl(fd, SIOCGIFADDR, (char *) ifrp))) 
				{
					strcpy(devices[i].ip, inet_ntoa(((struct sockaddr_in *) (&ifrp->ifr_addr))->sin_addr));
				}

				
#ifndef _BSD
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
#else
				struct ifaddrs *list;
				struct ether_addr *ea;
				if(getifaddrs(&list) >= 0)
				{
					struct ifaddrs *cur;        
					for(cur = list; cur != NULL; cur = cur->ifa_next)
					{
					if(cur->ifa_addr->sa_family != AF_LINK)
						continue;
					if (strncmp(cur->ifa_name, ifrp->ifr_name, IFNAMSIZ) == 0)
					{
						struct sockaddr_dl *sdl;
						sdl = (struct sockaddr_dl *)cur->ifa_addr;
		                ea = (const struct ether_addr *)LLADDR(sdl);
					}
				} 
					snprintf(devices[i].mac, sizeof(devices[i].mac),
					    "%02x %02x %02x %02x %02x %02x",
						(unsigned char) ea->octet[0],
						(unsigned char) ea->octet[1],
						(unsigned char) ea->octet[2],
						(unsigned char) ea->octet[3],
						(unsigned char) ea->octet[4],
						(unsigned char) ea->octet[5]
						);
					freeifaddrs(list);
				}

#endif
#if 0
				printf("%d. %s size: %d\n", i, ifrp->ifr_name, n);
				printf("%s\n", devices[i].ip);
				printf("UP: %s\n", devices[i].live? "true" : "false");
				printf("ether addr: %s\n", devices[i].mac);
#endif
				i++;
			}
		} 
	}
	close(fd);

	return i;
}




INT		ethcard_send_packet(ETHCARD *ethcard, BYTE *buf, INT len)
{
    return write(ethcard->fd, buf, len);
}


static THREADRET raw_socket_loop_thread(THREAD *self)
{
	CHAR buf[2*32767];
	
	ETHCARD_LOOP_RECV_PROC_PARAM *pp, p;
	ssize_t len = 0;

	fd_set set;
	struct timeval tv;

	
	pp = (ETHCARD_LOOP_RECV_PROC_PARAM *)self->param;
	p.ethcard = pp->ethcard;
	p.proc = pp->proc;

	BYTE *pkt_data;

	os_thread_init_complete(self);	

	while(os_thread_is_running(self) || os_thread_is_paused(self))
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&set);
		FD_SET(p.ethcard->fd, &set);
		int count;
		count = select(p.ethcard->fd + 1, &set, NULL, NULL, &tv);
		//dprintf("selected %d fds\n", count);
		if( FD_ISSET(p.ethcard->fd, &set) )
		{
			len = read(p.ethcard->fd, buf, sizeof(buf));
			if (len == -1)
			{
				perror("read error:");
			}
//			dprintf("read finished. len = %d\n",len);
			struct bpf_hdr *hdr = (struct bpf_hdr *)buf;
			pkt_data = (BYTE *)buf + hdr->bh_hdrlen;
			
			if(len > 0)
			{
				p.proc(p.ethcard, pkt_data , len);
			}
			else
			{
				//must be something wrong....
			}			
		}
		else
		{
//			dprintf("select over without set\n");
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
    struct ifreq ifr;
    struct bpf_program bpf_pro={4,insns};
	
    int bpf;
    int blen;
    
    int i;
    
    char device[sizeof "/dev/bpf000"];

    /*
     *  Go through all the minors and find one that isn't in use.
     */
    for (i = 0;;i++)
    {
        sprintf(device, "/dev/bpf%d", i);

        bpf = open(device, O_RDWR | O_NONBLOCK);
        if (bpf == -1 && errno == EBUSY)
        {
            /*
             *  Device is busy.
             */
            continue;
        }
        else
        {
            /*
             *  Either we've got a valid file descriptor, or errno is not
             *  EBUSY meaning we've probably run out of devices.
             */
            break;
        }
    }
        
	if( bpf == -1 )
	{
		dprintf("open /dev/bpf%d error\n", i);
		return NULL;
	}
	
	strcpy(ifr.ifr_name, name);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;
	int immediate;
	immediate = 1;
	int mblen = 2*32767;

	
	if (-1 == ioctl(bpf, BIOCIMMEDIATE, &immediate))
	{
		perror("error");
		dprintf("ioctl BIOCIMMEDIATE error\n");
		return NULL;
	}
	if (-1 == ioctl(bpf, BIOCGBLEN, &blen))
	{
		perror("error");
		dprintf("ioctl BIOCGBLEN error\n");
		return NULL;
	}
	if (-1 == ioctl(bpf, BIOCSBLEN, &mblen))
	{
		perror("ioctl BIOCSBLEN error");
		return NULL;
	}

	if (-1 == ioctl(bpf, BIOCSETIF, &ifr))
	{
		perror("error");
		dprintf("ioctl BIOCSETIF error\n");
		return NULL;
	}

	if (-1 == ioctl(bpf, BIOCSETF, &bpf_pro))
	{
		perror("error");
		dprintf("ioctl BIOCSETF error\n");
		return NULL;
	}

	if (-1 == ioctl(bpf, BIOCFLUSH))
	{
		perror("error");
		dprintf("ioctl BIOCFLUSH error\n");
		return NULL;
	}

	if (-1 == ioctl(bpf, BIOCSRTIMEOUT, &timeout))
	{
		perror("error");
		dprintf("ioctl BIOCSRTIMEOUT error\n");
		return NULL;
	}

	ec = os_new(ETHCARD, 1);
	
	ec->fd = bpf;
	ec->blen = blen;
	
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


#endif //_BPF

