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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

static const char *dbb1_types(unsigned char val)
{
	/* DBB1 Payload Type (SMPTE 12-2:2014 Table 2) */

	if (val >= 0x08 && val <= 0x7c)
		return "Locally generated time address and user data";

	switch (val) {
	case 0x00:
		return "Linear time code (ATC_LTC)";
	case 0x01:
		return "ATC_VITC1";
	case 0x02:
		return "ATC_VITC2";
	case 0x03:
	case 0x04:
	case 0x05:
		return "User defined";
	case 0x06:
		return "Film data block (transferred from reader)";
	case 0x07:
		return "Production data block (transferered from reader)";
	case 0x7d:
		return "Video tape data block (locally generated)";
	case 0x7e:
		return "Film data block (locally generated)";
	case 0x7f:
		return "Production data block (locally generated)";
	default:
		return "Reserved";
	}
}

int klvanc_alloc_SMPTE_12_2(struct klvanc_packet_smpte_12_2_s **outPkt)
{
	struct klvanc_packet_smpte_12_2_s *pkt = (struct klvanc_packet_smpte_12_2_s *) calloc(1, sizeof(struct klvanc_packet_smpte_12_2_s));
	if (pkt == NULL)
		return -1;

        *outPkt = pkt;
        return 0;
}

void klvanc_free_SMPTE_12_2(void *p)
{
	struct klvanc_packet_smpte_12_2_s *pkt = p;

	if (pkt == NULL)
		return;

	free(pkt);
}

static unsigned bcd2uint(uint8_t bcd)
{
	unsigned low  = bcd & 0xf;
	unsigned high = bcd >> 4;
	if (low > 9 || high > 9)
		return 0;
	return low + 10*high;
}

static int gcd (int a, int b)
{
	int r;
	while (b > 0) {
		r = a % b;
		a = b;
		b = r;
	}
	return a;
}

int klvanc_create_SMPTE_12_2_from_ST370(uint32_t st370_tc,
					int frate_num, int frate_den,
					struct klvanc_packet_smpte_12_2_s **pkt)
{
	struct klvanc_packet_smpte_12_2_s *timecode;
	int gcd_val, ret;

	gcd_val = gcd(frate_num, frate_den);
	frate_num /= gcd_val;
	frate_den /= gcd_val;

	ret = klvanc_alloc_SMPTE_12_2(&timecode);
	if (ret < 0)
		return ret;
	*pkt = timecode;

	/* See SMPTE ST 314M-2005 Sec 4.4.2.2.1 "Time code pack (TC)" */
	timecode->frames = bcd2uint(st370_tc >> 24 & 0x3f);
	timecode->seconds = bcd2uint(st370_tc >> 16 & 0x7f);
	timecode->minutes = bcd2uint(st370_tc >> 8  & 0x7f);
	timecode->hours = bcd2uint(st370_tc & 0x3f);
	if ((frate_num == 1 && frate_den == 50) ||
	    (frate_num == 1 && frate_den == 25)) {
		/* Field bit */
		if (st370_tc & 0x00800000) {
			timecode->dbb1 = 0x02; /* ATC_VITC2 */
			timecode->flag75 = 0x01;
		} else {
			timecode->dbb1 = 0x01; /* ATC_VITC1 */
		}
	} else {
		/* Field bit */
		if (st370_tc & 0x00000080) {
			timecode->dbb1 = 0x02; /* ATC_VITC2 */
			timecode->flag35 = 0x01;
		} else {
			timecode->dbb1 = 0x01; /* ATC_VITC1 */
		}
		/* Drop frame flag */
		if (st370_tc & 0x40000000) {
			timecode->flag14 = 0x01;
		}
	}

	return 0;
}

