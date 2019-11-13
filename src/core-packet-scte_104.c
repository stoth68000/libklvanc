/*
 * Copyright (c) 2016-2017 Kernel Labs Inc. All Rights Reserved
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

static const char *gpiEdge(unsigned char edge)
{
	switch (edge) {
	case 0x00: return "Open->Closed";
	case 0x01: return "Closed->Open";
	default:   return "Undefined";
	}
}

static void print_debug_member_timestamp(struct klvanc_context_s *ctx, struct klvanc_multiple_operation_message_timestamp *ts)
{
	PRINT_DEBUG( " m->timestamp type = 0x%02x ", ts->time_type);
	switch (ts->time_type) {
	case 1:
		PRINT_DEBUG("(UTC Time)\n");
		PRINT_DEBUG("   m->timestamp value = %d.%06d (UTC seconds)\n", ts->time_type_1.UTC_seconds, ts->time_type_1.UTC_microseconds);
                break;
        case 2:
		PRINT_DEBUG("(SMPTE VITC timecode)\n");
		PRINT_DEBUG("   m->timestamp value = %02d:%02d:%02d:%02d (hh:mm:ss:ff)\n", ts->time_type_2.hours, ts->time_type_2.minutes,
		       ts->time_type_2.seconds, ts->time_type_2.frames);
                break;
        case 3:
		PRINT_DEBUG("(GPI input)\n");
		PRINT_DEBUG("   m->timestamp GPI number = %d\n", ts->time_type_3.GPI_number);
		PRINT_DEBUG("   m->timestamp GPI edge = 0x%02x (%s)\n", ts->time_type_3.GPI_edge, gpiEdge(ts->time_type_3.GPI_edge));
                break;
        case 0:
                /* The spec says no time is defined, this is a legitimate state. */
		PRINT_DEBUG("(none)\n");
		break;
        default:
		PRINT_DEBUG("(unknown/unsupported)\n");
        }
}

static const char *spliceInsertTypeName(unsigned char type)
{
	switch (type) {
	case 0x0000:                return "reserved";
	case SPLICESTART_NORMAL:    return "spliceStart_normal";
	case SPLICESTART_IMMEDIATE: return "spliceStart_immediate";
	case SPLICEEND_NORMAL:      return "spliceEnd_normal";
	case SPLICEEND_IMMEDIATE:   return "spliceEnd_immediate";
	case SPLICE_CANCEL:         return "splice_cancel";
	default:                    return "Undefined";
	}
}

static const char *som_operationName(unsigned short opID)
{
	if ((opID >= 0x8000) && (opID <= 0xbfff))
		return "User Defined";
	if ((opID >= 0x0013) && (opID <= 0x00ff))
		return "Reserved for future basic requests";
	switch (opID) {
	case 0x0000: return "general_response_data";
	case 0x0001: return "initial_request_data";
	case 0x0002: return "initial_response_data";
	case 0x0003: return "alive_request_data";
	case 0x0004: return "alive_response_data";
	case 0x0005:
	case 0x0006: return "User Defined";
	case 0x0007: return "inject_response_data";
	case 0x0008: return "inject_complete_response_data";
	case 0x0009: return "config_request_data";
	case 0x000a: return "config_response_data";
	case 0x000b: return "provisioning_request_data";
	case 0x000c: return "provisioning_response_data";
	case 0x000f: return "fault_request_data";
	case 0x0010: return "fault_response_data";
	case 0x0011: return "AS_alive_request_data";
	case 0x0012: return "AS_alive_response_data";
	default:     return "Reserved";
	}
}

static const char *mom_operationName(unsigned short opID)
{
	if ((opID >= 0xc000) && (opID <= 0xFFFE))
		return "User Defined";

	switch (opID) {
	case 0x0100: return "inject_section_data_request";
	case 0x0101: return "splice_request_data";
	case 0x0102: return "splice_null_request_data";
	case 0x0103: return "start_schedule_download_request_data";
	case 0x0104: return "time_signal_request_data";
	case 0x0105: return "transmit_schedule_request_data";
	case 0x0106: return "component_mode_DPI_request_data";
	case 0x0107: return "encrypted_DPI_request_data";
	case 0x0108: return "insert_descriptor_request_data";
	case 0x0109: return "insert_DTMF_descriptor_request_data";
	case 0x010a: return "insert_avail_descriptor_request_data";
	case 0x010b: return "insert_segmentation_descriptor_request_data";
	case 0x010c: return "proprietary_command_request_data";
	case 0x010d: return "schedule_component_mode_request_data";
	case 0x010e: return "schedule_definition_data_request";
	case 0x010f: return "insert_tier_data";
	case 0x0110: return "insert_time_descriptor";
	case 0x0300: return "delete_controlword_data_request";
	case 0x0301: return "update_controlword_data_request";
	default:     return "Reserved";
	}
}

