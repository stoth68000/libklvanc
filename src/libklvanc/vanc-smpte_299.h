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

#ifndef _VANC_SMPTE_299_H
#define _VANC_SMPTE_299_H

#include <libklvanc/vanc-packets.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct klvanc_smpte_299_delay_s {
	uint8_t valid;
	int32_t samples;
};

struct klvanc_packet_smpte_299_control_s {
	struct klvanc_packet_header_s hdr;
	uint8_t standard_part;
	uint8_t group;
	uint8_t first_channel;
	uint8_t last_channel;
	uint16_t audio_frame_number;
	uint8_t rate_code;
	uint32_t sample_rate_hz;
	uint8_t asynchronous;
	uint8_t active_channel_mask;
	struct klvanc_smpte_299_delay_s delay[2];
	uint16_t reserved[2];
	uint16_t payload_length;
	uint8_t payload[255];
};

int klvanc_dump_SMPTE_299_CONTROL(struct klvanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif

#endif
