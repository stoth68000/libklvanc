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

	PRINT_DEBUG("%s() EIA608: %02x %02x %02x : field %d line_offset %d cc_data_1 %02x cc_data_2 %02x\n",
		    __func__,
		    pkt->payload[0],
		    pkt->payload[1],
		    pkt->payload[2],
		    pkt->field,
		    pkt->line_offset,
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

        /* See SMPTE ST 334-1:2015 Annex B */
	if (pkt->payload[0] & 0x80)
		pkt->field = 0;
	else
		pkt->field = 1;
	pkt->line_offset = pkt->payload[0] & 0x1f;
	pkt->cc_data_1 = pkt->payload[1];
	pkt->cc_data_2 = pkt->payload[2];

	ctx->callbacks->eia_608(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_create_EIA_608(struct klvanc_packet_eia_608_s **pkt)
{
	struct klvanc_packet_eia_608_s *p = calloc(1, sizeof(*p));
	if (p == NULL)
		return -ENOMEM;

	*pkt = p;
	return 0;
}

void klvanc_destroy_EIA_608(struct klvanc_packet_eia_608_s *pkt)
{
	free(pkt);
}

int klvanc_convert_EIA_608_to_packetBytes(struct klvanc_packet_eia_608_s *pkt, uint8_t **bytes, uint16_t *byteCount)
{
	if (!pkt || !bytes) {
		return -1;
	}

	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -ENOMEM;

	*bytes = malloc(255);
	if (*bytes == NULL) {
		klbs_free(bs);
		return -ENOMEM;
	}

	/* Serialize the struct into a binary blob */
	klbs_write_set_buffer(bs, *bytes, 255);

	if (pkt->field == 0)
		klbs_write_bits(bs, 1, 1);
	else
		klbs_write_bits(bs, 0, 1);

	/* Reserved */
	klbs_write_bits(bs, 0, 2);
	klbs_write_bits(bs, pkt->line_offset, 5);

	/* CC Payload */
	klbs_write_bits(bs, pkt->cc_data_1, 8);
	klbs_write_bits(bs, pkt->cc_data_2, 8);

	klbs_write_buffer_complete(bs);

	*byteCount = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

int klvanc_convert_EIA_608_to_words(struct klvanc_packet_eia_608_s *pkt, uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	int ret;

	ret = klvanc_convert_EIA_608_to_packetBytes(pkt, &buf, &byteCount);
	if (ret != 0)
		return ret;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x02, 0x61, buf, byteCount, words, wordCount, 10);

	free(buf);

	return 0;
}
