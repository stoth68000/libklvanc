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

#include <stdio.h>
#include <stdlib.h>
#include <libklvanc/vanc.h>

/* CALLBACKS for message notification */
static int cb_AFD(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_afd_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	klvanc_dump_AFD(ctx, pkt);

	return 0;
}

static int cb_EIA_708B(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_708b_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	klvanc_dump_EIA_708B(ctx, pkt);

	return 0;
}

static int cb_EIA_608(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_608_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	klvanc_dump_EIA_608(ctx, pkt);

	return 0;
}

static int cb_SCTE_104(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
	int ret = -1;
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	klvanc_dump_SCTE_104(ctx, pkt);

	uint16_t *words;
	uint16_t wordCount;
	ret = klvanc_convert_SCTE_104_to_words(ctx, pkt, &words, &wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		return -1;
	}

	printf("Final Output\n");
	for (int i = 0; i < wordCount; i++) {
		printf("%04x ", words[i]);
	}
	printf("\n");
	free(words);

	return 0;
}

static int cb_all(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_header_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);
	return 0;
}

static int cb_VANC_TYPE_KL_UINT64_COUNTER(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_kl_u64le_counter_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);
	return 0;
}

static struct klvanc_callbacks_s callbacks =
{
	.afd			= cb_AFD,
	.eia_708b		= cb_EIA_708B,
	.eia_608		= cb_EIA_608,
	.scte_104		= cb_SCTE_104,
	.all			= cb_all,
	.kl_i64le_counter	= cb_VANC_TYPE_KL_UINT64_COUNTER,
};
/* END - CALLBACKS for message notification */

