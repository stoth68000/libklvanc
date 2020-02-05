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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char *cc_type_lookup(int cc_type)
{
	switch (cc_type) {
	case 0x00: return "NTSC line 21 field 1 CC";
	case 0x01: return "NTSC line 21 field 2 CC";
	case 0x02: return "DTVCC Channel Packet Data";
	case 0x03: return "DTVCC Channel Packet Start";
	default: return "Unknown";
	}
}

static const char *cc_framerate_lookup(int frate)
{
	switch (frate) {
	case 0x00: return "Forbidden";
	case 0x01: return "23.976";
	case 0x02: return "24";
	case 0x03: return "25";
	case 0x04: return "29.97";
	case 0x05: return "30";
	case 0x06: return "50";
	case 0x07: return "59.94";
	case 0x08: return "60";
	default: return "Reserved";
	}
}

int klvanc_create_eia708_cdp(struct klvanc_packet_eia_708b_s **pkt)
{
	struct klvanc_packet_eia_708b_s *p = calloc(1, sizeof(*p));
	if (p == NULL)
		return -ENOMEM;

	/* Pre-set some mandatory fields.  These are ID fields with well-defined
           values an API user would never want to have to explicitly set on their
           own.  See CEA-708 Sec 11.2 for their meaning.  */
	p->header.cdp_identifier = 0x9669;
	p->ccdata.ccdata_id = 0x72;
	p->ccsvc.ccsvcinfo_id = 0x73;
	p->footer.cdp_footer_id = 0x74;

	*pkt = p;
	return 0;
}

