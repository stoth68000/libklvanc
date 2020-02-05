
#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Find spec at 

int klvanc_dump_SDP(struct klvanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);
	struct klvanc_packet_sdp_s *pkt = p;
	PRINT_DEBUG("Subtitle Description Packet struct\n");
	PRINT_DEBUG_MEMBER_INT(pkt->identifier)
	PRINT_DEBUG_MEMBER_INT(pkt->format_code);
	for (int i=0; i<5; ++i) {
		PRINT_DEBUG_MEMBER_INT(pkt->descriptors[i].line);
		PRINT_DEBUG_MEMBER_INT(pkt->descriptors[i].field);
		for (int j=0; j<45; ++j) {
			PRINT_DEBUG(" %02x", pkt->descriptors[i].data[j]);
		}
		PRINT_DEBUG("\n");
	}
	PRINT_DEBUG_MEMBER_INT(pkt->sequence_counter);
	PRINT_DEBUG("\n");
	return KLAPI_OK;
}

int parse_SDP(struct klvanc_context_s *ctx,
				struct klvanc_packet_header_s *hdr, void **pp)
{
	int i;
	int payloadBIndex = 0;

	if (ctx->callbacks == NULL || ctx->callbacks->sdp == NULL)
		return KLAPI_OK;

	if (ctx->verbose)
		PRINT_DEBUG("%s()\n", __func__);

        if ( ( (hdr->payload[0] & 0x0ff) != 0x51)||( (hdr->payload[1] & 0x0ff) != 0x15))
	{
//		if (ctx->verbose)
			PRINT_ERR("Identifiers for Subtitling Description Packet don't match: %x %x\n", hdr->payload[0], hdr->payload[1]);
		return -EINVAL;
	}


	struct klvanc_packet_sdp_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;
	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	uint8_t length = hdr->payload[2] & 0x00ff;

	pkt->identifier = ((uint16_t)(hdr->payload[0]&0xff))<<8 | (hdr->payload[1]&0xff);
	pkt->format_code = hdr->payload[3]&0xff;
	for (i=0; (i<length - 4 - 4) && (i<5); ++i)
	{
       		pkt->descriptors[i].line = hdr->payload[4+i] &0x1f;
		pkt->descriptors[i].field = hdr->payload[4+i]&0x80; 
		if ( (hdr->payload[4+i] & 0xff) != 0x00)
		{
			for (int j=0; j<45; ++j)
				pkt->descriptors[i].data[j] = hdr->payload[9+(45*payloadBIndex)+j];
			payloadBIndex++;
		}
	}
	pkt->sequence_counter = ((uint16_t)(hdr->payload[9+(45*payloadBIndex)+1]&0xff)<<8) | (hdr->payload[9+(45*payloadBIndex)+2]&0xff);
	
	ctx->callbacks->sdp(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

int klvanc_create_SDP(struct klvanc_packet_sdp_s **pkt)
{
	struct klvanc_packet_sdp_s *p = calloc(1, sizeof(*p));
	if (p == NULL)
		return -ENOMEM;

	*pkt = p;
	return 0;
}

void klvanc_destroy_SDP(struct klvanc_packet_sdp_s *pkt)
{
	free(pkt);
}
