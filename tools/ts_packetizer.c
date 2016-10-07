
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ts_packetizer.h"

/* Convert PES data into a series of TS packets */
int packetizer(uint8_t *buf, unsigned int byteCount, uint8_t **pkts, uint32_t *packetCount,
	int packetSize, uint8_t *cc, uint16_t pid)
{
	if ((!buf) || (byteCount == 0) || (!pkts) || (!packetCount) || (packetSize != 188) || (!cc) || (pid > 0x1fff))
		return -1;

	unsigned int offset = 0;

	int max = packetSize - 4;
	int packets = ((byteCount / max) + 1) * packetSize;
	int cnt = 0;

	uint8_t *arr = calloc(packets, packetSize);

	unsigned int rem = byteCount - offset;
	while (rem) {
		if (rem > max)
			rem = max;

		uint8_t *p = arr + (cnt * packetSize);
		*(p + 0) = 0x47;
		*(p + 1) = pid >> 8;
		*(p + 2) = pid;
		*(p + 3) = 0x10 | ((*cc) & 0x0f);
		memcpy(p + 4, buf + offset, rem);
		memset(p + 4 + rem, 0xff, packetSize - rem - 4);
		offset += rem;
		(*cc)++;

		if (cnt++ == 0)
			*(p + 1) |= 0x40; /* PES Header */

		rem = byteCount - offset;
	}

	*pkts = arr;
	*packetCount = cnt;
	return 0;
}
