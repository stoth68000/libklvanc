
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

	for (int i = 0; i < 5; ++i) {
		PRINT_DEBUG_MEMBER_INT(pkt->descriptors[i].line);
		PRINT_DEBUG_MEMBER_INT(pkt->descriptors[i].field);
		for (int j = 0; j < 45; ++j) {
			PRINT_DEBUG(" %02x", pkt->descriptors[i].data[j]);
		}
		PRINT_DEBUG("\n");
	}

	PRINT_DEBUG_MEMBER_INT(pkt->sequence_counter);
	PRINT_DEBUG(" pkt->checksum = 0x%02x (%s)\n",
		    pkt->checksum,
		    pkt->checksum_valid == 1 ? "VALID" : "INVALID");
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

	if (((hdr->payload[0] & 0x0ff) != 0x51)
	    || ((hdr->payload[1] & 0x0ff) != 0x15)) {
//              if (ctx->verbose)
		PRINT_ERR
		    ("Identifiers for Subtitling Description Packet don't match: %x %x\n",
		     hdr->payload[0], hdr->payload[1]);
		return -EINVAL;
	}

	struct klvanc_packet_sdp_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	uint8_t length = hdr->payload[2] & 0x00ff;

	pkt->identifier =
	    ((uint16_t) (hdr->payload[0] & 0xff)) << 8 | (hdr->
							  payload[1] & 0xff);

	pkt->format_code = hdr->payload[3] & 0xff;
	for (i = 0; (i < length - 4 - 4) && (i < 5); ++i) {
		pkt->descriptors[i].line = hdr->payload[4 + i] & 0x1f;
		pkt->descriptors[i].field = (hdr->payload[4 + i] & 0x80) >> 7;
		if ((hdr->payload[4 + i] & 0xff) != 0x00) {
			for (int j = 0; j < 45; ++j)
				pkt->descriptors[i].data[j] =
				    hdr->payload[9 + (45 * payloadBIndex) + j];
			payloadBIndex++;
		}
	}
	
	pkt->sequence_counter =
	    ((uint16_t) (hdr->payload[9 + (45 * payloadBIndex) + 1] & 0xff) <<
	     8) | (hdr->payload[9 + (45 * payloadBIndex) + 2] & 0xff);
	pkt->checksum = hdr->payload[length - 1];

	/* Validate checksum */
	uint8_t sum = 0;
	for (int i = 0; i < hdr->payloadLengthWords; i++) {
		sum += hdr->payload[i];
	}
	if (sum == 0)
		pkt->checksum_valid = 1;
	else
		pkt->checksum_valid = 0;

	ctx->callbacks->sdp(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

void klvanc_finalize_SDP(struct klvanc_packet_sdp_s *pkt, uint16_t seqNum)
{
	pkt->sequence_counter = seqNum;
}

int klvanc_convert_SDP_to_packetBytes(struct klvanc_packet_sdp_s *pkt, uint8_t **bytes, uint16_t *byteCount)
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

	/* Serialize the SDP struct into a binary blob */
	klbs_write_set_buffer(bs, *bytes, 255);

	klbs_write_bits(bs, pkt->identifier, 16);
	klbs_write_bits(bs, 0x00, 8); /* Length, we'll update afterward */
	klbs_write_bits(bs, pkt->format_code, 8);
	for (int i = 0; i < 5; i++) {
		if (pkt->descriptors[i].line != 0) {
			klbs_write_bits(bs, pkt->descriptors[i].field, 1);
			klbs_write_bits(bs, 0x00, 2); /* OP-47 Sec 5.4.2 */
			klbs_write_bits(bs, pkt->descriptors[i].line, 5);
		} else {
			klbs_write_bits(bs, 0x00, 8);
		}
	}

	for (int i = 0; i < 5; i++) {
		if (pkt->descriptors[i].line != 0) {
			for (int j = 0; j < 45; j++)
				klbs_write_bits(bs, pkt->descriptors[i].data[j], 8);
		}
	}
	klbs_write_bits(bs, 0x74, 8); /* Footer id */
	klbs_write_bits(bs, pkt->sequence_counter, 16);
	klbs_write_bits(bs, 0x00, 8); /* Checksum (we'll update afterward) */
	klbs_write_buffer_complete(bs);

	uint8_t *payload = *bytes;
	uint8_t packet_length = klbs_get_byte_count(bs);

	/* Length */
	payload[2] = packet_length; /* Length */

	/* Checksum */
	uint8_t sum = 0;
	for (int i = 0; i < packet_length - 1; i++) {
		sum += payload[i];
	}
	payload[packet_length - 1] = ~sum + 1;

	*byteCount = packet_length;
	klbs_free(bs);

	return 0;
}

int klvanc_convert_SDP_to_words(struct klvanc_packet_sdp_s *pkt, uint16_t **words, uint16_t *wordCount)
{
	uint8_t *buf;
	uint16_t byteCount;
	int ret;

	ret = klvanc_convert_SDP_to_packetBytes(pkt, &buf, &byteCount);
	if (ret != 0)
		return ret;

	/* Create the final array of VANC bytes (with correct DID/SDID,
	   checksum, etc) */
	klvanc_sdi_create_payload(0x02, 0x43, buf, byteCount, words, wordCount, 10);

	free(buf);

	return 0;
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
