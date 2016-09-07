#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int dump_SCTE_104(struct vanc_context_s *ctx, void *p)
{
	struct packet_scte_104_s *pkt = p;

	if (ctx->verbose)
		printf("%s() %p\n", __func__, (void *)pkt);

	return KLAPI_OK;
}

int parse_SCTE_104(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_scte_104_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	if (ctx->callbacks && ctx->callbacks->scte_104)
		ctx->callbacks->scte_104(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

