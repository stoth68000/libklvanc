/*
 * Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved
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
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <libklvanc/smpte337_detector.h>

struct smpte337_detector_s *smpte337_detector_alloc(smpte337_detector_callback cb, void *cbContext)
{
	struct smpte337_detector_s *ctx = calloc(1, sizeof(*ctx));
	if (!ctx)
		return NULL;

	ctx->cb = cb;
	ctx->cbContext = cbContext;
	ctx->rb = rb_new_threadsafe(32 * 1024, 256 * 1024);
	if (!ctx->rb) {
		free(ctx);
		return NULL;
	}

	return ctx;
}

void smpte337_detector_free(struct smpte337_detector_s *ctx)
{
	rb_free(ctx->rb);
	free(ctx);
}

static void handleCallback(struct smpte337_detector_s *ctx, uint8_t datamode, uint8_t datatype,
	uint32_t payload_bitCount, uint8_t *payload)
{
	ctx->cb(ctx->cbContext, ctx, datamode, datatype, payload_bitCount, payload);
}

/* 16b mode is largely untested, fair wanring. */
static size_t smpte337_detector_write_16b(struct smpte337_detector_s *ctx, uint8_t *buf,
	uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame,
	uint32_t frameStrideBytes,
	uint32_t spanCount)
{
	size_t consumed = 0;

	uint16_t *p = (uint16_t *)buf;
	for (int i = 0; i < audioFrames; i++) {

		uint16_t *q = p;
		for (int k = 0; k < spanCount; k++) {
			/* Sample in N words into a byte orientied buffer */
			uint8_t *x = (uint8_t *)q;

			/* Flush the word into the fifo MSB first */
			int didOverflow = 0;
			rb_write_with_state(ctx->rb, ((const char *)x) + 1, 1, &didOverflow);
			if (didOverflow) {
				fprintf(stderr, "overflow occured.\n");
			}
			rb_write_with_state(ctx->rb, ((const char *)x) + 0, 1, &didOverflow);
			if (didOverflow) {
				fprintf(stderr, "overflow occured.\n");
			}
			q++;
			consumed += 2;
		}

		p += (frameStrideBytes / sizeof(uint16_t));
	}
	return consumed;
}

static size_t smpte337_detector_write_32b(struct smpte337_detector_s *ctx, uint8_t *buf,
	uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame,
	uint32_t frameStrideBytes,
	uint32_t spanCount)
{
	size_t consumed = 0;

	uint32_t *p = (uint32_t *)buf;
	for (int i = 0; i < audioFrames; i++) {

		uint32_t *q = p;
		for (int k = 0; k < spanCount; k++) {
			/* Sample in N words into a byte orientied buffer */
			uint8_t *x = (uint8_t *)q;

			if (ctx->wordLength == 16) {
				/* Flush the word into the fifo MSB first */
				int didOverflow = 0;
				rb_write_with_state(ctx->rb, ((const char *)x) + 3, 1, &didOverflow);
				if (didOverflow) {
					fprintf(stderr, "overflow occured.\n");
				}
				rb_write_with_state(ctx->rb, ((const char *)x) + 2, 1, &didOverflow);
				if (didOverflow) {
					fprintf(stderr, "overflow occured.\n");
				}
				consumed += 2;
			} else
			if (ctx->wordLength == 24) {
				/* Flush the word into the fifo MSB first */
				int didOverflow = 0;
				rb_write_with_state(ctx->rb, ((const char *)x) + 3, 1, &didOverflow);
				if (didOverflow) {
					fprintf(stderr, "overflow occured.\n");
				}
				rb_write_with_state(ctx->rb, ((const char *)x) + 2, 1, &didOverflow);
				if (didOverflow) {
					fprintf(stderr, "overflow occured.\n");
				}
				rb_write_with_state(ctx->rb, ((const char *)x) + 1, 1, &didOverflow);
				if (didOverflow) {
					fprintf(stderr, "overflow occured.\n");
				}
				consumed += 3;
			}
			q++;
		}

		p += (frameStrideBytes / sizeof(uint32_t));
	}
	return consumed;
}

static int smpte337_detector_hunt_syncwords(struct smpte337_detector_s *ctx, uint8_t *buf,
	uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame,
	uint32_t frameStrideBytes,
	uint32_t spanCount)
{
	uint32_t *p = (uint32_t *)buf;
	for (int i = 0; i < audioFrames - 1; i++) {

		uint32_t Pa = *p;
		uint32_t Pb = *(p + (frameStrideBytes / sizeof(uint32_t)));

		if ((Pa == 0xf8720000) && (Pb == 0x4e1f0000)) {
			return 16;
		} else
		if ((Pa == 0x6f872000) && (Pb == 0x54e1f000)) {
			return 20;
		} else
		if ((Pa == 0x96f87200) && (Pb == 0xa54e1f00)) {
			return 24;
		}

		p += (frameStrideBytes / sizeof(uint32_t));
	}
	return 0;
}

