#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dump_EIA_608(struct vanc_context_s *ctx, void *p)
{
	struct packet_eia_608_s *pkt = p;

	if (ctx->verbose)
		printf("%s() %p\n", __func__, (void *)pkt);

	printf("%s() EIA608: %02x %02x %02x : marker-bits %02x cc_valid %d cc_type %d cc_data_1 %02x cc_data_2 %02x\n",
		__func__,
		pkt->payload[0],
		pkt->payload[1],
		pkt->payload[2],
		pkt->marker_bits,
		pkt->cc_valid,
		pkt->cc_type,
		pkt->cc_data_1,
		pkt->cc_data_2);

	return KLAPI_OK;
}

int parse_EIA_608(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_eia_608_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

        /* Parsed */
	pkt->payload[0] = hdr->payload[0];
	pkt->payload[1] = hdr->payload[1];
	pkt->payload[2] = hdr->payload[2];

	pkt->marker_bits = pkt->payload[0] >> 3;
	pkt->cc_valid = (pkt->payload[0] >> 2) & 0x01;
	pkt->cc_type = pkt->payload[0] & 0x03;
	pkt->cc_data_1 = pkt->payload[1];
	pkt->cc_data_2 = pkt->payload[2];

	if (ctx->callbacks && ctx->callbacks->eia_608)
		ctx->callbacks->eia_608(ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

