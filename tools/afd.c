/*
 * Copyright (c) 2020 Kernel Labs Inc. All Rights Reserved
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
#include <libklvanc/vanc.h>

/* Normally we don't use a global, but we know our test harness will never be
   multi-threaded, and this is a really easy way to get the results out of the
   callback for comparison */
static uint16_t vancResult[16384];
static size_t vancResultCount;
static int passCount = 0;
static int failCount = 0;

#define SHOW_DETAIL 1

/* CALLBACKS for message notification */
static int cb_afd(void *callback_context, struct klvanc_context_s *ctx,
		      struct klvanc_packet_afd_s *pkt)
{
	int ret = -1;

#ifdef SHOW_DETAIL
	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	ret = klvanc_dump_AFD(ctx, pkt);
	if (ret != 0) {
		fprintf(stderr, "Error dumping AFD packet!\n");
		return -1;
	}
#endif

	uint16_t *words;
	uint16_t wordCount;
	ret = klvanc_convert_AFD_to_words(pkt, &words, &wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		return -1;
	}

	memcpy(vancResult, words, wordCount * sizeof(uint16_t));
	vancResultCount = wordCount;
	free(words);

	return 0;
}

static struct klvanc_callbacks_s callbacks =
{
	.afd		= cb_afd,
};
/* END - CALLBACKS for message notification */

/* svcinfo_present flag set but section missing */
unsigned short test_data_afd_1[] = {
	0x0000, 0x03ff, 0x03ff, 0x0241, 0x0205, 0x0108, 0x0200, 0x0200,
	0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x0200, 0x014e
};

static int test_afd_u16(struct klvanc_context_s *ctx, const unsigned short *arr, int items)
{
	int mismatch = 0;

	printf("\nParsing a new AFD VANC packet (%d words)......\n", items);

	/* Clear out any previous results in case the callback never fires */
	vancResultCount = 0;

	printf("Original Input\n");
	for (int i = 0; i < items; i++) {
		printf("%04x ", arr[i]);
	}
	printf("\n");

	int ret = klvanc_packet_parse(ctx, 9, arr, items);

	printf("Final output\n");
	for (int i = 0; i < vancResultCount; i++) {
		printf("%04x ", vancResult[i]);
	}
	printf("\n");

	for (int i = 0; i < vancResultCount; i++) {
		if (arr[i] != vancResult[i]) {
			fprintf(stderr, "Mismatch starting at offset 0x%02x\n", i);
			mismatch = 1;
			break;
		}
	}
	if (vancResultCount == 0) {
		/* No output at all.  This is usually because the VANC parser choked
		   on the VANC checksum and thus the parser never ran */
		fprintf(stderr, "No output generated\n");
		mismatch = 1;
	}

	if (mismatch) {
		printf("Printing mismatched structure:\n");
		failCount++;
		ret = klvanc_packet_parse(ctx, 13, vancResult, vancResultCount);
	} else {
		printf("Original and generated versions match!\n");
		passCount++;
	}

	return ret;
}

#ifdef UNUSED
static int test_afd(struct klvanc_context_s *ctx, const uint8_t *buf, size_t bufSize)
{
	int numWords = bufSize / 2;
	int ret;


	uint16_t *arr = malloc(bufSize);
	if (arr == NULL)
		return -1;

	for (int i = 0; i < numWords; i++) {
		arr[i] = buf[i * 2] << 8 | buf[i * 2 + 1];
	}

	ret = test_afd_u16(ctx, arr, numWords);

	free(arr);

	return ret;
}
#endif

int afd_main(int argc, char *argv[])
{
	struct klvanc_context_s *ctx;
	int ret;

	if (klvanc_context_create(&ctx) < 0) {
		fprintf(stderr, "Error initializing library context\n");
		exit(1);
	}
#ifdef SHOW_DETAIL
	ctx->verbose = 1;
#endif
	ctx->callbacks = &callbacks;
	printf("Library initialized.\n");

	ret = test_afd_u16(ctx, test_data_afd_1, sizeof(test_data_afd_1) / sizeof(unsigned short));
	if (ret < 0)
		fprintf(stderr, "AFD-1 failed to parse\n");

	klvanc_context_destroy(ctx);
	printf("Library destroyed.\n");

	printf("Final result: PASS: %d/%d, Failures: %d\n",
	       passCount, passCount + failCount, failCount);
	if (failCount != 0)
		return 1;
	return 0;
}
