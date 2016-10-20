/*
 * Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved
 *
 * Address: Kernel Labs Inc., PO Box 745, St James, NY. 11780
 * Contact: sales@kernellabs.com
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "ts_packetizer.h"

/* Convert PES data into a series of TS packets */
int ts_packetizer(uint8_t *buf, unsigned int byteCount, uint8_t **pkts, uint32_t *packetCount,
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
