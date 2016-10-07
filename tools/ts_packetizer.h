
#ifndef TS_PACKETIZER_H
#define TS_PACKETIZER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int packetizer(uint8_t *buf, unsigned int byteCount, uint8_t **pkts, uint32_t *packetCount, int packetSize, uint8_t *cc, uint16_t pid);

#ifdef __cplusplus
};
#endif


#endif /* TS_PACKETIZER_H */
