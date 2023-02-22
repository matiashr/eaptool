#include <stdio.h>
#include "eth.h"

void dumpmac( uint8_t a_mac[6] ) 
{
	int i;
	printf("[");
	for(i=0; i < 6; i++ ) {
		printf("%02x",a_mac[i]&0xff );
		if( i < 5 ) printf(":");
	}
	printf("]");
}