void klvanc_destroy_eia708_cdp(struct klvanc_packet_eia_708b_s *pkt)
{
	free(pkt);
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

int klvanc_set_framerate_EIA_708B(struct klvanc_packet_eia_708b_s *pkt, int num, int den)
{
	int gcd_val = gcd(num, den);
	num /= gcd_val;
	den /= gcd_val;
	if (num == 1001 && den == 24000)
		pkt->header.cdp_frame_rate = 0x01;
	else if (num == 1 && den == 24)
		pkt->header.cdp_frame_rate = 0x02;
	else if (num == 1 && den == 25)
		pkt->header.cdp_frame_rate = 0x03;
	else if (num == 1001 && den == 30000)
		pkt->header.cdp_frame_rate = 0x04;
	else if (num == 1 && den == 30)
		pkt->header.cdp_frame_rate = 0x05;
	else if (num == 1 && den == 50)
		pkt->header.cdp_frame_rate = 0x06;
	else if (num == 1001 && den == 60000)
		pkt->header.cdp_frame_rate = 0x07;
	else if (num == 1 && den == 60)
		pkt->header.cdp_frame_rate = 0x08;
	else
		return -1;
	return 0;
}

int klvanc_dump_EIA_708B(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_eia_708b_s *pkt = p;

	if (ctx->verbose)
		PRINT_DEBUG("%s() %p\n", __func__, (void *)pkt);

	PRINT_DEBUG(" pkt->header.cdp_identifier = 0x%04x (%s)\n",
		    pkt->header.cdp_identifier,
		    pkt->header.cdp_identifier == 0x9669 ? "VALID" : "INVALID");

	PRINT_DEBUG_MEMBER_INTI(pkt->header.cdp_length, 1);
	PRINT_DEBUG(" pkt->header.cdp_frame_rate = 0x%02x (%s FPS)\n",
		    pkt->header.cdp_frame_rate,
		    cc_framerate_lookup(pkt->header.cdp_frame_rate));
	PRINT_DEBUG_MEMBER_INTI(pkt->header.time_code_present, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.ccdata_present, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.svcinfo_present, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.svc_info_start, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.svc_info_change, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.svc_info_complete, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.caption_service_active, 1);
	PRINT_DEBUG_MEMBER_INTI(pkt->header.cdp_hdr_sequence_cntr, 1);
	if (pkt->header.time_code_present) {
		PRINT_DEBUG("  pkt->tcdata.time_code_section_id = 0x%02x (%s)\n",
			    pkt->tc.time_code_section_id,
			    pkt->tc.time_code_section_id == 0x71 ? "VALID" : "INVALID");
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_10hrs, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_1hrs, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_10min, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_1min, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_field_flag, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_10sec, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_1sec, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.drop_frame_flag, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_10fr, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->tc.tc_1fr, 2);
		PRINT_DEBUG("  timecode = %02d:%02d:%02d:%02d\n",
			    pkt->tc.tc_10hrs * 10 + pkt->tc.tc_1hrs,
			    pkt->tc.tc_10min * 10 + pkt->tc.tc_1min,
			    pkt->tc.tc_10sec * 10 + pkt->tc.tc_1sec,
			    pkt->tc.tc_10fr * 10 + pkt->tc.tc_1fr);
	}

	if (pkt->header.ccdata_present) {
		PRINT_DEBUG("  pkt->ccdata.ccdata_id = 0x%02x (%s)\n",
			    pkt->ccdata.ccdata_id,
			    pkt->ccdata.ccdata_id == 0x72 ? "VALID" : "INVALID");
		PRINT_DEBUG_MEMBER_INTI(pkt->ccdata.cc_count, 2);
		for (int i = 0; i < pkt->ccdata.cc_count; i++) {
			PRINT_DEBUG("  pkt->ccdata.cc[%d]\n", i);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccdata.cc[i].cc_valid, 3);
			PRINT_DEBUG("   pkt->ccdata.cc[i].cc_type = 0x%02x (%s)\n",
				    pkt->ccdata.cc[i].cc_type,
				    cc_type_lookup(pkt->ccdata.cc[i].cc_type));
			PRINT_DEBUG_MEMBER_INTI(pkt->ccdata.cc[i].cc_data[0], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccdata.cc[i].cc_data[1], 3);
		}
	}

	if (pkt->header.svcinfo_present) {
		PRINT_DEBUG("  pkt->svcinfo.ccsvcinfo_id = 0x%02x (%s)\n",
			    pkt->ccsvc.ccsvcinfo_id,
			    pkt->ccsvc.ccsvcinfo_id == 0x73 ? "VALID" : "INVALID");
		PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc_info_start, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc_info_change, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc_info_complete, 2);
		PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc_count, 2);
		for (int i = 0; i < pkt->ccsvc.svc_count; i++) {
			/* Format defined in ATSC A/65:2013 Sec 6.9.2 */
			PRINT_DEBUG("  pkt->ccsvc.svc[%d]\n", i);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].caption_service_number, 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[0], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[1], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[2], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[3], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[4], 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].svc_data_byte[5], 3);
			PRINT_DEBUG("   pkt->ccsvc.svc[i].language = %s\n", pkt->ccsvc.svc[i].language);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].digital_cc, 3);
			if (pkt->ccsvc.svc[i].digital_cc) {
				PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].csn, 3);
			} else {
				PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].line21_field, 3);
			}
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].easy_reader, 3);
			PRINT_DEBUG_MEMBER_INTI(pkt->ccsvc.svc[i].wide_aspect_ratio, 3);
		}
	}

	/* FIXME: add support for "future_section" (Sec 11.2.7) */

	PRINT_DEBUG(" pkt->footer.cdp_footer_id = 0x%02x (%s)\n",
		    pkt->footer.cdp_footer_id,
		    pkt->footer.cdp_footer_id == 0x74 ? "VALID" : "INVALID");
	PRINT_DEBUG(" pkt->footer.cdp_ftr_sequence_cntr = 0x%02x (%s)\n",
		    pkt->footer.cdp_ftr_sequence_cntr,
		    pkt->footer.cdp_ftr_sequence_cntr == pkt->header.cdp_hdr_sequence_cntr ? "Matches Header" : "INVALID: does not match header");

	PRINT_DEBUG(" pkt->footer.packet_checksum = 0x%02x (%s)\n",
		    pkt->footer.packet_checksum,
		    pkt->checksum_valid == 1 ? "VALID" : "INVALID");

	return KLAPI_OK;
}

