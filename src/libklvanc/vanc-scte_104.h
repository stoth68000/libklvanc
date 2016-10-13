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
#define MO_INIT_REQUEST_DATA     0x101

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

struct multiple_operation_message_operation {
	unsigned short opID;
	unsigned short data_length;
	unsigned char *data;
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
	struct splice_request_data sr_data;
	struct multiple_operation_message mo_msg;
};

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 */
int dump_SCTE_104(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_SCTE_104_H */
