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

/**
 * @file	vanc-smpte_2108_1.h
 * @author	Devin Heitmueller <dheitmueller@kernellabs.com>
 * @copyright	Copyright (c) 2023 Kernel Labs Inc. All Rights Reserved.
 * @brief	SMPTE ST 2108-1 HDR/WCG over VANC
 */

#ifndef _VANC_SMPTE_2108_1_H
#define _VANC_SMPTE_2108_1_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_S2108_1_FRAMES 10

/* HDR Frame types (See Sec 5.3.1 Table 3) */
#define KLVANC_HDR_STATIC1 0x00 /* H.265 */
#define KLVANC_HDR_STATIC2 0x01 /* H.265 */
#define KLVANC_HDR_DYNAMIC2 0x02 /* ATSC A/341 */
/* Codes 3-5 are reserved */
#define KLVANC_HDR_DYNAMIC5 0x06 /* ETSI TS 103 433-1 SL-HDR */

struct klvanc_packet_smpte_2108_1_static1 {
	uint16_t display_primaries_x[3];
	uint16_t display_primaries_y[3];
	uint16_t white_point_x;
	uint16_t white_point_y;
	uint32_t max_display_mastering_luminance;
	uint32_t min_display_mastering_luminance;
};

struct klvanc_packet_smpte_2108_1_static2 {
	uint16_t max_content_light_level;
	uint16_t max_pic_average_light_level;
};

struct klvanc_s2108_1_frame {
	uint8_t frame_type;
	uint8_t frame_length;
	union {
		struct klvanc_packet_smpte_2108_1_static1 static1;
		struct klvanc_packet_smpte_2108_1_static2 static2;
	};
};

/**
 * @brief Describes an ST2108 packet
 */
struct klvanc_packet_smpte_2108_1_s
{
	struct klvanc_packet_header_s hdr;

	uint8_t payload[256];
	unsigned int payloadLengthBytes;

	uint8_t num_frames;
	struct klvanc_s2108_1_frame frames[MAX_S2108_1_FRAMES];
};

/**
 * @brief       Create SMPTE ST 2108_1 HDR metadata
 * @param[in]	struct klvanc_packet_smpte_2108_1_s **pkt - Pointer to the newly created structure
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_alloc_SMPTE_2108_1(struct klvanc_packet_smpte_2108_1_s **pkt);

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_SMPTE_2108_1(struct klvanc_context_s *ctx, void *p);

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	void *p - Pointer to struct (klvanc_packet_smpte_2108_1_s *)
 */
void klvanc_free_SMPTE_2108_1(void *p);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_SMPTE_2108_1_H */
