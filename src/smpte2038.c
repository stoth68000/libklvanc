/* Copyright (c) 2016 Kernel Labs Inc. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libklvanc/smpte2038.h>

#define VANC8(n) ((n) & 0xff)

static void hexdump(unsigned char *buf, unsigned int len, int bytesPerRow /* Typically 16 */)
{
        for (unsigned int i = 0; i < len; i++)
                printf("%02x%s", buf[i], ((i + 1) % bytesPerRow) ? " " : "\n");
        printf("\n");
}

void smpte2038_anc_data_packet_free(struct smpte2038_anc_data_packet_s *pkt)
{
	if (!pkt)
		return;

	for (int i = 0; i < pkt->lineCount; i++) {
		struct smpte2038_anc_data_line_s *l = pkt->lines + i;
		if (l->data_count)
			free(l->user_data_words);
	}
	if (pkt->lineCount)
		free(pkt->lines);

	free(pkt);
}

#define SHOW_LINE_U32(indent, fn) printf("%s%s = %d (0x%x)\n", indent, #fn, fn, fn);
#define SHOW_LINE_U64(indent, fn) printf("%s%s = %ld (0x%lx)\n", indent, #fn, fn, fn);

void smpte2038_anc_data_packet_dump(struct smpte2038_anc_data_packet_s *h)
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
		struct smpte2038_anc_data_line_s *l = &h->lines[i];
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
int smpte2038_parse_pes_packet(uint8_t *section, unsigned int byteCount, struct smpte2038_anc_data_packet_s **result)
{
	int ret = -1;

	struct smpte2038_anc_data_packet_s *h = calloc(sizeof(*h), 1);

// MMM
	struct bs_context_s *bs = bs_alloc();
        bs_read_set_buffer(bs, section, byteCount);

	h->packet_start_code_prefix = bs_read_bits(bs, 24);
	VALIDATE(h->packet_start_code_prefix, 1);

	h->stream_id = bs_read_bits(bs, 8);
	VALIDATE(h->stream_id, 0xBD);

	h->PES_packet_length = bs_read_bits(bs, 16);
	bs_read_bits(bs, 2);
	h->PES_scrambling_control = bs_read_bits(bs, 2);
	VALIDATE(h->PES_scrambling_control, 0);

	h->PES_priority = bs_read_bits(bs, 1);
	h->data_alignment_indicator = bs_read_bits(bs, 1);
	VALIDATE(h->data_alignment_indicator, 1);
	h->copyright = bs_read_bits(bs, 1);
	h->original_or_copy = bs_read_bits(bs, 1);

	h->PTS_DTS_flags = bs_read_bits(bs, 2);
	h->ESCR_flag = bs_read_bits(bs, 1);
	h->ES_rate_flag = bs_read_bits(bs, 1);
	h->DSM_trick_mode_flag = bs_read_bits(bs, 1);
	h->additional_copy_info_flag = bs_read_bits(bs, 1);
	h->PES_CRC_flag = bs_read_bits(bs, 1);
	h->PES_extension_flag = bs_read_bits(bs, 1);
	VALIDATE(h->PTS_DTS_flags, 2);
	VALIDATE(h->ESCR_flag, 0);
	VALIDATE(h->ES_rate_flag, 0);
	VALIDATE(h->DSM_trick_mode_flag, 0);
	VALIDATE(h->additional_copy_info_flag, 0);
	VALIDATE(h->PES_CRC_flag, 0);
	VALIDATE(h->PES_extension_flag, 0);

	h->PES_header_data_length = bs_read_bits(bs, 8);
	bs_read_bits(bs, 4);
	VALIDATE(h->PES_header_data_length, 5);

	/* PTS Handling */
	uint64_t a = (uint64_t)bs_read_bits(bs, 3) << 30;
	bs_read_bits(bs, 1);

	uint64_t b = (uint64_t)bs_read_bits(bs, 15) << 15;
	bs_read_bits(bs, 1);

	uint64_t c = (uint64_t)bs_read_bits(bs, 15);
	bs_read_bits(bs, 1);

	h->PTS = a | b | c;

	/* Do we have any lines remaining in the packet? */
	int rem = (h->PES_packet_length + 6) - 15;
	while (rem > 4) {
		h->lineCount++;
		h->lines = realloc(h->lines, h->lineCount * sizeof(struct smpte2038_anc_data_line_s));

		struct smpte2038_anc_data_line_s *l = h->lines + (h->lineCount - 1);
		memset(l, 0, sizeof(*l));

		l->reserved_000000 = bs_read_bits(bs, 6);
		VALIDATE(l->reserved_000000, 0);

		l->c_not_y_channel_flag = bs_read_bits(bs, 1);
		VALIDATE(l->c_not_y_channel_flag, 0);

		l->line_number = bs_read_bits(bs, 11);
		//VALIDATE(l->line_number, 9);

		l->horizontal_offset = bs_read_bits(bs, 12);
		//VALIDATE(l->horizontal_offset, 0);

		l->DID = bs_read_bits(bs, 10);

		l->SDID = bs_read_bits(bs, 10);

		l->data_count = bs_read_bits(bs, 10);

		/* Lets put the checksum at the end of the array then pull it back
		 * into the checksum field later, it makes for easier processing.
		 */
		l->user_data_words = calloc(sizeof(uint16_t), l->data_count + 1);
		for (uint16_t i = 0; i < VANC8(l->data_count); i++)
			l->user_data_words[i] = bs_read_bits(bs, 10);

		l->checksum_word = bs_read_bits(bs, 10);
		printf("checksum_word = %x\n", l->checksum_word);
	
		rem = (h->PES_packet_length + 6) - bs_get_byte_count(bs);

		/* Clock in any stuffing bits */
		bs_read_byte_stuff(bs);
	}

	*result = h;
	ret = 0;
err:
	if (bs)
		bs_free(bs);
	return ret;
}

