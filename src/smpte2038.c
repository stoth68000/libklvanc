/*
 * Copyright (c) 2016-2019 Kernel Labs Inc. All Rights Reserved
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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <libklvanc/smpte2038.h>
#include "klbitstream_readwriter.h"

#define VANC8(n) ((n) & 0xff)

#if 0
static void hexdump(unsigned char *buf, unsigned int len, int bytesPerRow /* Typically 16 */)
{
        for (unsigned int i = 0; i < len; i++)
                printf("%02x%s", buf[i], ((i + 1) % bytesPerRow) ? " " : "\n");
        printf("\n");
}
#endif

void klvanc_smpte2038_anc_data_packet_free(struct klvanc_smpte2038_anc_data_packet_s *pkt)
{
	if (!pkt)
		return;

	for (int i = 0; i < pkt->lineCount; i++) {
		struct klvanc_smpte2038_anc_data_line_s *l = pkt->lines + i;
		if (VANC8(l->data_count))
			free(l->user_data_words);
	}
	if (pkt->lineCount)
		free(pkt->lines);

	free(pkt);
}

#define SHOW_LINE_U32(indent, fn) printf("%s%s = %d (0x%x)\n", indent, #fn, fn, fn);
#define SHOW_LINE_U64(indent, fn) printf("%s%s = %" PRIu64 " (0x%" PRIx64 ")\n", indent, #fn, fn, fn);

void klvanc_smpte2038_anc_data_packet_dump(struct klvanc_smpte2038_anc_data_packet_s *h)
{
	printf("%s()\n", __func__);
	SHOW_LINE_U32("  ", h->packet_start_code_prefix);
	SHOW_LINE_U32("  ", h->stream_id);
	SHOW_LINE_U32("  ", h->PES_packet_length);
	SHOW_LINE_U32("  ", h->PES_scrambling_control);
	SHOW_LINE_U32("  ", h->PES_priority);
	SHOW_LINE_U32("  ", h->data_alignment_indicator);
	SHOW_LINE_U32("  ", h->copyright);
	SHOW_LINE_U32("  ", h->original_or_copy);
	SHOW_LINE_U32("  ", h->PTS_DTS_flags);
	SHOW_LINE_U32("  ", h->ESCR_flag);
	SHOW_LINE_U32("  ", h->ES_rate_flag);
	SHOW_LINE_U32("  ", h->DSM_trick_mode_flag);
	SHOW_LINE_U32("  ", h->additional_copy_info_flag);
	SHOW_LINE_U32("  ", h->PES_CRC_flag);
	SHOW_LINE_U32("  ", h->PES_extension_flag);
	SHOW_LINE_U32("  ", h->PES_header_data_length);
	SHOW_LINE_U64("  ", h->PTS);
	SHOW_LINE_U32("  ", h->lineCount);
	for (int i = 0; i < h->lineCount; i++) {
		struct klvanc_smpte2038_anc_data_line_s *l = &h->lines[i];
		printf("  LineEntry[%02d]\n", i);
		SHOW_LINE_U32("\t\t", l->line_number);
		SHOW_LINE_U32("\t\t", l->c_not_y_channel_flag);
		SHOW_LINE_U32("\t\t", l->horizontal_offset);
		SHOW_LINE_U32("\t\t", l->DID);
		SHOW_LINE_U32("\t\t", l->SDID);
		SHOW_LINE_U32("\t\t", l->data_count);
		printf("\t\t\t");
		for (int j = 1; j <= VANC8(l->data_count); j++) {
			printf("%03x ", l->user_data_words[j - 1]);
			if (j % 16 == 0)
				printf("\n\t\t\t");
		}
		printf("\n");
		SHOW_LINE_U32("\t\t", l->checksum_word);
	}
}

#define VALIDATE(obj, val) if ((obj) != (val)) { printf("%s is invalid\n", #obj); goto err; }

