/*
 * Copyright (c) 2026 Kernel Labs Inc. All Rights Reserved
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
#include <libklvanc/vanc-checksum.h>
#include <libklvanc/vanc-smpte_299.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static struct klvanc_packet_smpte_299_control_s decoded;
static unsigned int callback_count;
static int pass_count;
static int fail_count;

static int callback(void *opaque, struct klvanc_context_s *ctx,
		struct klvanc_packet_smpte_299_control_s *packet)
{
	(void)opaque;
	(void)ctx;
	decoded = *packet;
	callback_count++;
	return 0;
}

static struct klvanc_callbacks_s callbacks = {
	.smpte_299_control = callback,
};

static void check(int condition, const char *message)
{
	if (condition) {
		pass_count++;
		return;
	}
	fail_count++;
	fprintf(stderr, "FAIL: %s\n", message);
}

static uint16_t wire_word(uint16_t value)
{
	value &= 0x1ffu;
	return value | ((value & 0x100u) ? 0 : 0x200u);
}

static void encode_delay(uint16_t *words, int32_t samples, int valid)
{
	uint32_t raw = (uint32_t)samples & 0x03ffffffu;
	words[0] = wire_word((uint16_t)(((raw & 0xffu) << 1) |
		(valid ? 1u : 0u)));
	words[1] = wire_word((uint16_t)((raw >> 8) & 0x1ffu));
	words[2] = wire_word((uint16_t)((raw >> 17) & 0x1ffu));
}

static int decode(struct klvanc_context_s *ctx, uint8_t did,
		const uint16_t payload[11])
{
	uint8_t empty[11] = {0};
	uint16_t *words = NULL;
	uint16_t word_count = 0;
	int result = klvanc_sdi_create_payload(0, did, empty, 11, &words,
		&word_count, 10);
	if (result)
		return result;
	for (unsigned int i = 0; i < 11; ++i)
		words[6 + i] = payload[i];
	words[word_count - 1] = klvanc_checksum_calculate(words + 3,
		word_count - 4);
	memset(&decoded, 0, sizeof(decoded));
	callback_count = 0;
	result = klvanc_packet_parse(ctx, 9, words, word_count);
	free(words);
	return result;
}

static void test_st299_1(struct klvanc_context_s *ctx)
{
	uint16_t payload[11] = {0};
	payload[0] = wire_word(0x101);
	payload[1] = wire_word(0x009); /* 96 kHz, asynchronous. */
	payload[2] = wire_word(0x005); /* First and third channel active. */
	encode_delay(&payload[3], 12345, 1);
	encode_delay(&payload[6], -321, 1);
	payload[9] = wire_word(0);
	payload[10] = wire_word(0);

	check(decode(ctx, 0xe2, payload) >= 0, "ST 299-1 packet parses");
	check(callback_count == 1, "ST 299-1 callback fires once");
	check(decoded.standard_part == 1 && decoded.group == 2,
	      "ST 299-1 DID maps to group 2");
	check(decoded.first_channel == 5 && decoded.last_channel == 8,
	      "ST 299-1 group maps to channels 5-8");
	check(decoded.audio_frame_number == 0x101,
	      "nine-bit audio frame number is preserved");
	check(decoded.rate_code == 4 && decoded.sample_rate_hz == 96000 &&
	      decoded.asynchronous, "rate and asynchronous mode decode");
	check(decoded.active_channel_mask == 0x5,
	      "active-channel mask decodes");
	check(decoded.delay[0].valid && decoded.delay[0].samples == 12345,
	      "positive channel-pair delay decodes");
	check(decoded.delay[1].valid && decoded.delay[1].samples == -321,
	      "negative channel-pair delay decodes");
}

static void test_st299_2(struct klvanc_context_s *ctx)
{
	uint16_t payload[11] = {0};
	for (unsigned int i = 0; i < 11; ++i)
		payload[i] = wire_word(0);
	payload[1] = wire_word(0); /* 48 kHz, synchronous. */
	payload[2] = wire_word(0x0f);

	check(decode(ctx, 0xa0, payload) >= 0, "ST 299-2 packet parses");
	check(callback_count == 1, "ST 299-2 callback fires once");
	check(decoded.standard_part == 2 && decoded.group == 8,
	      "ST 299-2 DID maps to group 8");
	check(decoded.first_channel == 29 && decoded.last_channel == 32,
	      "ST 299-2 group maps to channels 29-32");
	check(decoded.sample_rate_hz == 48000 && !decoded.asynchronous,
	      "ST 299-2 rate and synchronous mode decode");
	check(decoded.active_channel_mask == 0x0f,
	      "ST 299-2 active channels decode");
}

static void test_malformed_length(struct klvanc_context_s *ctx)
{
	uint8_t payload[10] = {0};
	uint16_t *words = NULL;
	uint16_t word_count = 0;
	callback_count = 0;
	check(klvanc_sdi_create_payload(0, 0xe3, payload, 10, &words,
	      &word_count, 10) == 0, "malformed fixture builds");
	klvanc_packet_parse(ctx, 9, words, word_count);
	check(callback_count == 0, "malformed packet does not invoke callback");
	free(words);
}

int smpte299_main(int argc, char *argv[])
{
	struct klvanc_context_s *ctx = NULL;
	(void)argc;
	(void)argv;
	if (klvanc_context_create(&ctx) < 0 || !ctx) {
		fprintf(stderr, "Unable to create libklvanc context\n");
		return 1;
	}
	ctx->callbacks = &callbacks;
	test_st299_1(ctx);
	test_st299_2(ctx);
	test_malformed_length(ctx);
	klvanc_context_destroy(ctx);
	printf("Final result: PASS: %d/%d, Failures: %d\n",
	       pass_count, pass_count + fail_count, fail_count);
	return fail_count ? 1 : 0;
}