static void run_detector(struct smpte337_detector_s *ctx)
{
	int skipped = 0;

#define PEEK_LEN 16
	uint8_t dat[PEEK_LEN];
	while(1) {
		if (rb_used(ctx->rb) < PEEK_LEN)
			break;

		if (rb_peek(ctx->rb, (char *)&dat[0], PEEK_LEN) < PEEK_LEN)
			break;

		/* Find the supported patterns - In this case, AC3 only in 16bit mode */
		/* See SMPTE 337M 2015 spec table 6.
		 * Pa = dat0/1
		 * Pb = dat2/3 ... etc
		 */
		if (dat[0] == 0x96 && dat[1] == 0xf8 && dat[2] == 0x72 && dat[3] == 0xa5 && dat[4] == 0x4e && dat[5] == 0x1f) {
			/* Confirmed.... pa = 24bit, pb = 24bit */
#if 1
			for (int h = 0; h < 12; h++)
				printf("%02x ", dat[h]);
			printf("\n");

			printf("mode = %d, type = %d\n", (dat[8] >> 5) & 0x03, (dat[8] & 0x1f));
#endif
			/* Check the burst_info.... Make sure we find AC3 */
			if ((dat[8] & 0x1f) == 0x01) {
				/* Bits 0-4 datatype, 1 = AC3 */
				/* Bits 5-6 datamode, 2 = 24bit */
				/* Bits   7 errorflg, 0 = no error */
				uint32_t payload_bitCount = (dat[9] << 16) | dat[10] << 8 | dat[11];
				uint32_t payload_byteCount = payload_bitCount / 8;
				
				if (rb_used(ctx->rb) >= (8 + payload_byteCount)) {
					char *payload = NULL;
					size_t l = rb_read_alloc(ctx->rb, &payload, 12 + payload_byteCount);
					if (l != (8 + payload_byteCount)) {
						fprintf(stderr, "[smpte337_detector] Warning, rb read failure.\n");

						/* Intensionally flush the ring and start acquisition again. */
						rb_empty(ctx->rb);
					} else {
						handleCallback(ctx, (dat[8] >> 5) & 0x03, dat[8] & 0x1f,
							payload_bitCount, (uint8_t *)payload + 12);
					}
					if (payload)
						free(payload);
				} else {
					/* Not enough data in the ring buffer, come back next time. */
					break;
				}

			} else {
				fprintf(stderr, "[smpte337_detector] Does not support datatype 0x%02x in %d bit words, skipping.\n",
					dat[7] & 0x1f, ctx->wordLength);
				rb_discard(ctx->rb, 1); /* Pop a byte, and continue the search */
				skipped++;
			}
		} else
		if (dat[0] == 0xF8 && dat[1] == 0x72 && dat[2] == 0x4e && dat[3] == 0x1f) {
			/* Confirmed.... pa = 16bit, pb = 16bit */

			/* Check the burst_info.... */
			if ((dat[5] & 0x1f) == 0x01) {
				/* Bits 0-4 datatype, 1 = AC3 */
				/* Bits 5-6 datamode, 0 = 16bit */
				/* Bits   7 errorflg, 0 = no error */
				uint32_t payload_bitCount = (dat[6] << 8) | dat[7];
				uint32_t payload_byteCount = payload_bitCount / 8;
				
				if (rb_used(ctx->rb) >= (8 + payload_byteCount)) {
					char *payload = NULL;
					size_t l = rb_read_alloc(ctx->rb, &payload, 8 + payload_byteCount);
					if (l != (8 + payload_byteCount)) {
						fprintf(stderr, "[smpte337_detector] Warning, rb read failure.\n");

						/* Intensionally flush the ring and start acquisition again. */
						rb_empty(ctx->rb);
					} else {
						handleCallback(ctx, (dat[5] >> 5) & 0x03, dat[5] & 0x1f,
							payload_bitCount, (uint8_t *)payload + 8);
					}
					if (payload)
						free(payload);
				} else {
					/* Not enough data in the ring buffer, come back next time. */
					break;
				}

			} else {
				fprintf(stderr, "[smpte337_detector] Does not support datatype 0x%02x in %d bit words, skipping.\n",
					dat[7] & 0x1f, ctx->wordLength);
				rb_discard(ctx->rb, 1); /* Pop a byte, and continue the search */
				skipped++;
			}
		} else {
			rb_discard(ctx->rb, 1); /* Pop a byte, and continue the search */
			skipped++;
		}

	} /* while */
}

size_t smpte337_detector_write(struct smpte337_detector_s *ctx, uint8_t *buf,
	uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame,
	uint32_t frameStrideBytes, uint32_t spanCount)
{
	if ((!buf) || (!audioFrames) || (!channelsPerFrame) || (!frameStrideBytes) ||
		((sampleDepth != 16) && (sampleDepth != 32)) ||
		(spanCount == 0) || (spanCount > channelsPerFrame)) {
		return 0;
	}

	if (ctx->wordLength == 0) {
		int ret = smpte337_detector_hunt_syncwords(ctx, buf, audioFrames, sampleDepth,
			channelsPerFrame, frameStrideBytes, spanCount);
		if (ret > 0) {
			printf("Syncronized with %dbit words\n", ret);
			ctx->wordLength = ret;
		}
	}

	if (ctx->wordLength == 0)
		return 0;

	size_t ret = 0;
	if (sampleDepth == 16) {
		ret = smpte337_detector_write_16b(ctx, buf, audioFrames, sampleDepth,
			channelsPerFrame, frameStrideBytes, spanCount);
	} else
	if (sampleDepth == 32) {
		ret = smpte337_detector_write_32b(ctx, buf, audioFrames, sampleDepth,
			channelsPerFrame, frameStrideBytes, spanCount);
	}

	/* Now all the fifo contains byte stream re-ordered data, run the detector. */
	run_detector(ctx);

	return ret;
}