static int smpte2038_parse_pes_payload_int(struct klbs_context_s *bs, struct klvanc_smpte2038_anc_data_packet_s *h)
{
	int rem = klbs_get_buffer_size(bs) - klbs_get_byte_count(bs);
	int udwByteCount;
	int byteAligned = 0;

	while (rem > 4) {
		h->lines = realloc(h->lines, (h->lineCount + 1) * sizeof(struct klvanc_smpte2038_anc_data_line_s));

		struct klvanc_smpte2038_anc_data_line_s *l = h->lines + h->lineCount;
		memset(l, 0, sizeof(*l));

		l->reserved_000000 = klbs_read_bits(bs, 6);
		VALIDATE(l->reserved_000000, 0);

		l->c_not_y_channel_flag = klbs_read_bits(bs, 1);
		VALIDATE(l->c_not_y_channel_flag, 0);

		l->line_number = klbs_read_bits(bs, 11);
		//VALIDATE(l->line_number, 9);

		l->horizontal_offset = klbs_read_bits(bs, 12);
		//VALIDATE(l->horizontal_offset, 0);

		l->DID = klbs_read_bits(bs, 10);

		l->SDID = klbs_read_bits(bs, 10);

		l->data_count = klbs_read_bits(bs, 10);

		/* Lets put the checksum at the end of the array then pull it back
		 * into the checksum field later, it makes for easier processing.
		 */
		l->user_data_words = calloc(sizeof(uint16_t), VANC8(l->data_count) + 1);

		udwByteCount = (((VANC8(l->data_count) + 1) * 10) / 8);

		/* Ensure we not overrunning because of bad data. */
		if (udwByteCount > (klbs_get_buffer_size(bs) - klbs_get_byte_count(bs))) {
			goto err;
		}
		for (uint16_t i = 0; i < VANC8(l->data_count); i++)
			l->user_data_words[i] = klbs_read_bits(bs, 10);

		l->checksum_word = klbs_read_bits(bs, 10);
		h->lineCount++;

		rem = klbs_get_buffer_size(bs) - klbs_get_byte_count(bs);

		/* Bug in some third party SMPTE2038 processors. They place
		 * byte alignment bits inbetween multiple lines, when no
		 * stuffing is required as per the spec.
		 * See st2038-2008.pdf page 5 of 17.
		 * We have to detect and remove these alignment bits, else
		 * attempts to parse the following line result in illegal data.
		 */

		/*
		 * Start by checking if the bitstreamreader reader thinks its already
		 * byte aligned.
		 */
		byteAligned = bs->reg_used == 0;

		/* Clock in any stuffing bits */
		klbs_read_byte_stuff(bs);

		/* If we were already aligned BEFORE we call klbs_read_byte_stuff(),
		 * to enture the reader was byte aligned, and we have remaining data then
		 * Flush stuffing bits if they exist.
		 */
		if (byteAligned && rem) {
			while (klbs_peek_bits(bs, 1) == 1)
				klbs_read_bit(bs);
		}
	}
	return 0;
err:
	return -1;
}

int klvanc_smpte2038_parse_pes_payload(uint8_t *payload, unsigned int byteCount, struct klvanc_smpte2038_anc_data_packet_s **result)
{
	int ret;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	struct klvanc_smpte2038_anc_data_packet_s *h = calloc(sizeof(*h), 1);
	if (h == NULL) {
		klbs_free(bs);
		return -1;
	}

        klbs_read_set_buffer(bs, payload, byteCount);

	ret = smpte2038_parse_pes_payload_int(bs, h);

	*result = h;

	if (bs)
		klbs_free(bs);

	return ret;
}

