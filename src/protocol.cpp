/*
 * (c) 2023 Matias Henttunen
 * mattimob@gmail.com
 * */
#include "eth.h"
#include "protocol.h"
#include "frames.h"

EapProtocol::EapProtocol(std::string interface):
		m_if(interface),
		m_setup(false),
		m_verbose(false),
		cycleIndex(0)
{

}

EapProtocol::~EapProtocol()
{
}


void EapProtocol::setVerbose(bool s)
{
	m_verbose=s;
}


bool EapProtocol::close()
{
		::close(m_sockfd);
		return true;
}

bool EapProtocol::setup()
{
		m_setup = false;
		if ((m_sockfd = socket(AF_PACKET, SOCK_RAW,  htons(PROTOCOL_ID))) == -1) {
				perror("socket");
				m_setup=false;
		} else {
				m_setup = true;
		}

		/* Get the index of the interface to send on */
		memset(&if_idx, 0, sizeof(struct ifreq));
		strncpy(if_idx.ifr_name, m_if.c_str(), IFNAMSIZ-1);
		if (ioctl(m_sockfd, SIOCGIFINDEX, &if_idx) < 0) {
				perror("SIOCGIFINDEX");
				exit(1);
		}
		/* Get the MAC address of the interface to send on */
		memset(&if_mac, 0, sizeof(struct ifreq));
		strncpy(if_mac.ifr_name, m_if.c_str(), IFNAMSIZ-1);
		if (ioctl(m_sockfd, SIOCGIFHWADDR, &if_mac) < 0) {
				perror("SIOCGIFHWADDR");
				exit(1);
		} else {
				if( m_verbose) {
				printf("Mymac:");
				dumpmac( (uint8_t*)if_mac.ifr_hwaddr.sa_data );printf("\n");
				}
		}
		return m_setup;
}

bool EapProtocol::setNetworkVariable(MacAddress& addr, Variable& value )
{
		bool status=false;
		int stat;
		int tx_len = 0;
		char sendbuf[1500];
		struct ether_header *eh = (struct ether_header *) sendbuf;
		struct eapframe* iframe;
		struct networkvar* nvar;

		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		tx_len = 0;

		/* Address length*/
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		socket_address.sll_halen = ETH_ALEN;

		iframe = (struct eapframe*)&sendbuf[tx_len];
		tx_len = sizeof(struct eapframe);
		/* set dst mac */
		iframe->dst[0] = addr.a;
		iframe->dst[1] = addr.b;
		iframe->dst[2] = addr.c;
		iframe->dst[3] = addr.d;
		iframe->dst[4] = addr.e;
		iframe->dst[5] = addr.f;

		/* set  source mac */
		iframe->src[0] = if_mac.ifr_hwaddr.sa_data[0];
		iframe->src[1] = if_mac.ifr_hwaddr.sa_data[1];
		iframe->src[2] = if_mac.ifr_hwaddr.sa_data[2];
		iframe->src[3] = if_mac.ifr_hwaddr.sa_data[3];
		iframe->src[4] = if_mac.ifr_hwaddr.sa_data[4];
		iframe->src[5] = if_mac.ifr_hwaddr.sa_data[5];
		// Type 
		iframe->type = htons(PROTOCOL_ID);

		iframe->echeader.length = 4;
		iframe->echeader.type = 4;
		iframe->networkvars.publisher[0] = 1;
		iframe->networkvars.publisher[1] = 2;
		iframe->networkvars.publisher[2] = 3;
		iframe->networkvars.publisher[3] = 4;
		iframe->networkvars.publisher[4] = 5;
		iframe->networkvars.publisher[5] = 6;
		iframe->networkvars.cycleix = htons(cycleIndex++);
		iframe->networkvars.count = 1;
		nvar =  (struct networkvar*)( sendbuf + sizeof(struct eapframe) );
		for(size_t i=0;i<iframe->networkvars.count;i++) {
				uint8_t *payload =  &nvar->d;
				nvar->nvheader.len = 1;
				nvar->nvheader.id  = value.id;
				switch( value.size )
				{ 
						case sizeof(uint8_t): *((uint32_t*)(payload)) = value.value.u8 ; break;
						case sizeof(uint16_t): *((uint32_t*)(payload)) = value.value.u32; break;
						case sizeof(uint32_t):
					    {
							   *((uint32_t*)(payload)) = value.value.u32; 
						}break;
						default:
						{
							printf("Unsupported type size %d\n", value.size );
							return false;
						}break;
				}
				tx_len += sizeof(struct networkvar);
		}
		if( m_verbose )  {
				printf(FRAMEFMT);
				printf("[");
				for(int i=0;i < tx_len;i++ ) {
						printf("%2x, ",sendbuf[i] &0xff);
				}
				printf("]:%d bytes\n", tx_len);
		}
		if( stat=sendto(m_sockfd, sendbuf, tx_len, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll)) < 0) {
				printf("Socket:%d len:%d errorcode=%d\n", m_sockfd, tx_len,  stat );
				perror("Send header ");
				exit(1);
		}
		return status;
}

