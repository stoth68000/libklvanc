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
 * @file	vanc-afd.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	SMPTE 2016-3 Vertical Ancillary Data Mapping of Active Format Description and Bar Data
 */

#ifndef _VANC_AFD_H
#define _VANC_AFD_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
enum klvanc_payload_aspect_ratio_e
{
	ASPECT_UNDEFINED = 0,
	ASPECT_4x3,
	ASPECT_16x9,
};

/**
 * @brief	TODO - Brief description goes here.
 */
enum klvanc_payload_afd_e
{
	AFD_UNDEFINED = 0x00,
	AFD_BOX_16x9_TOP = 0x02,
	AFD_BOX_14x9_TOP = 0x03,
	AFD_BOX_16x9_CENTER = 0x04,
	AFD_FULL_FRAME = 0x08,
	AFD_FULL_FRAME_ALT = 0x09,
	AFD_16x9_CENTER = 0x0a,
	AFD_14x9_CENTER = 0x0b,
	AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER = 0x0d,
	AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER = 0x0e,
	AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER = 0x0f,
};

enum klvanc_payload_afd_barflags {
	BARS_NONE = 0x00,
	BARS_LEFTRIGHT = 0x03,
	BARS_TOPBOTTOM = 0x0c,
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_afd_s
{
	struct klvanc_packet_header_s hdr;
	enum klvanc_payload_aspect_ratio_e aspectRatio;
	enum klvanc_payload_afd_e afd;
	enum klvanc_payload_afd_barflags barDataFlags;
	unsigned short top;
	unsigned short bottom;
	unsigned short left;
	unsigned short right;
};

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum payload_afd_e afd - Brief description goes here.
 * @return	Success - User facing printable string.
 * @return	Error - NULL
 */
const char *klvanc_afd_to_string(enum klvanc_payload_afd_e afd);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum payload_aspect_ratio_e ar - Brief description goes here.
 * @return	Success - User facing printable string.
 * @return	Error - NULL
 */
const char *klvanc_aspectRatio_to_string(enum klvanc_payload_aspect_ratio_e ar);

/**
 * @brief	Return a string representing the bar flags field
 * @param[in]	enum klvanc_payload_afd_barflags flags - Value of flags field
 * @return	Success - User facing printable string.
 * @return	Error - NULL
 */
const char *klvanc_barFlags_to_string(enum klvanc_payload_afd_barflags flags);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_AFD(struct klvanc_context_s *ctx, void *p);

/**
 * @brief	Create an AFD VANC packet
 * @param[out]	struct klvanc_packet_afd_s **pkt - Pointer to newly created packet
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_AFD(struct klvanc_packet_afd_s **pkt);

/**
 * @brief	Destroy an AFD VANC packet
 * @param[in]	struct klvanc_packet_afd_s *pkt - Packet to be destroyed
 */
void klvanc_destroy_AFD(struct klvanc_packet_afd_s *pkt);

/**
 * @brief	Set the AFD value on an AFD packet
 * @param[in]	struct klvanc_packet_afd_s *pkt - Packet to be modified
 * @param[in]	unsigned char val - Value to set the AFD field to
 * @return	  0 - Success
 * @return	< 0 - Unknown framerate specified
 */
int klvanc_set_AFD_val(struct klvanc_packet_afd_s *pkt, unsigned char val);

/**
 * @brief	Convert type struct klvanc_packet_afd_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct klvanc_packet_afd_s *pkt - A AFD VANC entry
 * @param[out]	uint16_t **words - An array of words representing a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_AFD_to_words(struct klvanc_packet_afd_s *pkt, uint16_t **words, uint16_t *wordCount);

/**
 * @brief	Convert type struct klvanc_packet_afd_s into a block of bytes which represents\n
 *              an AFD packet
 *              On success, caller MUST free the resulting *bytes array.
 * @param[in]	struct klvanc_packet_afd_s *pkt - A AFD VANC entry, received from the AFD parser
 * @param[out]	uint8_t **bytes - An array of bytes representing the serialized AFD packet
 * @param[out]	uint16_t *byteCount - Number of bytes in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_AFD_to_packetBytes(struct klvanc_packet_afd_s *pkt, uint8_t **bytes, uint16_t *byteCount);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_AFD_H */
