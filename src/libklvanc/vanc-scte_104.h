/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_SCTE_104_H
#define _VANC_SCTE_104_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

#define INIT_REQUEST_DATA     0x01
#define SPLICESTART_IMMEDIATE 0x02
#define SPLICEEND_IMMEDIATE   0x04

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
};

int dump_SCTE_104(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_SCTE_104_H */
