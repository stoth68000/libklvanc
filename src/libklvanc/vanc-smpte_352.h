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

#ifndef _VANC_SMPTE_352_H
#define _VANC_SMPTE_352_H

#include <libklvanc/vanc-packets.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum klvanc_smpte_352_aspect_ratio_e {
	KLVANC_S352_ASPECT_UNKNOWN = 0,
	KLVANC_S352_ASPECT_4_3,
	KLVANC_S352_ASPECT_16_9,
};

enum klvanc_smpte_352_transfer_e {
	KLVANC_S352_TRANSFER_SDR = 0,
	KLVANC_S352_TRANSFER_HLG,
	KLVANC_S352_TRANSFER_PQ,
	KLVANC_S352_TRANSFER_UNSPECIFIED,
};

struct klvanc_packet_smpte_352_s {
	struct klvanc_packet_header_s hdr;
	uint8_t bytes[4];
	uint8_t version;
	uint8_t payload_id;
	uint8_t progressive_transport;
	uint8_t progressive_picture;
	uint8_t picture_rate_code;
	uint32_t picture_rate_numerator;
	uint32_t picture_rate_denominator;
	uint8_t sampling_structure;
	uint8_t bit_depth;
	uint8_t channel_assignment;
	uint16_t horizontal_pixels;
	enum klvanc_smpte_352_aspect_ratio_e aspect_ratio;
	enum klvanc_smpte_352_transfer_e transfer_characteristic;
	uint8_t colorimetry;
	uint8_t ictcp;
};

const char *klvanc_smpte_352_payload_id_to_string(uint8_t payload_id);
const char *klvanc_smpte_352_picture_rate_to_string(uint8_t code);
const char *klvanc_smpte_352_sampling_to_string(uint8_t code);
const char *klvanc_smpte_352_aspect_ratio_to_string(
		enum klvanc_smpte_352_aspect_ratio_e aspect_ratio);
const char *klvanc_smpte_352_transfer_to_string(
		enum klvanc_smpte_352_transfer_e transfer);

int klvanc_dump_SMPTE_352(struct klvanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif

#endif
