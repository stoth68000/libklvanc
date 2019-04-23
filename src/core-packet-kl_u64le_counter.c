/*
 * Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved
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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

int klvanc_create_KL_U64LE_COUNTER(struct klvanc_packet_kl_u64le_counter_s **pkt)
{
	struct klvanc_packet_kl_u64le_counter_s *p = calloc(1, sizeof(*p));
	if (p == NULL)
		return -ENOMEM;

	*pkt = p;
	return 0;
}

int klvanc_dump_KL_U64LE_COUNTER(struct klvanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_kl_u64le_counter_s *pkt = p;

	PRINT_DEBUG("%s() KL_U64LE_COUNTER: %" PRIu64 " [%" PRIx64 "]\n", __func__, pkt->counter, pkt->counter);

	return KLAPI_OK;
}

int parse_KL_U64LE_COUNTER(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_kl_u64le_counter_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	pkt->counter = 0;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[0]) << 56;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[1]) << 48;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[2]) << 40;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[3]) << 32;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[4]) << 24;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[5]) << 16;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[6]) <<  8;
	pkt->counter |= (uint64_t)sanitizeWord(hdr->payload[7]);

	if (ctx->callbacks && ctx->callbacks->kl_i64le_counter)
		ctx->callbacks->kl_i64le_counter(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_convert_KL_U64LE_COUNTER_to_words(struct klvanc_packet_kl_u64le_counter_s *pkt,
					     uint16_t **words, uint16_t *wordCount)
{
	uint8_t buf[8];
	buf[0] = pkt->counter >> 56;
	buf[1] = pkt->counter >> 48;
	buf[2] = pkt->counter >> 40;
	buf[3] = pkt->counter >> 32;
	buf[4] = pkt->counter >> 24;
	buf[5] = pkt->counter >> 16;
	buf[6] = pkt->counter >> 8;
	buf[7] = pkt->counter;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0xfe, 0x40, buf, sizeof(buf), words, wordCount, 10);
}
