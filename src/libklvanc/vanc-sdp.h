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

struct klvanc_sdp_desc_s
{
	uint8_t line;  // Line number, only 5 bits
	uint8_t field; // Field number, only 1 bit, 0 = even field	 
	uint8_t data[45]; // WSS Teletext data representing a subtitle line
};

struct klvanc_packet_sdp_s
{
	struct klvanc_packet_header_s hdr;
	uint16_t identifier;
	enum klvanc_sdp_format_code_e format_code;
	struct klvanc_sdp_desc_s descriptors[5];
	uint16_t sequence_counter;
};

int klvanc_dump_SDP(struct klvanc_context_s *ctx, void *p);

int klvanc_create_SDP(struct klvanc_packet_sdp_s **pkt);

void klvanc_destroy_SDP(struct klvanc_packet_sdp_s *pkt);

int klvanc_convert_SDP_to_words(struct klvanc_packet_sdp_s *pkt, uint16_t **words, uint16_t *wordCount);

int klvanc_convert_SDP_to_packetBytes(struct klvanc_packet_sdp_s *pkt, uint8_t **bytes, uint16_t *byteCount);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_SDP_H */