int parse_EIA_708B(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void **pp)
{
	struct klbs_context_s *bs;
	uint8_t next_section_id;

	if (ctx->callbacks == NULL || ctx->callbacks->eia_708b == NULL)
		return KLAPI_OK;

	bs = klbs_alloc();
	if (bs == NULL)
		return -ENOMEM;

	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_eia_708b_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt) {
		klbs_free(bs);
		return -ENOMEM;
	}

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	/* Extract the 8-bit bitstream from the 10-bit payload */
	for (int i = 0; i < 256; i++)
		pkt->payload[i] = hdr->payload[i];
	pkt->payloadLengthBytes = hdr->payloadLengthWords;

	/* Parse the bitstream */
	klbs_read_set_buffer(bs, pkt->payload, pkt->payloadLengthBytes);

	/* CDP Header (Sec 11.2.2) */
	if (klbs_get_byte_count_free(bs) < 7) {
		free(pkt);
		klbs_free(bs);
		return -ENOMEM;
	}
	pkt->header.cdp_identifier = klbs_read_bits(bs, 16);
	pkt->header.cdp_length = klbs_read_bits(bs, 8);
	pkt->header.cdp_frame_rate = klbs_read_bits(bs, 4);
	klbs_read_bits(bs, 4); /* Reserved */
	pkt->header.time_code_present = klbs_read_bits(bs, 1);
	pkt->header.ccdata_present = klbs_read_bits(bs, 1);
	pkt->header.svcinfo_present = klbs_read_bits(bs, 1);
	pkt->header.svc_info_start = klbs_read_bits(bs, 1);
	pkt->header.svc_info_change = klbs_read_bits(bs, 1);
	pkt->header.svc_info_complete = klbs_read_bits(bs, 1);
	pkt->header.caption_service_active = klbs_read_bits(bs, 1);
	klbs_read_bits(bs, 1); /* Reserved */
	pkt->header.cdp_hdr_sequence_cntr = klbs_read_bits(bs, 16);

	if (klbs_get_byte_count_free(bs) < 1) {
		free(pkt);
		klbs_free(bs);
		return -ENOMEM;
	}
	next_section_id = klbs_read_bits(bs, 8);

	if (next_section_id == 0x71) {
		if (klbs_get_byte_count_free(bs) < 5) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}
		/* timecode_section (Sec 11.2.3) */
		pkt->tc.time_code_section_id = next_section_id;
		klbs_read_bits(bs, 2); /* Reserved */
		pkt->tc.tc_10hrs = klbs_read_bits(bs, 2);
		pkt->tc.tc_1hrs = klbs_read_bits(bs, 4);
		klbs_read_bits(bs, 1); /* Reserved */
		pkt->tc.tc_10min = klbs_read_bits(bs, 3);
		pkt->tc.tc_1min = klbs_read_bits(bs, 4);
		pkt->tc.tc_field_flag = klbs_read_bits(bs, 1);
		pkt->tc.tc_10sec = klbs_read_bits(bs, 3);
		pkt->tc.tc_1sec = klbs_read_bits(bs, 4);
		pkt->tc.drop_frame_flag = klbs_read_bits(bs, 1);
		/* Note: differs between 708-B and subsequent
		   revisions of spec */
		klbs_read_bits(bs, 1); /* zero */
		pkt->tc.tc_10fr = klbs_read_bits(bs, 2);
		pkt->tc.tc_1fr = klbs_read_bits(bs, 4);
		next_section_id = klbs_read_bits(bs, 8);
	}

	if (next_section_id == 0x72) {
		if (klbs_get_byte_count_free(bs) < 2) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}
		/* cc_data_section (Sec 11.2.4) */
		pkt->ccdata.ccdata_id = next_section_id;
		klbs_read_bits(bs, 3); /* Marker Bits */
		pkt->ccdata.cc_count = klbs_read_bits(bs, 5);

		if (klbs_get_byte_count_free(bs) < (pkt->ccdata.cc_count * 3)) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}
		for (int i = 0; i < pkt->ccdata.cc_count; i++) {
			klbs_read_bits(bs, 5); /* Marker Bits */
			pkt->ccdata.cc[i].cc_valid = klbs_read_bits(bs, 1);
			pkt->ccdata.cc[i].cc_type = klbs_read_bits(bs, 2);
			pkt->ccdata.cc[i].cc_data[0] = klbs_read_bits(bs, 8);
			pkt->ccdata.cc[i].cc_data[1] = klbs_read_bits(bs, 8);
		}
		next_section_id = klbs_read_bits(bs, 8);
	}

	if (next_section_id == 0x73) {
		if (klbs_get_byte_count_free(bs) < 3) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}
		/* ccsvcinfo_section (Sec 11.2.5) */
		pkt->ccsvc.ccsvcinfo_id = next_section_id;
		klbs_read_bits(bs, 1); /* Marker Bits */
		pkt->ccsvc.svc_info_start = klbs_read_bits(bs, 1);
		pkt->ccsvc.svc_info_change = klbs_read_bits(bs, 1);
		pkt->ccsvc.svc_info_complete = klbs_read_bits(bs, 1);
		pkt->ccsvc.svc_count = klbs_read_bits(bs, 4);

		/* Abort the parse if we don't have enough data in the bitstream. */
		if (klbs_get_byte_count_free(bs) < pkt->ccsvc.svc_count * 7) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}

		for (int i = 0; i < pkt->ccsvc.svc_count; i++) {
			klbs_read_bits(bs, 3); /* Marker Bits */
			pkt->ccsvc.svc[i].caption_service_number = klbs_read_bits(bs, 5);
			for (int n = 0; n < 6; n++) {
				pkt->ccsvc.svc[i].svc_data_byte[n] = klbs_read_bits(bs, 8);
			}

			pkt->ccsvc.svc[i].language[0] = pkt->ccsvc.svc[i].svc_data_byte[0];
			pkt->ccsvc.svc[i].language[1] = pkt->ccsvc.svc[i].svc_data_byte[1];
			pkt->ccsvc.svc[i].language[2] = pkt->ccsvc.svc[i].svc_data_byte[2];
			if (pkt->ccsvc.svc[i].svc_data_byte[3] & 0x80) {
				pkt->ccsvc.svc[i].digital_cc = 1;
				pkt->ccsvc.svc[i].csn = pkt->ccsvc.svc[i].svc_data_byte[3] & 0x3f;
			} else {
				pkt->ccsvc.svc[i].line21_field = pkt->ccsvc.svc[i].svc_data_byte[3] & 0x01;
			}
			if (pkt->ccsvc.svc[i].svc_data_byte[4] & 0x80)
				pkt->ccsvc.svc[i].easy_reader = 1;
			if (pkt->ccsvc.svc[i].svc_data_byte[4] & 0x40)
				pkt->ccsvc.svc[i].wide_aspect_ratio = 1;
		}
		next_section_id = klbs_read_bits(bs, 8);
	}

	/* FIXME: add support for "future_section" (Sec 11.2.7) */

	if (next_section_id == 0x74) {
		/* cdp_footer section (Sec 11.2.6) */
		if (klbs_get_byte_count_free(bs) < 3) {
			free(pkt);
			klbs_free(bs);
			return -ENOMEM;
		}
		pkt->footer.cdp_footer_id = next_section_id;
		pkt->footer.cdp_ftr_sequence_cntr = klbs_read_bits(bs, 16);
		pkt->footer.packet_checksum = klbs_read_bits(bs, 8);
	}

	/* Validate checksum */
	uint8_t sum = 0;
	for (int i = 0; i < pkt->payloadLengthBytes; i++) {
		sum += pkt->payload[i];
	}
	if (sum == 0)
		pkt->checksum_valid = 1;
	else
		pkt->checksum_valid = 0;

	ctx->callbacks->eia_708b(ctx->callback_context, ctx, pkt);

	klbs_free(bs);

	*pp = pkt;
	return KLAPI_OK;
}