int klvanc_smpte2038_parse_pes_packet(uint8_t *section, unsigned int byteCount, struct klvanc_smpte2038_anc_data_packet_s **result)
{
	int ret = -1;
	struct klbs_context_s *bs = klbs_alloc();
	if (bs == NULL)
		return -1;

	struct klvanc_smpte2038_anc_data_packet_s *h = calloc(sizeof(*h), 1);
	if (h == NULL) {
		klbs_free(bs);
		return -1;
	}

        klbs_read_set_buffer(bs, section, byteCount);

	h->packet_start_code_prefix = klbs_read_bits(bs, 24);
	VALIDATE(h->packet_start_code_prefix, 1);

	h->stream_id = klbs_read_bits(bs, 8);
	VALIDATE(h->stream_id, 0xBD);

	h->PES_packet_length = klbs_read_bits(bs, 16);
	klbs_read_bits(bs, 2);
	h->PES_scrambling_control = klbs_read_bits(bs, 2);
	VALIDATE(h->PES_scrambling_control, 0);

	h->PES_priority = klbs_read_bits(bs, 1);
	h->data_alignment_indicator = klbs_read_bits(bs, 1);
	h->copyright = klbs_read_bits(bs, 1);
	h->original_or_copy = klbs_read_bits(bs, 1);

	h->PTS_DTS_flags = klbs_read_bits(bs, 2);
	h->ESCR_flag = klbs_read_bits(bs, 1);
	h->ES_rate_flag = klbs_read_bits(bs, 1);
	h->DSM_trick_mode_flag = klbs_read_bits(bs, 1);
	h->additional_copy_info_flag = klbs_read_bits(bs, 1);
	h->PES_CRC_flag = klbs_read_bits(bs, 1);
	h->PES_extension_flag = klbs_read_bits(bs, 1);
	VALIDATE(h->PTS_DTS_flags, 2);
	VALIDATE(h->ESCR_flag, 0);
	VALIDATE(h->ES_rate_flag, 0);
	VALIDATE(h->DSM_trick_mode_flag, 0);
	VALIDATE(h->additional_copy_info_flag, 0);
	VALIDATE(h->PES_CRC_flag, 0);
	VALIDATE(h->PES_extension_flag, 0);

	h->PES_header_data_length = klbs_read_bits(bs, 8);
	klbs_read_bits(bs, 4);
	VALIDATE(h->PES_header_data_length, 5);

	/* PTS Handling */
	uint64_t a = (uint64_t)klbs_read_bits(bs, 3) << 30;
	klbs_read_bits(bs, 1);

	uint64_t b = (uint64_t)klbs_read_bits(bs, 15) << 15;
	klbs_read_bits(bs, 1);

	uint64_t c = (uint64_t)klbs_read_bits(bs, 15);
	klbs_read_bits(bs, 1);

	h->PTS = a | b | c;

	ret = smpte2038_parse_pes_payload_int(bs, h);
	*result = h;

	if (bs)
		klbs_free(bs);
	return ret;

err:
	if (h)
		free(h);
	if (bs)
		klbs_free(bs);
	return ret;
}

#define KLVANC_SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET 14
#define KLVANC_SMPTE2038_PACKETIZER_DEBUG 0

int klvanc_smpte2038_packetizer_alloc(struct klvanc_smpte2038_packetizer_s **ctx)
{
	struct klvanc_smpte2038_packetizer_s *p = calloc(1, sizeof(*p));
	if (!p)
		return -1;

	/* Leave enough space for us to prefix the PES header */
	p->bufused = KLVANC_SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET;
	p->buflen = 16384;
	p->buffree = p->buflen - p->bufused;
	p->buf = calloc(1, p->buflen);
	if (!p->buf) {
		free(p);
		return -1;
	}
	p->bs = klbs_alloc();

	*ctx = p;
	return 0;
}

static __inline void klvanc_smpte2038_buffer_recalc(struct klvanc_smpte2038_packetizer_s *ctx)
{
	ctx->buffree = ctx->buflen - ctx->bufused;
}

static void klvanc_smpte2038_buffer_adjust(struct klvanc_smpte2038_packetizer_s *ctx, uint32_t newsizeBytes)
{
#if KLVANC_SMPTE2038_PACKETIZER_DEBUG
	printf("%s(%d)\n", __func__, newsizeBytes);
#endif
	if (newsizeBytes > (128 * 1024)) {
		fprintf(stderr, "%s() buffer exceeds impossible limit, with %d additional bytes\n", __func__, newsizeBytes);
		abort();
	}
	ctx->buf = realloc(ctx->buf, newsizeBytes);
	ctx->buflen = newsizeBytes;
	if (ctx->bufused > ctx->buflen)
		ctx->bufused = ctx->buflen;

	klvanc_smpte2038_buffer_recalc(ctx);
}

