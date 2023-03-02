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
		uint8_t sendbuf[1514];
		memset(sendbuf, 0, sizeof(sendbuf));
		framesize = 0;
		/* Address length*/
		socket_address.sll_ifindex = if_idx.ifr_ifindex;
		socket_address.sll_halen = ETH_ALEN;
		struct ethercatframe_s* ethcat;
		struct processdata_s* pdata;
		struct pdo_s* nvar;
		ethcat = (struct ethercatframe_s*)&sendbuf[framesize];
		framesize = sizeof(struct ethercatframe_s);
		pdata = (struct processdata_s*)&sendbuf[framesize];
		framesize += sizeof(struct processdata_s);
		nvar = (struct pdo_s *)&sendbuf[framesize];
		framesize += sizeof(struct pdo_s);

		/* set dst mac */
		ethcat->dst[0] = addr.a;
		ethcat->dst[1] = addr.b;
		ethcat->dst[2] = addr.c;
		ethcat->dst[3] = addr.d;
		ethcat->dst[4] = addr.e;
		ethcat->dst[5] = addr.f;

		/* set  source mac */
		ethcat->src[0] = if_mac.ifr_hwaddr.sa_data[0];
		ethcat->src[1] = if_mac.ifr_hwaddr.sa_data[1];
		ethcat->src[2] = if_mac.ifr_hwaddr.sa_data[2];
		ethcat->src[3] = if_mac.ifr_hwaddr.sa_data[3];
		ethcat->src[4] = if_mac.ifr_hwaddr.sa_data[4];
		ethcat->src[5] = if_mac.ifr_hwaddr.sa_data[5];
		// Type 
		ethcat->type = htons(PROTOCOL_ID);

		ethcat->echeader.length = 4;
		ethcat->echeader.type = NETWORK_VAR;
		pdata->publisher[0] = publisherId[0];
		pdata->publisher[1] = publisherId[1];
		pdata->publisher[2] = publisherId[2];
		pdata->publisher[3] = publisherId[3];
		pdata->publisher[4] = publisherId[4];
		pdata->publisher[5] = publisherId[5];
		pdata->cycleix = htons(cycleIndex++);
		pdata->pdocount = 1;
		uint8_t* pos=( sendbuf+ sizeof(struct ethercatframe_s) + sizeof(struct pdo_s));
		struct pdo_s* pdo          = (struct pdo_s*)( sendbuf + sizeof(struct ethercatframe_s) + sizeof(struct processdata_s));
		for(size_t i=0;i<pdata->pdocount;i++) {
				nvar->len = value.size;
				nvar->id  = value.id;
				switch( value.size )
				{ 
						case sizeof(uint8_t):{
													 uint8_t *payload =  (uint8_t*)(sendbuf+framesize);
													 *((uint8_t*)(payload)) = value.value.u8 ;
											 }break;
						case sizeof(uint16_t):{
													 uint16_t *payload =  (uint16_t*)(sendbuf+framesize);
													  *((uint16_t*)(payload)) = value.value.u32; 
											  }break;
						case sizeof(uint32_t):
											  {
													 uint32_t *payload =  (uint32_t*)(sendbuf+framesize);
													 *((uint32_t*)(payload)) = value.value.u32; 
											  }break;
						default:
											  {
													  printf("Unsupported type size %ld\n", value.size );
													  return false;
											  }break;
				}
				framesize += sizeof(struct pdo_s);
				framesize += value.size;
				pdo = (struct pdo_s*)( sendbuf);
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
	struct ethercatframe_s* ethcat= (struct ethercatframe_s*) (buf);
	struct processdata_s *pdata = (struct processdata_s*)( sizeof(struct ethercatframe_s) + buf );
	struct pdo_s* pdo =  (struct pdo_s*)( buf + sizeof(struct ethercatframe_s) + sizeof(struct processdata_s));

	/* Check the packet is from a_mac node */
	{
		int match=1;
		if( (ethcat->src[0] != a_mac.a) &&  
			(ethcat->src[1] != a_mac.b) &&
			(ethcat->src[2] != a_mac.c) &&
			(ethcat->src[3] != a_mac.d) &&
			(ethcat->src[4] != a_mac.e) &&
			(ethcat->src[5] != a_mac.f) 
		) {
				//printf("%d (%x != %x )\n",i, a_mac[i], ethcat->src[i] );
				match = 0;
		}
		if(match == 0) {
			 return -1;
		}
	}
	if( ntohs(ethcat->type) == PROTOCOL_ID ) {
			if ( m_verbose) {
				//	printf("\033[1;0H\n");
					printf("Type: 0x%x (EAP)\n", ethcat->type );	
					printf("Len : 0x%x\n", ethcat->echeader.length );
					printf("DST : ");
					for (size_t i=0; i< 6; i++){
							printf("%02x:", buf[i]);
					}
					printf("\n");
					printf("SRC : ");
					for (size_t i=6; i<12; i++){
							printf("%02x:", buf[i]);
					}
					printf("\n");
					printf("valid: 0x%x\n", ethcat->echeader.valid );
					printf("type : 0x%x\n", ethcat->echeader.type &0xf );
					printf("cycleix : 0x%x\n", pdata->cycleix);
					printf("publisher:");
					for(size_t i=1;i<sizeof(pdata->publisher);i++) {
							printf("%d.", pdata->publisher[i] );
					}
					printf("varcnt: %d\n", pdata->pdocount);
			}
			uint8_t* pos=( buf + sizeof(struct ethercatframe_s) + sizeof(struct processdata_s));
			for( int i=0; i < pdata->pdocount; i++ ) {
					uint8_t* dt = pos + sizeof(struct pdo_s);
					r_value.id = pdo->id;
					r_value.size=pdo->len;
					switch( pdo->len )
					{
							case 1: {
											r_value.value.u8 = *((uint8_t*)( dt));
									}break;
							case 2: {
											r_value.value.u16 = *((uint16_t*)( dt ));
									} break;
							case 4: {
											r_value.value.u32 = *((uint32_t*)( dt ));
									}break;
					}
					pos += sizeof(struct pdo_s) + pdo->len ;
					pdo = (struct pdo_s*)( pos );
			}
			return 1;
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
		struct ethercatframe_s *ethcat= (struct ethercatframe_s*) (buf);
		struct processdata_s *pdata = (struct processdata_s*)( sizeof(struct ethercatframe_s) + buf );
		struct pdo_s* pdo =  (struct pdo_s*)( buf + sizeof(struct ethercatframe_s) + sizeof(struct processdata_s));

		int i;
		int match=1;
		if( (ethcat->src[0] != a_mac.a) &&  
						(ethcat->src[1] != a_mac.b) &&
						(ethcat->src[2] != a_mac.c) &&
						(ethcat->src[3] != a_mac.d) &&
						(ethcat->src[4] != a_mac.e) &&
						(ethcat->src[5] != a_mac.f) 
		  ) {
				match = 0;
		}
		if(match == 0) {
				return 0;
		}

		if( ntohs(ethcat->type) == PROTOCOL_ID ) {
				uint8_t* pos=( buf + sizeof(struct ethercatframe_s) + sizeof(struct processdata_s));
				for(i=0;i<pdata->pdocount;i++) {
						Variable* var = new Variable();
						var->id = (pdo->id);
						var->size = pdo->len;
						uint8_t* dt = pos + sizeof(struct pdo_s);
						switch( pdo->len )
						{
								case 1:
										{
												uint8_t *payload = (uint8_t*)( dt );
												var->value.u32 = *payload;
										}break;
								case 2:
										{
												uint16_t *payload = (uint16_t*)( dt );
												var->value.u16 = *payload;
										}break;

								case 4:
										{
												uint32_t *payload = (uint32_t*)(dt);
												var->value.u32 = *payload;
										}break;
								default:
										{
												printf("unexpected size %d\n",pdo->len);
												return 0;
										}break;
						}
						r_values.push_back( var );
						pos += sizeof(struct pdo_s) + pdo->len ;
						pdo = (struct pdo_s*)( pos );
				}
				return pdata->pdocount;
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
						std::vector<Variable*> values;
						if( parse_frames( numbytes,  buf,  addr, values) > 0 ) {
								for ( auto val: values ){
										if( val->id == r_value.id ) {
												r_value.value = val->value;
												if( m_verbose) {
														printf("ID match %d = %d\n",val->id, r_value.id );
												} 
												return true;
										} else {
												//printf("ID match %d != %d\n",rval.id, r_value.id );
										}
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
