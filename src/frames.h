#ifndef FRAMES_H
#define FRAMES_H
#include <stdint.h>

#define BUF_SIZ         4096
#define FRAMEFMT\
	"+-----------------------+-----------------------+-------+--------+---------------------+-------+-------+---+-------+-------+--------+------+----------------------------------------------------------------------------------------------------+\n"\
	"|     dst               |        src            | type  |echdr   |  publisher          | count |cycleix|pad|  id   | hash  | len    |quali |       values                                                                                       |\n"\
	"+-----------------------+-----------------------+-------+--------+---------------------+-------+-------+---+-------+-------+--------+------+----------------------------------------------------------------------------------------------------+\n"\


#define PROTOCOL_ID     0x88A4 //ethercat

struct eapframe {
	uint8_t dst[6];
	uint8_t src[6];
	uint16_t type;
	struct {
		uint16_t length:11;
		uint16_t valid:1;
		uint16_t type:3;
	}echeader;
	/* PAYLOAD */
	struct {
		uint8_t publisher[6];
		uint16_t count;
		uint16_t cycleix;
	}networkvars;
	uint16_t padding;
} __attribute__((packed));

struct networkvar {
	struct {
		uint16_t id;
		uint16_t hash;
		uint16_t len;
		uint16_t quality;
	}nvheader;
	/* payload data */
	uint8_t d;
} __attribute__((packed));


#endif
