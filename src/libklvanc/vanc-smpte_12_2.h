/*
 * Copyright (c) 2019 Kernel Labs Inc. All Rights Reserved
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
 * @file	vanc-smpte_12_2.h
 * @author	Devin Heitmueller <dheitmueller@kernellabs.com>
 * @copyright	Copyright (c) 2019 Kernel Labs Inc. All Rights Reserved.
 * @brief	SMPTE ST 12-2 Timecode over VANC
 */

#ifndef _VANC_SMPTE_12_2_H
#define _VANC_SMPTE_12_2_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif

/* DBB1 payload types (See Sec 6.2.1 Table 2) */
#define KLVANC_ATC_LTC 0x00
#define KLVANC_ATC_VITC1 0x01
#define KLVANC_ATC_VITC2 0x02
/* Codes 3-5 are user defined */
#define KLVANC_FILM_DATA_BLOCK 0x06
#define KLVANC_PROD_DATA_BLOCK 0x07
/* Codes 0x08 to 0x7c are "locally generated time address and user data (user defined) */
#define KLVANC_VID_TAPE_DATA_BLOCK_LOCAL 0x7d
#define KLVANC_FILM_DATA_BLOCK_LOCAL 0x7e
#define KLVANC_PROD_DATA_BLOCK_LOCAL 0x7f
/* Codes 0x80 to 0xff are Reserved */

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_smpte_12_2_s
{
	struct klvanc_packet_header_s hdr;

	unsigned char payload[256];
	unsigned int payloadLengthBytes;

	uint8_t dbb1;
	uint8_t dbb2;

	uint8_t vitc_line_select;
	uint8_t line_duplication_flag;
	uint8_t tc_validity_flag;
	uint8_t user_bits_process_flag;

	/* Timecode data */
	uint8_t frames;
	uint8_t seconds;
	uint8_t minutes;
	uint8_t hours;

	/* See ST 12-1:2014 Table 11 for the meanings of these
	   flags, which may vary depending on the framerate */

	/* Drop frame flag in 30/60.  Always zero in 25/50/24/48 */
	uint8_t flag14;
	/* Color frame flag in 30/60/25/50.  Always zero in 24/48 */
	uint8_t flag15;
	/* Field Bit/LTC Polarity in 30/60/24/48.  BGF0 in 25/50 */
	uint8_t flag35;
	/* BGF0 in 30/60/24/48.  BGF2 in 25/50 */
	uint8_t flag55;
	/* BGF1 in all framerates */
	uint8_t flag74;
	/* BGF2 in 30/60/24/48.  Field Bit/LTC Polarity in 25/50 */
	uint8_t flag75;
};

/**
 * @brief       Create SMPTE ST 12-2 timecode
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_alloc_SMPTE_12_2(struct klvanc_packet_smpte_12_2_s **pkt);

/**
 * @brief       Create SMPTE ST 12-2 timecode from SMPTE 370 / 314 timecode
 * @param[in]	uint32_t st370_tc - Packed binary representation of timecode as described in SMPTE ST 370:2013 Sec 3.4.2.2.1 and SMPTE S314M-2015 Sec 4.4.2.2.1
 * @param[in]	int frate_num - framerate numerator (e.g. 1001)
 * @param[in]	int frate_den - framerate denominator (e.g. 30000)
 * @param[out]	struct klvanc_packet_smpte_12_2_s **pkt - newly created packet
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_SMPTE_12_2_from_ST370(uint32_t st370_tc,
					int frate_num, int frate_den,
					struct klvanc_packet_smpte_12_2_s **pkt);


/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_SMPTE_12_2(struct klvanc_context_s *ctx, void *p);

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	void *p - Pointer to struct (klvanc_packet_smpte_12_2_s *)
 */
void klvanc_free_SMPTE_12_2(void *p);

/**
 * @brief	Convert type struct packet_smpte_12_2_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct packet_smpte_12_2_s *pkt - A SMPTE 12_2-2 VANC entry, received from the 12_2 parser
 * @param[out]	uint16_t **words - An array of words reppresenting a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_SMPTE_12_2_to_words(struct klvanc_context_s *ctx,
				     struct klvanc_packet_smpte_12_2_s *pkt,
				     uint16_t **words, uint16_t *wordCount);

/**
 * @brief	Convert type struct packet_smpte_12_2_s into a block of bytes which can be\n
 *              embedded into a VANC line
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct packet_smpte_12_2_s *pkt - A SMPTE 12_2 VANC entry, received from the 12_2 parser
 * @param[out]	uint8_t **bytes - An array of words reppresenting a fully formed vanc line.
 * @param[out]	uint16_t *byteCount - Number of byes in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_SMPTE_12_2_to_packetBytes(struct klvanc_context_s *ctx,
					   const struct klvanc_packet_smpte_12_2_s *pkt,
					   uint8_t **bytes, uint16_t *byteCount);

/**
 * @brief	Determine the appropriate line to insert this S-12 packet onto.
 *            This takes into consideration interoperability with legacy
 *            equipment, as described in SMPTE S-12-2:2014 Sec 8.2.1.
 * @param[in]	int dbb1 - The Payload type of this S-12-2 packet
 * @param[in]	int lineCount - The number of lines in the video frame
 * @param[in]	int interlaced - whether the video frame is interlaced
 * @return      > 0 Line number to insert into
 * @return      < 0 - Error
 */

int klvanc_SMPTE_12_2_preferred_line(int dbb1, int lineCount, int interlaced);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_SMPTE_12_2_H */
