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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int klvanc_dump_EIA_608(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_eia_608_s *pkt = p;

	if (ctx->verbose)
		PRINT_DEBUG("%s() %p\n", __func__, (void *)pkt);

	PRINT_DEBUG("%s() EIA608: %02x %02x %02x : marker-bits %02x cc_valid %d cc_type %d cc_data_1 %02x cc_data_2 %02x\n",
		    __func__,
		    pkt->payload[0],
		    pkt->payload[1],
		    pkt->payload[2],
		    pkt->marker_bits,
		    pkt->cc_valid,
		    pkt->cc_type,
		    pkt->cc_data_1,
		    pkt->cc_data_2);

	return KLAPI_OK;
}

int parse_EIA_608(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void **pp)
{

	if (ctx->callbacks == NULL || ctx->callbacks->eia_608 == NULL)
		return KLAPI_OK;

	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_eia_608_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

        /* Parsed */
	pkt->payload[0] = hdr->payload[0];
	pkt->payload[1] = hdr->payload[1];
	pkt->payload[2] = hdr->payload[2];

	pkt->marker_bits = pkt->payload[0] >> 3;
	pkt->cc_valid = (pkt->payload[0] >> 2) & 0x01;
	pkt->cc_type = pkt->payload[0] & 0x03;
	pkt->cc_data_1 = pkt->payload[1];
	pkt->cc_data_2 = pkt->payload[2];

	ctx->callbacks->eia_608(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

