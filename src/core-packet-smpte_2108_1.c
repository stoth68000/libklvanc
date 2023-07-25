/*
 * Copyright (c) 2023 Kernel Labs Inc. All Rights Reserved
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


static const char *lookupFrameType(unsigned char frame_type)
{
	switch (frame_type) {
	case 0x00: return "Static Metadata Type 1 (Mastering display color volume)";
	case 0x01: return "Static Metadata Type 2 (Content light level information)";
	case 0x02: return "Dynamic Metadata Type 1 (ATSC A/341 ST 2094-10_data)";
	case 0x06: return "Dynamic Metadata Type 5 (SL-HDR information)";
	default:   return "Reserved";
	}
}

int klvanc_dump_SMPTE_2108_1(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_smpte_2108_1_s *pkt = p;
	char *colorchan = "GBR";

	if (ctx->verbose)
		PRINT_DEBUG("%s() %p\n", __func__, (void *)pkt);

	for (int n = 0; n < pkt->num_frames; n++) {
		struct klvanc_s2108_1_frame *frame = &pkt->frames[n];
		PRINT_DEBUG(" frame_type = 0x%02x - %s\n", frame->frame_type, lookupFrameType(frame->frame_type));
		PRINT_DEBUG(" frame_length = 0x%02x\n", frame->frame_length);

		if (frame->frame_type == KLVANC_HDR_STATIC1) {
			for (int i = 0; i < 3; i++) {
				uint16_t val =  frame->static1.display_primaries_x[i];
				PRINT_DEBUG("   display_primaries_x[%d] = %d (%c: %f)\n", i, val, colorchan[i], (float)val * 0.00002);
				val = frame->static1.display_primaries_y[i];
				PRINT_DEBUG("   display_primaries_y[%d] = %d (%c: %f)\n", i, val, colorchan[i], (float)val * 0.00002);
			}

			PRINT_DEBUG("   white_point_x = %d (%f)\n", frame->static1.white_point_x,
				    (float)frame->static1.white_point_x * 0.00002);
			PRINT_DEBUG("   white_point_y = %d (%f)\n", frame->static1.white_point_y,
				    (float)frame->static1.white_point_y * 0.00002);
			PRINT_DEBUG("   max_display_mastering_luminance = %d (%f)\n", frame->static1.max_display_mastering_luminance,
				    (float)frame->static1.max_display_mastering_luminance * 0.0001);
			PRINT_DEBUG("   min_display_mastering_luminance = %d (%f)\n", frame->static1.min_display_mastering_luminance,
				    (float)frame->static1.min_display_mastering_luminance * 0.0001);
		} else if (frame->frame_type == KLVANC_HDR_STATIC2) {
			PRINT_DEBUG("   max_content_light_level = %d\n", frame->static2.max_content_light_level);
			PRINT_DEBUG("   max_pic_average_light_level = %d\n", frame->static2.max_pic_average_light_level);
		} else {
			PRINT_DEBUG("   Decoding of frame type 0x%02x not supported\n", frame->frame_type);
		}
	}

	return KLAPI_OK;
}

int parse_SMPTE_2108_1(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void **pp)
{
	struct klbs_context_s *bs;

	if (ctx->callbacks == NULL || ctx->callbacks->smpte_2108_1 == NULL)
		return KLAPI_OK;

	bs = klbs_alloc();
	if (bs == NULL)
		return -ENOMEM;

	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_smpte_2108_1_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt) {
		klbs_free(bs);
		return -ENOMEM;
        }

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	/* Extract the 8-bit bitstream from the 10-bit payload */
	for (int i = 0; i < hdr->payloadLengthWords; i++) {
		pkt->payload[i] = hdr->payload[i];
	}
	pkt->payloadLengthBytes = hdr->payloadLengthWords;

	/* Parse the bitstream */
	klbs_read_set_buffer(bs, pkt->payload, pkt->payloadLengthBytes);

	while (klbs_get_byte_count_free(bs) > 0) {
		struct klvanc_s2108_1_frame *frame = &pkt->frames[pkt->num_frames];
		if (klbs_get_byte_count_free(bs) < 2) {
			/* Not enough to read type and length?  malformed */
			break;
		}

		frame->frame_type = klbs_read_bits(bs, 8);
		frame->frame_length = klbs_read_bits(bs, 8);

		if (klbs_get_byte_count_free(bs) < frame->frame_length) {
			/* Not enough to read payload?  malformed */
			break;
		}

		/* Skip SEI payload type and SEI payload length */
		klbs_read_bits(bs, 16);

		if (frame->frame_type == KLVANC_HDR_STATIC1) {
			for (int i = 0; i < 3; i++) {
				frame->static1.display_primaries_x[i] = klbs_read_bits(bs, 16);
				frame->static1.display_primaries_y[i] = klbs_read_bits(bs, 16);
			}
			frame->static1.white_point_x = klbs_read_bits(bs, 16);
			frame->static1.white_point_y = klbs_read_bits(bs, 16);
			frame->static1.max_display_mastering_luminance = klbs_read_bits(bs, 32);
			frame->static1.min_display_mastering_luminance = klbs_read_bits(bs, 32);
		} else if (frame->frame_type == KLVANC_HDR_STATIC2) {
			frame->static2.max_content_light_level = klbs_read_bits(bs, 16);
			frame->static2.max_pic_average_light_level = klbs_read_bits(bs, 16);
		} else {
			klbs_read_bits(bs, 8 * (frame->frame_length - 2));
		}
		pkt->num_frames++;
	}

	ctx->callbacks->smpte_2108_1(ctx->callback_context, ctx, pkt);

	klbs_free(bs);

	*pp = pkt;
	return KLAPI_OK;
}
