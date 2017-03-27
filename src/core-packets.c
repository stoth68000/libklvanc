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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int isValidHeader(struct vanc_context_s *ctx, unsigned short *arr, unsigned int len)
{
	int ret = 0;
	if (len > 7) {
		if ((*(arr + 0) < 3) && ((*(arr + 1) & 0x3fc) == 0x3fc) && ((*(arr + 2) & 0x3fc) == 0x3fc))
			ret = 1;
	}

	if (ctx->verbose > 1)
		printf("%04x %04x %04x %s\n", *(arr + 0), *(arr + 1), *(arr + 2), ret ? "valid": "invalid");
	return ret;
}

static struct type_s
{
	unsigned short did, sdid;
	enum packet_type_e type;
	const char *spec;
	const char *description;
	int (*parse)(struct vanc_context_s *, struct packet_header_s *, void **);
	int (*dump)(struct vanc_context_s *, void *);
} types[] = {
	{ 0x40, 0xfe, VANC_TYPE_KL_UINT64_COUNTER, "KLABS", "UINT64 LE Frame Counter", parse_KL_U64LE_COUNTER, dump_KL_U64LE_COUNTER, },
	{ 0x41, 0x05, VANC_TYPE_PAYLOAD_INFORMATION, "SMPTE 2016-3 AFD", "Payload Information", parse_PAYLOAD_INFORMATION, dump_PAYLOAD_INFORMATION, },
	{ 0x41, 0x07, VANC_TYPE_SCTE_104, "SMPTE Packet Type 2", "SCTE 104", parse_SCTE_104, dump_SCTE_104, },
#ifdef SCTE_104_PACKET_TYPE_1
	{ 0x80, 0x07, VANC_TYPE_SCTE_104, "SMPTE Packet Type 1 (Deprecated)", "SCTE 104", parse_SCTE_104, dump_SCTE_104, },
#endif
	{ 0x61, 0x01, VANC_TYPE_EIA_708B, "SMPTE", "EIA_708B", parse_EIA_708B, dump_EIA_708B, },
	{ 0x61, 0x02, VANC_TYPE_EIA_608, "SMPTE", "EIA_608", parse_EIA_608, dump_EIA_608, },
};

static enum packet_type_e lookupTypeByDID(unsigned short did, unsigned short sdid)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].did == did) && (types[i].sdid == sdid))
			return types[i].type;
	}

	return VANC_TYPE_UNDEFINED;
}

const char *vanc_lookupDescriptionByType(enum packet_type_e type)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if (types[i].type == type)
			return types[i].description;
	}

	return "UNDEFINED";
}

const char *vanc_lookupSpecificationByType(enum packet_type_e type)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if (types[i].type == type)
			return types[i].spec;
	}

	return "UNDEFINED";
}


static int parseByType(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	*pp = 0;
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].type == hdr->type) && (types[i].parse))
			return types[i].parse(ctx, hdr, pp);
	}

	return -EINVAL;
}

static int dumpByType(struct vanc_context_s *ctx, struct packet_header_s *hdr)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].type == hdr->type) && (types[i].dump))
			return types[i].dump(ctx, hdr);
	}

	return -EINVAL;
}

static int parse(struct vanc_context_s *ctx, unsigned short *arr, unsigned int len,
	struct packet_header_s **hdr)
{
	if (!isValidHeader(ctx, arr, len)) {
		return -EINVAL;
	}

	struct packet_header_s *p = calloc(1, sizeof(struct packet_header_s));
	if (!p)
		return -ENOMEM;

	memcpy(&p->raw[0], arr, len * sizeof(unsigned short));
	p->rawLengthWords = len;
	p->adf[0] = *(arr + 0);
	p->adf[1] = *(arr + 1);
	p->adf[2] = *(arr + 2);
	p->did = sanitizeWord(*(arr + 3));
	p->dbnsdid = sanitizeWord(*(arr + 4));
	if (p->payloadLengthWords > (sizeof(p->payload) / sizeof(unsigned short)))
		return -ENOMEM;

	p->payloadLengthWords = sanitizeWord(*(arr + 5));

	int i;
	for (i = 0; i < p->payloadLengthWords; i++) {
		p->payload[i] = *(arr + 6 + i);
	}
	p->checksum = *(arr + 6 + i);
	p->checksumValid = vanc_checksum_is_valid(arr + 3,
		p->payloadLengthWords + 4 /* payload + header + len + crc */);

	p->type = lookupTypeByDID(p->did, p->dbnsdid);

	*hdr = p;
	return KLAPI_OK;
}

void vanc_dump_words_console(uint16_t *vanc, int maxlen, unsigned int linenr, int onlyvalid)
{
	if (onlyvalid && (*(vanc + 1) != 0x3ff) && (*(vanc + 2) != 0x3ff))
		return;

	fprintf(stderr, "LineNr: %03d ADF: [%03x][%03x][%03x] DID: [%03x] DBN/SDID: [%03x] DC: [%03x]\n",
		linenr, *(vanc + 0), *(vanc + 1), *(vanc + 2), *(vanc + 3),
		*(vanc + 4), *(vanc + 5));

	fprintf(stderr, "           Desc: %s [SMPTE %s]\n",
		klvanc_didLookupDescription(*(vanc + 3) & 0xff, *(vanc + 4) & 0xff),
		klvanc_didLookupSpecification(*(vanc + 3) & 0xff, *(vanc + 4) & 0xff));

	/* Spec says DC is a maximum number of 255 words */
	int i, words = *(vanc + 5) & 0xff;
	fprintf(stderr, "           Data: ");
	for (i = 6; i < (6 + words); i++) {
		fprintf(stderr, "[%03x] ", *(vanc + i));
	}
	fprintf(stderr, "\n             CS: [%03x]\n", *(vanc + i));
}