static const char *seg_type_id(unsigned char type_id)
{
	/* Values come from SCTE 35 2019, Sec 10.3.3.1 (Table 22) */
	switch (type_id) {
	case 0x00: return "Not Indicated";
	case 0x01: return "Content Identification";
	case 0x10: return "Program Start";
	case 0x11: return "Program End";
	case 0x12: return "Program Early Termination";
	case 0x13: return "Program Breakaway";
	case 0x14: return "Program Resumption";
	case 0x15: return "Program Runover Planned";
	case 0x16: return "Program Runover Unplanned";
	case 0x17: return "Program Overlap Start";
	case 0x18: return "Program Blackout Override";
	case 0x19: return "Program Start - In Progress";
	case 0x20: return "Chapter Start";
	case 0x21: return "Chapter End";
	case 0x22: return "Break Start";
	case 0x23: return "Break End";
	case 0x24: return "Opening Credit Start";
	case 0x25: return "Opening Credit End";
	case 0x26: return "Closing Credit Start";
	case 0x27: return "Closing Credit End";
	case 0x30: return "Provider Advertisement Start";
	case 0x31: return "Provider Advertisement End";
	case 0x32: return "Distributor Advertisement Start";
	case 0x33: return "Distributor Advertisement End";
	case 0x34: return "Provider Placement Opportunity Start";
	case 0x35: return "Provider Placement Opportunity End";
	case 0x36: return "Distributor Placement Opportunity Start";
	case 0x37: return "Distributor Placement Opportunity End";        
	case 0x38: return "Provider Overlay Placement Start";
	case 0x39: return "Provider Overlay Placement End";
	case 0x3A: return "Distributor Overlay Placement Start";
	case 0x3B: return "Distributor Overlay Placement End";
	case 0x40: return "Unscheduled Event Start";
	case 0x41: return "Unscheduled Event End";
	case 0x50: return "Network Start";
	case 0x51: return "Network End";
	default:   return "Unknown";
	}
}

static const char *seg_upid_type(unsigned char upid_type)
{
	/* Values come from SCTE 35 2016, Sec 10.3.3.1, Table 21 */
	switch (upid_type) {
	case 0x00: return "Not Used";
	case 0x01: return "User Defined (Deprecated)";
	case 0x02: return "ISCI (Deprecated)";
	case 0x03: return "Ad-ID";
	case 0x04: return "UMID";
	case 0x05: return "ISAN (Deprecated)";
	case 0x06: return "ISAN";
	case 0x07: return "TID";
	case 0x08: return "TI";
	case 0x09: return "ADI";
	case 0x0a: return "EIDR";
	case 0x0b: return "ATSC Content Identifier";
	case 0x0c: return "MPU()";
	case 0x0d: return "MID()";
	case 0x0e: return "ADS Information";
	case 0x0f: return "URI";
	default:   return "Reserved";
	}
}

static const char *seg_device_restrictions(unsigned char val)
{
	/* Values come from SCTE 35 2016, Sec 10.3.3.1, Table 21 */
	switch (val) {
	case 0x00: return "Restrict Group 0";
	case 0x01: return "Restrict Group 1";
	case 0x02: return "Restrict Group 2";
	case 0x03: return "None";
	default:   return "Unknown";
	}
}

static void hexdump(struct klvanc_context_s *ctx, unsigned char *buf, unsigned int len, int bytesPerRow /* Typically 16 */, char *indent)
{
	PRINT_DEBUG("%s", indent);
	for (unsigned int i = 0; i < len; i++)
		PRINT_DEBUG("%02x%s", buf[i], ((i + 1) % bytesPerRow) ? " " : "\n%s");
	PRINT_DEBUG("\n");
}

static unsigned char *parse_splice_request_data(struct klvanc_context_s *ctx, unsigned char *p,
						struct klvanc_splice_request_data *d)
{
	d->splice_insert_type  = *(p++);
	d->splice_event_id     = *(p + 0) << 24 | *(p + 1) << 16 | *(p + 2) <<  8 | *(p + 3); p += 4;
	d->unique_program_id   = *(p + 0) << 8 | *(p + 1); p += 2;
	d->pre_roll_time       = *(p + 0) << 8 | *(p + 1); p += 2;
	d->brk_duration        = *(p + 0) << 8 | *(p + 1); p += 2;
	d->avail_num           = *(p++);
	d->avails_expected     = *(p++);
	d->auto_return_flag    = *(p++);

	/* TODO: We don't support splice cancel, but we'll pass it through with a warning. */
	switch (d->splice_insert_type) {
	case SPLICESTART_IMMEDIATE:
	case SPLICEEND_IMMEDIATE:
	case SPLICESTART_NORMAL:
	case SPLICEEND_NORMAL:
	case SPLICE_CANCEL:
		break;
	default:
		/* We don't support this splice command */
		PRINT_ERR("%s() splice_insert_type 0x%x [%s], error.\n", __func__,
			d->splice_insert_type,
		spliceInsertTypeName(d->splice_insert_type));
	}

	return p;
}

#define MAX_DESC_SIZE 255
static int gen_splice_request_data(const struct klvanc_splice_request_data *d,
				   unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->splice_insert_type, 8);
	klbs_write_bits(bs, d->splice_event_id, 32);
	klbs_write_bits(bs, d->unique_program_id, 16);
	klbs_write_bits(bs, d->pre_roll_time, 16);
	klbs_write_bits(bs, d->brk_duration, 16);
	klbs_write_bits(bs, d->avail_num, 8);
	klbs_write_bits(bs, d->avails_expected, 8);
	klbs_write_bits(bs, d->auto_return_flag, 8);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static int gen_splice_null_request_data(unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL)
		return -1;

	/* splice_null_request has no actual body, so nothing to do but return an
	   empty buffer */

	*outBuf = buf;
	*outSize = 0;

	return 0;
}

static int gen_time_signal_request_data(const struct klvanc_time_signal_request_data *d,
					unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->pre_roll_time, 16);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_descriptor_request_data(unsigned char *p,
						    struct klvanc_insert_descriptor_request_data *d,
						    unsigned int descriptor_size)
{
	d->descriptor_count = *(p++);
	d->total_length = descriptor_size;

	memset(d->descriptor_bytes, 0, sizeof(d->descriptor_bytes));
	if (d->total_length < sizeof(d->descriptor_bytes)) {
		memcpy(d->descriptor_bytes, p, d->total_length);
	}
	p += d->total_length;

	return p;
}

