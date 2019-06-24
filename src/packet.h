/* Reliable UDP - Bovi Andrea, Pavia Roberto
 * Packet
 */

#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#include "constants.h"

// Packet Structure
struct pkt {
    uint64_t seq_num;
    uint32_t status;
    uint8_t data[MAX_PK_DATA_SIZE];
};

typedef struct pkt PACK;

#endif                          // PACKET_H
