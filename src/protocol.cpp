/*
 * (c) 2023 Matias Henttunen
 * mattimob@gmail.com
 * */
#include <vector>
#include <map>
#include "eth.h"
#include "protocol.h"
#include "frames.h"
#include <nlohmann/json.hpp>
#include <zmq.hpp>
#include "mcp/queue.h"

EapProtocol::EapProtocol(std::string interface):
		m_if(interface),
		m_version(3),
		cycleIndex(0),
		m_setup(false),
		m_verbose(false)
{
	publisherId[0] = 1;
	publisherId[1] = 2;
	publisherId[2] = 3;
	publisherId[3] = 4;
	publisherId[4] = 5;
	publisherId[5] = 6;
}

EapProtocol::~EapProtocol()
{
}


void EapProtocol::setVerbose(bool s)
{
	m_verbose=s;
}

void EapProtocol::setPublisherId( int id[6] )
{
	publisherId[0] = id[0];
	publisherId[1] = id[1];
	publisherId[2] = id[2];
	publisherId[3] = id[3];
	publisherId[4] = id[4];
	publisherId[5] = id[5];
	printf("Publisher: %2x:%2x:%2x:%2x:%2x:%2x\n", 
					id[0],
					id[1],
					id[2],
					id[3],
					id[4],
					id[5]);
}

bool EapProtocol::close()
{
		::close(m_sockfd);
		return true;
}

void EapProtocol::setEapVersion( uint32_t ver )
{
		m_version = ver;
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
		int framesize = 0;
		char sendbuf[1500];
		struct eapframe* eapframe;
		struct networkvar* nvar;

		/* Construct the Ethernet header */
		memset(sendbuf, 0, BUF_SIZ);
		framesize = 0;

		/* Address length*/
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		socket_address.sll_halen = ETH_ALEN;

		eapframe = (struct eapframe*)&sendbuf[framesize];
		framesize = sizeof(struct eapframe);
		/* set dst mac */
		eapframe->dst[0] = addr.a;
		eapframe->dst[1] = addr.b;
		eapframe->dst[2] = addr.c;
		eapframe->dst[3] = addr.d;
		eapframe->dst[4] = addr.e;
		eapframe->dst[5] = addr.f;

		/* set  source mac */
		eapframe->src[0] = if_mac.ifr_hwaddr.sa_data[0];
		eapframe->src[1] = if_mac.ifr_hwaddr.sa_data[1];
		eapframe->src[2] = if_mac.ifr_hwaddr.sa_data[2];
		eapframe->src[3] = if_mac.ifr_hwaddr.sa_data[3];
		eapframe->src[4] = if_mac.ifr_hwaddr.sa_data[4];
		eapframe->src[5] = if_mac.ifr_hwaddr.sa_data[5];
		// Type 
		eapframe->type = htons(PROTOCOL_ID);

		eapframe->echeader.length = 4;
		eapframe->echeader.type = NETWORK_VAR;
		eapframe->networkvars.publisher[0] = publisherId[0];
		eapframe->networkvars.publisher[1] = publisherId[1];
		eapframe->networkvars.publisher[2] = publisherId[2];
		eapframe->networkvars.publisher[3] = publisherId[3];
		eapframe->networkvars.publisher[4] = publisherId[4];
		eapframe->networkvars.publisher[5] = publisherId[5];
		eapframe->networkvars.cycleix = htons(cycleIndex++);
		eapframe->networkvars.count = 1;
		nvar =  (struct networkvar*)( sendbuf + sizeof(struct eapframe) );
		for(size_t i=0;i<eapframe->networkvars.count;i++) {
				nvar->nvheader.len = value.size;
				nvar->nvheader.id  = value.id;
				switch( value.size )
				{ 
						case sizeof(uint8_t):{
													 uint8_t *payload =  (uint8_t*)&nvar->d;
													 *((uint8_t*)(payload)) = value.value.u8 ;
											 }break;
						case sizeof(uint16_t):{
													 uint16_t *payload =  (uint16_t*)&nvar->d;
													  *((uint16_t*)(payload)) = value.value.u32; 
											  }break;
						case sizeof(uint32_t):
											  {
													 uint32_t *payload =  (uint32_t*) &nvar->d;
													 *((uint32_t*)(payload)) = value.value.u32; 
											  }break;
						default:
											  {
													  printf("Unsupported type size %ld\n", value.size );
													  return false;
											  }break;
				}
				framesize += sizeof(struct networkvar);
				framesize += value.size;
		}
		if( m_verbose )  {
				printf(FRAMEFMT);
				printf("[");
				for(int i=0;i < framesize;i++ ) {
						printf("%2x, ",sendbuf[i] &0xff);
				}
				printf("]:%d bytes\n", framesize);
		}
		if( (stat=sendto(m_sockfd, sendbuf, framesize, 0, (struct sockaddr*)&socket_address, sizeof(struct sockaddr_ll))) < 0) {
				printf("Socket:%d len:%d errorcode=%d\n", m_sockfd, framesize,  stat );
				perror("Send header ");
				exit(1);
		}
		return status;
}

