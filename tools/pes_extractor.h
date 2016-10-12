/* Copyright (c) 2016 Kernel Labs Inc. */

/* A mechanism to collect PES_PRIVATE_1 packets from
 * ISO13818 transport packets, dequeue them and feed them
 * to the caller via callbacks.
 */

#ifndef PES_EXTRATOR_H
#define PES_EXTRATOR_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include "klringbuffer.h"

typedef void (*pes_extractor_callback)(void *cb_context, unsigned char *buf, int byteCount);
struct pes_extractor_s
{
	uint16_t pid;
	KLRingBuffer *rb;

	int packet_size;
	void *cb_context;
	pes_extractor_callback cb;
};

/* PES Extractor mechanism, so convert MULTIPLE TS packets containing PES VANC, into PES array. */
int pe_init(struct pes_extractor_s *pe, void *user_context, pes_extractor_callback cb, uint16_t pid);

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
void pe_processPacket(struct pes_extractor_s *pe, unsigned char *pkt, int len);
size_t pe_push(struct pes_extractor_s *pe, unsigned char *pkt, int packetCount);

#endif /* PES_EXTRATOR_H */