#define SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET 14
#define SMPTE2038_PACKETIZER_DEBUG 0

int smpte2038_packetizer_alloc(struct smpte2038_packetizer_s **ctx)
{
	struct smpte2038_packetizer_s *p = calloc(1, sizeof(*p));
	if (!p)
		return -1;

	/* Leave enough space for us to prefix the PES header */
	p->bufused = SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET;
	p->buflen = 16384;
	p->buffree = p->buflen - p->bufused;
	p->buf = calloc(1, p->buflen);
	if (!p->buf) {
		free(p);
		return -1;
	}
	p->bs = bs_alloc();

	*ctx = p;
	return 0;
}

static __inline void smpte2038_buffer_recalc(struct smpte2038_packetizer_s *ctx)
{
	ctx->buffree = ctx->buflen - ctx->bufused;
}

static void smpte2038_buffer_adjust(struct smpte2038_packetizer_s *ctx, uint32_t newsizeBytes)
{
#if SMPTE2038_PACKETIZER_DEBUG
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

	smpte2038_buffer_recalc(ctx);
}

void smpte2038_packetizer_free(struct smpte2038_packetizer_s **ctx)
{
	if (!ctx && (*ctx == 0))
		return;

	free((*ctx)->buf);
	bs_free((*ctx)->bs);
	memset(*ctx, 0, sizeof(struct smpte2038_packetizer_s));
	free(*ctx);
}

int smpte2038_packetizer_begin(struct smpte2038_packetizer_s *ctx)
{
	ctx->bufused = SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET;
	smpte2038_buffer_recalc(ctx);
	memset(ctx->buf, 0xff, ctx->buflen);

	return 0;
}

int smpte2038_packetizer_append(struct smpte2038_packetizer_s *ctx, struct packet_header_s *pkt)
{
#if SMPTE2038_PACKETIZER_DEBUG
	printf("%s()\n", __func__);
#endif
	uint16_t offset = 0; /* TODO: Horizontal offset */
	uint32_t reqd = pkt->payloadLengthWords * sizeof(uint16_t);

	if ((reqd + 64 /* PES fields - headroom */) > ctx->buffree)
		smpte2038_buffer_adjust(ctx, ctx->buflen + 16384);

	/* Prepare a new 2038 line and add it to the existing buffer */

        bs_write_set_buffer(ctx->bs, ctx->buf + ctx->bufused, ctx->buffree);
        bs_write_bits(ctx->bs, 0, 6);				/* '000000' */
        bs_write_bits(ctx->bs, 0, 1);				/* c_not_y_channel_flag */
        bs_write_bits(ctx->bs, pkt->lineNr, 11);		/* line_number */
        bs_write_bits(ctx->bs, offset, 12);			/* horizontal_offset */
        bs_write_bits(ctx->bs, pkt->did, 10);			/* DID */
        bs_write_bits(ctx->bs, pkt->dbnsdid, 10);		/* SDID */
        bs_write_bits(ctx->bs, pkt->payloadLengthWords, 10);	/* data_count */
	for (int i = 0; i < pkt->payloadLengthWords; i++)
        	bs_write_bits(ctx->bs, pkt->payload[i], 10);	/* user_data_word */
       	bs_write_bits(ctx->bs, pkt->checksum, 10);		/* checksum_word */
	bs_write_byte_stuff(ctx->bs, 1);			/* Stuffing byte if required to end on byte alignment. */

#if 0
	/* add stuffing_byte so the stream is easier to eyeball debug. */
	for (int i = 0; i < 8; i++)
		bs_write_bits(ctx->bs, 0xff, 8);
#endif

	/* Close (actually its 'align') the bitstream buffer */
	bs_write_buffer_complete(ctx->bs);

	/* Finally, update our original buffer indexes to accomodate any writing by the bitstream. */
	ctx->bufused += bs_get_byte_count(ctx->bs);
	smpte2038_buffer_recalc(ctx);
#if SMPTE2038_PACKETIZER_DEBUG
	printf("bufused = %d buffree = %d\n", ctx->bufused, ctx->buffree);
#endif
	return 0;
}