static int gen_descriptor_request_data(const struct klvanc_insert_descriptor_request_data *d,
				       unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->descriptor_count, 8);
	for (int i = 0; i < d->total_length; i++)
		klbs_write_bits(bs, d->descriptor_bytes[i], 8);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}


static unsigned char *parse_dtmf_request_data(unsigned char *p,
					      struct klvanc_dtmf_descriptor_request_data *d)
{
	d->pre_roll_time = *(p++);
	d->dtmf_length = *(p++);
	memset(d->dtmf_char, 0, sizeof(d->dtmf_char));
	if (d->dtmf_length <= sizeof(d->dtmf_char)) {
		memcpy(d->dtmf_char, p, d->dtmf_length);
	}
	p += d->dtmf_length;

	return p;
}

static int gen_dtmf_request_data(const struct klvanc_dtmf_descriptor_request_data *d,
				 unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->pre_roll_time, 8);
	klbs_write_bits(bs, d->dtmf_length, 8);
	for (int i = 0; i < d->dtmf_length; i++)
		klbs_write_bits(bs, d->dtmf_char[i], 8);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_avail_request_data(unsigned char *p,
					       struct klvanc_avail_descriptor_request_data *d)
{
	d->num_provider_avails = *(p++);
	memset(d->provider_avail_id, 0, sizeof(d->provider_avail_id));

	for (int i = 0; i < d->num_provider_avails; i++) {
		d->provider_avail_id[i] = *(p + 0) << 24 | *(p + 1) << 16 | *(p + 2) <<  8 | *(p + 3); p += 4;
	}

	return p;
}

static int gen_avail_request_data(const struct klvanc_avail_descriptor_request_data *d,
				  unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);
	klbs_write_bits(bs, d->num_provider_avails, 8);
	for (int i = 0; i < d->num_provider_avails; i++)
		klbs_write_bits(bs, d->provider_avail_id[i], 32);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_segmentation_request_data(unsigned char *p,
						      struct klvanc_segmentation_descriptor_request_data *d)
{
	d->event_id = *(p + 0) << 24 | *(p + 1) << 16 | *(p + 2) <<  8 | *(p + 3); p += 4;
	d->event_cancel_indicator = *(p++);
	d->duration = (p[0] << 8) | p[1];
	p += 2;
	d->upid_type = *(p++);
	d->upid_length = *(p++);

	memset(d->upid, 0, sizeof(d->upid));

	if (d->upid_length > sizeof(d->upid))
		return NULL;
	memcpy(d->upid, p, d->upid_length);
	p += d->upid_length;
	d->type_id = *(p++);
	d->segment_num = *(p++);
	d->segments_expected = *(p++);
	d->duration_extension_frames = *(p++);
	d->delivery_not_restricted_flag = *(p++);
	d->web_delivery_allowed_flag = *(p++);
	d->no_regional_blackout_flag = *(p++);
	d->archive_allowed_flag = *(p++);
	d->device_restrictions = *(p++);

	return p;
}

static int gen_segmentation_request_data(const struct klvanc_segmentation_descriptor_request_data *d,
					 unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->event_id, 32);
	klbs_write_bits(bs, d->event_cancel_indicator, 8);
	klbs_write_bits(bs, d->duration, 16);
	klbs_write_bits(bs, d->upid_type, 8);
	klbs_write_bits(bs, d->upid_length, 8);

	for (int i = 0; i < d->upid_length; i++)
		klbs_write_bits(bs, d->upid[i], 8);

	klbs_write_bits(bs, d->type_id, 8);
	klbs_write_bits(bs, d->segment_num, 8);
	klbs_write_bits(bs, d->segments_expected, 8);
	klbs_write_bits(bs, d->duration_extension_frames, 8);
	klbs_write_bits(bs, d->delivery_not_restricted_flag, 8);
	klbs_write_bits(bs, d->web_delivery_allowed_flag, 8);
	klbs_write_bits(bs, d->no_regional_blackout_flag, 8);
	klbs_write_bits(bs, d->archive_allowed_flag, 8);
	klbs_write_bits(bs, d->device_restrictions, 8);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_proprietary_command_request_data(unsigned char *p,
							     struct klvanc_proprietary_command_request_data *d,
							     unsigned int descriptor_size)
{
	memset(d->proprietary_data, 0, sizeof(d->proprietary_data));

	d->proprietary_id = *(p + 0) << 24 | *(p + 1) << 16 | *(p + 2) <<  8 | *(p + 3); p += 4;
	d->proprietary_command = *(p++);
	d->data_length = descriptor_size - 5;

	memcpy(d->proprietary_data, p, d->data_length);
	p+= d->data_length;

	return p;
}

static int gen_proprietary_command_request_data(const struct klvanc_proprietary_command_request_data *d,
						unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->proprietary_id, 32);
	klbs_write_bits(bs, d->proprietary_command, 8);

	for (int i = 0; i < d->data_length; i++)
		klbs_write_bits(bs, d->proprietary_data[i], 8);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_tier_data(unsigned char *p, struct klvanc_tier_data *d)
{
	d->tier_data = *(p + 0) << 8 | *(p + 1); p += 2;

	/* SCTE 104:2015 Sec 9.8.9.1 says the top four bits must be zero */
	d->tier_data &= 0x0fff;

	return p;
}

