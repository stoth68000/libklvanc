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
 * @file	vanc-scte_104.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _VANC_SCTE_104_H
#define _VANC_SCTE_104_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
#define SO_INIT_REQUEST_DATA     0x001

/**
 * @brief       TODO - Brief description goes here.
 */
#define MO_SPLICE_REQUEST_DATA     0x101

/**
 * @brief       TODO - Brief description goes here.
 */
#define MO_SPLICE_NULL_REQUEST_DATA  0x102

/**
 * @brief       TODO - Brief description goes here.
 */
#define MO_TIME_SIGNAL_REQUEST_DATA  0x104
#define MO_INSERT_DESCRIPTOR_REQUEST_DATA    0x108
#define MO_INSERT_DTMF_REQUEST_DATA  0x109
#define MO_INSERT_SEGMENTATION_REQUEST_DATA  0x10b

/**
 * @brief       TODO - Brief description goes here.
 */
#define SPLICESTART_NORMAL    0x01

/**
 * @brief       TODO - Brief description goes here.
 */
#define SPLICESTART_IMMEDIATE 0x02

/**
 * @brief       TODO - Brief description goes here.
 */
#define SPLICEEND_NORMAL      0x03

/**
 * @brief       TODO - Brief description goes here.
 */
#define SPLICEEND_IMMEDIATE   0x04

/**
 * @brief       TODO - Brief description goes here.
 */
#define SPLICE_CANCEL         0x05

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__UNIQUE_PROGRAM_ID(pkt) ((pkt)->sr_data.unique_program_id)

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__SPLICE_EVENT_ID(pkt) ((pkt)->sr_data.splice_event_id)

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__AUTO_RETURN_FLAGS(pkt) ((pkt)->sr_data.auto_return_flag)

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__DURATION(pkt) ((pkt)->sr_data.brk_duration)

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__AVAIL_NUM(pkt) ((pkt)->sr_data.avail_num)

/**
 * @brief       TODO - Brief description goes here.
 */
#define SCTE104_SR_DATA_FIELD__AVAILS_EXPECTED(pkt) ((pkt)->sr_data.avails_expected)

/**
 * @brief       TODO - Brief description goes here.
 */
struct single_operation_message
{
	/* single_operation_message */
	/* SCTE Spec table 7.1 */
	unsigned short opID;
	unsigned short messageSize;
	unsigned short result;
	unsigned short result_extension;
	unsigned char protocol_version;
	unsigned char AS_index;
	unsigned char message_number;
	unsigned short DPI_PID_index;
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct multiple_operation_message_timestamp
{
	/* SCTE Spec table 11.2 */
	unsigned char time_type;
	union {
		struct {
			unsigned int UTC_seconds;
			unsigned short UTC_microseconds;
		} time_type_1;
		struct {
			unsigned char hours;
			unsigned char minutes;
			unsigned char seconds;
			unsigned char frames;
		} time_type_2;
		struct {
			unsigned char GPI_number;
			unsigned char GPI_edge;
		} time_type_3;
	};
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct splice_request_data
{
	/* SCTE 104 Table 8-5 */
	unsigned int splice_insert_type;
	unsigned int splice_event_id;
	unsigned short unique_program_id;
	unsigned short pre_roll_time;	/* In 1/10's of a second */
	unsigned short brk_duration;	/* In 1/10's of a second */
	unsigned char avail_num;
	unsigned char avails_expected;
	unsigned char auto_return_flag;
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct insert_descriptor_request_data
{
	/* SCTE 104 Table 8-27 */
	unsigned int descriptor_count;
	unsigned int total_length;
	unsigned char descriptor_bytes[255];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct dtmf_descriptor_request_data
{
	/* SCTE 104 Table 8-28 */
	unsigned short pre_roll_time;	/* In 1/10's of a second */
	unsigned int dtmf_length;
	char dtmf_char[7];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct segmentation_descriptor_request_data
{
	/* SCTE 104 Table 8-29 */
	unsigned int event_id;
	unsigned int event_cancel_indicator;
	unsigned int duration; /* In seconds */
	unsigned int upid_type;
	unsigned int upid_length;
	unsigned char upid[255];
	unsigned int type_id;
	unsigned int segment_num;
	unsigned int segments_expected;
	unsigned int duration_extension_frames;
	unsigned int delivery_not_restricted_flag;
	unsigned int web_delivery_allowed_flag;
	unsigned int no_regional_blackout_flag;
	unsigned int archive_allowed_flag;
	unsigned int device_restrictions;
};

struct multiple_operation_message_operation {
	unsigned short opID;
	unsigned short data_length;
	unsigned char *data;
	union {
		struct splice_request_data sr_data;
		struct dtmf_descriptor_request_data dtmf_data;
		struct segmentation_descriptor_request_data segmentation_data;
		struct insert_descriptor_request_data descriptor_data;
	};
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct multiple_operation_message
{
	/* multiple_operation_message */
	/* SCTE Spec table 7.2 */
	unsigned short rsvd;
	unsigned short messageSize;
	unsigned char protocol_version;
	unsigned char AS_index;
	unsigned char message_number;
	unsigned short DPI_PID_index;
	unsigned char SCTE35_protocol_version;
	struct multiple_operation_message_timestamp timestamp;
	unsigned char num_ops;
	struct multiple_operation_message_operation *ops;
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct packet_scte_104_s
{
	struct packet_header_s hdr;

	/* See SMPTE 2010:2008 page 5 */
	unsigned char payloadDescriptorByte;
	int version;
	int continued_pkt;
	int following_pkt;
	int duplicate_msg;

	unsigned char payload[256];
	unsigned int payloadLengthBytes;

	struct single_operation_message so_msg;
	struct multiple_operation_message mo_msg;
};

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int dump_SCTE_104(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_SCTE_104_H */
