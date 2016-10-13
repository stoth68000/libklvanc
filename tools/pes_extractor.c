/* Copyright (c) 2016 Kernel Labs Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "hexdump.h"
#include "pes_extractor.h"

#define MAX_PES_SIZE 16384

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
	int has_sync = 0;
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
		if (!has_sync && rb_used(pe->rb) > 16) {
			unsigned char hdr[] = { 0, 0, 1, 0xbd };
			unsigned char b[4];
			rb_read(pe->rb, (char *)&b[0], sizeof(b));
			for (size_t i = 0; i < rb_used(pe->rb); i++) {
				if (memcmp(b, hdr, sizeof(hdr)) == 0) {
					has_sync = 1;
					break;
				}
				b[0] = b[1];
				b[1] = b[2];
				b[2] = b[3];
				rb_read(pe->rb, (char *)&b[3], 1);
			}
		}

		if (!has_sync) {
			/* Need more data */
			break;
		}

		/* We have at least one viable message, probably..... */
		char l[2];
		rb_peek(pe->rb, (char *)&l[0], sizeof(l));
		uint16_t pes_length = l[0] << 8 | l[1];

		if (rb_used(pe->rb) + 2 > pes_length) {
			/* Dequeue and process a complete pes message */
			unsigned char *msg = malloc(pes_length + 6);
			if (msg) {
				msg[0] = 0;
				msg[1] = 0;
				msg[2] = 1;
				msg[3] = 0xbd;

				/* Read the peeked length and the entire message */
				rb_read(pe->rb, (char *)msg + 4, pes_length + 2);
				//hexdump(msg, pes_length + 6, 16);
				if (pe->cb)
					pe->cb(pe->cb_context, msg, pes_length + 6);
				free(msg);
			}
		} else {
			break; /* Need more data */
		}

		has_sync = 0;
	}
}

size_t pe_push(struct pes_extractor_s *pe, unsigned char *pkt, int packetCount)
{
        if ((!pe) || (packetCount < 1) || (!pkt))
                return 0;

        for (int i = 0; i < packetCount; i++) {
		uint16_t pid = ((*(pkt + 1) << 8) | *(pkt + 2)) & 0x1fff;
                if (pid == pe->pid) {
                        pe_processPacket(pe, pkt + (i * pe->packet_size), pe->packet_size);
                }
        }
        return packetCount;
}
