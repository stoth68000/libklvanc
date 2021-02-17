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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hexdump.h"
#include "pes_extractor.h"

#define MAX_PES_SIZE 16384
#define LOCAL_DEBUG 0

/* PES Extractor mechanism, so convert MULTIPLE TS packets containing PES VANC, into PES array. */
int pe_alloc(struct pes_extractor_s **pe, void *user_context, pes_extractor_callback cb, uint16_t pid)
{
	struct pes_extractor_s *p = calloc(1, sizeof(*p));
	if (!p)
		return -1;

	p->rb = rb_new(MAX_PES_SIZE, 1048576);
	if (!p->rb) {
		free(p);
		return -1;
	}

	p->pid = pid;
	p->cb_context = user_context;
	p->cb = cb;
	p->packet_size = 188;

	*pe = p;
	return 0;
}

void pe_free(struct pes_extractor_s **pe)
{
	rb_free((*pe)->rb);
	free(*pe);
}

/* Take a single transport packet.
 * Calculate where the data begins.
 * Place all the data into a ring buffer.
 * Walk the ring buffer, locating any PES private data packets,
 * callback for each packet we detect.
 * ONLY PES_PRIVATE packets are supported, type 0xBD with
 * a valid length field.
 * Other packet types would be trivial to add, but know that
 * only VIDEO ES streams may have the pes_length value of zero
 * according to the spec.
 */
static void pe_processPacket(struct pes_extractor_s *pe, unsigned char *pkt, int len)
{
#if LOCAL_DEBUG
	printf("%s(len = %d)\n", __func__, len);
#endif
        int offset = 4;
#if 0
	if (*(pkt + 1) & 0x40)
                section_offset++;
#endif

        unsigned char adaption = (*(pkt + 3) >> 4) & 0x03;
        if ((adaption == 2) || (adaption == 3)) {
                if (offset == 4)
                        offset++;
                offset += *(pkt + 4);
        }

	/* Regardless, append all packete data from offset to end of packet into the buffer */
	size_t wlen = pe->packet_size - offset;
	size_t l = rb_write(pe->rb, (const char *)pkt + offset, wlen);
	if (l != wlen) {
		printf("Write error, l = %zu, wlen = %zu\n", l, wlen);
		return;
	}

	/* Now, peek in the buffer, obtain packet sync and dequeue packets */
	while (1) {
		/* If we have no yet syncronized, make that happen */
		if (!pe->has_sync && rb_used(pe->rb) > 16) {
			unsigned char hdr[] = { 0, 0, 1, 0xbd };
			unsigned char b[4];
			rb_read(pe->rb, (char *)&b[0], sizeof(b));
			for (size_t i = 0; i < rb_used(pe->rb); i++) {
				if (memcmp(b, hdr, sizeof(hdr)) == 0) {
					pe->has_sync = 1;
					break;
				}
				b[0] = b[1];
				b[1] = b[2];
				b[2] = b[3];
				rb_read(pe->rb, (char *)&b[3], 1);
			}
		}

		if (!pe->has_sync) {
			/* Need more data */
#if LOCAL_DEBUG
			printf("Need more data #1\n");
#endif
			break;
		}

		/* We have at least one viable message, probably..... */
		char l[2];
		if (rb_peek(pe->rb, (char *)&l[0], 2) != 2) {
			fprintf(stderr, "Unable to peek two ring buffer bytes\n");
			break;
		}

		uint16_t pes_length = ((l[0] & 0xff) << 8) | (l[1] & 0xff);

		if (rb_used(pe->rb) >= pes_length + 2) {
			/* Dequeue and process a complete pes message */
			unsigned char *msg = malloc(pes_length + 6);
			if (msg) {
				msg[0] = 0;
				msg[1] = 0;
				msg[2] = 1;
				msg[3] = 0xbd;

				/* Read the peeked length and the entire message */
				rb_read(pe->rb, (char *)msg + 4, pes_length + 2);
#if LOCAL_DEBUG
				hexdump(msg, pes_length + 6, 16);
#endif
				if (pe->cb)
					pe->cb(pe->cb_context, msg, pes_length + 6);
				free(msg);
			}
			pe->has_sync = 0;
		} else {
#if LOCAL_DEBUG
			printf("Need more data #2 - got 0x%x (%d) need 0x%x (%d)\n",
				rb_used(pe->rb) + 2,
				rb_used(pe->rb) + 2,
				pes_length,
				pes_length);
#endif
			break; /* Need more data */
		}
	}
}

size_t pe_push(struct pes_extractor_s *pe, unsigned char *pkt, int packetCount)
{
#if LOCAL_DEBUG
	printf("%s(packetCount = 0x%x)\n", __func__, packetCount);
#endif
        if ((!pe) || (packetCount < 1) || (!pkt))
                return 0;

        for (int i = 0; i < packetCount; i++) {
		uint16_t pid = ((*(pkt + (i * pe->packet_size) + 1) << 8) | *(pkt + (i * pe->packet_size) + 2)) & 0x1fff;
                if (pid == pe->pid) {
                        pe_processPacket(pe, pkt + (i * pe->packet_size), pe->packet_size);
                }
        }
        return packetCount;
}