/* return the size in bytes of the newly created buffer */
int smpte2038_packetizer_end(struct smpte2038_packetizer_s *ctx)
{
	if (ctx->bufused == SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET)
		return -1;
#if SMPTE2038_PACKETIZER_DEBUG
	printf("%s() used = %d\n", __func__, ctx->bufused);
#endif
	/* Now generate a correct looking PES frame and output it */
	/* See smpte 2038-2008 - Page 5, Table 2 for description. */

	/* Set the bitstream to the start of the buffer, we need to be careful
	 * and not tramplpe the VANC that starts at offset 15.
	 */
	bs_write_set_buffer(ctx->bs, ctx->buf, 15);

	/* PES Header - Bug: bitstream can't write 32bit values */
	bs_write_bits(ctx->bs, 1, 24);		/* packet_start_code_prefix */
	bs_write_bits(ctx->bs, 0xBD, 8);		/* stream_id */
	bs_write_bits(ctx->bs, 0, 16);		/* PES_packet_length */
	bs_write_bits(ctx->bs, 2, 2);		/* '10' fixed value */
	bs_write_bits(ctx->bs, 0, 2);		/* PES_scrambling_control (not scrambled) */
	bs_write_bits(ctx->bs, 0, 1);		/* PES_priority */
	bs_write_bits(ctx->bs, 1, 1);		/* data_alignment_indicator (aligned) */
	bs_write_bits(ctx->bs, 0, 1);		/* copyright (not-copyright) */
	bs_write_bits(ctx->bs, 0, 1);		/* original-or-copy (copy) */
	bs_write_bits(ctx->bs, 2, 2);		/* PTS_DTS_flags (PTS Present) */
	bs_write_bits(ctx->bs, 0, 1);		/* ESCR_flag (not present) */
	bs_write_bits(ctx->bs, 0, 1);		/* ES_RATE_flag (not present) */
	bs_write_bits(ctx->bs, 0, 1);		/* DSM_TRICK_MODE_flag (not present) */
	bs_write_bits(ctx->bs, 0, 1);		/* additional_copy_info_flag (not present) */
	bs_write_bits(ctx->bs, 0, 1);		/* PES_CRC_flag (not present) */
	bs_write_bits(ctx->bs, 0, 1);		/* PES_EXTENSION_flag (not present) */
	bs_write_bits(ctx->bs, 5, 8);		/* PES_HEADER_DATA_length */
	bs_write_bits(ctx->bs, 2, 4);		/* '0010' fixed value */

	uint64_t pts = 0; /* TODO */
	bs_write_bits(ctx->bs, (pts >> 30), 3);			/* PTS[32:30] */
	bs_write_bits(ctx->bs, 1, 1);				/* marker_bit */
	bs_write_bits(ctx->bs, (pts >> 15) & 0xefff, 15);	/* PTS[29:15] */
	bs_write_bits(ctx->bs, 1, 1);				/* marker_bit */
	bs_write_bits(ctx->bs, (pts & 0xefff), 15);		/* PTS[14:0] */
	bs_write_bits(ctx->bs, 1, 1);				/* marker_bit */

	/* Close (actually its 'align') the bitstream buffer */
	bs_write_buffer_complete(ctx->bs);

	int len = ctx->bufused - 6;
	ctx->buf[4] = (len >> 8) & 0xff;
	ctx->buf[5] = len & 0xff;

#if SMPTE2038_PACKETIZER_DEBUG
	hexdump(ctx->buf, ctx->bufused, 32);
	printf("%d buffer length\n", ctx->bufused);
	printf("%d bs used\n", ctx->bs->reg_used);
#endif

	return 0;
}

int smpte2038_convert_line_to_words(struct smpte2038_anc_data_line_s *l, uint16_t **words, uint16_t *wordCount)
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
	arr[i++] = l->DID;
	arr[i++] = l->SDID;
	arr[i++] = l->data_count;
	for (int j = 0; j < VANC8(l->data_count); j++)
		arr[i++] = l->user_data_words[j];
	arr[i++] = l->checksum_word;

	*words = arr;
	*wordCount = i;
	return 0;
}

