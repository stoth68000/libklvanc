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
#include <libklvanc/vanc-smpte_299.h>

#include "core-private.h"

#include <stdlib.h>
#include <string.h>

static uint32_t sample_rate_hz(uint8_t code)
{
	switch (code) {
	case 0: return 48000;
	case 1: return 44100;
	case 2: return 32000;
	case 4: return 96000;
	default: return 0;
	}
}

static struct klvanc_smpte_299_delay_s decode_delay(
		const unsigned short *words)
{
	uint32_t raw = ((uint32_t)(words[0] >> 1) & 0xffu) |
		(((uint32_t)words[1] & 0x1ffu) << 8) |
		(((uint32_t)words[2] & 0x1ffu) << 17);
	struct klvanc_smpte_299_delay_s delay = {
		.valid = words[0] & 1u,
		.samples = (raw & (1u << 25))
			? (int32_t)(raw | 0xfc000000u) : (int32_t)raw,
	};
	return delay;
}

int parse_SMPTE_299_CONTROL(struct klvanc_context_s *ctx,
		struct klvanc_packet_header_s *hdr, void **pp)
{
	if (!((hdr->did >= 0xe0 && hdr->did <= 0xe3) ||
	      (hdr->did >= 0xa0 && hdr->did <= 0xa3)) ||
	    hdr->payloadLengthWords != 11)
		return -EINVAL;

	struct klvanc_packet_smpte_299_control_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	pkt->standard_part = hdr->did >= 0xe0 ? 1 : 2;
	pkt->group = hdr->did >= 0xe0 ? 0xe4 - hdr->did : 0xa8 - hdr->did;
	pkt->first_channel = (pkt->group - 1) * 4 + 1;
	pkt->last_channel = pkt->first_channel + 3;
	pkt->audio_frame_number = hdr->payload[0] & 0x1ffu;
	pkt->rate_code = (hdr->payload[1] >> 1) & 7u;
	pkt->sample_rate_hz = sample_rate_hz(pkt->rate_code);
	pkt->asynchronous = hdr->payload[1] & 1u;
	pkt->active_channel_mask = hdr->payload[2] & 0x0fu;
	pkt->delay[0] = decode_delay(&hdr->payload[3]);
	pkt->delay[1] = decode_delay(&hdr->payload[6]);
	pkt->reserved[0] = hdr->payload[9] & 0x1ffu;
	pkt->reserved[1] = hdr->payload[10] & 0x1ffu;
	pkt->payload_length = hdr->payloadLengthWords;
	for (unsigned int i = 0; i < hdr->payloadLengthWords; i++)
		pkt->payload[i] = sanitizeWord(hdr->payload[i]);

	if (ctx->callbacks && ctx->callbacks->smpte_299_control)
		ctx->callbacks->smpte_299_control(ctx->callback_context, ctx, pkt);
	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_dump_SMPTE_299_CONTROL(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_smpte_299_control_s *pkt = p;
	PRINT_DEBUG("SMPTE ST 299-%u audio control group %u, %u Hz, active mask 0x%x\n",
		pkt->standard_part, pkt->group, pkt->sample_rate_hz,
		pkt->active_channel_mask);
	return 0;
}
