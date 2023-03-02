#ifndef FRAMES_H
#define FRAMES_H
/*
 * (c) 2023 Matias Henttunen
 * mattimob@gmail.com
 * */
#include <stdint.h>

#define BUF_SIZ         4096
#define FRAMEFMT\
	"+-----------------------+-----------------------+-------+--------+---------------------+-------+-------+---+-------+-------+--------+------+----------------------------------------------------------------------------------------------------+\n"\
	"|     dst               |        src            | type  |echdr   |  publisher          | count |cycleix|pad|  id   | hash  | len    |quali |       values                                                                                       |\n"\
	"+-----------------------+-----------------------+-------+--------+---------------------+-------+-------+---+-------+-------+--------+------+----------------------------------------------------------------------------------------------------+\n"\


#define PROTOCOL_ID     0x88A4 //ethercat

#define NETWORK_VAR 4

struct ethercatframe_s {
    uint8_t dst[6];
    uint8_t src[6];
    uint16_t type;
    struct {
        uint16_t length:11;
        uint16_t valid:1;
        uint16_t type:3;
    }echeader __attribute__((packed));
} __attribute__((packed));

/* Process data frame header */
struct processdata_s {
        uint8_t publisher[6];
        uint16_t pdocount;
        uint16_t cycleix;
        uint8_t res;
        uint8_t eapsm;
}__attribute__((packed));

struct pdo_s {
        uint16_t id;
        uint16_t hash;
        uint16_t len;
        uint16_t quality;
        /* payload data */
} __attribute__((packed));



#endif
