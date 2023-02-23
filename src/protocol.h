#ifndef PROTOCOL_H
#define PROTOCOL_H
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/ip.h>
#include <linux/udp.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/ether.h>
#include <thread>
#include <vector>
#include <string>

class Variable
{
		public:
				size_t size;
				uint32_t id;	
				union {
						int32_t i32;
						uint32_t u32;
						int16_t i16;
						uint32_t u16;
						int8_t i8;
						uint8_t u8;
				}value;
};

struct MacAddress {
	uint8_t a;
	uint8_t b;
	uint8_t c;
	uint8_t d;
	uint8_t e;
	uint8_t f;
	void dump () {
			printf("[");
			printf("%02x:",a&0xff );
			printf("%02x:",b&0xff );
			printf("%02x:",c&0xff );
			printf("%02x:",d&0xff );
			printf("%02x:",e&0xff );
			printf("%02x",f&0xff );
			printf("]\n");
	}
};


class EapProtocol
{
	public:
			EapProtocol(std::string interface );
			virtual ~EapProtocol();
			bool setup();
			bool close();
			void setVerbose(bool s);
			void setEapVersion( uint32_t ver );
	public:	// Single variable read/write interface
			void setPublisherId( int id[6] );
			bool setNetworkVariable( MacAddress& addr, Variable& value );
			bool getNetworkVariable( MacAddress& addr, Variable& r_value );
	public:	
			void sniff();
			void start( MacAddress& addr);
			void wait();
			static void process(EapProtocol*);
			bool waitMessage();
	private:
			bool m_quit;
			std::thread m_worker;
			MacAddress m_workerMac;
	private:
			int parse_frame(int numbytes, uint8_t* buf, MacAddress& a_mac, Variable& value );
			int parse_frames(int numbytes, uint8_t* buf, MacAddress& a_mac, std::vector<Variable*>& r_values );
	private:
		std::string m_if;
		struct ifreq if_mac;
		struct ifreq if_idx;
		uint8_t m_mac[6];
		int m_sockfd;
		struct sockaddr_ll socket_address;
	private:
		uint32_t m_version;
		int publisherId[6];
		int cycleIndex;		//current cycle index
	public:
		bool m_setup;
		bool m_verbose;
};

#endif
