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
 * @file	smpte2038.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	Functions to parse, create and inspect SMPTE2038 formatted packets.
 */

#ifndef SMPTE2038_H
#define SMPTE2038_H

#include <libklvanc/vanc-packets.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_smpte2038_anc_data_line_s
{
	/* Based on data taken from the ADTEC EN-100 encodeer,
	 * DID, SDID, DATA_COUNT, WORDS and CHECKSUM are
	 * all marked with parity, don't forget to strip
	 * bits 9:8 before you trust these values, esp. data_count.
	 */
	uint8_t		reserved_000000;
	uint8_t		c_not_y_channel_flag;
	uint16_t	line_number;
	uint16_t	horizontal_offset;
	uint16_t	DID;
	uint16_t	SDID;
	uint16_t	data_count;
	uint16_t	*user_data_words;
	uint16_t	checksum_word;
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_smpte2038_anc_data_packet_s
{
	uint32_t	packet_start_code_prefix;
	uint8_t		stream_id;
	uint16_t	PES_packet_length;
	uint8_t		reserved_10;
	uint8_t		PES_scrambling_control;
	uint8_t		PES_priority;
	uint8_t		data_alignment_indicator;
	uint8_t		copyright;
	uint8_t		original_or_copy;
	uint8_t		PTS_DTS_flags;
	uint8_t		ESCR_flag;
	uint8_t		ES_rate_flag;
	uint8_t		DSM_trick_mode_flag;
	uint8_t		additional_copy_info_flag;
	uint8_t		PES_CRC_flag;
	uint8_t		PES_extension_flag;
	uint8_t		PES_header_data_length;
	uint8_t		reserved_0010;
	uint64_t	PTS;

	int lineCount;
	struct klvanc_smpte2038_anc_data_line_s *lines;
};

/**
 * @brief	Inspect a section, if its deemed valid, create a VANC packet and return it to the caller.\n
 *              Typically this is a line of SDI data, 10bit video. We parse the content in this function,\n
 *              if we find a VANC header signiture we'll create an ancillary packet to represet it,\n
 *              we'll attempt to parse the structure and return a user representation of it.\n\n
 *              Callers must release the returned struct using klvanc_smpte2038_anc_data_packet_free().
 * @param[in]	uint8_t *section - An array of memory that likely contains a valic (or invalid) VANC message.
 * @param[in]	unsigned int byteCount - Length of section.
 * @param[out]	struct klvanc_smpte2038_anc_data_packet_s **result - Packet
 * @result	0 - Success, **result is valid for future use.
 * @result	< 0 - Error
 */
int  klvanc_smpte2038_parse_pes_packet(uint8_t *section, unsigned int byteCount, struct klvanc_smpte2038_anc_data_packet_s **result);

/**
 * @brief	Inspect a section payload.  This function is identical to klvanc_smpte2038_parse_pes_packet(),\n
 *              except it takes just the payload section of a section, rather than the full section which\n
 *              would include the section headers (e.g. start code prefix, stream id, etc).  This routine\n
 *              is for use in cases where the calling application is responsible for parsing the PES and\n
 *              only hands off the payload section to decoders (such as the mpegts demux in libavformat).\n\n
 *              Callers must release the returned struct using klvanc_smpte2038_anc_data_packet_free().
 * @param[in]	uint8_t *section - An array of memory that likely contains a valic (or invalid) VANC message.
 * @param[in]	unsigned int byteCount - Length of section.
 * @param[out]	struct klvanc_smpte2038_anc_data_packet_s **result - Packet
 * @result	0 - Success, **result is valid for future use.
 * @result	< 0 - Error
 */
int  klvanc_smpte2038_parse_pes_payload(uint8_t *payload, unsigned int byteCount, struct klvanc_smpte2038_anc_data_packet_s **result);

/**
 * @brief	Inspect structure and output textual information to console.
 * @param[in]	struct klvanc_smpte2038_anc_data_packet_s *pkt - Packet
 */
void klvanc_smpte2038_anc_data_packet_dump(struct klvanc_smpte2038_anc_data_packet_s *h);

/**
 * @brief	Deallocate and release a previously allocated pkt, see klvanc_smpte2038_parse_section().
 * @param[in]	struct klvanc_smpte2038_anc_data_packet_s *pkt - Packet
 */
void klvanc_smpte2038_anc_data_packet_free(struct klvanc_smpte2038_anc_data_packet_s *pkt);

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_smpte2038_packetizer_s
{
	uint8_t *buf;
	uint32_t buflen;
	uint32_t bufused;
	uint32_t buffree;
	struct   klbs_context_s *bs;
};

/**
 * @brief	Allocate a context so we can use this with the rest of the library.
 * @param[out]	struct klvanc_smpte2038_packetizer_s **ctx - Context
 * @return      0 - Success
 * @return    < 0 - Error
 */
int klvanc_smpte2038_packetizer_alloc(struct klvanc_smpte2038_packetizer_s **ctx);

/**
 * @brief	Deallocate and release a previously allocated context, see klvanc_smpte2038_packetizer_alloc().
 * @param[in]	struct klvanc_smpte2038_packetizer_s **ctx - Context
 */
void klvanc_smpte2038_packetizer_free(struct klvanc_smpte2038_packetizer_s **ctx);

/**
 * @brief	Initialize state, typically done at the beginning of each incoming SDI frame\n
 *              must be done before attempting to append decoded VANC packets.
 * @param[in]	struct klvanc_smpte2038_packetizer_s **ctx - Context
 * @return      0 - Success
 * @return    < 0 - Error
 */
int klvanc_smpte2038_packetizer_begin(struct klvanc_smpte2038_packetizer_s *ctx);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct klvanc_smpte2038_packetizer_s *ctx
 * @param[in]	struct klvanc_packet_header_s *pkt
 * @return	TODO.
 */
int klvanc_smpte2038_packetizer_append(struct klvanc_smpte2038_packetizer_s *ctx,
				       struct klvanc_packet_header_s *pkt);

/**
 * @brief	Finalize VANC collection state. Typically done when the last VANC line in a frame\n
 *              has been passed to klvanc_smpte2038_packetizer_append().\n
 *              Don't attempt to append without first calling klvanc_smpte2038_packetizer_begin().
 * @param[in]	struct klvanc_smpte2038_packetizer_s **ctx - Context
 * @param[in]	struct packet_header_s *pkt - A fully decoded VANC packet, from the vanc_*() callbacks.
 * @return      0 - Success
 * @return    < 0 - Error
 */
int klvanc_smpte2038_packetizer_end(struct klvanc_smpte2038_packetizer_s *ctx, uint64_t pts);

/**
 * @brief	Convert type struct klvanc_smpte2038_anc_data_line_s into a more traditional line of\n
 *              vanc words, so that we may push it into the vanc parser.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct klvanc_smpte2038_anc_data_line_s *line - A line of decomposed vanc, received from the SMPTE2038 parser.
 * @param[out]	uint16_t **words - An array of words reppresenting a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_smpte2038_convert_line_to_words(struct klvanc_smpte2038_anc_data_line_s *l, uint16_t **words, uint16_t *wordCount);

#ifdef __cplusplus
};
#endif

#endif /* SMPTE2038_H */
