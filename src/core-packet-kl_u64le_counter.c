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

int dump_KL_U64LE_COUNTER(struct vanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_kl_u64le_counter_s *pkt = p;

	printf("%s() KL_U64LE_COUNTER: %" PRIu64 " [%" PRIx64 "]\n", __func__, pkt->counter, pkt->counter);

	return KLAPI_OK;
}

int parse_KL_U64LE_COUNTER(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_kl_u64le_counter_s *pkt = calloc(1, sizeof(*pkt));
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

