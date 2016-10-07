/* Copyright (c) 2016 Kernel Labs Inc. */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <libklvanc/smpte2038.h>

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

#define SHOW_LINE_U32(indent, fn) printf("%s%s = %x\n", indent, #fn, fn);
#define SHOW_LINE_U64(indent, fn) printf("%s%s = %ld (%lx)\n", indent, #fn, fn, fn);

void smpte2038_smpte2038_anc_data_packet_dump(struct smpte2038_anc_data_packet_s *h)
{
	printf("%s()\n", __func__);
	SHOW_LINE_U32("", h->packet_start_code_prefix);
	SHOW_LINE_U32("", h->stream_id);
	SHOW_LINE_U32("", h->PES_scrambling_control);
	SHOW_LINE_U32("", h->PES_priority);
	SHOW_LINE_U32("", h->data_alignment_indicator);
	SHOW_LINE_U32("", h->copyright);
	SHOW_LINE_U32("", h->original_or_copy);
	SHOW_LINE_U32("", h->PTS_DTS_flags);
	SHOW_LINE_U32("", h->ESCR_flag);
	SHOW_LINE_U32("", h->ES_rate_flag);
	SHOW_LINE_U32("", h->DSM_trick_mode_flag);
	SHOW_LINE_U32("", h->additional_copy_info_flag);
	SHOW_LINE_U32("", h->PES_CRC_flag);
	SHOW_LINE_U32("", h->PES_extension_flag);
	SHOW_LINE_U32("", h->PES_header_data_length);
	SHOW_LINE_U64("", h->PTS);
	for (int i = 0; i < h->lineCount; i++) {
		struct smpte2038_anc_data_line_s *l = &h->lines[i];
		printf("LineEntry[%02d]\n", i);
		SHOW_LINE_U32("\t\t", l->line_number);
		SHOW_LINE_U32("\t\t", l->c_not_y_channel_flag);
		SHOW_LINE_U32("\t\t", l->horizontal_offset);
		SHOW_LINE_U32("\t\t", l->DID);
		SHOW_LINE_U32("\t\t", l->SDID);
		SHOW_LINE_U32("\t\t", l->data_count);
		printf("\t\t");
		for (int j = 0; j < l->data_count; j++)
			printf("%03x ", l->user_data_words[j]);
		printf("\n");
	}
}