void klvanc_smpte2038_packetizer_free(struct klvanc_smpte2038_packetizer_s **ctx)
{
	if (!ctx)
		return;
	if (*ctx == 0)
		return;

	struct klvanc_smpte2038_packetizer_s *p = *ctx;
	if (p->buf)
		free(p->buf);
	klbs_free(p->bs);
	memset(p, 0, sizeof(struct klvanc_smpte2038_packetizer_s));
	free(p);
}

int klvanc_smpte2038_packetizer_begin(struct klvanc_smpte2038_packetizer_s *ctx)
{
	ctx->bufused = KLVANC_SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET;
	klvanc_smpte2038_buffer_recalc(ctx);
	memset(ctx->buf, 0xff, ctx->buflen);

	return 0;
}

int klvanc_smpte2038_packetizer_append(struct klvanc_smpte2038_packetizer_s *ctx, struct klvanc_packet_header_s *pkt)
{
#if KLVANC_SMPTE2038_PACKETIZER_DEBUG
	printf("%s()\n", __func__);
#endif
	uint16_t offset = 0; /* TODO: Horizontal offset */
	uint32_t reqd = pkt->payloadLengthWords * sizeof(uint16_t);

	if ((reqd + 64 /* PES fields - headroom */) > ctx->buffree)
		klvanc_smpte2038_buffer_adjust(ctx, ctx->buflen + 16384);

	/* Prepare a new 2038 line and add it to the existing buffer */

        klbs_write_set_buffer(ctx->bs, ctx->buf + ctx->bufused, ctx->buffree);
        klbs_write_bits(ctx->bs, 0, 6);				/* '000000' */
        klbs_write_bits(ctx->bs, 0, 1);				/* c_not_y_channel_flag */
        klbs_write_bits(ctx->bs, pkt->lineNr, 11);		/* line_number */
        klbs_write_bits(ctx->bs, offset, 12);			/* horizontal_offset */
        klbs_write_bits(ctx->bs, pkt->did, 10);			/* DID */
        klbs_write_bits(ctx->bs, pkt->dbnsdid, 10);		/* SDID */
        klbs_write_bits(ctx->bs, pkt->payloadLengthWords, 10);	/* data_count */
	for (int i = 0; i < pkt->payloadLengthWords; i++)
        	klbs_write_bits(ctx->bs, pkt->payload[i], 10);	/* user_data_word */
       	klbs_write_bits(ctx->bs, pkt->checksum, 10);		/* checksum_word */
	klbs_write_byte_stuff(ctx->bs, 1);			/* Stuffing byte if required to end on byte alignment. */

#if 0
	/* add stuffing_byte so the stream is easier to eyeball debug. */
	for (int i = 0; i < 8; i++)
		klbs_write_bits(ctx->bs, 0xff, 8);
#endif

	/* Close (actually its 'align') the bitstream buffer */
	klbs_write_buffer_complete(ctx->bs);

	/* Finally, update our original buffer indexes to accomodate any writing by the bitstream. */
	ctx->bufused += klbs_get_byte_count(ctx->bs);
	klvanc_smpte2038_buffer_recalc(ctx);
#if KLVANC_SMPTE2038_PACKETIZER_DEBUG
	printf("bufused = %d buffree = %d\n", ctx->bufused, ctx->buffree);
#endif
	return 0;
}