void klvanc_finalize_EIA_708B(struct klvanc_packet_eia_708b_s *pkt, uint16_t seqNum)
{
	pkt->header.cdp_hdr_sequence_cntr = seqNum;
	pkt->footer.cdp_ftr_sequence_cntr = seqNum;
}

int klvanc_convert_EIA_708B_to_packetBytes(struct klvanc_packet_eia_708b_s *pkt, uint8_t **bytes, uint16_t *byteCount)
{
	if (!pkt || !bytes) {
		return -1;
	}

	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -ENOMEM;

	*bytes = malloc(255);
	if (*bytes == NULL) {
		klbs_free(bs);
		return -ENOMEM;
	}

	/* Serialize the EIA-708 struct into a binary blob */
	klbs_write_set_buffer(bs, *bytes, 255);

	/* CDP Header (Sec 11.2.2) */
	klbs_write_bits(bs, pkt->header.cdp_identifier, 16);
	klbs_write_bits(bs, 0x00, 8); /* length will be filled in later */
	klbs_write_bits(bs, pkt->header.cdp_frame_rate, 4);
	klbs_write_bits(bs, 0x0f, 4); /* Reserved */
	klbs_write_bits(bs, pkt->header.time_code_present, 1);
	klbs_write_bits(bs, pkt->header.ccdata_present, 1);
	klbs_write_bits(bs, pkt->header.svcinfo_present, 1);
	klbs_write_bits(bs, pkt->header.svc_info_start, 1);
	klbs_write_bits(bs, pkt->header.svc_info_change, 1);
	klbs_write_bits(bs, pkt->header.svc_info_complete, 1);
	klbs_write_bits(bs, pkt->header.caption_service_active, 1);
	klbs_write_bits(bs, 0x01, 1); /* Reserved */
	klbs_write_bits(bs, pkt->header.cdp_hdr_sequence_cntr, 16);

        if (pkt->header.time_code_present && pkt->tc.time_code_section_id == 0x71) {
		/* timecode_section (Sec 11.2.3) */
		klbs_write_bits(bs, pkt->tc.time_code_section_id, 8);
		klbs_write_bits(bs, 0x03, 2); /* Reserved */
		klbs_write_bits(bs, pkt->tc.tc_10hrs, 2);
		klbs_write_bits(bs, pkt->tc.tc_1hrs, 4);
		klbs_write_bits(bs, 0x01, 1); /* Reserved */
		klbs_write_bits(bs, pkt->tc.tc_10min, 3);
		klbs_write_bits(bs, pkt->tc.tc_1min, 4);
		klbs_write_bits(bs, pkt->tc.tc_field_flag, 1);
		klbs_write_bits(bs, pkt->tc.tc_10sec, 3);
		klbs_write_bits(bs, pkt->tc.tc_1sec, 4);
		klbs_write_bits(bs, 0x01, 1); /* Reserved */
		klbs_write_bits(bs, pkt->tc.tc_10fr, 3);
		klbs_write_bits(bs, pkt->tc.tc_1fr, 4);
        }

	if (pkt->header.ccdata_present && pkt->ccdata.ccdata_id == 0x72) {
		/* cc_data_section (Sec 11.2.4) */
		klbs_write_bits(bs, pkt->ccdata.ccdata_id, 8);
		klbs_write_bits(bs, 0x07, 3); /* Marker bits */
		klbs_write_bits(bs, pkt->ccdata.cc_count, 5);
		for (int i = 0; i < pkt->ccdata.cc_count; i++) {
			klbs_write_bits(bs, 0x1f, 5); /* Marker bits */
			klbs_write_bits(bs, pkt->ccdata.cc[i].cc_valid, 1);
			klbs_write_bits(bs, pkt->ccdata.cc[i].cc_type, 2);
			klbs_write_bits(bs, pkt->ccdata.cc[i].cc_data[0], 8);
			klbs_write_bits(bs, pkt->ccdata.cc[i].cc_data[1], 8);
		}
	}

	if (pkt->header.svcinfo_present && pkt->ccsvc.ccsvcinfo_id == 0x73) {
		/* ccsvcinfo_section (Sec 11.2.5) */
		klbs_write_bits(bs, pkt->ccsvc.ccsvcinfo_id, 8);
		klbs_write_bits(bs, 0x01, 1); /* Marker bits */
		klbs_write_bits(bs, pkt->ccsvc.svc_info_start, 1);
		klbs_write_bits(bs, pkt->ccsvc.svc_info_change, 1);
		klbs_write_bits(bs, pkt->ccsvc.svc_info_complete, 1);
		klbs_write_bits(bs, pkt->ccsvc.svc_count, 4);
		for (int i = 0; i < pkt->ccsvc.svc_count; i++) {
			klbs_write_bits(bs, 0x07, 3); /* Marker bits */
			klbs_write_bits(bs, pkt->ccsvc.svc[i].caption_service_number, 5);
			for (int n = 0; n < 6; n++) {
				klbs_write_bits(bs, pkt->ccsvc.svc[i].svc_data_byte[n], 8);
			}
		}
	}

	/* cdp_footer section (Sec 11.2.6) */
	klbs_write_bits(bs, pkt->footer.cdp_footer_id, 8);
	klbs_write_bits(bs, pkt->footer.cdp_ftr_sequence_cntr, 16);
	klbs_write_bits(bs, pkt->footer.packet_checksum, 8);

	klbs_write_buffer_complete(bs);

	/* Set length */
	(*bytes)[2] = klbs_get_byte_count(bs);

	/* Compute CDP checksum as last byte */
	uint8_t sum = 0;
	for (int i = 0; i < klbs_get_byte_count(bs) - 1; i++) {
		sum += (*bytes)[i];
	}
	(*bytes)[klbs_get_byte_count(bs) - 1] = ~sum + 1;

	*byteCount = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

int klvanc_convert_EIA_708B_to_words(struct klvanc_packet_eia_708b_s *pkt, uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	int ret;

	ret = klvanc_convert_EIA_708B_to_packetBytes(pkt, &buf, &byteCount);
	if (ret != 0)
		return ret;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x01, 0x61, buf, byteCount, words, wordCount, 10);

	free(buf);

	return 0;
}