int parse_SMPTE_12_2(struct klvanc_context_s *ctx,
		   struct klvanc_packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	if (hdr->payloadLengthWords != 0x10)
		return -EINVAL;

	struct klvanc_packet_smpte_12_2_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	/* DBB1 Payload Type (SMPTE 12-2:2014 Sec 6.2.1) */
	for (int i = 0; i < 8; i++) {
		pkt->dbb1 |= ((hdr->payload[i] >> 3) & 0x01) << i;
	}

	/* DBB2 Payload Type (SMPTE 12-2:2014 Sec 6.2.2) */
	for (int i = 0; i < 8; i++) {
		pkt->dbb2 |= ((hdr->payload[i+8] >> 3) & 0x01) << i;
	}
	pkt->vitc_line_select = pkt->dbb2 & 0x1f;
	if (pkt->dbb2 & 0x20)
		pkt->line_duplication_flag = 1;
	if (pkt->dbb2 & 0x40)
		pkt->tc_validity_flag = 1;
	if (pkt->dbb2 & 0x80)
		pkt->user_bits_process_flag = 1;

	/* Payload for ATC_LTC, ATC_VITC1 or ATC_VITC2 (SMPTE 12-2:2008 Sec 6.3) */
	if (pkt->dbb1 == 0 || pkt->dbb1 == 0x01 || pkt->dbb1 == 0x02) {
		pkt->frames = (hdr->payload[0] >> 4) & 0x0f;
		if (hdr->payload[2] & 0x10)
			pkt->frames += 10;
		if (hdr->payload[2] & 0x20)
			pkt->frames += 20;
		if (hdr->payload[2] & 0x40)
			pkt->flag14 = 1;
		if (hdr->payload[2] & 0x80)
			pkt->flag15 = 1;
		pkt->seconds = (hdr->payload[4] >> 4) & 0x0f;
		if (hdr->payload[6] & 0x10)
			pkt->seconds += 10;
		if (hdr->payload[6] & 0x20)
			pkt->seconds += 20;
		if (hdr->payload[6] & 0x40)
			pkt->seconds += 40;
		if (hdr->payload[6] & 0x80)
			pkt->flag35 = 1;
		pkt->minutes = (hdr->payload[8] >> 4) & 0x0f;
		if (hdr->payload[10] & 0x10)
			pkt->minutes += 10;
		if (hdr->payload[10] & 0x20)
			pkt->minutes += 20;
		if (hdr->payload[10] & 0x40)
			pkt->minutes += 40;
		if (hdr->payload[10] & 0x80)
			pkt->flag55 = 1;
		pkt->hours = (hdr->payload[12] >> 4) & 0x0f;
		if (hdr->payload[14] & 0x10)
			pkt->hours += 10;
		if (hdr->payload[14] & 0x20)
			pkt->hours += 20;
		if (hdr->payload[14] & 0x40)
			pkt->flag74 = 1;
		if (hdr->payload[14] & 0x80)
			pkt->flag75 = 1;
	} else {
		PRINT_DEBUG("DBB type parsing not yet implemented for dbb1 type 0x%x\n",
			pkt->dbb1);
	}

	if (ctx->callbacks && ctx->callbacks->smpte_12_2)
		ctx->callbacks->smpte_12_2(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_dump_SMPTE_12_2(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_smpte_12_2_s *pkt = p;

	if (ctx->verbose)
		PRINT_DEBUG("%s() %p\n", __func__, (void *)pkt);

	PRINT_DEBUG(" DBB1 = %02x (%s)\n", pkt->dbb1, dbb1_types(pkt->dbb1));
	PRINT_DEBUG(" DBB2 = %02x\n", pkt->dbb2);
	PRINT_DEBUG(" DBB2 VITC line select = 0x%02x\n", pkt->vitc_line_select);
	PRINT_DEBUG(" DBB2 line duplication flag = %d\n", pkt->line_duplication_flag);
	PRINT_DEBUG(" DBB2 time code validity = %d\n", pkt->tc_validity_flag);
	PRINT_DEBUG(" DBB2 (User bits) process bit = %d\n", pkt->user_bits_process_flag);

	PRINT_DEBUG(" Timecode = %02d:%02d:%02d:%02d\n", pkt->hours, pkt->minutes,
		    pkt->seconds, pkt->frames);

	return 0;
}

int klvanc_convert_SMPTE_12_2_to_packetBytes(struct klvanc_context_s *ctx,
					   const struct klvanc_packet_smpte_12_2_s *pkt,
					   uint8_t **bytes, uint16_t *byteCount)
{
	uint8_t dbb2;
	uint8_t *buf;

	if (!pkt || !bytes) {
		return -1;
	}

	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = malloc(16);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the Timecode into a binary blob conforming to SMPTE 12-1 */
	klbs_write_set_buffer(bs, buf, 16);

        /* FIXME: Assumes VITC code */

	/* See SMPTE 12-2:2014 Table 6 */
	if (pkt->dbb1 == 0 || pkt->dbb1 == 0x01 || pkt->dbb1 == 0x02) {
		/* UDW 1 */
		klbs_write_bits(bs, pkt->frames % 10, 4); /* Units of frames 1-8 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 2 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 1 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 3 */
		klbs_write_bits(bs, pkt->flag14, 1);
		klbs_write_bits(bs, pkt->flag15, 1);
		klbs_write_bits(bs, (pkt->frames / 20) & 0x01, 1); /* Tens of frames 20 */
		klbs_write_bits(bs, (pkt->frames / 10) & 0x01, 1); /* Tens of frames 10 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 4 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 2 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 5 */
		klbs_write_bits(bs, pkt->seconds % 10, 4); /* Units of seconds 1-8 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 6 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 3 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 7 */
		klbs_write_bits(bs, pkt->flag35, 1); /* Flag */
		klbs_write_bits(bs, (pkt->seconds / 40) & 0x01, 1); /* Tens of seconds 40 */
		klbs_write_bits(bs, (pkt->seconds / 20) & 0x01, 1); /* Tens of seconds 20 */
		klbs_write_bits(bs, (pkt->seconds / 10) & 0x01, 1); /* Tens of seconds 10 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 8 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 4 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 9 */
		klbs_write_bits(bs, pkt->minutes % 10, 4); /* Units of minutes 1-8 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 10 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 5 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 11 */
		klbs_write_bits(bs, pkt->flag55, 1); /* Flag */
		klbs_write_bits(bs, (pkt->minutes / 40) & 0x01, 1); /* Tens of minutes 40 */
		klbs_write_bits(bs, (pkt->minutes / 20) & 0x01, 1); /* Tens of minutes 20 */
		klbs_write_bits(bs, (pkt->minutes / 10) & 0x01, 1); /* Tens of minutes 10 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 12 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 6 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 13 */
		klbs_write_bits(bs, pkt->hours % 10, 4); /* Units of hours 1-8 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 14 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 7 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 15 */
		klbs_write_bits(bs, pkt->flag74, 1);
		klbs_write_bits(bs, pkt->flag75, 1);
		klbs_write_bits(bs, (pkt->hours / 20) & 0x01, 1); /* Tens of hours 20 */
		klbs_write_bits(bs, (pkt->hours / 10) & 0x01, 1); /* Tens of hours 10 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
		/* UDW 16 */
		klbs_write_bits(bs, 0x00, 4); /* Binary group 8 */
		klbs_write_bits(bs, 0x00, 4); /* b0-b3 */
	} else {
		PRINT_DEBUG("DBB1 type not yet supported: %02x\n", pkt->dbb1);
	}

	klbs_write_buffer_complete(bs);

	/* Now go back and fill in DBB1/DBB2 */
	for (int i = 0; i < 8; i++) {
		buf[i] |= ((pkt->dbb1 >> i) & 0x01) << 3;
	}
	dbb2 = pkt->vitc_line_select;
	if (pkt->line_duplication_flag)
		dbb2 |= 0x20;
	if (pkt->tc_validity_flag)
		dbb2 |= 0x40;
	if (pkt->user_bits_process_flag)
		dbb2 |= 0x80;

	for (int i = 0; i < 8; i++) {
		buf[i+8] |= ((dbb2 >> i) & 0x01) << 3;
	}

#if 0
	PRINT_DEBUG("Resulting buffer size=%d\n", klbs_get_byte_count(bs));
	PRINT_DEBUG(" ->payload  = ");
	for (int i = 0; i < klbs_get_byte_count(bs); i++) {
		PRINT_DEBUG("%02x ", buf[i]);
	}
	PRINT_DEBUG("\n");
#endif

	*bytes = buf;
	*byteCount = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

int klvanc_convert_SMPTE_12_2_to_words(struct klvanc_context_s *ctx,
				     struct klvanc_packet_smpte_12_2_s *pkt,
				     uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	int ret;

	ret = klvanc_convert_SMPTE_12_2_to_packetBytes(ctx, pkt, &buf, &byteCount);
	if (ret != 0)
		return -1;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x60, 0x60, buf, byteCount, words, wordCount, 10);

	free(buf);

	return 0;
}

struct s12_lines {
	int payload_type;
	int linecount;
	int interlaced;
	int preferred_line;
};

int klvanc_SMPTE_12_2_preferred_line(int dbb1, int lineCount, int interlaced)
{
    /* The table below is constructed by using ST 12-2:2014 Sec 8.2.1 for HD
       resolutions, and Sec 8.2.2 for SD (which says as early as posssible
       after second line of switching point as defined in SMPTE RP-168) */

    struct s12_lines lines[] = {
        {KLVANC_ATC_VITC1, 1080, 1, 9},
        {KLVANC_ATC_VITC1, 1080, 0, 9},
        {KLVANC_ATC_VITC1, 720, 0, 9},
        {KLVANC_ATC_VITC1, 486, 1, 12},
        {KLVANC_ATC_VITC1, 576, 1, 8},
        {KLVANC_ATC_VITC2, 1080, 1, 571},
        {KLVANC_ATC_VITC2, 1080, 0, 9},
        {KLVANC_ATC_VITC2, 720, 0, 9},
        {KLVANC_ATC_VITC2, 486, 1, 275},
        {KLVANC_ATC_VITC2, 576, 1, 321},
    };

    for (int i = 0; i < (sizeof(lines)/sizeof(struct s12_lines)); i++) {
	    if (dbb1 == lines[i].payload_type &&
		lineCount == lines[i].linecount &&
		interlaced == lines[i].interlaced) {
		    return lines[i].preferred_line;
	    }
    }

    if (dbb1 != KLVANC_ATC_VITC1 && dbb1 != KLVANC_ATC_VITC1 &&
        dbb1 != KLVANC_ATC_LTC) {
	    /* Table 7 says any line except 9, 10, and 571, so we choose
	       line 11 */
	    return 11;
    }

    /* Not found */
    return -1;
}
