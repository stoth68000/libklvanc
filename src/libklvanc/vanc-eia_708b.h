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
 * @file	vanc-eia_708b.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _VANC_EIA_708B_H
#define _VANC_EIA_708B_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  


struct packet_eia_708b_cdp_header
{
	uint16_t cdp_identifier;
	uint8_t cdp_length;
	uint8_t cdp_frame_rate;
	uint8_t time_code_present;
	uint8_t ccdata_present;
	uint8_t svcinfo_present;
	uint8_t svc_info_start;
	uint8_t svc_info_change;
	uint8_t svc_info_complete;
	uint8_t caption_service_active;
	uint16_t cdp_hdr_sequence_cntr;;
};

struct packet_eia_708b_time_code_section
{
	uint8_t time_code_section_id;
	uint8_t tc_10hrs;
	uint8_t tc_1hrs;
	uint8_t tc_10min;
	uint8_t tc_1min;
	uint8_t tc_field_flag;
	uint8_t tc_10sec;
	uint8_t tc_1sec;
	uint8_t drop_frame_flag;
	uint8_t tc_10fr;
	uint8_t tc_1fr;
};

struct packet_eia_708b_ccdata_entry
{
	uint8_t cc_valid;
	uint8_t cc_type;
	uint8_t cc_data[2];
};

#define MAX_CC_COUNT 30
struct packet_eia_708b_ccdata_section
{
	uint8_t ccdata_id;
	uint8_t cc_count;
	struct packet_eia_708b_ccdata_entry cc[MAX_CC_COUNT];
};


struct packet_eia_708b_ccsvcinfo_entry
{
	uint8_t caption_service_number;
	uint8_t svc_data_byte[6];
};

#define MAX_CCSVC_COUNT 16
struct packet_eia_708b_ccsvcinfo_section
{
	uint8_t ccsvcinfo_id;
	uint8_t svc_info_start;
	uint8_t svc_info_change;
	uint8_t svc_info_complete;
	uint8_t svc_count;
	struct packet_eia_708b_ccsvcinfo_entry svc[MAX_CCSVC_COUNT];
};

struct packet_eia_708b_cdp_footer
{
	uint8_t cdp_footer_id;
	uint16_t cdp_ftr_sequence_cntr;
	uint8_t packet_checksum;
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_eia_708b_s
{
	struct packet_header_s hdr;
	uint8_t payload[256];
	unsigned int payloadLengthBytes;
	int checksum_valid;
	struct packet_eia_708b_cdp_header header;
	struct packet_eia_708b_time_code_section tc;
	struct packet_eia_708b_ccdata_section ccdata;
	struct packet_eia_708b_ccsvcinfo_section ccsvc;
	struct packet_eia_708b_cdp_footer footer;
};

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int dump_EIA_708B(struct vanc_context_s *ctx, void *p);

/**
 * @brief	Create an EIA-708 VANC packet
 * @param[out]	struct packet_eia_708b_s **pkt - Pointer to newly created packet
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_eia708_cdp(struct packet_eia_708b_s **pkt);

/**
 * @brief	Destroy an EIA-708 VANC packet
 * @param[in]	struct packet_eia_708b_s *pkt - Packet to be destroyed
 * @return	none
 */
void klvanc_destroy_eia708_cdp(struct packet_eia_708b_s *pkt);

/**
 * @brief	Set the framerate on an EIA-708 packet
 * @param[in]	struct packet_eia_708b_s *pkt - Packet to be modified
 * @param[in]	int num - numerator (e.g. 1001)
 * @return	  0 - success
 * @return	< 0 - Unknown framerate specified
 */
int klvanc_set_framerate_EIA_708B(struct packet_eia_708b_s *pkt, int num, int den);

/**
 * @brief	Finalize a packet and prepare to serialize to output
 * @param[in]	struct packet_eia_708b_s *pkt - A EIA-608 VANC entry, received from the EIA-708 parser
 * @param[in]	uint16_t seqNum - Sequence Number.  This value should increment with each packet output over the final SDI link.
 */
void klvanc_finalize_EIA_708B(struct packet_eia_708b_s *pkt, uint16_t seqNum);

/**
 * @brief	Convert type struct packet_eia_708b_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct packet_eia_708bs *pkt - A EIA-708 VANC entry, received from the EIA-708 parser
 * @param[out]	uint16_t **words - An array of words reppresenting a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int convert_EIA_708B_to_words(struct packet_eia_708b_s *pkt, uint16_t **words, uint16_t *wordCount);

/**
 * @brief	Convert type struct packet_eia_708b_s into a block of bytes which represents\n
 *              an EIA-708 packet
 *              On success, caller MUST free the resulting *bytes array.
 * @param[in]	struct packet_eia_708b_s *pkt - A EIA-608 VANC entry, received from the EIA-708 parser
 * @param[out]	uint8_t **bytes - An array of bytes representing the serialized CDP packet
 * @param[out]	uint16_t *byteCount - Number of bytes in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int convert_EIA_708B_to_packetBytes(struct packet_eia_708b_s *pkt, uint8_t **bytes, uint16_t *byteCount);


#ifdef __cplusplus
};
#endif  

#endif /* _VANC_EIA_708B_H */