/* return the size in bytes of the newly created buffer */
int klvanc_smpte2038_packetizer_end(struct klvanc_smpte2038_packetizer_s *ctx, uint64_t pts)
{
	if (ctx->bufused == KLVANC_SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET)
		return -1;
#if KLVANC_SMPTE2038_PACKETIZER_DEBUG
	printf("%s() used = %d\n", __func__, ctx->bufused);
#endif
	/* Now generate a correct looking PES frame and output it */
	/* See smpte 2038-2008 - Page 5, Table 2 for description. */

	/* Set the bitstream to the start of the buffer, we need to be careful
	 * and not tramplpe the VANC that starts at offset 15.
	 */
	klbs_write_set_buffer(ctx->bs, ctx->buf, 15);

	/* PES Header - Bug: bitstream can't write 32bit values */
	klbs_write_bits(ctx->bs, 1, 24);		/* packet_start_code_prefix */
	klbs_write_bits(ctx->bs, 0xBD, 8);		/* stream_id */
	klbs_write_bits(ctx->bs, 0, 16);		/* PES_packet_length */
	klbs_write_bits(ctx->bs, 2, 2);		/* '10' fixed value */
	klbs_write_bits(ctx->bs, 0, 2);		/* PES_scrambling_control (not scrambled) */
	klbs_write_bits(ctx->bs, 0, 1);		/* PES_priority */
	klbs_write_bits(ctx->bs, 1, 1);		/* data_alignment_indicator (aligned) */
	klbs_write_bits(ctx->bs, 0, 1);		/* copyright (not-copyright) */
	klbs_write_bits(ctx->bs, 0, 1);		/* original-or-copy (copy) */
	klbs_write_bits(ctx->bs, 2, 2);		/* PTS_DTS_flags (PTS Present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* ESCR_flag (not present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* ES_RATE_flag (not present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* DSM_TRICK_MODE_flag (not present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* additional_copy_info_flag (not present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* PES_CRC_flag (not present) */
	klbs_write_bits(ctx->bs, 0, 1);		/* PES_EXTENSION_flag (not present) */
	klbs_write_bits(ctx->bs, 5, 8);		/* PES_HEADER_DATA_length */
	klbs_write_bits(ctx->bs, 2, 4);		/* '0010' fixed value */

	klbs_write_bits(ctx->bs, (pts >> 30), 3);			/* PTS[32:30] */
	klbs_write_bits(ctx->bs, 1, 1);				/* marker_bit */
	klbs_write_bits(ctx->bs, (pts >> 15) & 0x7fff, 15);	/* PTS[29:15] */
	klbs_write_bits(ctx->bs, 1, 1);				/* marker_bit */
	klbs_write_bits(ctx->bs, (pts & 0x7fff), 15);		/* PTS[14:0] */
	klbs_write_bits(ctx->bs, 1, 1);				/* marker_bit */

	/* Close (actually its 'align') the bitstream buffer */
	klbs_write_buffer_complete(ctx->bs);

	int len = ctx->bufused - 6;
	ctx->buf[4] = (len >> 8) & 0xff;
	ctx->buf[5] = len & 0xff;

#if KLVANC_SMPTE2038_PACKETIZER_DEBUG
	hexdump(ctx->buf, ctx->bufused, 32);
	printf("%d buffer length\n", ctx->bufused);
	printf("%d bs used\n", ctx->bs->reg_used);
#endif

	return 0;
}

int klvanc_smpte2038_convert_line_to_words(struct klvanc_smpte2038_anc_data_line_s *l, uint16_t **words, uint16_t *wordCount)
{
	if (!l || !words || !wordCount)
		return -1;

	uint16_t *arr = malloc((7 + VANC8(l->data_count)) * sizeof(uint16_t));
	if (!arr)
		return -ENOMEM;

	int i = 0;
	arr[i++] = 0;
	arr[i++] = 0x3ff;
	arr[i++] = 0x3ff;

	/* If parity bits are already present, pass them through rather than recomputing them
	   This avoids running the parity function against a value which already contains a
	   parity bit (the parity bit present in the input could change the outcome of the
	   parity computation).  It also avoids cases where we might turn a bad parity into
	   a good parity value (e.g. the input is malformed and recomputing the parity would
	   result in bad data having a good parity). */
	if (l->DID & 0x300)
		arr[i++] = l->DID;
	else
		arr[i++] = l->DID | (__builtin_parity(l->DID) ? 0x100 : 0x200);

	if (l->SDID & 0x300)
		arr[i++] = l->SDID;
	else
		arr[i++] = l->SDID | (__builtin_parity(l->SDID) ? 0x100 : 0x200);

	if (l->data_count & 0x300)
		arr[i++] = l->data_count;
	else
		arr[i++] = l->data_count | (__builtin_parity(l->data_count) ? 0x100 : 0x200);

	for (int j = 0; j < VANC8(l->data_count); j++)
		arr[i++] = l->user_data_words[j];
	arr[i++] = l->checksum_word;

	*words = arr;
	*wordCount = i;
	return 0;
}

