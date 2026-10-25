/*
 * Copyright (c) 2026 Kernel Labs Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

static unsigned int decode_dbb(const struct klvanc_packet_header_s *hdr,
			       unsigned int offset)
{
	unsigned int value = 0;
	for (unsigned int bit = 0; bit < 8; bit++)
		value |= ((hdr->payload[offset + bit] >> 3) & 1) << bit;
	return value;
}

int parse_SMPTE_12_3(struct klvanc_context_s *ctx,
		     struct klvanc_packet_header_s *hdr, void **pp)
{
	struct klvanc_packet_smpte_12_3_s *pkt;
	unsigned int rate_id;

	if (ctx->callbacks == NULL || ctx->callbacks->smpte_12_3 == NULL)
		return KLAPI_OK;
	if (hdr->payloadLengthWords != 0x10)
		return -EINVAL;

	pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;
	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	pkt->dbb1 = decode_dbb(hdr, 0);
	pkt->dbb2 = decode_dbb(hdr, 8);
	if ((pkt->dbb1 & 0xf0) != 0x80 || (pkt->dbb2 & 0x80)) {
		free(pkt);
		return -EINVAL;
	}

	pkt->bitstream_number = pkt->dbb1 & 0x0f;
	rate_id = (pkt->dbb2 >> 5) & 0x03;
	if (rate_id == 3) {
		free(pkt);
		return -EINVAL;
	}
	pkt->super_frame_rate = rate_id == 0 ? 24 : rate_id == 1 ? 25 : 30;
	pkt->frames_per_super_frame = pkt->dbb2 & 0x1f;
	if (!pkt->frames_per_super_frame) {
		free(pkt);
		return -EINVAL;
	}

	pkt->super_frames = (hdr->payload[0] >> 4) & 0x0f;
	pkt->super_frames += ((hdr->payload[2] >> 4) & 0x03) * 10;
	pkt->drop_frame = (hdr->payload[2] >> 6) & 1;
	pkt->sub_frame_2 = (hdr->payload[2] >> 7) & 1;
	pkt->seconds = (hdr->payload[4] >> 4) & 0x0f;
	pkt->seconds += ((hdr->payload[6] >> 4) & 0x07) * 10;
	pkt->sub_frame_1 = (hdr->payload[6] >> 7) & 1;
	pkt->minutes = (hdr->payload[8] >> 4) & 0x0f;
	pkt->minutes += ((hdr->payload[10] >> 4) & 0x07) * 10;
	pkt->sub_frame_3 = (hdr->payload[10] >> 7) & 1;
	pkt->hours = (hdr->payload[12] >> 4) & 0x0f;
	pkt->hours += ((hdr->payload[14] >> 4) & 0x03) * 10;
	pkt->sub_frame_4 = (hdr->payload[14] >> 6) & 1;
	pkt->sub_frame_5 = (hdr->payload[14] >> 7) & 1;

	/* ST 12-3 moves sub-frame_1 to bit 59 for the 100-frame case. */
	if (pkt->super_frame_rate == 25 && pkt->frames_per_super_frame == 4) {
		pkt->sub_frame_1 = pkt->sub_frame_5;
		pkt->sub_frame_5 = 0;
	}
	pkt->frame_identifier = pkt->sub_frame_1 * 2 + pkt->sub_frame_2;
	if (pkt->frames_per_super_frame == 5)
		pkt->frame_identifier = pkt->sub_frame_1 * 4 +
			pkt->sub_frame_2 * 2 + pkt->sub_frame_3;
	if (pkt->frame_identifier >= pkt->frames_per_super_frame ||
	    pkt->super_frames >= pkt->super_frame_rate ||
	    pkt->seconds >= 60 || pkt->minutes >= 60 || pkt->hours >= 24) {
		free(pkt);
		return -EINVAL;
	}
	pkt->frames = pkt->super_frames * pkt->frames_per_super_frame +
		pkt->frame_identifier;

	ctx->callbacks->smpte_12_3(ctx->callback_context, ctx, pkt);
	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_dump_SMPTE_12_3(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_smpte_12_3_s *pkt = p;
	(void)ctx;
	PRINT_DEBUG(" DBB1 = %02x (bitstream %u)\n", pkt->dbb1,
		    pkt->bitstream_number);
	PRINT_DEBUG(" DBB2 = %02x (super-frame rate %u, N %u)\n", pkt->dbb2,
		    pkt->super_frame_rate, pkt->frames_per_super_frame);
	PRINT_DEBUG(" Timecode = %02u:%02u:%02u%c%03u\n", pkt->hours,
		    pkt->minutes, pkt->seconds, pkt->drop_frame ? ';' : ':',
		    pkt->frames);
	return 0;
}