void klvanc_dump_packet_console(struct vanc_context_s *ctx, struct packet_header_s *hdr)
{
	printf("hdr->type   = %d\n", hdr->type);
	printf(" ->adf      = 0x%04x/0x%04x/0x%04x\n", hdr->adf[0], hdr->adf[1], hdr->adf[2]);
	printf(" ->did/sdid = 0x%02x / 0x%02x [%s %s] via SDI line %d\n",
		hdr->did,
		hdr->dbnsdid,
		klvanc_didLookupSpecification(hdr->did, hdr->dbnsdid),
		klvanc_didLookupDescription(hdr->did, hdr->dbnsdid),
		hdr->lineNr);
	printf(" ->h_offset = %d\n", hdr->horizontalOffset);
	printf(" ->checksum = 0x%04x (%s)\n", hdr->checksum, hdr->checksumValid ? "VALID" : "INVALID");
	printf(" ->payloadLengthWords = %d\n", hdr->payloadLengthWords);
	printf(" ->payload  = ");
	for (int i = 0; i < hdr->payloadLengthWords; i++)
		printf("%02x ", sanitizeWord(hdr->payload[i]));
	printf("\n");
}

int vanc_packet_parse(struct vanc_context_s *ctx, unsigned int lineNr, unsigned short *arr, unsigned int len)
{
	int attempts = 0;
	VALIDATE(ctx);
	VALIDATE(arr);
	VALIDATE(len);

	if (len > 16384) {
		/* Safety */
		fprintf(stderr, "%s() length %d exceeds 16384, ignoring.\n", __func__, len);
		return -EINVAL;
	}

	/* Scan the entire line for vanc frames */
	unsigned int i = 0;
	while (i < len - 7) {
		/* Do a basic header parse */
		struct packet_header_s *hdr;
		int ret = parse(ctx, arr + i, len - i, &hdr);
		if (ret < 0) {
			i++;
			continue;
		}

		hdr->horizontalOffset = i;
		hdr->lineNr = lineNr;

		/* Dump the packet header and basic VANC types if required. */
		if (ctx->verbose)
			klvanc_dump_packet_console(ctx, hdr);

		/* The number of frames we attempted to parse */
		attempts++;

		/* Update the internal VANC cache */
		vanc_cache_update(ctx, hdr);

		if (ctx->callbacks && ctx->callbacks->all)
			ctx->callbacks->all(ctx->callback_context, ctx, hdr);

		/* formally decode the entire packet */
		void *decodedPacket;
		ret = parseByType(ctx, hdr, &decodedPacket);
		if (ret == KLAPI_OK) {
			if (ctx->verbose == 2) {
				ret = dumpByType(ctx, decodedPacket);
				if (ret < 0) {
					fprintf(stderr, "Failed to dump by type, missing dumper function?\n");
				}
			}
		} else {
 			if (klrestricted_code_path_block_execute(&ctx->rcp_failedToDecode)) {
				fprintf(stderr, "Failed parsing by type\n");
				klvanc_dump_packet_console(ctx, hdr);
			 }
		}

		if (decodedPacket)
			free(decodedPacket);

		free(hdr);

		/* Minimum packet length is 7, so lets move things
		 * on a little faster....
		 */
		i += 7;
	}

	return attempts;
}

int vanc_sdi_create_payload(uint8_t sdid, uint8_t did,
        const uint8_t *src, uint16_t srcByteCount,
        uint16_t **dst, uint16_t *dstWordCount,
        uint32_t bitDepth)
{
	if ((bitDepth != 10) || (!sdid) || (!did) || (!src) || (!srcByteCount) || (!dst) || (!dstWordCount))
		return -1;

	int header_length = 6 + 1; /* Header 6 and checksum footer 1 */
	uint16_t *arr = calloc(2, srcByteCount + header_length);

	uint16_t *v = arr;

	*(v++) = 0x000;
	*(v++) = 0x3ff;
	*(v++) = 0x3ff;
	*(v++) = did;
	*(v++) = sdid;
	*(v++) = srcByteCount;
	for (int i = 0; i < srcByteCount; i++)
		*(v++) = *(src + i);

	/* Generate Parity
	 * VANC header 0/3ff/3ff = 3
	 * sdid/did/count = 3
	 */
	for (int i = 3; i < (srcByteCount + 3 + 3); i++) {
		if (__builtin_parity(arr[i]))
			arr[i] |= 0x100;
		else
			arr[i] |= 0x200;
	}

	/* Calculate checksum */
	uint16_t sum = 0;
	int i;
	for (i = 3; i < (srcByteCount + 3 + 3); i++) {
		sum += arr[i];
		sum &= 0x1ff;
	}
	*(v++) = sum | ((~sum & 0x100) << 1);

	*dstWordCount = v - arr;
	*dst = arr;

	return 0;
}

int vanc_packet_copy(struct packet_header_s **dst, struct packet_header_s *src)
{
	*dst = malloc(sizeof(*src));
	memcpy(*dst, src, sizeof(*src));
	return 0;
}

void vanc_packet_free(struct packet_header_s *src)
{
	free(src);
}

