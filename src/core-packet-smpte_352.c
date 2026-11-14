/*
 * Copyright (c) 2026 Kernel Labs Inc. All Rights Reserved
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

const char *klvanc_smpte_352_payload_id_to_string(uint8_t id)
{
	switch (id) {
	case 0x81: return "ST 259 SD video payload";
	case 0x84: return "ST 292-1 720-line video payload";
	case 0x85: return "ST 292-1 1080-line video payload";
	case 0x87: return "ST 372 dual-link 1080-line video payload";
	case 0x89: return "ST 425-1 1080-line Level A video payload";
	case 0x8a: return "ST 425-1 dual-link 1080-line Level B video payload";
	case 0x8c: return "ST 425-1 two-channel 1080-line Level B video payload";
	default: return "Unrecognized video payload";
	}
}

const char *klvanc_smpte_352_picture_rate_to_string(uint8_t code)
{
	static const char *const names[16] = {
		"Undefined", "Reserved", "23.98", "24", "47.95", "25",
		"29.97", "30", "48", "50", "59.94", "60",
		"Reserved", "Reserved", "Reserved", "Reserved"
	};
	return names[code & 0x0f];
}

const char *klvanc_smpte_352_sampling_to_string(uint8_t code)
{
	static const char *const names[16] = {
		"4:2:2 YCbCr", "4:4:4 YCbCr", "4:4:4 GBR", "4:2:0",
		"4:2:2:4 YCbCrA", "4:4:4:4 YCbCrA", "4:4:4:4 GBRA",
		"Application specific", "4:2:2:4 YCbCrD", "4:4:4:4 YCbCrD",
		"4:4:4:4 GBRD", "Reserved", "Reserved", "Reserved",
		"4:4:4 XYZ", "Reserved"
	};
	return names[code & 0x0f];
}

const char *klvanc_smpte_352_aspect_ratio_to_string(
		enum klvanc_smpte_352_aspect_ratio_e aspect)
{
	switch (aspect) {
	case KLVANC_S352_ASPECT_4_3: return "4:3";
	case KLVANC_S352_ASPECT_16_9: return "16:9";
	default: return "Unknown";
	}
}

const char *klvanc_smpte_352_transfer_to_string(
		enum klvanc_smpte_352_transfer_e transfer)
{
	switch (transfer) {
	case KLVANC_S352_TRANSFER_SDR: return "SDR-TV";
	case KLVANC_S352_TRANSFER_HLG: return "HLG";
	case KLVANC_S352_TRANSFER_PQ: return "PQ";
	default: return "Unspecified";
	}
}

static void decode_picture_rate(struct klvanc_packet_smpte_352_s *pkt)
{
	switch (pkt->picture_rate_code) {
	case 0x2: pkt->picture_rate_numerator = 24000; pkt->picture_rate_denominator = 1001; break;
	case 0x3: pkt->picture_rate_numerator = 24; pkt->picture_rate_denominator = 1; break;
	case 0x4: pkt->picture_rate_numerator = 48000; pkt->picture_rate_denominator = 1001; break;
	case 0x5: pkt->picture_rate_numerator = 25; pkt->picture_rate_denominator = 1; break;
	case 0x6: pkt->picture_rate_numerator = 30000; pkt->picture_rate_denominator = 1001; break;
	case 0x7: pkt->picture_rate_numerator = 30; pkt->picture_rate_denominator = 1; break;
	case 0x8: pkt->picture_rate_numerator = 48; pkt->picture_rate_denominator = 1; break;
	case 0x9: pkt->picture_rate_numerator = 50; pkt->picture_rate_denominator = 1; break;
	case 0xa: pkt->picture_rate_numerator = 60000; pkt->picture_rate_denominator = 1001; break;
	case 0xb: pkt->picture_rate_numerator = 60; pkt->picture_rate_denominator = 1; break;
	default: break;
	}
}

int parse_SMPTE_352(struct klvanc_context_s *ctx,
		struct klvanc_packet_header_s *hdr, void **pp)
{
	if (hdr->payloadLengthWords != 4)
		return -EINVAL;

	struct klvanc_packet_smpte_352_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	for (unsigned int i = 0; i < 4; i++)
		pkt->bytes[i] = sanitizeWord(hdr->payload[i]);

	pkt->version = pkt->bytes[0] >> 7;
	pkt->payload_id = pkt->bytes[0];
	pkt->progressive_transport = pkt->bytes[1] >> 7;
	pkt->progressive_picture = (pkt->bytes[1] >> 6) & 1;
	pkt->picture_rate_code = pkt->bytes[1] & 0x0f;
	decode_picture_rate(pkt);
	pkt->sampling_structure = pkt->bytes[2] & 0x0f;
	pkt->channel_assignment = pkt->bytes[3] >> 6;
	pkt->bit_depth = (pkt->bytes[3] & 0x03) == 1 ? 10 :
		(pkt->bytes[3] & 0x03) == 2 ? 12 :
		(pkt->bytes[3] & 0x03) == 3 ? 10 : 8;

	if (pkt->payload_id == 0x85) {
		pkt->horizontal_pixels = (pkt->bytes[2] & 0x40) ? 2048 : 1920;
		pkt->aspect_ratio = (pkt->bytes[2] & 0x20) ?
			KLVANC_S352_ASPECT_16_9 : KLVANC_S352_ASPECT_UNKNOWN;
		pkt->transfer_characteristic =
			(enum klvanc_smpte_352_transfer_e)((pkt->bytes[1] >> 4) & 0x03);
		pkt->colorimetry = ((pkt->bytes[2] >> 6) & 0x02) |
			((pkt->bytes[2] >> 4) & 0x01);
		pkt->ictcp = (pkt->bytes[3] >> 4) & 1;
	} else {
		pkt->aspect_ratio = (pkt->bytes[2] & 0x80) ?
			KLVANC_S352_ASPECT_16_9 : KLVANC_S352_ASPECT_4_3;
		pkt->transfer_characteristic = KLVANC_S352_TRANSFER_UNSPECIFIED;
	}

	if (ctx->callbacks && ctx->callbacks->smpte_352)
		ctx->callbacks->smpte_352(ctx->callback_context, ctx, pkt);
	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_dump_SMPTE_352(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_smpte_352_s *pkt = p;
	PRINT_DEBUG("SMPTE ST 352 payload: %02x %02x %02x %02x (%s)\n",
		pkt->bytes[0], pkt->bytes[1], pkt->bytes[2], pkt->bytes[3],
		klvanc_smpte_352_payload_id_to_string(pkt->payload_id));
	PRINT_DEBUG("  %s transport, %s picture, %s fps, %s, %u-bit, %s\n",
		pkt->progressive_transport ? "progressive" : "interlaced",
		pkt->progressive_picture ? "progressive" : "interlaced",
		klvanc_smpte_352_picture_rate_to_string(pkt->picture_rate_code),
		klvanc_smpte_352_sampling_to_string(pkt->sampling_structure),
		pkt->bit_depth,
		klvanc_smpte_352_aspect_ratio_to_string(pkt->aspect_ratio));
	return KLAPI_OK;
}
