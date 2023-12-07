
/**
 * @file	vanc-sdp.h
 * @author	Yves De Muyter <yves@alfavisio.be>, tidy-ups: Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2023 Kernel Labs Inc. All Rights Reserved.
 * @brief	WST / OP-47 / RDD8 - Subtitling Distribution Packet (SDP) Format
 */

#ifndef _VANC_SDP_H
#define _VANC_SDP_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif

enum klvanc_sdp_format_code_e
{
	SDP_WSS_TELETEXT = 0x02
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_sdp_desc_s
{
	uint8_t line;  // Line number, only 5 bits
	uint8_t field; // Field number, only 1 bit, 0 = even field	 
	uint8_t data[45]; // WSS Teletext data representing a subtitle line
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_sdp_s
{
	struct klvanc_packet_header_s hdr;
	uint16_t identifier;
	enum klvanc_sdp_format_code_e format_code;
	struct klvanc_sdp_desc_s descriptors[5];
	uint16_t sequence_counter;
};

int klvanc_dump_SDP(struct klvanc_context_s *ctx, void *p);

/**
 * @brief       Allocate a new SDP packet
 * @param[out]	struct klvanc_packet_sdp_s **pkt - object created
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_SDP(struct klvanc_packet_sdp_s **pkt);

/**
 * @brief       Free a previously allocated SDP packet.
 * @param[in]	struct klvanc_packet_sdp_s *pkt - object to be freed
 */
void klvanc_destroy_SDP(struct klvanc_packet_sdp_s *pkt);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_SDP_H */