static int gen_tier_data(const struct klvanc_tier_data *d, unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	/* SCTE 104:2015 Sec 9.8.9.1 says the top four bits must be zero */
	klbs_write_bits(bs, d->tier_data & 0x0fff, 16);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_time_descriptor(unsigned char *p, struct klvanc_time_descriptor_data *d)
{
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return NULL;

	klbs_read_set_buffer(bs, p, 12);

	d->TAI_seconds = klbs_read_bits(bs, 48);
	d->TAI_ns = klbs_read_bits(bs, 32);
	d->UTC_offset = klbs_read_bits(bs, 16);

	klbs_free(bs);

	return p;
}

static int gen_time_descriptor(const struct klvanc_time_descriptor_data *d,
			       unsigned char **outBuf, uint16_t *outSize)
{
	unsigned char *buf;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	buf = (unsigned char *) malloc(MAX_DESC_SIZE);
	if (buf == NULL) {
		klbs_free(bs);
		return -1;
	}

	/* Serialize the SCTE 104 request into a binary blob */
	klbs_write_set_buffer(bs, buf, MAX_DESC_SIZE);

	klbs_write_bits(bs, d->TAI_seconds, 48);
	klbs_write_bits(bs, d->TAI_ns, 32);
	klbs_write_bits(bs, d->UTC_offset, 16);

	klbs_write_buffer_complete(bs);

	*outBuf = buf;
	*outSize = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

static unsigned char *parse_mom_timestamp(struct klvanc_context_s *ctx, unsigned char *p,
					  struct klvanc_multiple_operation_message_timestamp *ts)
{
	ts->time_type = *(p++);
	switch (ts->time_type) {
	case 1:
		ts->time_type_1.UTC_seconds      = *(p + 0) << 24 | *(p + 1) << 16 | *(p + 2) << 8 | *(p + 3);
		ts->time_type_1.UTC_microseconds = *(p + 4) << 8 | *(p + 5);
		p += 6;
		break;
	case 2:
		ts->time_type_2.hours   = *(p + 0);
		ts->time_type_2.minutes = *(p + 1);
		ts->time_type_2.seconds = *(p + 2);
		ts->time_type_2.frames  = *(p + 3);
		p += 4;
		break;
	case 3:
		ts->time_type_3.GPI_number = *(p + 0);
		ts->time_type_3.GPI_edge   = *(p + 1);
		p += 2;
		break;
	case 0:
		/* The spec says no time is defined, this is a legitimate state. */
		break;
	default:
		PRINT_ERR("%s() unsupported time_type 0x%x, assuming no time.\n", __func__, ts->time_type);
	}

	return p;
}

static int dump_mom(struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
	struct klvanc_multiple_operation_message *m = &pkt->mo_msg;

	PRINT_DEBUG("SCTE104 multiple_operation_message struct\n");
	PRINT_DEBUG_MEMBER_INT(pkt->payloadDescriptorByte);

	PRINT_DEBUG_MEMBER_INT(m->rsvd);
	PRINT_DEBUG("    rsvd = %s\n", m->rsvd == 0xFFFF ? "Multiple_Ops (Reserved)" : "UNSUPPORTED");
	PRINT_DEBUG_MEMBER_INT(m->messageSize);
	PRINT_DEBUG_MEMBER_INT(m->protocol_version);
	PRINT_DEBUG_MEMBER_INT(m->AS_index);
	PRINT_DEBUG_MEMBER_INT(m->message_number);
	PRINT_DEBUG_MEMBER_INT(m->DPI_PID_index);
	PRINT_DEBUG_MEMBER_INT(m->SCTE35_protocol_version);
	print_debug_member_timestamp(ctx, &m->timestamp);
	PRINT_DEBUG_MEMBER_INT(m->num_ops);

	for (int i = 0; i < m->num_ops; i++) {
		struct klvanc_multiple_operation_message_operation *o = &m->ops[i];
		PRINT_DEBUG("\n opID[%d] = %s\n", i, mom_operationName(o->opID));
		PRINT_DEBUG_MEMBER_INT(o->opID);
		PRINT_DEBUG_MEMBER_INT(o->data_length);
		if (o->data_length)
			hexdump(ctx, o->data, o->data_length, 32, "    ");
		if (o->opID == MO_SPLICE_REQUEST_DATA) {
			struct klvanc_splice_request_data *d = &o->sr_data;
			PRINT_DEBUG_MEMBER_INT(d->splice_insert_type);
			PRINT_DEBUG("    splice_insert_type = %s\n", spliceInsertTypeName(d->splice_insert_type));
			PRINT_DEBUG_MEMBER_INT(d->splice_event_id);
			PRINT_DEBUG_MEMBER_INT(d->unique_program_id);
			PRINT_DEBUG_MEMBER_INT(d->pre_roll_time);
			PRINT_DEBUG("    pre_roll_time = %d (milliseconds)\n", d->pre_roll_time);
			PRINT_DEBUG_MEMBER_INT(d->brk_duration);
			PRINT_DEBUG("    break_duration = %d (1/10th seconds)\n", d->brk_duration);
			PRINT_DEBUG_MEMBER_INT(d->avail_num);
			PRINT_DEBUG_MEMBER_INT(d->avails_expected);
			PRINT_DEBUG_MEMBER_INT(d->auto_return_flag);
		} else if (o->opID == MO_TIME_SIGNAL_REQUEST_DATA) {
			struct klvanc_time_signal_request_data *d = &o->timesignal_data;
			PRINT_DEBUG_MEMBER_INT(d->pre_roll_time);
		} else if (o->opID == MO_INSERT_DESCRIPTOR_REQUEST_DATA) {
			struct klvanc_insert_descriptor_request_data *d = &o->descriptor_data;
			PRINT_DEBUG_MEMBER_INT(d->descriptor_count);
			PRINT_DEBUG_MEMBER_INT(d->total_length);
			for (int j = 0; j < d->total_length; j++) {
				PRINT_DEBUG_MEMBER_INT(d->descriptor_bytes[j]);
			}
		} else if (o->opID == MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA) {
			struct klvanc_avail_descriptor_request_data *d = &o->avail_descriptor_data;
			PRINT_DEBUG_MEMBER_INT(d->num_provider_avails);
			for (int j = 0; j < d->num_provider_avails; j++) {
				PRINT_DEBUG_MEMBER_INT(d->provider_avail_id[j]);
			}
		} else if (o->opID == MO_INSERT_DTMF_REQUEST_DATA) {
			struct klvanc_dtmf_descriptor_request_data *d = &o->dtmf_data;
			PRINT_DEBUG_MEMBER_INT(d->pre_roll_time);
			PRINT_DEBUG_MEMBER_INT(d->dtmf_length);
			for (int j = 0; j < d->dtmf_length; j++) {
				PRINT_DEBUG_MEMBER_INT(d->dtmf_char[j]);
			}
		} else if (o->opID == MO_INSERT_SEGMENTATION_REQUEST_DATA) {
			struct klvanc_segmentation_descriptor_request_data *d = &o->segmentation_data;
			PRINT_DEBUG_MEMBER_INT(d->event_id);
			PRINT_DEBUG_MEMBER_INT(d->event_cancel_indicator);
			PRINT_DEBUG_MEMBER_INT(d->duration);
			PRINT_DEBUG("    duration = %d (seconds)\n", d->duration);
			PRINT_DEBUG_MEMBER_INT(d->upid_type);
			PRINT_DEBUG(" d->upid_type = 0x%02x (%s)\n", d->upid_type, seg_upid_type(d->upid_type));
			PRINT_DEBUG_MEMBER_INT(d->upid_length);
			for (int j = 0; j < d->upid_length; j++) {
				PRINT_DEBUG_MEMBER_INT(d->upid[j]);
			}
			PRINT_DEBUG(" d->type_id = 0x%02x (%s)\n", d->type_id, seg_type_id(d->type_id));
			PRINT_DEBUG_MEMBER_INT(d->segment_num);
			PRINT_DEBUG_MEMBER_INT(d->segments_expected);
			PRINT_DEBUG_MEMBER_INT(d->duration_extension_frames);
			PRINT_DEBUG_MEMBER_INT(d->delivery_not_restricted_flag);
			PRINT_DEBUG_MEMBER_INT(d->web_delivery_allowed_flag);
			PRINT_DEBUG_MEMBER_INT(d->no_regional_blackout_flag);
			PRINT_DEBUG_MEMBER_INT(d->archive_allowed_flag);
			PRINT_DEBUG(" d->device_restrictions = 0x%02x (%s)\n", d->device_restrictions,
			       seg_device_restrictions(d->device_restrictions));
		} else if (o->opID == MO_INSERT_TIER_DATA) {
			struct klvanc_tier_data *d = &o->tier_data;
			PRINT_DEBUG_MEMBER_INT(d->tier_data);
		} else if (o->opID == MO_INSERT_TIME_DESCRIPTOR) {
			struct klvanc_time_descriptor_data *d = &o->time_data;
			PRINT_DEBUG_MEMBER_INT64(d->TAI_seconds);
			PRINT_DEBUG_MEMBER_INT(d->TAI_ns);
			PRINT_DEBUG_MEMBER_INT(d->UTC_offset);
		}

	}

	return KLAPI_OK;
}

static int dump_som(struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
#ifdef SPLICE_REQUEST_SINGLE
        struct klvanc_splice_request_data *d = &pkt->sr_data;
#endif
	struct klvanc_single_operation_message *m = &pkt->so_msg;

	PRINT_DEBUG("SCTE104 single_operation_message struct\n");
	PRINT_DEBUG_MEMBER_INT(pkt->payloadDescriptorByte);

	PRINT_DEBUG_MEMBER_INT(m->opID);
	PRINT_DEBUG("   opID = %s\n", som_operationName(m->opID));
	PRINT_DEBUG_MEMBER_INT(m->messageSize);
	PRINT_DEBUG("   message_size = %d bytes\n", m->messageSize);
	PRINT_DEBUG_MEMBER_INT(m->result);
	PRINT_DEBUG_MEMBER_INT(m->result_extension);
	PRINT_DEBUG_MEMBER_INT(m->protocol_version);
	PRINT_DEBUG_MEMBER_INT(m->AS_index);
	PRINT_DEBUG_MEMBER_INT(m->message_number);
	PRINT_DEBUG_MEMBER_INT(m->DPI_PID_index);

#ifdef SPLICE_REQUEST_SINGLE
	if (m->opID == SO_INIT_REQUEST_DATA) {
		PRINT_DEBUG_MEMBER_INT(d->splice_insert_type);
		PRINT_DEBUG("   splice_insert_type = %s\n", spliceInsertTypeName(d->splice_insert_type));
		PRINT_DEBUG_MEMBER_INT(d->splice_event_id);
		PRINT_DEBUG_MEMBER_INT(d->unique_program_id);
		PRINT_DEBUG_MEMBER_INT(d->pre_roll_time);
		PRINT_DEBUG_MEMBER_INT(d->brk_duration);
		PRINT_DEBUG("   break_duration = %d (1/10th seconds)\n", d->brk_duration);
		PRINT_DEBUG_MEMBER_INT(d->avail_num);
		PRINT_DEBUG_MEMBER_INT(d->avails_expected);
		PRINT_DEBUG_MEMBER_INT(d->auto_return_flag);
	} else
#endif
		PRINT_DEBUG("   unsupported m->opID = 0x%x\n", m->opID);

	for (int i = 0; i < pkt->payloadLengthBytes; i++)
		PRINT_DEBUG("%02x ", pkt->payload[i]);
	PRINT_DEBUG("\n");

	return KLAPI_OK;
}

int klvanc_alloc_SCTE_104(uint16_t opId, struct klvanc_packet_scte_104_s **outPkt)
{
	struct klvanc_packet_scte_104_s *pkt = (struct klvanc_packet_scte_104_s *) calloc(1, sizeof(struct klvanc_packet_scte_104_s));
	if (pkt == NULL)
		return -1;

	pkt->payloadDescriptorByte = 0x08;
	pkt->version = 0x08;
	pkt->continued_pkt = 0;
	pkt->following_pkt = 0;
	pkt->duplicate_msg = 0;

	/* Note, we take advantage of the SOM OpID field even with
	   Multiple Operation Messages */
	pkt->so_msg.opID = opId;
	if (opId == 0xffff) {
		/* Set some reasonable defaults for the MOM properties */
		pkt->mo_msg.rsvd = 0xffff;
	}

	*outPkt = pkt;
	return 0;
}

int klvanc_dump_SCTE_104(struct klvanc_context_s *ctx, void *p)
{
	struct klvanc_packet_scte_104_s *pkt = p;

	if (ctx->verbose)
		PRINT_DEBUG("%s() %p\n", __func__, (void *)pkt);

	if (pkt->so_msg.opID == SO_INIT_REQUEST_DATA)
		return dump_som(ctx, pkt);

	return dump_mom(ctx, pkt);
}

void klvanc_free_SCTE_104(void *p)
{
	struct klvanc_packet_scte_104_s *pkt = p;
	struct klvanc_multiple_operation_message *m;

	if (pkt == NULL)
		return;

	m = &pkt->mo_msg;
	for (int i = 0; i < m->num_ops; i++) {
		free(m->ops[i].data);
	}
	free(m->ops);

	free(pkt);
}

int parse_SCTE_104(struct klvanc_context_s *ctx,
		   struct klvanc_packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

	struct klvanc_packet_scte_104_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

        pkt->payloadDescriptorByte = hdr->payload[0];
        pkt->version               = (pkt->payloadDescriptorByte >> 3) & 0x03;
        pkt->continued_pkt         = (pkt->payloadDescriptorByte >> 2) & 0x01;
        pkt->following_pkt         = (pkt->payloadDescriptorByte >> 1) & 0x01;
        pkt->duplicate_msg         = pkt->payloadDescriptorByte & 0x01;

	/* We only support SCTE104 messages of type
	 * single_operation_message() that are completely
	 * self containined with a single VANC line, and
	 * are not continuation messages.
	 * Eg. payloadDescriptor value 0x08.
	 */
	if (pkt->payloadDescriptorByte != 0x08) {
		free(pkt);
		return -1;
	}

	/* First byte is the padloadDescriptor, the rest is the SCTE104 message...
	 * up to 200 bytes in length item 5.3.3 page 7 */
	for (int i = 0; i < 200; i++)
		pkt->payload[i] = hdr->payload[1 + i];

	struct klvanc_single_operation_message *m = &pkt->so_msg;
	struct klvanc_multiple_operation_message *mom = &pkt->mo_msg;

	/* Make sure we put the opID in the SOM reegardless of
	 * whether the message ius single or multiple.
	 * We rely on checking som.opID during the dump process
	 * to determinate the structure type.
	 */
	m->opID = pkt->payload[0] << 8 | pkt->payload[1];

#ifdef SPLICE_REQUEST_SINGLE
	if (m->opID == SO_INIT_REQUEST_DATA) {

		/* TODO: Will we ever see a trigger in a SOM. Interal discussion says
		 *       no. We'll leave this active for the time being, pending removal.
		 */
		m->messageSize      = pkt->payload[2] << 8 | pkt->payload[3];
		m->result           = pkt->payload[4] << 8 | pkt->payload[5];
		m->result_extension = pkt->payload[6] << 8 | pkt->payload[7];
		m->protocol_version = pkt->payload[8];
		m->AS_index         = pkt->payload[9];
		m->message_number   = pkt->payload[10];
		m->DPI_PID_index    = pkt->payload[11] << 8 | pkt->payload[12];

		struct klvanc_splice_request_data *d = &pkt->sr_data;

		d->splice_insert_type  = pkt->payload[13];
		d->splice_event_id     = pkt->payload[14] << 24 |
			pkt->payload[15] << 16 | pkt->payload[16] <<  8 | pkt->payload[17];
		d->unique_program_id   = pkt->payload[18] << 8 | pkt->payload[19];
		d->pre_roll_time       = pkt->payload[20] << 8 | pkt->payload[21];
		d->brk_duration        = pkt->payload[22] << 8 | pkt->payload[23];
		d->avail_num           = pkt->payload[24];
		d->avails_expected     = pkt->payload[25];
		d->auto_return_flag    = pkt->payload[26];

		/* TODO: We only support spliceStart_immediate and spliceEnd_immediate */
		switch (d->splice_insert_type) {
		case SPLICESTART_IMMEDIATE:
		case SPLICEEND_IMMEDIATE:
			break;
		default:
			/* We don't support this splice command */
			PRINT_ERR("%s() splice_insert_type 0x%x, error.\n", __func__, d->splice_insert_type);
			free(pkt);
			return -1;
		}
	} else
#endif
	if (m->opID == 0xFFFF /* Multiple Operation Message */) {
		mom->rsvd                    = pkt->payload[0] << 8 | pkt->payload[1];
		mom->messageSize             = pkt->payload[2] << 8 | pkt->payload[3];
		mom->protocol_version        = pkt->payload[4];
		mom->AS_index                = pkt->payload[5];
		mom->message_number          = pkt->payload[6];
		mom->DPI_PID_index           = pkt->payload[7] << 8 | pkt->payload[8];
		mom->SCTE35_protocol_version = pkt->payload[9];

		unsigned char *p = &pkt->payload[10];
		p = parse_mom_timestamp(ctx, p, &mom->timestamp);
		
		mom->num_ops = *(p++);
		mom->ops = calloc(mom->num_ops, sizeof(struct klvanc_multiple_operation_message_operation));
		if (!mom->ops) {
			PRINT_ERR("%s() unable to allocate momo ram, error.\n", __func__);
			free(pkt);
			return -1;
		}

		for (int i = 0; i < mom->num_ops; i++) {
			struct klvanc_multiple_operation_message_operation *o = &mom->ops[i];
			o->opID = *(p + 0) << 8 | *(p + 1);
			o->data_length = *(p + 2) << 8 | *(p + 3);
			o->data = malloc(o->data_length);
			if (!o->data) {
				PRINT_ERR("%s() Unable to allocate memory for mom op, error.\n", __func__);
				free(pkt);
				return -1;
			} else {
				memcpy(o->data, p + 4, o->data_length);
			}
			p += (4 + o->data_length);

			if (o->opID == MO_SPLICE_REQUEST_DATA)
				parse_splice_request_data(ctx, o->data, &o->sr_data);
			else if (o->opID == MO_INSERT_DESCRIPTOR_REQUEST_DATA)
				parse_descriptor_request_data(o->data, &o->descriptor_data,
					o->data_length - 1);
			else if (o->opID == MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA)
				parse_avail_request_data(o->data,
							 &o->avail_descriptor_data);
			else if (o->opID == MO_INSERT_DTMF_REQUEST_DATA)
				parse_dtmf_request_data(o->data, &o->dtmf_data);
			else if (o->opID == MO_INSERT_SEGMENTATION_REQUEST_DATA)
				parse_segmentation_request_data(o->data, &o->segmentation_data);
			else if (o->opID == MO_PROPRIETARY_COMMAND_REQUEST_DATA)
				parse_proprietary_command_request_data(o->data, &o->proprietary_data,
								       o->data_length);
			else if (o->opID == MO_INSERT_TIER_DATA)
				parse_tier_data(o->data, &o->tier_data);
			else if (o->opID == MO_INSERT_TIME_DESCRIPTOR)
				parse_time_descriptor(o->data, &o->time_data);

#if 0
			PRINT_DEBUG("opID = 0x%04x [%s], length = 0x%04x : ", o->opID, mom_operationName(o->opID), o->data_length);
			hexdump(ctx, o->data, o->data_length, 32, "");
#endif
		}

		/* We'll parse this message but we'll only look for INIT_REQUEST_DATA
		 * sub messages, and construct a splice_request_data message.
		 * The rest of the message types will be ignored.
		 */
	}
	else {
		PRINT_ERR("%s() Unsupported opID = %x, error.\n", __func__, m->opID);
		free(pkt);
		return -1;
	}

	if (ctx->callbacks && ctx->callbacks->scte_104)
		ctx->callbacks->scte_104(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_convert_SCTE_104_to_packetBytes(struct klvanc_context_s *ctx,
					   const struct klvanc_packet_scte_104_s *pkt,
					   uint8_t **bytes, uint16_t *byteCount)
{
	const struct klvanc_multiple_operation_message *m;

	if (!pkt || !bytes) {
		return -1;
	}

	if (pkt->so_msg.opID != 0xffff) {
		/* We don't currently support anything but Multiple Operation
		   Messages */
		PRINT_ERR("msg opid not 0xffff.  Provided=0x%x\n", pkt->so_msg.opID);
		return -1;
	}

	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	*bytes = malloc(255);
	if (*bytes == NULL) {
		klbs_free(bs);
		return -1;
	}

	m = &pkt->mo_msg;

	/* Serialize the SCTE 104 into a binary blob */
	klbs_write_set_buffer(bs, *bytes, 255);

	klbs_write_bits(bs, 0xffff, 16); /* reserved */

	klbs_write_bits(bs, m->messageSize, 16);
	klbs_write_bits(bs, m->protocol_version, 8);
	klbs_write_bits(bs, m->AS_index, 8);
	klbs_write_bits(bs, m->message_number, 8);
	klbs_write_bits(bs, m->DPI_PID_index, 16);
	klbs_write_bits(bs, m->SCTE35_protocol_version, 8);
	klbs_write_bits(bs, m->timestamp.time_type, 8);

	const struct klvanc_multiple_operation_message_timestamp *ts = &m->timestamp;
	switch(ts->time_type) {
	case 1:
		klbs_write_bits(bs, ts->time_type_1.UTC_seconds, 32);
		klbs_write_bits(bs, ts->time_type_1.UTC_microseconds, 16);
		break;
	case 2:
		klbs_write_bits(bs, ts->time_type_2.hours, 8);
		klbs_write_bits(bs, ts->time_type_2.minutes, 8);
		klbs_write_bits(bs, ts->time_type_2.seconds, 8);
		klbs_write_bits(bs, ts->time_type_2.frames, 8);
		break;
	case 3:
		klbs_write_bits(bs, ts->time_type_3.GPI_number, 8);
		klbs_write_bits(bs, ts->time_type_3.GPI_edge, 8);
		break;
	case 0:
		/* No time standard defined */
		break;
	default:
		PRINT_ERR("%s() unsupported time_type 0x%x, assuming no time.\n",
			__func__, ts->time_type);
		break;
	}

	klbs_write_bits(bs, m->num_ops, 8);
	for (int i = 0; i < m->num_ops; i++) {
		unsigned char *outBuf = NULL;
		uint16_t outSize = 0;
		const struct klvanc_multiple_operation_message_operation *o = &m->ops[i];
		switch (o->opID) {
		case MO_SPLICE_REQUEST_DATA:
			gen_splice_request_data(&o->sr_data, &outBuf, &outSize);
			break;
		case MO_SPLICE_NULL_REQUEST_DATA:
			gen_splice_null_request_data(&outBuf, &outSize);
			break;
		case MO_TIME_SIGNAL_REQUEST_DATA:
			gen_time_signal_request_data(&o->timesignal_data, &outBuf, &outSize);
			break;
		case MO_INSERT_DESCRIPTOR_REQUEST_DATA:
			gen_descriptor_request_data(&o->descriptor_data, &outBuf, &outSize);
			break;
		case MO_INSERT_DTMF_REQUEST_DATA:
			gen_dtmf_request_data(&o->dtmf_data, &outBuf, &outSize);
			break;
		case MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA:
			gen_avail_request_data(&o->avail_descriptor_data, &outBuf, &outSize);
			break;
		case MO_INSERT_SEGMENTATION_REQUEST_DATA:
			gen_segmentation_request_data(&o->segmentation_data, &outBuf, &outSize);
			break;
		case MO_PROPRIETARY_COMMAND_REQUEST_DATA:
			gen_proprietary_command_request_data(&o->proprietary_data, &outBuf, &outSize);
			break;
		case MO_INSERT_TIER_DATA:
			gen_tier_data(&o->tier_data, &outBuf, &outSize);
			break;
		case MO_INSERT_TIME_DESCRIPTOR:
			gen_time_descriptor(&o->time_data, &outBuf, &outSize);
			break;
		default:
			PRINT_ERR("Unknown operation type 0x%04x\n", o->opID);
			continue;
		}
		/* FIXME */

		klbs_write_bits(bs, o->opID, 16);
		klbs_write_bits(bs, outSize, 16);
		for (int j = 0; j < outSize; j++) {
			klbs_write_bits(bs, outBuf[j], 8);
		}
		free(outBuf);
	}
	klbs_write_buffer_complete(bs);

	/* Recompute the total message size now that everything has been serialized to
	   a single buffer. */
	uint16_t buffer_size = klbs_get_byte_count(bs);
	(*bytes)[2] = buffer_size >> 8;
	(*bytes)[3] = buffer_size & 0xff;

#if 0
	PRINT_DEBUG("Resulting buffer size=%d\n", klbs_get_byte_count(bs));
	PRINT_DEBUG(" ->payload  = ");
	for (int i = 0; i < klbs_get_byte_count(bs); i++) {
		PRINT_DEBUG("%02x ", (*bytes)[i]);
	}
	PRINT_DEBUG("\n");
#endif

	*byteCount = klbs_get_byte_count(bs);
	klbs_free(bs);

	return 0;
}

int klvanc_convert_SCTE_104_packetbytes_to_SMPTE_2010(struct klvanc_context_s *ctx,
                                                      uint8_t *inBytes, uint16_t inCount,
                                                      uint8_t **bytes, uint16_t *byteCount)
{
	uint8_t *out;
	uint16_t len;

	/* For now we just support standalone 2010 packets (i.e. type 0x08), which is
	   all that is needed for SCTE-104 simple profile.  We don't support fragmenting
	   a SCTE-104 packet across multiple 2010 packets */

	/* Maximum permitted size for a Multiple Operation Message within a single
	   SMPTE 2010 packet is 254 (ST 2010:2008 Sec 5.4). */
	if (inCount > 254)
		return -1;

	len = inCount + 1;
	out = malloc(len);
	if (out == NULL)
		return -1;

	out[0] = 0x08; /* SMPTE 2010 Payload Descriptor */
	memcpy(&out[1], inBytes, inCount);

	*bytes = out;
	*byteCount = len;
	return 0;
}

int klvanc_convert_SCTE_104_to_words(struct klvanc_context_s *ctx,
				     struct klvanc_packet_scte_104_s *pkt,
				     uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	uint8_t *s2010Packet;
	uint16_t s2010Count;
	int ret;

	ret = klvanc_convert_SCTE_104_to_packetBytes(ctx, pkt, &buf, &byteCount);
	if (ret != 0)
		return -1;

	ret = klvanc_convert_SCTE_104_packetbytes_to_SMPTE_2010(ctx, buf, byteCount,
								&s2010Packet, &s2010Count);
	free(buf);
	if (ret != 0) {
		return -1;
	}

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x07, 0x41, s2010Packet, s2010Count, words, wordCount, 10);
	free(s2010Packet);

	return 0;
}

int klvanc_SCTE_104_Add_MOM_Op(struct klvanc_packet_scte_104_s *pkt, uint16_t opId,
			       struct klvanc_multiple_operation_message_operation **op)
{
	struct klvanc_multiple_operation_message *mom = &pkt->mo_msg;
	mom->num_ops++;
	mom->ops = realloc(mom->ops,
			   mom->num_ops * sizeof(struct klvanc_multiple_operation_message_operation));
	*op = &mom->ops[mom->num_ops - 1];
	memset(*op, 0, sizeof(struct klvanc_multiple_operation_message_operation));
	(*op)->opID = opId;

	return 0;
}
