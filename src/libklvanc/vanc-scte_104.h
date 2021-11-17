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
 * @brief	SCTE-104 Automation System to Compression System Communications Applications Program Interface 
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
#define MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA  0x10a
#define MO_INSERT_SEGMENTATION_REQUEST_DATA  0x10b
#define MO_PROPRIETARY_COMMAND_REQUEST_DATA  0x10c
#define MO_INSERT_TIER_DATA  0x10f
#define MO_INSERT_TIME_DESCRIPTOR  0x110

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
struct klvanc_single_operation_message
{
	/* single_operation_message */
	/* SCTE104 Spec 2012 - table 7.1 */
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
struct klvanc_multiple_operation_message_timestamp
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
struct klvanc_splice_request_data
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
struct klvanc_time_signal_request_data
{
	/* SCTE 104 Table 8-23 */
	unsigned short pre_roll_time;	/* In milliseconds */
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_avail_descriptor_request_data
{
	/* SCTE 104 Table 8-26 */
	unsigned int num_provider_avails;
	uint32_t provider_avail_id[255];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_insert_descriptor_request_data
{
	/* SCTE 104 Table 8-27 */
	unsigned int descriptor_count;
	unsigned int total_length;
	unsigned char descriptor_bytes[255];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_dtmf_descriptor_request_data
{
	/* SCTE 104 Table 8-28 */
	unsigned char pre_roll_time;	/* In 1/10's of a second */
	unsigned int dtmf_length;
	char dtmf_char[7];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_segmentation_descriptor_request_data
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

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_time_descriptor_data
{
	/* SCTE 104 2015 Table 9-32 */
	uint64_t TAI_seconds;
	unsigned int TAI_ns;
	unsigned int UTC_offset;
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_proprietary_command_request_data
{
	/* SCTE 104 Table 9-30 */
	unsigned int proprietary_id;
	unsigned int proprietary_command;
	unsigned int data_length;
	unsigned char proprietary_data[255];
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_tier_data
{
	/* SCTE 104 Table 9-31 */
	unsigned short tier_data;
};

struct klvanc_multiple_operation_message_operation {
	unsigned short opID;
	unsigned short data_length;
	unsigned char *data;
	union {
		struct klvanc_splice_request_data sr_data;
		struct klvanc_time_signal_request_data timesignal_data;
		struct klvanc_dtmf_descriptor_request_data dtmf_data;
		struct klvanc_segmentation_descriptor_request_data segmentation_data;
		struct klvanc_avail_descriptor_request_data avail_descriptor_data;
		struct klvanc_insert_descriptor_request_data descriptor_data;
		struct klvanc_proprietary_command_request_data proprietary_data;
		struct klvanc_tier_data tier_data;
		struct klvanc_time_descriptor_data time_data;
	};
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_multiple_operation_message
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
	struct klvanc_multiple_operation_message_timestamp timestamp;
	unsigned char num_ops;
	struct klvanc_multiple_operation_message_operation *ops;
};

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_scte_104_s
{
	struct klvanc_packet_header_s hdr;

	/* See SMPTE 2010:2008 page 5 */
	unsigned char payloadDescriptorByte;
	int version;
	int continued_pkt;
	int following_pkt;
	int duplicate_msg;

	unsigned char payload[256]; /* ST2010-2008 section 5.3.3 */
	unsigned int payloadLengthBytes;

	struct klvanc_single_operation_message so_msg;
	struct klvanc_multiple_operation_message mo_msg;
};


/**
 * @brief       Create a SCTE-104 structure
 * @param[in]	uint16_t opId - SCTE-104 Operation to be created.  Note that at present only
 *              Multiple Operation Messages (type 0xffff) are supported.
 * @param[out]	struct klvanc_packet_scte_104_s **pkt - The resulting SCTE-104 VANC entry
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_alloc_SCTE_104(uint16_t opId, struct klvanc_packet_scte_104_s **pkt);

/**
 * @brief       Print out the properties of a SCTE-104 structure
 * @param[in]	struct vanc_context_s *ctx, pointer to an existing libklvanc context structure
 * @param[in]	void *p - pointer to klvanc_packet_scte_104_s packet to be processed
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_SCTE_104(struct klvanc_context_s *ctx, void *p);

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	void *p - Pointer to struct (klvanc_packet_scte_104_s *)
 */
void klvanc_free_SCTE_104(void *p);

/**
 * @brief       Encapsulate a SCTE-104 packet into a SMPTE 2010 packet (suitable for embedding
 *              in a VANC packet).  Currently only creation of unfragmented 2010 packets is supported.\n\n
 *              On success, caller MUST free the resulting *bytes array.\n\n
 *              See SMPTE Standard ST2010:2008 "Vertical Ancillary Data Mapping of ANSI/SCTE 104 Messages"
 *              for details of the output packet format.
 * @param[in]	struct vanc_context_s *ctx, pointer to an existing libklvanc context structure
 * @param[in]	uint8_t *inBytes - Pointer to SCTE-104 packet bytes
 * @param[in]	uint16_t *inCount - Number of bytes in SCTE-104 packet
 * @param[out]	uint8_t **bytes - Pointer to resulting SMPTE 2010 packet
 * @param[out]	uint16_t *byteCount - Number of bytes in resulting SMPTE 2010 packet
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_SCTE_104_packetbytes_to_SMPTE_2010(struct klvanc_context_s *ctx,
                                                      uint8_t *inBytes, uint16_t inCount,
                                                      uint8_t **bytes, uint16_t *byteCount);

/**
 * @brief	Convert type struct packet_scte_104_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct vanc_context_s *ctx, pointer to an existing libklvanc context structure
 * @param[in]	struct packet_scte_104_s *pkt - A SCTE-104 VANC entry, received from the SCTE-104 parser
 * @param[out]	uint16_t **words - An array of words representing a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_SCTE_104_to_words(struct klvanc_context_s *ctx,
				     struct klvanc_packet_scte_104_s *pkt,
				     uint16_t **words, uint16_t *wordCount);

/**
 * @brief	Convert type struct packet_scte_104_s into a block of bytes which can be
 *              serialized over TCP or embedded in an SMPTE 2010 packet to be sent over SDI VANC.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct vanc_context_s *ctx, pointer to an existing libklvanc context structure
 * @param[in]	struct packet_scte_104_s *pkt - A SCTE-104 VANC entry, received from the SCTE-104 parser
 * @param[out]	uint8_t **bytes - An array of words representing a fully formed SCTE-104 packet.
 * @param[out]	uint16_t *byteCount - Number of bytes in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_SCTE_104_to_packetBytes(struct klvanc_context_s *ctx,
					   const struct klvanc_packet_scte_104_s *pkt,
					   uint8_t **bytes, uint16_t *byteCount);

int klvanc_SCTE_104_Add_MOM_Op(struct klvanc_packet_scte_104_s *pkt, uint16_t opId,
			       struct klvanc_multiple_operation_message_operation **op);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_SCTE_104_H */
