#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *afd_to_string(enum payload_afd_e afd)
{
	switch(afd) {
	case AFD_BOX_16x9_TOP:
		return "AFD_BOX_16x9_TOP";
	case AFD_BOX_14x9_TOP:
		return "AFD_BOX_14x9_TOP";
	case AFD_BOX_16x9_CENTER:
		return "AFD_16x9_CENTER";
	case AFD_FULL_FRAME:
		return "AFD_FULL_FRAME";
	case AFD_16x9_CENTER:
		return "AFD_16x9_CENTER";
	case AFD_14x9_CENTER:
		return "AFD_14x9_CENTER";
	case AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER:
		return "AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER";
	case AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER:
		return "AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER";
	case AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER:
		return "AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER";
	default:
        	return "AFD_UNDEFINED";
	}
}

const char *aspectRatio_to_string(enum payload_aspect_ratio_e ar)
{
	switch(ar) {
	case ASPECT_4x3:
		return "ASPECT_4x3";
	case ASPECT_16x9:
		return "ASPECT_16x9";
	default:
        	return "ASPECT_UNDEFINED";
	}
}

int dump_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, void *p)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_payload_information_s *pkt = p;

	printf("%s() AFD: %s Aspect Ratio: %s Flags: 0x%x Value1: 0x%x Value2: 0x%x\n", __func__,
		afd_to_string(pkt->afd),
		aspectRatio_to_string(pkt->aspectRatio),
		pkt->barDataFlags,
		pkt->barDataValue[0],
		pkt->barDataValue[1]
		);

	return KLAPI_OK;
}

int parse_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_payload_information_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));
	unsigned char afd = (sanitizeWord(hdr->payload[0]) >> 3) & 0x0f;

	switch(afd) {
	case 0x02:
		pkt->afd = AFD_BOX_16x9_TOP;
		break;
	case 0x03:
		pkt->afd = AFD_BOX_14x9_TOP;
		break;
	case 0x04:
        	pkt->afd = AFD_BOX_16x9_CENTER;
		break;
	case 0x08:
        	pkt->afd = AFD_FULL_FRAME;
		break;
	case 0x0a:
        	pkt->afd = AFD_16x9_CENTER;
		break;
	case 0x0b:
        	pkt->afd = AFD_14x9_CENTER;
		break;
	case 0x0d:
        	pkt->afd = AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER;
		break;
	case 0x0e:
        	pkt->afd = AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER;
		break;
	case 0x0f:
        	pkt->afd = AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER;
		break;
	default:
        	pkt->afd = AFD_UNDEFINED;
	}

	if (sanitizeWord(hdr->payload[0]) & 0x04)
		pkt->aspectRatio = ASPECT_16x9;
	else
		pkt->aspectRatio = ASPECT_4x3;

	pkt->barDataFlags = sanitizeWord(hdr->payload[3]) >> 4;
	pkt->barDataValue[0]  = sanitizeWord(hdr->payload[4]) << 8;
	pkt->barDataValue[0] |= sanitizeWord(hdr->payload[5]);
	pkt->barDataValue[1]  = sanitizeWord(hdr->payload[6]) << 8;
	pkt->barDataValue[1] |= sanitizeWord(hdr->payload[7]);

	if (ctx->callbacks && ctx->callbacks->payload_information)
		ctx->callbacks->payload_information(ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