#define VALIDATE(obj, val) if ((obj) != (val)) { printf("%s is invalid\n", #obj); goto err; }
int smpte2038_parse_section(uint8_t *section, unsigned int byteCount, struct smpte2038_anc_data_packet_s **result)
{
	int ret = -1;

	uint8_t *p = &section[0];
	struct smpte2038_anc_data_packet_s *h = calloc(sizeof(*h), 1);
	h->packet_start_code_prefix = *(p + 0) << 16 | *(p + 1) << 8 | *(p + 2); p += 3;
	VALIDATE(h->packet_start_code_prefix, 1);

	h->stream_id = *(p + 0); p++;
	VALIDATE(h->stream_id, 0xBD);

	h->PES_packet_length = *(p + 0) << 8 | *(p + 1); p += 2;
	h->PES_scrambling_control = (*(p + 0) >> 4) & 0x03;
	VALIDATE(h->PES_scrambling_control, 0);

	h->PES_priority = *(p + 0) & 0x08 ? 1 : 0;
	h->data_alignment_indicator = *(p + 0)  & 0x04 ? 1 : 0;
	VALIDATE(h->data_alignment_indicator, 1);
	h->copyright = *(p + 0) & 0x02 ? 1 : 0;
	h->original_or_copy = *(p + 0) & 0x01 ? 1 : 0;
	p++;

	h->PTS_DTS_flags = (*(p + 0) >> 6) & 0x03;
	h->ESCR_flag = *(p + 0) & 0x20 ? 1 : 0;
	h->ES_rate_flag = *(p + 0) & 0x10 ? 1 : 0;
	h->DSM_trick_mode_flag = *(p + 0) & 0x08 ? 1 : 0;
	h->additional_copy_info_flag = *(p + 0) & 0x04 ? 1 : 0;
	h->PES_CRC_flag = *(p + 0) & 0x02 ? 1 : 0;
	h->PES_extension_flag = *(p + 0) & 0x01 ? 1 : 0;
	p++;
	VALIDATE(h->PTS_DTS_flags, 2);
	VALIDATE(h->ESCR_flag, 0);
	VALIDATE(h->ES_rate_flag, 0);
	VALIDATE(h->DSM_trick_mode_flag, 0);
	VALIDATE(h->additional_copy_info_flag, 0);
	VALIDATE(h->PES_CRC_flag, 0);
	VALIDATE(h->PES_extension_flag, 0);

	h->PES_header_data_length = *(p + 0);
	p++;
	VALIDATE(h->PES_header_data_length, 5);

printf("PTS_DTS_flags = %x\n", h->PTS_DTS_flags);
printf("PES_extension_flag = %x\n", h->PES_extension_flag);
printf("PES_packet_length = %x\n", h->PES_packet_length);
	/* PTS Handling */
	uint64_t a, b, c;
	a = (( *(p + 0) >> 1) & 0x07) << 30;
	b = (((*(p + 1) << 8 | *(p + 2)) >> 1) & 0xefff) << 15;
	c = (( *(p + 3) << 8 | *(p + 4)) >> 1) & 0xefff;
	h->PTS = a | b | c;
	p += 5;

printf("PTS = %ld\n", h->PTS);

	/* Do we have any lines remaining in the packet? */
	int linenr = 0;
	int rem = (h->PES_packet_length + 6) - (p - section);
	while (rem > 4) {
printf("\nlinenr = %d, rem = %d\n", ++linenr, rem);
		h->lineCount++;
		h->lines = realloc(h->lines, h->lineCount * sizeof(struct smpte2038_anc_data_line_s));

		struct smpte2038_anc_data_line_s *l = h->lines + (h->lineCount - 1);
		memset(l, 0, sizeof(*l));

		l->reserved_000000 = *(p + 0) >> 2;
		l->c_not_y_channel_flag = *(p + 0) & 0x02 ? 1 : 0;
		l->line_number |= (*(p + 0) & 0x01) << 10;
		l->line_number |= (*(p + 1) & 0xff) << 2;
		l->line_number |= (*(p + 2) & 0xc0) >> 6;
printf("line_number = %d\n", l->line_number);
		//VALIDATE(l->line_number, 9);

		l->horizontal_offset |= (*(p + 2) & 0x3f) << 6;
		l->horizontal_offset |= (*(p + 3) & 0xfc) >> 2;
		VALIDATE(l->horizontal_offset, 0);

		l->DID |= (*(p + 3) & 0x03) << 8;
		l->DID |= *(p + 4);
		p += 5;
printf("DID = %x\n", l->DID);

		l->SDID |= *(p + 0) << 2;
		l->SDID |= (*(p + 1) & 0xc0) >> 6;
		p += 1;
printf("SDID = %x\n", l->SDID);

		l->data_count = (*(p + 0) & 0x3f) << 4 | ((*(p + 1) & 0xf0) >> 4);
		l->data_count &= 0xff;
		p += 1;
printf("data_count = %x\n", l->data_count);

		int shifter = 4;
		uint16_t *v;
		/* Lets put the checksum at the end of the array then pull it back
		 * into the checksum field later, it makes for easier processing.
		 */
		l->user_data_words = calloc(sizeof(uint16_t), l->data_count + 1);
		for (uint16_t i = 1; i <= (l->data_count + 1); i++) {
			v = &l->user_data_words[i - 1];

			if (shifter == 0) {
				*v = *(p + 0) << 2 | ((*(p + 1) & 0xc0) >> 6);
				p++;
				shifter = 6;
			} else
			if (shifter == 6) {
				*v = (*(p + 0) & 0x3f) << 4 | ((*(p + 1) & 0xf0) >> 4);
				p += 1;
				shifter = 4;
			} else
			if (shifter == 4) {
				*v = (*(p + 0) & 0x0f) << 6 | ((*(p + 1) & 0xfc) >> 2);
				p += 1;
				shifter = 2;
			} else
			if (shifter == 2) {
				*v = (*(p + 0) & 0x03) << 8 | *(p + 1);
				p += 2;
				shifter = 0;
			}

			// printf("word = %04x, shifter = %d\n", *v, shifter);

		}
		if (v) {
			l->checksum_word = *v;
			p++;
		}
		else {
			/* No vanc words, but we still have a checksum */
			l->checksum_word = *(p + 0) << 2 | ((*(p + 1) & 0xc0) >> 6);
			/* and we must have six padding bits, skip over them. */
			p += 2;
		}
		printf("checksum_word = %x\n", l->checksum_word);
	
		rem = (h->PES_packet_length + 6) - (p - section);
	}

	*result = h;
	ret = 0;
err:
	return ret;
}

#define SMPTE2038_PACKETIZER_BUFFER_RESET_OFFSET 15
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

	/* add stuffing_byte so the stream is easier to eyeball debug. */
	for (int i = 0; i < 8; i++)
		bs_write_bits(ctx->bs, 0xff, 8);

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

#if SMPTE2038_PACKETIZER_DEBUG
	hexdump(ctx->buf, ctx->bufused, 32);
	printf("%d buffer length\n", ctx->bufused);
	printf("%d bs used\n", ctx->bs->reg_used);
#endif

	return 0;
}

