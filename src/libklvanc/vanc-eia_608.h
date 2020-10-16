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

/**
 * @file	vanc-eia_608.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	EIA/CEA-608 Closed Captions
 */

#ifndef _VANC_EIA_608_H
#define _VANC_EIA_608_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_eia_608_s
{
	struct klvanc_packet_header_s hdr;
	int nr;
	unsigned char payload[3];

	/* Parsed */
	int field;
	int line_offset;
	unsigned char cc_data_1;
	unsigned char cc_data_2;
};

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_EIA_608(struct klvanc_context_s *ctx, void *p);

/**
 * @brief	Create an EIA-608 VANC packet
 * @param[out]	struct klvanc_packet_eia_608_s **pkt - Pointer to newly created packet
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_EIA_608(struct klvanc_packet_eia_608_s **pkt);

/**
 * @brief	Destroy an EIA-608 VANC packet
 * @param[in]	struct klvanc_packet_eia_608_s *pkt - Packet to be destroyed
 */
void klvanc_destroy_EIA_608(struct klvanc_packet_eia_608_s *pkt);

/**
 * @brief	Convert type struct klvanc_packet_eia_608_s into a block of bytes which represents\n
 *              an EIA-608 packet (without DID/SDID/DC/checksum)
 *              On success, caller MUST free the resulting *bytes array.
 * @param[in]	struct klvanc_packet_eia_608_s *pkt - An EIA-608 VANC entry, received from the EIA-608 parser
 * @param[out]	uint8_t **bytes - An array of bytes representing the serialized EIA-608 packet
 * @param[out]	uint16_t *byteCount - Number of bytes in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_EIA_608_to_packetBytes(struct klvanc_packet_eia_608_s *pkt, uint8_t **bytes, uint16_t *byteCount);

/**
 * @brief	Convert type struct klvanc_packet_eia_608_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct klvanc_packet_eia_608_s *pkt - A EIA-608 VANC entry
 * @param[out]	uint16_t **words - An array of words representing a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_EIA_608_to_words(struct klvanc_packet_eia_608_s *pkt, uint16_t **words, uint16_t *wordCount);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_EIA_608_H */
