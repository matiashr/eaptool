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
#include "frames.h"
#include "eth.h"
#include "protocol.h"

#define MY_DEST_MAC0	0x01
#define MY_DEST_MAC1	0x01
#define MY_DEST_MAC2	0x05
#define MY_DEST_MAC3	0x04
#define MY_DEST_MAC4	0x00
#define MY_DEST_MAC5	0x00

static int raw=0;

static void help(char* app )
{
		printf("Usage: %s -i <eth0,1..> -m <mac> -h -l\n",app );
		printf("flags:\n");
		printf("   -i <device>          ; network device to use, example eth0 \n");
		printf("   -l                   ; sniff\n");
		printf("   -r                   ; raw mode\n");
		printf("   -v                   ; verbose \n");
		printf("   -r <no>              ; read no \n");
		printf("   -t <no> <value>      ; write no=value\n");
		printf("   -m 00:01:05:2e:1e:b2 ; listen to mac \n");
		printf("   -m <mac>             ; listen for data from node with mac, giving mac of all zeros will show all nodes, ignoring src mac \n");
		printf("Examples:\n");
		printf("note: must provide all characters for mac addresses, i.e. 00:01... not 0:1:...\n");
		printf("Example: sudo ./%s -i enx9884e302236a -t \"123 456\" - Send nvid 123 = 456\n",app);
		printf("Example: sudo ./%s -i enx9884e302236a -R 123 - Read nvid 123\n",app);

}

int main(int argc, char *argv[])
{
		MacAddress dst;
		bool verbose=false;
		enum {  SNIFF, SEND,READ }todo = SNIFF;
		char ifName[10];
		int  readvar=NULL;
		char* txdata=NULL;
		int c;
		if( getuid() != 0 ) {
				printf("Must run as root, try sudo %s\n", argv[0]);
				help(argv[0]);
				exit(1);
		}
		// default dst
		dst.a = MY_DEST_MAC0;
		dst.b = MY_DEST_MAC1;
		dst.c = MY_DEST_MAC2;
		dst.d = MY_DEST_MAC3;
		dst.e = MY_DEST_MAC4;
		dst.f = MY_DEST_MAC5;

		strcpy(ifName, DEFAULT_IF);
		while ((c = getopt (argc, argv, "i:lhm:t:rR:v")) != -1) {
				switch (c)
				{
						case 'v':
								{
										verbose=true;
								}break;
						case 'r':
								{
										//printf("Raw mode\n");
										raw=1;
								}break;
						case 't':
								{
										txdata = optarg;
										todo = SEND;
								}break;
						case 'R':
								{
										readvar = atoi(optarg);
										todo = READ;
								}break;

						case 'i':
								strcpy(ifName, optarg);
								break;
						case 'l':
								todo=SNIFF;
								break;
						case 'h':
								help(argv[0]);
								exit(0);
								break;
						case 'm':
								{
										sscanf(optarg,"%2x:%2x:%2x:%2x:%2x:%2x",
														&dst.a,  
														&dst.b,  
														&dst.c,  
														&dst.d,  
														&dst.e,  
														&dst.f
											  );
								}break;
				}
		}
		if( verbose) {
				printf("Using: %s raw:%d\n", ifName,raw);
				printf("Using mac: ");
				dst.dump();
		}

		EapProtocol p(ifName);
		p.setup();
		p.setVerbose(verbose);
		switch( todo)
		{
				case READ:
						{
								Variable value;
								value.id =  readvar;
								value.size = sizeof(uint32_t);
								p.getNetworkVariable( dst, value);
								switch( value.size )
								{
									case 1:
									{
										printf("%d\n", value.value.u32 );
									}break;
									case 2:
									{
										printf("%d\n", value.value.u16 );
									}break;
									case 4:
									{
										printf("%d\n", value.value.u32 );
									}break;
								}
						}break;
				case SEND:
						{
								uint32_t nvid;
								uint32_t data;
								sscanf(txdata,"%d %d", &nvid, &data );
								Variable value;
								value.id = nvid;
								value.size = sizeof(uint32_t);
								value.value.u32 = data;
								p.setNetworkVariable( dst, value);
						}break;
				case SNIFF:
						{
								p.sniff();
						}break;
		}
		return 0;
}


/* vim: set foldmethod=syntax:set list! */