int EapProtocol::parse_frame( int numbytes, uint8_t* buf, MacAddress& a_mac, Variable& r_value )
{
	struct eapframe *eapframe = (struct eapframe*) (buf);
	struct networkvar *nvar = (struct networkvar*)( sizeof(struct eapframe) + buf );
	size_t i;

	/* Check the packet is from a_mac node */
	{
		int match=1;
		if( (eapframe->src[0] != a_mac.a) &&  
			(eapframe->src[1] != a_mac.b) &&
			(eapframe->src[2] != a_mac.c) &&
			(eapframe->src[3] != a_mac.d) &&
			(eapframe->src[4] != a_mac.e) &&
			(eapframe->src[5] != a_mac.f) 
		) {
				//printf("%d (%x != %x )\n",i, a_mac[i], eapframe->src[i] );
				match = 0;
		}
		if(match == 0) {
			 return -1;
		}
	}
	if( ntohs(eapframe->type) == PROTOCOL_ID ) {
			if ( m_verbose) {
					printf("\033[1;0H\n");
					printf("Type: 0x%x (EAP)\n", eapframe->type );	
					printf("Len : 0x%x\n", eapframe->echeader.length );
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
					printf("valid: 0x%x\n", eapframe->echeader.valid );
					printf("type : 0x%x\n", eapframe->echeader.type &0xf );
					printf("cycleix : 0x%x\n", eapframe->networkvars.cycleix);
					printf("publisher:");
					for(i=1;i<sizeof(eapframe->networkvars.publisher);i++) {
							printf("%d.", eapframe->networkvars.publisher[i] );
					}
					printf("varcnt: %d\n", eapframe->networkvars.count);
			}
			for(i=0;i<eapframe->networkvars.count;i++) {
					r_value.id = (nvar[i].nvheader.id);
					if (m_verbose ) {
							printf("VAR %ld id=%d length:%d quality:%x\n", 
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

/*
 *	Network vars may contain multiple values
 *	parse all and return in list
 *
 * */
int EapProtocol::parse_frames(int numbytes, uint8_t* buf, MacAddress& a_mac, std::vector<Variable*>& r_values )
{
		struct eapframe *eapframe = (struct eapframe*) (buf);
		struct networkvar *nvar = (struct networkvar*)( sizeof(struct eapframe) + buf );
		int i;
		int match=1;
		if( (eapframe->src[0] != a_mac.a) &&  
						(eapframe->src[1] != a_mac.b) &&
						(eapframe->src[2] != a_mac.c) &&
						(eapframe->src[3] != a_mac.d) &&
						(eapframe->src[4] != a_mac.e) &&
						(eapframe->src[5] != a_mac.f) 
		  ) {
				match = 0;
		}
		if(match == 0) {
				return 0;
		}

		if( ntohs(eapframe->type) == PROTOCOL_ID ) {
				for(i=0;i<eapframe->networkvars.count;i++) {
						Variable* var = new Variable();
						var->id = (nvar[i].nvheader.id);
						switch( nvar[i].nvheader.len )
						{
								case 1:
										{
												uint8_t *payload = (uint8_t*)( &nvar[i].d );
												var->value.u32 = *payload;
												var->size = nvar[i].nvheader.len;
										}break;
								case 4:
										{
												uint32_t *payload = (uint32_t*)( &nvar[i].d );
												var->value.u32 = *payload;
												var->size = nvar[i].nvheader.len;
										}break;
								default:
										{
												printf("unexpected size %d\n", nvar[i].nvheader.len);
												var->size = nvar[i].nvheader.len;
												return 0;
										}break;
						}
						r_values.push_back( var );
				}
				return eapframe->networkvars.count;
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

void EapProtocol::start( MacAddress& addr)
{	
	m_quit = false;
	m_worker = std::thread(process, this);
	m_workerMac = addr;
}

void EapProtocol::wait()
{	
		m_worker.join();
		printf("Terminated\n");
}


using json = nlohmann::json;
class MyReciver: public ServerCommand
{
    public:
        bool rxMessage( std::string msg, std::string& r_resp ) {
            json j;
            j["status"] = "Yeah";
            r_resp = j.dump();
            return true;
        }
};

/*************************************************************************************
 *	External interface 
 * for communicating with other process:es using zmq
 *
 *************************************************************************************/

/*
 * create thread
 * and parse frames as they are recived
 * does the following:
 * 1.keep a list of frames recieved
 * 2.on value change publish zmq message
 * 
 * */
void EapProtocol::process(EapProtocol* self)
{
	uint8_t buf[BUF_SIZ];
	printf("start background thread\n");
	//std::vector <Variable*> recieved;
	std::map<uint32_t, Variable*> recieved;
	IPCBroadcaster srv("tcp://*:5555" );
	srv.run();
	while(!self->m_quit)
		{
				int numbytes = recvfrom(self->m_sockfd, buf, sizeof(buf), 0, NULL, NULL);
				if( numbytes > 0 ) {
						int no=0;
						std::vector <Variable*> rx;
						if( (no=self->parse_frames( numbytes,  buf,  self->m_workerMac, rx)) > 0 ) {
								for( auto var : rx ) {
										std::map<uint32_t,Variable* >::iterator item = recieved.find(var->id);
										if( item != recieved.end()) { 
												if( item->second->value.u32 != var->value.u32 ) {
														item->second->value.u32 = var->value.u32 ;
														json j;
														j["id"] = var->id;
														j["size"] = var->size;
														j["value"] = var->value.u32;
														if( self->m_verbose ) {
																printf("%d = %d\n", var->id, var->value.u32  );
														}
														try{
																std::string data = j.dump();
																srv.send("/eap", data );
														}catch( ... ) {
																printf("Update failed\n");
														}
												}
										} else {
												/* variable not previously in list =>add it*/
												Variable* newvar = new Variable();
												newvar->id = var->id;
												newvar->size = var->size;
												newvar->value.u32 = var->value.u32;
												recieved[var->id] = newvar ;
										}
								}
						}
						if( rx.size() > 0 ) {
								rx.clear();
						}
				} 
		}
}


class Reciever : public IPCReciever
{
    public:
        Reciever( std::string a_listen ) : IPCReciever(a_listen){
		};
        ~Reciever() {
		};
		void onMessage( std::string topic, std::string data ) {
			printf("%s\n", data.c_str());
		}

};

bool EapProtocol::waitMessage()
{
	Reciever rx("tcp://localhost:5555");
	rx.recieve("/eap");
	rx.run();
	return true;
}


/* vim: set foldmethod=syntax:set list! */