static int test_AFD(struct klvanc_context_s *ctx)
{
	unsigned short arr[] = {
		0x000,
		0x3ff,
		0x3ff,
		0x241,
		0x105,
		0x108,
		0x07c, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x000, /* Payload Field */
		0x010, /* Payload Field */
		0x000, /* Payload Field */
		0x008, /* Payload Field */
		0x2e2, /* Checksum is valid */
	};

	/* report that this was from line 13, informational only. */
	int ret = klvanc_packet_parse(ctx, 13, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

static int test_EIA_708B(struct klvanc_context_s *ctx)
{
	unsigned short arr[] = {
		0x0, 0x0, 0x0, /* Spurious junk prefix for testing */
		0x000, 0x3ff, 0x3ff, 0x161, 0x101, 0x152, 0x296, 0x269,
		0x152, 0x14f, 0x277, 0x2b8, 0x1ad, 0x272, 0x1f4, 0x2fc,
		0x180, 0x180, 0x1fd, 0x180, 0x180, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200,
		0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa,
		0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200,
		0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa,
		0x200, 0x200, 0x2fa, 0x200, 0x200, 0x2fa, 0x200, 0x200,
		0x2fa, 0x200, 0x200, 0x173, 0x2d1, 0x1e0, 0x200, 0x200,
		0x200, 0x200, 0x200, 0x200, 0x274, 0x2b8, 0x1ad, 0x194,
		0x1b4 /* Checksum */
	};

	int ret = klvanc_packet_parse(ctx, 1, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

static int test_checksum()
{
	/* 3 words ADF, 31 words of message, 1 words checksum */
	unsigned short arr[] = {
		0x0000, 0x03ff, 0x03ff,
		0x0241, 0x0107, 0x011c, 0x0108, 0x0200, 0x0101, 0x0200, 0x021b,
		0x02ff, 0x02ff, 0x02ff, 0x02ff, 0x0200, 0x0200, 0x0200, 0x0200,
		0x0200, 0x0104, 0x0200, 0x0200, 0x0200, 0x0102, 0x0200, 0x0101,
		0x0200, 0x0200, 0x0200, 0x0200 ,0x0200, 0x0200, 0x0101
	};

	uint16_t sum = klvanc_checksum_calculate(&arr[3], 31);
	if (sum != 0x28c)
		return -1;

	unsigned short arr2[] = {
		0x0000, 0x03ff, 0x03ff,
		0x0241, 0x0107, 0x011c, 0x0108, 0x0200, 0x0101, 0x0200, 0x021b,
		0x02ff, 0x02ff, 0x02ff, 0x02ff, 0x0200, 0x0200, 0x0200, 0x0200,
		0x0200, 0x0104, 0x0200, 0x0200, 0x0200, 0x0102, 0x0200, 0x0101,
		0x0200, 0x0200, 0x0200, 0x0200 ,0x0200, 0x0200, 0x0101,
		0x028c
	};

	if (!klvanc_checksum_is_valid(&arr2[3], 32))
		return -1;

	printf("Checksum test passed.\n");

	return 0;
}

static unsigned char __0_vancentry[] = {
	0x00, 0x00, 0x03, 0xff, 0x03, 0xff, 0x02, 0x41, 0x01, 0x07, 0x01, 0x52,
	0x01, 0x08, 0x02, 0xff, 0x02, 0xff, 0x02, 0x00, 0x01, 0x51, 0x02, 0x00,
	0x02, 0x00, 0x01, 0x52, 0x02, 0x00, 0x02, 0x05, 0x02, 0x00, 0x02, 0x00,
	0x02, 0x06, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x01, 0x0e, 0x01, 0x02,
	0x01, 0x40, 0x02, 0x00, 0x02, 0x00, 0x01, 0x52, 0x02, 0x00, 0x01, 0x64,
	0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x02, 0x90, 0x02, 0x03, 0x01, 0x01,
	0x02, 0x00, 0x01, 0x01, 0x01, 0x04, 0x02, 0x00, 0x01, 0x02, 0x02, 0x00,
	0x02, 0x00, 0x01, 0x01, 0x02, 0x09, 0x02, 0x00, 0x02, 0x03, 0x02, 0x00,
	0x01, 0x01, 0x02, 0x30, 0x01, 0x01, 0x01, 0x0b, 0x02, 0x00, 0x02, 0x12,
	0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00,
	0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x01, 0x40, 0x02, 0x00, 0x02, 0x00,
	0x02, 0x00, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x02, 0x00, 0x02, 0x03,
	0x01, 0x01, 0x01, 0x02, 0x02, 0x00, 0x02, 0x00, 0x01, 0x01, 0x01, 0x08,
	0x02, 0x00, 0x01, 0x08, 0x01, 0x01, 0x01, 0x0b, 0x02, 0x05, 0x01, 0x54,
	0x02, 0x56, 0x02, 0x4e, 0x01, 0x54, 0x02, 0x00, 0x02, 0x06
};

static int test_scte_104(struct klvanc_context_s *ctx)
{
	printf("\nParsing a new SCTE104 VANC packet......\n");
	uint16_t *arr = malloc(sizeof(__0_vancentry) / 2 * sizeof(uint16_t));
	if (arr == NULL)
		return -1;

	for (int i = 0; i < (sizeof(__0_vancentry) / 2); i++) {
		arr[i] = __0_vancentry[i * 2] << 8 | __0_vancentry[i * 2 + 1];
	}

	printf("Original Input\n");
	for (int i = 0; i < (sizeof(__0_vancentry) / 2); i++) {
		printf("%04x ", arr[i]);
	}
	printf("\n");

	int ret = klvanc_packet_parse(ctx, 13, arr, sizeof(__0_vancentry) / sizeof(unsigned short));
	free(arr);

	return ret;
}

int demo_main(int argc, char *argv[])
{
	struct klvanc_context_s *ctx;
	int ret;

	if (klvanc_context_create(&ctx) < 0) {
		fprintf(stderr, "Error initializing library context\n");
		exit(1);
	}
	ctx->verbose = 1;
	ctx->callbacks = &callbacks;
	printf("Library initialized.\n");

	ret = test_AFD(ctx);
	if (ret < 0)
		fprintf(stderr, "AFD failed to parse\n");

	ret = test_EIA_708B(ctx);
	if (ret < 0)
		fprintf(stderr, "EIA_708B failed to parse\n");

	ret = test_scte_104(ctx);
	if (ret < 0)
		fprintf(stderr, "SCTE-104 failed to parse\n");

	ret = test_checksum();
	if (ret < 0)
		fprintf(stderr, "Checksum calculation failed\n");

	klvanc_context_destroy(ctx);
	printf("Library destroyed.\n");

	return 0;
}
