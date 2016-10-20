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
#include <stdint.h>
#include <pthread.h>
#include "klringbuffer.h"

/* The PES Extractor will call your application in the same thread as the pe_processPacket
 * call happens. The buffer passed will be automatically freed upon completion of each callback,
 * under no circumstances attempt to retain it.
 */
typedef void (*pes_extractor_callback)(void *cb_context, unsigned char *buf, int byteCount);
struct pes_extractor_s
{
	/* Private data. None of these members are considered user visible. */
	uint16_t pid;
	KLRingBuffer *rb;
	int packet_size;
	void *cb_context;
	pes_extractor_callback cb;
	int has_sync;
};

/* PES Extractor mechanism, so convert MULTIPLE TS packets containing PES VANC, into PES array. */
int pe_alloc(struct pes_extractor_s **pe, void *user_context, pes_extractor_callback cb, uint16_t pid);

/* Push one or more transport packets (buffer aligned) into the extraction framework. */
size_t pe_push(struct pes_extractor_s *pe, unsigned char *pkt, int packetCount);

/* dealloc any private members inside the user allocated struct pes_extractor_s object. */
void pe_free(struct pes_extractor_s **pe);

#endif /* PES_EXTRATOR_H */
