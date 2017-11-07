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

const char *klvanc_afd_to_string(enum klvanc_payload_afd_e afd)
{
	switch(afd) {
	case AFD_BOX_16x9_TOP:
		return "AFD_BOX_16x9_TOP";
	case AFD_BOX_14x9_TOP:
		return "AFD_BOX_14x9_TOP";
	case AFD_BOX_16x9_CENTER:
		return "AFD_16x9_CENTER";
	case AFD_FULL_FRAME:
		return "AFD_FULL_FRAME";
	case AFD_16x9_CENTER:
		return "AFD_16x9_CENTER";
	case AFD_14x9_CENTER:
		return "AFD_14x9_CENTER";
	case AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER:
		return "AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER";
	case AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER:
		return "AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER";
	case AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER:
		return "AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER";
	default:
        	return "AFD_UNDEFINED";
	}
}

const char *klvanc_aspectRatio_to_string(enum klvanc_payload_aspect_ratio_e ar)
{
	switch(ar) {
	case ASPECT_4x3:
		return "ASPECT_4x3";
	case ASPECT_16x9:
		return "ASPECT_16x9";
	default:
        	return "ASPECT_UNDEFINED";
	}
}

int klvanc_dump_PAYLOAD_INFORMATION(struct klvanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct klvanc_packet_payload_information_s *pkt = p;

	printf("%s() AFD: %s Aspect Ratio: %s Flags: 0x%x Value1: 0x%x Value2: 0x%x\n", __func__,
		klvanc_afd_to_string(pkt->afd),
		klvanc_aspectRatio_to_string(pkt->aspectRatio),
		pkt->barDataFlags,
		pkt->barDataValue[0],
		pkt->barDataValue[1]
		);

	return KLAPI_OK;
}

int parse_PAYLOAD_INFORMATION(struct klvanc_context_s *ctx,
			      struct klvanc_packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct klvanc_packet_payload_information_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	unsigned char afd = (sanitizeWord(hdr->payload[0]) >> 3) & 0x0f;

	switch(afd) {
	case 0x02:
		pkt->afd = AFD_BOX_16x9_TOP;
		break;
	case 0x03:
		pkt->afd = AFD_BOX_14x9_TOP;
		break;
	case 0x04:
        	pkt->afd = AFD_BOX_16x9_CENTER;
		break;
	case 0x08:
        	pkt->afd = AFD_FULL_FRAME;
		break;
	case 0x0a:
        	pkt->afd = AFD_16x9_CENTER;
		break;
	case 0x0b:
        	pkt->afd = AFD_14x9_CENTER;
		break;
	case 0x0d:
        	pkt->afd = AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER;
		break;
	case 0x0e:
        	pkt->afd = AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER;
		break;
	case 0x0f:
        	pkt->afd = AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER;
		break;
	default:
        	pkt->afd = AFD_UNDEFINED;
	}

	if (sanitizeWord(hdr->payload[0]) & 0x04)
		pkt->aspectRatio = ASPECT_16x9;
	else
		pkt->aspectRatio = ASPECT_4x3;

	pkt->barDataFlags = sanitizeWord(hdr->payload[3]) >> 4;
	pkt->barDataValue[0]  = sanitizeWord(hdr->payload[4]) << 8;
	pkt->barDataValue[0] |= sanitizeWord(hdr->payload[5]);
	pkt->barDataValue[1]  = sanitizeWord(hdr->payload[6]) << 8;
	pkt->barDataValue[1] |= sanitizeWord(hdr->payload[7]);

	if (ctx->callbacks && ctx->callbacks->payload_information)
		ctx->callbacks->payload_information(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