int EapProtocol::parse_frame( int numbytes, uint8_t* buf, MacAddress& a_mac, Variable& r_value )
{
	struct sockaddr_storage their_addr;
	struct eapframe *iframe = (struct eapframe*) (buf);
	struct networkvar *nvar = (struct networkvar*)( sizeof(struct eapframe) + buf );
	int i;
	int ret;

	/* Check the packet is from a_mac node */
	{
		int match=1;
		if( (iframe->src[0] != a_mac.a) &&  
			(iframe->src[1] != a_mac.b) &&
			(iframe->src[2] != a_mac.c) &&
			(iframe->src[3] != a_mac.d) &&
			(iframe->src[4] != a_mac.e) &&
			(iframe->src[5] != a_mac.f) 
		) {
				//printf("%d (%x != %x )\n",i, a_mac[i], iframe->src[i] );
				match = 0;
		}
		if(match == 0) {
			 return -1;
		}
	}
	if( ntohs(iframe->type) == PROTOCOL_ID ) {
			if ( m_verbose) {
					printf("\033[1;0H\n");
					printf("Type: 0x%x (EAP)\n", iframe->type );	
					printf("Len : 0x%x\n", iframe->echeader.length );
					printf("DST : ");
					for (i=0; i< 6; i++){
							printf("%02x:", buf[i]);
					}
					printf("\n");
					printf("SRC : ");
					for (i=6; i<12; i++){
							printf("%02x:", buf[i]);
					}
					printf("\n");
					printf("valid: 0x%x\n", iframe->echeader.valid );
					printf("type : 0x%x\n", iframe->echeader.type &0xf );
					printf("cycleix : 0x%x\n", iframe->networkvars.cycleix);
					printf("publisher:");
					for(i=1;i<sizeof(iframe->networkvars.publisher);i++) {
							printf("%d.", iframe->networkvars.publisher[i] );
					}
					printf("varcnt: %d\n", iframe->networkvars.count);
			}
			for(i=0;i<iframe->networkvars.count;i++) {
					r_value.id = (nvar[i].nvheader.id);
					if (m_verbose ) {
							printf("VAR %d id=%d length:%d quality:%x\n", 
											i,
											(nvar[i].nvheader.id), 
											nvar[i].nvheader.len, 
											nvar[i].nvheader.quality );
					}
					switch( nvar[i].nvheader.len )
					{
							case 1:
							{
									uint8_t *payload = (uint8_t*)( &nvar[i].d );
									r_value.value.u32 = *payload;
									r_value.size = nvar[i].nvheader.len;
							}break;
							case 4:
							{
									uint32_t *payload = (uint32_t*)( &nvar[i].d );
									r_value.value.u32 = *payload;
									r_value.size = nvar[i].nvheader.len;
							}break;
							default:
							{
									printf("unexpected size %d\n", nvar[i].nvheader.len);
									r_value.size = nvar[i].nvheader.len;
									return -1;
							}break;
					}
			}
	}
	return 0;
}

bool EapProtocol::getNetworkVariable(MacAddress& addr, Variable& r_value )
{
		bool status=true;
		uint8_t buf[BUF_SIZ];
		Variable rval;
		while( true ) {
				int numbytes = recvfrom(m_sockfd, buf, BUF_SIZ, 0, NULL, NULL);
				if( numbytes > 0 ) {
						struct timespec now;
						clock_gettime(CLOCK_MONOTONIC, &now);
						if( m_verbose) {
								printf("\ntime:%lld.%.9ld [s.ns]\n\n", (long long)now.tv_sec, now.tv_nsec);
								printf(FRAMEFMT);
								printf("[");
								for(int i=0;i < numbytes;i++ ) {
										printf("%2x  ",buf[i]);
								}
								printf("]:%d (bytes)\n", numbytes);
						} 
						if( parse_frame( numbytes,  buf,  addr, rval) == 0 ) {
								if( rval.id == r_value.id ) {
										r_value.value = rval.value;
										if( m_verbose) {
												printf("ID match %d = %d\n",rval.id, r_value.id );
										} 
										return true;
								} else {
									//printf("ID match %d != %d\n",rval.id, r_value.id );
								}
						}
				} 
		}
		return status;
}

void EapProtocol::sniff()
{
		uint8_t buf[BUF_SIZ];
		printf(FRAMEFMT);
		while(1)
		{
				int numbytes = recvfrom(m_sockfd, buf, BUF_SIZ, 0, NULL, NULL);
				if( numbytes > 0 ) {
						struct timespec now;
						clock_gettime(CLOCK_MONOTONIC, &now);
						printf("[");
						for(int i=0;i < numbytes;i++ ) {
								printf("%2x  ",buf[i]);
						}
						printf("]:%d (bytes)\n", numbytes);
				} 
				usleep(1000*40);
		}

}

/* vim: set foldmethod=syntax:set list! */
