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
	case AFD_UNDEFINED:
		return "AFD_UNDEFINED";
	case AFD_BOX_16x9_TOP:
		return "AFD_BOX_16x9_TOP";
	case AFD_BOX_14x9_TOP:
		return "AFD_BOX_14x9_TOP";
	case AFD_BOX_16x9_CENTER:
		return "AFD_16x9_CENTER";
	case AFD_FULL_FRAME:
		return "AFD_FULL_FRAME";
	case AFD_FULL_FRAME_ALT:
		return "AFD_FULL_FRAME_ALT";
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
		return "AFD_UNKNOWN";
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

const char *klvanc_barFlags_to_string(enum klvanc_payload_afd_barflags flags)
{
	switch(flags) {
	case BARS_NONE:
		return "NONE";
	case BARS_TOPBOTTOM:
		return "Top/Bottom";
	case BARS_LEFTRIGHT:
		return "Left/Right";
	default:
		return "INVALID";
	}
}

int klvanc_dump_AFD(struct klvanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_afd_s *pkt = p;

	PRINT_DEBUG("%s() AFD: %s (%d) Aspect Ratio: %s Bar Flags: %s (0x%x)\n", __func__,
		    klvanc_afd_to_string(pkt->afd),
		    pkt->afd,
		    klvanc_aspectRatio_to_string(pkt->aspectRatio),
		    klvanc_barFlags_to_string(pkt->barDataFlags),
		    pkt->barDataFlags);

	/* Sec 6.1 - For the two pairs of top/bottom and left/right, either both have
	   -   to be set or neither */
	if ((pkt->barDataFlags & 0x0c) == 0x08 || (pkt->barDataFlags & 0x0c) == 0x04)
		PRINT_DEBUG(" INVALID top/bottom pairing");

	if ((pkt->barDataFlags & 0x03) == 0x02 || (pkt->barDataFlags & 0x03) == 0x01)
		PRINT_DEBUG(" INVALID left right pairing");

	/* Make sure there isn't some illegal combination of horizontal/vertical
	   bits enabled (either top/bottom have to be enabled or left/right, but
	   it can't be both) */
	if ((pkt->barDataFlags & 0x0c) && (pkt->barDataFlags & 0x03))
		PRINT_DEBUG(" INVALID both horizontal/vertical bar flags set");

	/* Depending on which flags are set dictate which of the two values
	   contains a given value */
	if (pkt->barDataFlags == BARS_TOPBOTTOM) {
		PRINT_DEBUG(" Top bar = %d\n", pkt->top);
		PRINT_DEBUG(" Bottom bar = %d\n", pkt->bottom);
	}
	if (pkt->barDataFlags == BARS_LEFTRIGHT) {
		PRINT_DEBUG(" Left bar = %d\n", pkt->left);
		PRINT_DEBUG(" Right bar = %d\n", pkt->right);
	}

	return KLAPI_OK;
}

int parse_AFD(struct klvanc_context_s *ctx,
			      struct klvanc_packet_header_s *hdr, void **pp)
{
	if (ctx->callbacks == NULL || ctx->callbacks->afd == NULL)
		return KLAPI_OK;

	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_afd_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	unsigned char afd = (sanitizeWord(hdr->payload[0]) >> 3) & 0x0f;

	pkt->afd = afd;

	if (sanitizeWord(hdr->payload[0]) & 0x04)
		pkt->aspectRatio = ASPECT_16x9;
	else
		pkt->aspectRatio = ASPECT_4x3;

	pkt->barDataFlags = sanitizeWord(hdr->payload[3]) >> 4;
	if (pkt->barDataFlags == BARS_TOPBOTTOM) {
		pkt->top = sanitizeWord(hdr->payload[4]) << 8;
		pkt->top |= sanitizeWord(hdr->payload[5]);
		pkt->bottom = sanitizeWord(hdr->payload[6]) << 8;
		pkt->bottom |= sanitizeWord(hdr->payload[7]);
	} else if (pkt->barDataFlags == BARS_LEFTRIGHT) {
		pkt->left = sanitizeWord(hdr->payload[4]) << 8;
		pkt->left |= sanitizeWord(hdr->payload[5]);
		pkt->right = sanitizeWord(hdr->payload[6]) << 8;
		pkt->right |= sanitizeWord(hdr->payload[7]);
	}

	ctx->callbacks->afd(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_create_AFD(struct klvanc_packet_afd_s **pkt)
{
	struct klvanc_packet_afd_s *p = calloc(1, sizeof(*p));
	if (p == NULL)
		return -ENOMEM;

	*pkt = p;
	return 0;
}

void klvanc_destroy_AFD(struct klvanc_packet_afd_s *pkt)
{
	free(pkt);
}

static int klvanc_verify_afd_value(enum klvanc_payload_afd_e afd)
{
	switch(afd) {
	case AFD_UNDEFINED:
	case AFD_BOX_16x9_TOP:
	case AFD_BOX_14x9_TOP:
	case AFD_BOX_16x9_CENTER:
	case AFD_FULL_FRAME:
	case AFD_FULL_FRAME_ALT:
	case AFD_16x9_CENTER:
	case AFD_14x9_CENTER:
	case AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER:
	case AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER:
	case AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER:
		return 1;
	default:
		return 0;
	}
}

int klvanc_set_AFD_val(struct klvanc_packet_afd_s *pkt, unsigned char val)
{
	if (!klvanc_verify_afd_value(val))
		return -1;

	pkt->afd = val;
	return 0;
}

int klvanc_convert_AFD_to_packetBytes(struct klvanc_packet_afd_s *pkt, uint8_t **bytes, uint16_t *byteCount)
{
	unsigned char afd;

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

	/* Serialize the AFD struct into a binary blob */
	klbs_write_set_buffer(bs, *bytes, 255);

	afd = pkt->afd << 3;
	if (pkt->aspectRatio == ASPECT_16x9)
		afd |= 0x04;

	klbs_write_bits(bs, afd, 8);
	klbs_write_bits(bs, 0x00, 8); /* Reserved */
	klbs_write_bits(bs, 0x00, 8); /* Reserved */
	klbs_write_bits(bs, pkt->barDataFlags << 4, 8);
	if (pkt->barDataFlags == BARS_TOPBOTTOM) {
		klbs_write_bits(bs, pkt->top, 16);
		klbs_write_bits(bs, pkt->bottom, 16);
	} else if (pkt->barDataFlags == BARS_LEFTRIGHT) {
		klbs_write_bits(bs, pkt->left, 16);
		klbs_write_bits(bs, pkt->right, 16);
	} else {
		klbs_write_bits(bs, 0x00, 32);
	}

	klbs_write_buffer_complete(bs);

	*byteCount = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

int klvanc_convert_AFD_to_words(struct klvanc_packet_afd_s *pkt, uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	int ret;

	ret = klvanc_convert_AFD_to_packetBytes(pkt, &buf, &byteCount);
	if (ret != 0)
		return ret;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x05, 0x41, buf, byteCount, words, wordCount, 10);

	free(buf);

	return 0;
}
