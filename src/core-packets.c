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

static int isValidHeader(struct klvanc_context_s *ctx, unsigned short *arr, unsigned int len)
{
	int ret = 0;
	if (len > 7) {
		if ((*(arr + 0) < 3) && ((*(arr + 1) & 0x3fc) == 0x3fc) && ((*(arr + 2) & 0x3fc) == 0x3fc))
			ret = 1;
	}

	if (ctx->verbose > 1)
		PRINT_DEBUG("%04x %04x %04x %s\n", *(arr + 0), *(arr + 1), *(arr + 2), ret ? "valid": "invalid");
	return ret;
}

static struct type_s
{
	unsigned short did, sdid;
	enum klvanc_packet_type_e type;
	const char *spec;
	const char *description;
	int (*parse)(struct klvanc_context_s *, struct klvanc_packet_header_s *, void **);
	int (*dump)(struct klvanc_context_s *, void *);
	void (*free)(void *);
} types[] = {
	{ 0x40, 0xfe, VANC_TYPE_KL_UINT64_COUNTER, "KLABS", "UINT64 LE Frame Counter", parse_KL_U64LE_COUNTER, klvanc_dump_KL_U64LE_COUNTER, NULL, },
	{ 0x41, 0x05, VANC_TYPE_AFD, "SMPTE 2016-3 AFD", "Active Format Description", parse_AFD, klvanc_dump_AFD, NULL, },
	{ 0x41, 0x07, VANC_TYPE_SCTE_104, "SMPTE Packet Type 2", "SCTE 104", parse_SCTE_104, klvanc_dump_SCTE_104, klvanc_free_SCTE_104, },
#ifdef SCTE_104_PACKET_TYPE_1
	{ 0x80, 0x07, VANC_TYPE_SCTE_104, "SMPTE Packet Type 1 (Deprecated)", "SCTE 104", parse_SCTE_104, klvanc_dump_SCTE_104, klvanc_free_SCTE_104, },
#endif
	{ 0x61, 0x01, VANC_TYPE_EIA_708B, "SMPTE", "EIA_708B", parse_EIA_708B, klvanc_dump_EIA_708B, klvanc_destroy_eia708_cdp, },
	{ 0x61, 0x02, VANC_TYPE_EIA_608, "SMPTE", "EIA_608", parse_EIA_608, klvanc_dump_EIA_608, NULL, },
};

static enum klvanc_packet_type_e lookupTypeByDID(unsigned short did, unsigned short sdid)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].did == did) && (types[i].sdid == sdid))
			return types[i].type;
	}

	return VANC_TYPE_UNDEFINED;
}

const char *klvanc_lookupDescriptionByType(enum klvanc_packet_type_e type)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if (types[i].type == type)
			return types[i].description;
	}

	return "UNDEFINED";
}

const char *klvanc_lookupSpecificationByType(enum klvanc_packet_type_e type)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if (types[i].type == type)
			return types[i].spec;
	}

	return "UNDEFINED";
}

static int freeByType(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void *pp)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].type == hdr->type) && (types[i].free)) {
			types[i].free(pp);
			return 0;
		}
	}

	return -EINVAL;
}

static int parseByType(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr, void **pp)
{
	*pp = 0;
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].type == hdr->type) && (types[i].parse))
			return types[i].parse(ctx, hdr, pp);
	}

	return -EINVAL;
}

static int dumpByType(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr)
{
	for (int i = 0; i < (sizeof(types) / sizeof(struct type_s)); i++) {
		if ((types[i].type == hdr->type) && (types[i].dump))
			return types[i].dump(ctx, hdr);
	}

	return -EINVAL;
}

static int parse(struct klvanc_context_s *ctx, unsigned short *arr, unsigned int len,
	struct klvanc_packet_header_s **hdr)
{
	if (!isValidHeader(ctx, arr, len)) {
		return -EINVAL;
	}

	struct klvanc_packet_header_s *p = calloc(1, sizeof(struct klvanc_packet_header_s));
	if (!p)
		return -ENOMEM;

	memcpy(&p->raw[0], arr, len * sizeof(unsigned short));
	p->rawLengthWords = len;
	p->adf[0] = *(arr + 0);
	p->adf[1] = *(arr + 1);
	p->adf[2] = *(arr + 2);
	p->did = sanitizeWord(*(arr + 3));
	p->dbnsdid = sanitizeWord(*(arr + 4));
	if (p->payloadLengthWords > (sizeof(p->payload) / sizeof(unsigned short))) {
		free(p);
		return -ENOMEM;
	}

	p->payloadLengthWords = sanitizeWord(*(arr + 5));

	int i;
	for (i = 0; i < p->payloadLengthWords; i++) {
		p->payload[i] = *(arr + 6 + i);
	}
	p->checksum = *(arr + 6 + i);
	p->checksumValid = klvanc_checksum_is_valid(arr + 3,
		p->payloadLengthWords + 4 /* payload + header + len + crc */);
	if (!p->checksumValid)
		ctx->checksum_failures++;

	p->type = lookupTypeByDID(p->did, p->dbnsdid);

	*hdr = p;
	return KLAPI_OK;
}

void klvanc_dump_words_console(struct klvanc_context_s *ctx, uint16_t *vanc,
			       int maxlen, unsigned int linenr, int onlyvalid)
{
	if (onlyvalid && (*(vanc + 1) != 0x3ff) && (*(vanc + 2) != 0x3ff))
		return;

	PRINT_DEBUG("LineNr: %03d ADF: [%03x][%03x][%03x] DID: [%03x] DBN/SDID: [%03x] DC: [%03x]\n",
		    linenr, *(vanc + 0), *(vanc + 1), *(vanc + 2), *(vanc + 3),
		    *(vanc + 4), *(vanc + 5));

	PRINT_DEBUG("           Desc: %s [SMPTE %s]\n",
		    klvanc_didLookupDescription(*(vanc + 3) & 0xff, *(vanc + 4) & 0xff),
		    klvanc_didLookupSpecification(*(vanc + 3) & 0xff, *(vanc + 4) & 0xff));

	/* Spec says DC is a maximum number of 255 words */
	int i, words = *(vanc + 5) & 0xff;
	PRINT_DEBUG("           Data: ");
	for (i = 6; i < (6 + words); i++) {
		PRINT_DEBUG("[%03x] ", *(vanc + i));
	}
	PRINT_DEBUG("\n             CS: [%03x]\n", *(vanc + i));
}

void klvanc_dump_packet_console(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr)
{
	PRINT_DEBUG("hdr->type   = %d\n", hdr->type);
	PRINT_DEBUG(" ->adf      = 0x%04x/0x%04x/0x%04x\n", hdr->adf[0], hdr->adf[1], hdr->adf[2]);
	PRINT_DEBUG(" ->did/sdid = 0x%02x / 0x%02x [%s %s] via SDI line %d\n",
		    hdr->did,
		    hdr->dbnsdid,
		    klvanc_didLookupSpecification(hdr->did, hdr->dbnsdid),
		    klvanc_didLookupDescription(hdr->did, hdr->dbnsdid),
		    hdr->lineNr);
	PRINT_DEBUG(" ->h_offset = %d\n", hdr->horizontalOffset);
	PRINT_DEBUG(" ->checksum = 0x%04x (%s)\n", hdr->checksum, hdr->checksumValid ? "VALID" : "INVALID");
	PRINT_DEBUG(" ->payloadLengthWords = %d\n", hdr->payloadLengthWords);
	PRINT_DEBUG(" ->payload  = ");
	for (int i = 0; i < hdr->payloadLengthWords; i++)
		PRINT_DEBUG("%02x ", sanitizeWord(hdr->payload[i]));
	PRINT_DEBUG("\n");
}

int klvanc_packet_parse(struct klvanc_context_s *ctx, unsigned int lineNr, unsigned short *arr, unsigned int len)
{
	int attempts = 0;
	VALIDATE(ctx);
	VALIDATE(arr);
	VALIDATE(len);

	if (len > 16384) {
		/* Safety */
		PRINT_ERR("%s() length %d exceeds 16384, ignoring.\n", __func__, len);
		return -EINVAL;
	}

	/* Scan the entire line for vanc frames */
	unsigned int i = 0;
	while (i < len - 7) {
		/* Do a basic header parse */
		struct klvanc_packet_header_s *hdr;
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
		klvanc_cache_update(ctx, hdr);

		if (hdr->checksumValid || ctx->allow_bad_checksums) {
			if (ctx->callbacks && ctx->callbacks->all)
				ctx->callbacks->all(ctx->callback_context, ctx, hdr);

			/* formally decode the entire packet */
			void *decodedPacket;
			ret = parseByType(ctx, hdr, &decodedPacket);
			if (ret == KLAPI_OK) {
				if (ctx->verbose == 2) {
					ret = dumpByType(ctx, decodedPacket);
					if (ret < 0) {
						PRINT_ERR("Failed to dump by type, missing dumper function?\n");
					}
				}
			} else {
				if (klrestricted_code_path_block_execute(&ctx->rcp_failedToDecode)) {
					PRINT_ERR("Failed parsing by type\n");
					klvanc_dump_packet_console(ctx, hdr);
				}
			}

			if (decodedPacket)
				freeByType(ctx, hdr, decodedPacket);
		}

		free(hdr);

		/* Minimum packet length is 7, so lets move things
		 * on a little faster....
		 */
		i += 7;
	}

	return attempts;
}

int klvanc_sdi_create_payload(uint8_t sdid, uint8_t did,
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

int klvanc_packet_copy(struct klvanc_packet_header_s **dst, struct klvanc_packet_header_s *src)
{
	*dst = malloc(sizeof(*src));
	memcpy(*dst, src, sizeof(*src));
	return 0;
}

void klvanc_packet_free(struct klvanc_packet_header_s *src)
{
	free(src);
}

