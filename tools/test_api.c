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

/* Purpose: Functional test suite exercising the public API of libklvanc,
 *          plus tools/klringbuffer.c and tools/pes_extractor.c (foundational
 *          to every network/PES ingest path in this tree). This is a
 *          regression harness, not a fuzzer -- it is meant to be run after
 *          every change to catch behavioral breakage.
 *
 *          This is a review-only/test-only pass: NO source files other than
 *          this one, the dispatcher (klvanc_util.c) and the build files
 *          (Makefile.am) have been modified. Every bug found during the
 *          accompanying code review is left exactly as-is in the library;
 *          this file only adds coverage for it.
 *
 *          A number of the review's confirmed findings are genuine memory
 *          corruption bugs (OOB read/write, integer underflow feeding a
 *          malloc/memcpy size, use of a never-initialized pthread_mutex_t).
 *          Exercising those deliberately would crash or corrupt this test
 *          binary if run in-process, so each one is run inside a forked
 *          child via run_isolated() -- a crash there is caught and reported
 *          as a clean FAIL for that one test, instead of taking down the
 *          rest of the suite. See the "CRITICAL / MODERATE REGRESSION
 *          TESTS" section near the bottom for the full list, each annotated
 *          with the specific finding it targets.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <pthread.h>

#include <libklvanc/vanc.h>
#include <libklvanc/vanc-lines.h>

#include "klringbuffer.h"
#include "pes_extractor.h"

static int g_pass = 0;
static int g_fail = 0;

#define CHECK(cond, fmt, ...) do { \
	if (cond) { \
		g_pass++; \
	} else { \
		g_fail++; \
		printf("  [FAIL] %s:%d: " fmt "\n", __func__, __LINE__, ##__VA_ARGS__); \
	} \
} while (0)

#define SECTION(name) printf("\n=== %s ===\n", name)

/* ------------------------------------------------------------------- */
/* Process-isolated regression harness -- see file header for why.      */
/* ------------------------------------------------------------------- */
static void run_isolated(const char *name, void (*fn)(void))
{
	fflush(stdout);
	fflush(stderr);

	pid_t pid = fork();
	if (pid < 0) {
		g_fail++;
		printf("  [FAIL] %s: fork() failed\n", name);
		return;
	}

	if (pid == 0) {
		fn();
		_exit(125); /* fn() must always terminate via _exit() itself */
	}

	int status = 0;
	waitpid(pid, &status, 0);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
		g_pass++;
		printf("  [PASS] %s\n", name);
	} else if (WIFSIGNALED(status)) {
		g_fail++;
		printf("  [FAIL] %s: subprocess was killed by signal %d (%s)\n",
		       name, WTERMSIG(status), strsignal(WTERMSIG(status)));
	} else {
		g_fail++;
		printf("  [FAIL] %s: subprocess exited with status %d\n",
		       name, WEXITSTATUS(status));
	}
}

/* ------------------------------------------------------------------- */
/* Shared helper: build a well-formed VANC line for a given DID/SDID    */
/* using the library's own klvanc_sdi_create_payload(), so every test   */
/* below exercises real ADF/checksum/parity generation rather than      */
/* hand-rolled bytes.                                                   */
/* ------------------------------------------------------------------- */
static int build_vanc_line(uint8_t did, uint8_t sdid, const uint8_t *payload, uint16_t payloadLen,
			   uint16_t **words, uint16_t *wordCount)
{
	return klvanc_sdi_create_payload(sdid, did, payload, payloadLen, words, wordCount, 10);
}

/* ------------------------------------------------------------------- */
/* CORE: context, checksum, DID lookup, sdi_create_payload,             */
/* packet_copy/free/save/payload_append, packet_parse argument checks,  */
/* lookupDescriptionByType/lookupSpecificationByType.                   */
/* ------------------------------------------------------------------- */
static void test_core_context(void)
{
	SECTION("klvanc_context_create()/_destroy()/_dump()");

	struct klvanc_context_s *ctx = NULL;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	CHECK(ctx != NULL, "context pointer populated");
	if (!ctx)
		return;

	CHECK(klvanc_context_dump(ctx) == 0, "context_dump succeeds on a live context");
	CHECK(klvanc_context_dump(NULL) < 0, "context_dump rejects NULL");

	CHECK(klvanc_context_destroy(ctx) == 0, "context_destroy succeeds");
	CHECK(klvanc_context_destroy(NULL) < 0, "context_destroy rejects NULL");
}

static void test_core_checksum(void)
{
	SECTION("klvanc_checksum_calculate()/_is_valid()");

	uint16_t words[] = { 0x111, 0x222, 0x333 };
	uint16_t chk = klvanc_checksum_calculate(words, 3);
	CHECK(chk != 0, "checksum_calculate produces a nonzero value for nonzero input");

	uint16_t withchk[4] = { words[0], words[1], words[2], chk };
	CHECK(klvanc_checksum_is_valid(withchk, 4) == 1, "checksum_is_valid accepts a correctly computed checksum");

	withchk[3] ^= 0x1ff; /* corrupt */
	CHECK(klvanc_checksum_is_valid(withchk, 4) == 0, "checksum_is_valid rejects a corrupted checksum");
}

static void test_core_did(void)
{
	SECTION("klvanc_didLookupDescription()/_Specification()");

	/* 0x41/0x07 = SCTE-104, a well-known registered pairing */
	const char *desc = klvanc_didLookupDescription(0x41, 0x07);
	const char *spec = klvanc_didLookupSpecification(0x41, 0x07);
	CHECK(desc != NULL, "description lookup never returns NULL for a known pair");
	CHECK(spec != NULL, "specification lookup never returns NULL for a known pair");

	/* An unassigned pairing must still return a valid (non-NULL) string */
	CHECK(klvanc_didLookupDescription(0x00, 0x00) != NULL, "description lookup never returns NULL for an unknown pair");
	CHECK(klvanc_didLookupSpecification(0x00, 0x00) != NULL, "specification lookup never returns NULL for an unknown pair");
}

static void test_core_lookup_by_type(void)
{
	SECTION("klvanc_lookupDescriptionByType()/_SpecificationByType()");

	CHECK(klvanc_lookupDescriptionByType(VANC_TYPE_SCTE_104) != NULL, "description lookup by type never returns NULL");
	CHECK(klvanc_lookupSpecificationByType(VANC_TYPE_SCTE_104) != NULL, "specification lookup by type never returns NULL");
	CHECK(strcmp(klvanc_lookupDescriptionByType(VANC_TYPE_UNDEFINED), "UNDEFINED") == 0,
	      "unrecognized type reports UNDEFINED");
}

static void test_core_sdi_create_payload(void)
{
	SECTION("klvanc_sdi_create_payload()");

	uint8_t payload[] = { 0xde, 0xad, 0xbe, 0xef };
	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	int ret = build_vanc_line(0x41, 0x07, payload, sizeof(payload), &words, &wordCount);
	CHECK(ret == 0, "sdi_create_payload succeeds");
	CHECK(words != NULL, "words buffer allocated");
	CHECK(wordCount == 6 + sizeof(payload) + 1, "wordCount matches header(6) + payload + checksum(1)");
	if (words) {
		CHECK(words[0] == 0x000 && words[1] == 0x3ff && words[2] == 0x3ff, "ADF sequence correct");
		CHECK((words[3] & 0xff) == 0x41, "DID correct");
		CHECK((words[4] & 0xff) == 0x07, "SDID correct");
		CHECK((words[5] & 0xff) == sizeof(payload), "data count correct");
		free(words);
	}

	/* Argument validation */
	CHECK(build_vanc_line(0x41, 0x07, NULL, 4, &words, &wordCount) < 0, "rejects NULL src");
	CHECK(build_vanc_line(0x41, 0x07, payload, 0, &words, &wordCount) < 0, "rejects zero srcByteCount");
	CHECK(klvanc_sdi_create_payload(0x07, 0x41, payload, sizeof(payload), NULL, &wordCount, 10) < 0, "rejects NULL dst");
	CHECK(klvanc_sdi_create_payload(0x07, 0x41, payload, sizeof(payload), &words, &wordCount, 8) < 0, "rejects bitDepth != 10");
}

static void test_core_packet_parse_argcheck(void)
{
	SECTION("klvanc_packet_parse() argument validation");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	if (!ctx)
		return;

	uint16_t dummy[8] = { 0 };
	CHECK(klvanc_packet_parse(NULL, 1, dummy, 8) < 0, "rejects NULL ctx");
	CHECK(klvanc_packet_parse(ctx, 1, NULL, 8) < 0, "rejects NULL words array");
	CHECK(klvanc_packet_parse(ctx, 1, dummy, 0) < 0, "rejects zero wordCount");
	CHECK(klvanc_packet_parse(ctx, 1, dummy, 8) == 0, "a line with no valid ADF header parses zero frames, not an error");

	klvanc_context_destroy(ctx);
}

static void test_core_packet_copy_free_save(void)
{
	SECTION("klvanc_packet_copy()/_free()/_save()");

	uint8_t payload[] = { 0x01, 0x02, 0x03 };
	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(build_vanc_line(0x41, 0x05, payload, sizeof(payload), &words, &wordCount) == 0, "build source line");
	if (!words)
		return;

	/* Reuse the library's own header parser (not klvanc_packet_parse, which
	   requires a callback dispatch) via a minimal AFD callback round trip,
	   to get a genuine struct klvanc_packet_header_s to copy/save. */
	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");

	/* klvanc_packet_header_s isn't directly constructible outside the
	   library, so synthesize one the same way core-packets.c does: parse
	   the line manually is not exposed, so instead confirm copy/free/save
	   handle NULL/invalid input safely, and exercise save()'s own
	   filter logic, which does not require a fully-parsed packet. */
	CHECK(klvanc_packet_save(NULL, NULL, -1, -1) == -1, "packet_save rejects NULL dir/pkt");

	free(words);
	klvanc_context_destroy(ctx);
}

static void test_core_packet_payload_append(void)
{
	SECTION("klvanc_packet_payload_append()");

	struct klvanc_packet_header_s *dst = calloc(1, sizeof(*dst));
	struct klvanc_packet_header_s *src = calloc(1, sizeof(*src));
	CHECK(dst != NULL && src != NULL, "alloc dst/src headers");
	if (!dst || !src) {
		free(dst);
		free(src);
		return;
	}

	src->payloadLengthWords = 4;
	src->rawLengthWords = 4;
	for (int i = 0; i < 4; i++) {
		src->payload[i] = 0x100 + i;
		src->raw[i] = 0x200 + i;
	}

	CHECK(klvanc_packet_payload_append(dst, src, 0) == 0, "append succeeds for a small, in-bounds payload");
	CHECK(dst->payloadLengthWords == 4, "payloadLengthWords updated");
	CHECK(dst->payload[0] == 0x100, "payload content copied");

	/* Overflow rejection: claim a payload length that would exceed
	   LIBKLVANC_PACKET_MAX_PAYLOAD once appended. */
	dst->payloadLengthWords = LIBKLVANC_PACKET_MAX_PAYLOAD - 2;
	src->payloadLengthWords = 10;
	CHECK(klvanc_packet_payload_append(dst, src, 0) < 0, "append rejects a payload overflow");

	free(dst);
	free(src);
}

/* ------------------------------------------------------------------- */
/* AFD                                                                  */
/* ------------------------------------------------------------------- */
static int g_afd_fired;
static struct klvanc_packet_afd_s g_afd_last;
static int cb_afd(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_afd_s *pkt)
{
	g_afd_fired = 1;
	g_afd_last = *pkt;
	return 0;
}

static void test_afd(void)
{
	SECTION("AFD: create/set/convert/parse round-trip/destroy");

	struct klvanc_packet_afd_s *pkt = NULL;
	CHECK(klvanc_create_AFD(&pkt) == 0, "klvanc_create_AFD succeeds");
	CHECK(pkt != NULL, "AFD packet allocated");
	if (!pkt)
		return;

	CHECK(klvanc_set_AFD_val(pkt, 0x08) == 0, "set_AFD_val accepts a valid code");
	CHECK(klvanc_set_AFD_val(pkt, 0xFF) < 0, "set_AFD_val rejects an invalid code");

	CHECK(klvanc_afd_to_string(pkt->afd) != NULL, "afd_to_string never returns NULL");
	CHECK(klvanc_aspectRatio_to_string(pkt->aspectRatio) != NULL, "aspectRatio_to_string never returns NULL");
	CHECK(klvanc_barFlags_to_string(pkt->barDataFlags) != NULL, "barFlags_to_string never returns NULL");

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_AFD_to_words(pkt, &words, &wordCount) == 0, "convert_AFD_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_AFD_to_packetBytes(pkt, &bytes, &byteCount) == 0, "convert_AFD_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");
	free(bytes);

	/* Parse it back through the real dispatch path */
	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .afd = cb_afd };
	ctx->callbacks = &cb;
	g_afd_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 10, words, wordCount) >= 0, "packet_parse succeeds on generated AFD line");
	CHECK(g_afd_fired == 1, "AFD callback fired");
	CHECK(g_afd_last.afd == pkt->afd, "round-tripped afd code matches");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_destroy_AFD(pkt);
}

/* ------------------------------------------------------------------- */
/* EIA-608                                                              */
/* ------------------------------------------------------------------- */
static int g_eia608_fired;
static int cb_eia608(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_eia_608_s *pkt)
{
	g_eia608_fired = 1;
	return 0;
}

static void test_eia608(void)
{
	SECTION("EIA-608: create/convert/parse round-trip/destroy");

	struct klvanc_packet_eia_608_s *pkt = NULL;
	CHECK(klvanc_create_EIA_608(&pkt) == 0, "klvanc_create_EIA_608 succeeds");
	CHECK(pkt != NULL, "EIA-608 packet allocated");
	if (!pkt)
		return;

	pkt->cc_data_1 = 0x80;

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_EIA_608_to_words(pkt, &words, &wordCount) == 0, "convert_EIA_608_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_EIA_608_to_packetBytes(pkt, &bytes, &byteCount) == 0, "convert_EIA_608_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");
	free(bytes);

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .eia_608 = cb_eia608 };
	ctx->callbacks = &cb;
	g_eia608_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 11, words, wordCount) >= 0, "packet_parse succeeds on generated EIA-608 line");
	CHECK(g_eia608_fired == 1, "EIA-608 callback fired");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_destroy_EIA_608(pkt);
}

/* ------------------------------------------------------------------- */
/* EIA-708B                                                             */
/* ------------------------------------------------------------------- */
static int g_eia708b_fired;
static int cb_eia708b(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_eia_708b_s *pkt)
{
	g_eia708b_fired = 1;
	return 0;
}

static void test_eia708b(void)
{
	SECTION("EIA-708B: create/set_framerate/finalize/convert/parse round-trip/destroy");

	struct klvanc_packet_eia_708b_s *pkt = NULL;
	CHECK(klvanc_create_eia708_cdp(&pkt) == 0, "klvanc_create_eia708_cdp succeeds");
	CHECK(pkt != NULL, "EIA-708B packet allocated");
	if (!pkt)
		return;

	/* NOTE: klvanc_set_framerate_EIA_708B()'s (num, den) are inverted
	   relative to the conventional "fps = num/den" notation -- 29.97fps
	   must be passed as (1001, 30000), not (30000, 1001) -- confirmed
	   against the function's own internal table. */
	CHECK(klvanc_set_framerate_EIA_708B(pkt, 1001, 30000) == 0, "set_framerate accepts a valid rate (29.97fps)");
	CHECK(klvanc_set_framerate_EIA_708B(pkt, 7, 13) < 0, "set_framerate rejects a rate not in its supported table");
	/* (0, 0) is deliberately NOT exercised here: gcd(0,0) == 0, so the
	   function's internal `num /= gcd_val` is a division by zero. That's
	   UB -- it silently evaluates to 0 on this platform (ARM64, where
	   integer division by zero doesn't trap) but is expected to raise
	   SIGFPE on x86. See child_moderate_eia708b_framerate_div_by_zero(). */
	klvanc_finalize_EIA_708B(pkt, 1);

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_EIA_708B_to_words(pkt, &words, &wordCount) == 0, "convert_EIA_708B_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_EIA_708B_to_packetBytes(pkt, &bytes, &byteCount) == 0, "convert_EIA_708B_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");
	free(bytes);

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .eia_708b = cb_eia708b };
	ctx->callbacks = &cb;
	g_eia708b_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 9, words, wordCount) >= 0, "packet_parse succeeds on generated EIA-708B line");
	CHECK(g_eia708b_fired == 1, "EIA-708B callback fired");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_destroy_eia708_cdp(pkt);
}

/* ------------------------------------------------------------------- */
/* KL_U64LE_COUNTER                                                     */
/* ------------------------------------------------------------------- */
static int g_kl_counter_fired;
static uint64_t g_kl_counter_last;
static int cb_kl_counter(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_kl_u64le_counter_s *pkt)
{
	g_kl_counter_fired = 1;
	g_kl_counter_last = pkt->counter;
	return 0;
}

static void test_kl_u64le_counter(void)
{
	SECTION("KL_U64LE_COUNTER: create/convert/parse round-trip");

	struct klvanc_packet_kl_u64le_counter_s *pkt = NULL;
	CHECK(klvanc_create_KL_U64LE_COUNTER(&pkt) == 0, "klvanc_create_KL_U64LE_COUNTER succeeds");
	CHECK(pkt != NULL, "packet allocated");
	if (!pkt)
		return;

	pkt->counter = 0x1122334455667788ULL;

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_KL_U64LE_COUNTER_to_words(pkt, &words, &wordCount) == 0, "convert_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .kl_i64le_counter = cb_kl_counter };
	ctx->callbacks = &cb;
	g_kl_counter_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 12, words, wordCount) >= 0, "packet_parse succeeds on generated counter line");
	CHECK(g_kl_counter_fired == 1, "counter callback fired");
	CHECK(g_kl_counter_last == pkt->counter, "round-tripped counter value matches");

	free(words);
	free(pkt);
	klvanc_context_destroy(ctx);
}

/* ------------------------------------------------------------------- */
/* SCTE-104                                                             */
/* ------------------------------------------------------------------- */
static int g_scte104_fired;
static uint32_t g_scte104_event_id;
static int cb_scte104(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
	g_scte104_fired = 1;
	if (pkt->mo_msg.num_ops > 0 && pkt->mo_msg.ops[0].opID == MO_SPLICE_REQUEST_DATA)
		g_scte104_event_id = pkt->mo_msg.ops[0].sr_data.splice_event_id;
	return 0;
}

static void test_scte104(void)
{
	SECTION("SCTE-104: alloc/Add_MOM_Op/convert/dump/parse round-trip/free");

	struct klvanc_packet_scte_104_s *pkt = NULL;
	CHECK(klvanc_alloc_SCTE_104(0xffff, &pkt) == 0, "klvanc_alloc_SCTE_104 succeeds");
	CHECK(pkt != NULL, "SCTE-104 packet allocated");
	if (!pkt)
		return;

	struct klvanc_multiple_operation_message_operation *op = NULL;
	CHECK(klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op) == 0, "Add_MOM_Op(splice) succeeds");
	CHECK(op != NULL, "op pointer populated");
	if (op) {
		op->sr_data.splice_insert_type = 0x02; /* spliceStart_immediate */
		op->sr_data.splice_event_id = 0xABCD1234;
		op->sr_data.unique_program_id = 0x4567;
		op->sr_data.avail_num = 1;
		op->sr_data.avails_expected = 1;
	}

	struct klvanc_multiple_operation_message_operation *op2 = NULL;
	CHECK(klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op2) == 0, "Add_MOM_Op(tier) succeeds");
	if (op2)
		op2->tier_data.tier_data = 0x0FFF;

	CHECK(pkt->mo_msg.num_ops == 2, "num_ops reflects both added ops");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");

	CHECK(klvanc_dump_SCTE_104(ctx, pkt) == 0, "dump_SCTE_104 succeeds");

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_SCTE_104_to_words(ctx, pkt, &words, &wordCount) == 0, "convert_SCTE_104_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_SCTE_104_to_packetBytes(ctx, pkt, &bytes, &byteCount) == 0, "convert_SCTE_104_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");

	uint8_t *smpte = NULL;
	uint16_t smpteLen = 0;
	CHECK(klvanc_convert_SCTE_104_packetbytes_to_SMPTE_2010(ctx, bytes, byteCount, &smpte, &smpteLen) == 0,
	      "convert_packetbytes_to_SMPTE_2010 succeeds");
	CHECK(smpte != NULL && smpteLen > 0, "SMPTE-2010-wrapped bytes populated");
	free(bytes);
	free(smpte);

	/* Parse the generated words back through the real dispatch path */
	struct klvanc_callbacks_s cb = { .scte_104 = cb_scte104 };
	ctx->callbacks = &cb;
	g_scte104_fired = 0;
	g_scte104_event_id = 0;
	CHECK(klvanc_packet_parse(ctx, 13, words, wordCount) >= 0, "packet_parse succeeds on generated SCTE-104 line");
	CHECK(g_scte104_fired == 1, "SCTE-104 callback fired");
	CHECK(g_scte104_event_id == 0xABCD1234, "round-tripped splice_event_id matches");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_free_SCTE_104(pkt);
}

/* ------------------------------------------------------------------- */
/* SMPTE-12-2                                                           */
/* ------------------------------------------------------------------- */
static int g_smpte12_2_fired;
static int cb_smpte12_2(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_smpte_12_2_s *pkt)
{
	g_smpte12_2_fired = 1;
	return 0;
}

static void test_smpte12_2(void)
{
	SECTION("SMPTE-12-2: alloc/create_from_ST370/convert/parse round-trip/free/preferred_line");

	struct klvanc_packet_smpte_12_2_s *pkt = NULL;
	CHECK(klvanc_alloc_SMPTE_12_2(&pkt) == 0, "klvanc_alloc_SMPTE_12_2 succeeds");
	CHECK(pkt != NULL, "packet allocated");
	if (pkt)
		klvanc_free_SMPTE_12_2(pkt);

	struct klvanc_packet_smpte_12_2_s *tc = NULL;
	CHECK(klvanc_create_SMPTE_12_2_from_ST370(0x01020304, 30000, 1001, &tc) == 0,
	      "create_SMPTE_12_2_from_ST370 succeeds");
	CHECK(tc != NULL, "timecode packet allocated");
	if (!tc)
		return;

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_SMPTE_12_2_to_words(ctx, tc, &words, &wordCount) == 0, "convert_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_SMPTE_12_2_to_packetBytes(ctx, tc, &bytes, &byteCount) == 0, "convert_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");
	free(bytes);

	struct klvanc_callbacks_s cb = { .smpte_12_2 = cb_smpte12_2 };
	ctx->callbacks = &cb;
	g_smpte12_2_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 13, words, wordCount) >= 0, "packet_parse succeeds on generated SMPTE-12-2 line");
	CHECK(g_smpte12_2_fired == 1, "SMPTE-12-2 callback fired");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_free_SMPTE_12_2(tc);

	/* preferred_line: exercise real table entries (Sec 8.2.1) for
	   deterministic, spec-documented results. */
	CHECK(klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_VITC1, 1080, 1) == 9,
	      "VITC1/1080i -> line 9 per the Sec 8.2.1 table");
	CHECK(klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_VITC2, 1080, 1) == 571,
	      "VITC2/1080i -> line 571 per the Sec 8.2.1 table");
	CHECK(klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_VITC1, 576, 1) == 8,
	      "VITC1/576i -> line 8 per the Sec 8.2.1 table");

	/* MINOR finding regression check: for a line count/interlace
	   combination not in the table, the "any other line" fallback's
	   condition duplicates the KLVANC_ATC_VITC1 comparison instead of
	   checking KLVANC_ATC_VITC2 (core-packet-smpte_12_2.c:436). LTC and
	   VITC1 both correctly fall through to "not found" (-1) for an
	   unmapped combination; VITC2 should behave the same way, but the
	   bug instead routes it into the fallback and returns 11. */
	int unmatched_ltc   = klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_LTC, 999, 0);
	int unmatched_vitc1 = klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_VITC1, 999, 0);
	int unmatched_vitc2 = klvanc_SMPTE_12_2_preferred_line(KLVANC_ATC_VITC2, 999, 0);
	CHECK(unmatched_ltc == -1, "LTC with an unmapped line count reports \"not found\"");
	CHECK(unmatched_vitc1 == -1, "VITC1 with an unmapped line count reports \"not found\"");
	CHECK(unmatched_vitc2 == -1,
	      "VITC2 with an unmapped line count reports \"not found\" like its siblings "
	      "(currently fails: returns 11 due to the VITC1-checked-twice bug)");
}

/* ------------------------------------------------------------------- */
/* SMPTE-2108-1 -- no construction API exists (klvanc_alloc_SMPTE_2108_1/ */
/* klvanc_free_SMPTE_2108_1 are declared in the header but never         */
/* defined anywhere in src/ -- confirmed via repo-wide grep during the   */
/* review; calling either would fail at link time). Parse-side only.    */
/* ------------------------------------------------------------------- */
static int g_smpte2108_1_fired;
static uint8_t g_smpte2108_1_num_frames;
static int cb_smpte2108_1(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_smpte_2108_1_s *pkt)
{
	g_smpte2108_1_fired = 1;
	g_smpte2108_1_num_frames = pkt->num_frames;
	return 0;
}

static void test_smpte2108_1(void)
{
	SECTION("SMPTE-2108-1: parse a single well-formed STATIC2 frame");

	/* frame_type=STATIC2(0x01), frame_length=6 (2 SEI + 4 payload bytes),
	   SEI type/length (2 bytes, ignored by the parser), then the 4-byte
	   static2 payload (max_content_light_level, max_pic_average_light_level). */
	uint8_t payload[] = {
		KLVANC_HDR_STATIC2, 6,
		0x00, 0x00,             /* SEI payload type/length, skipped */
		0x03, 0xe8,              /* max_content_light_level = 1000 */
		0x01, 0xf4,              /* max_pic_average_light_level = 500 */
	};

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(build_vanc_line(0x41, 0x0c, payload, sizeof(payload), &words, &wordCount) == 0,
	      "build a well-formed SMPTE-2108-1 line");
	if (!words)
		return;

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .smpte_2108_1 = cb_smpte2108_1 };
	ctx->callbacks = &cb;
	g_smpte2108_1_fired = 0;
	g_smpte2108_1_num_frames = 0xff;
	CHECK(klvanc_packet_parse(ctx, 14, words, wordCount) >= 0, "packet_parse succeeds");
	CHECK(g_smpte2108_1_fired == 1, "SMPTE-2108-1 callback fired");
	CHECK(g_smpte2108_1_num_frames == 1, "exactly one frame parsed");

	free(words);
	klvanc_context_destroy(ctx);
}

/* ------------------------------------------------------------------- */
/* SDP (OP-47)                                                          */
/* ------------------------------------------------------------------- */
static int g_sdp_fired;
static int cb_sdp(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_sdp_s *pkt)
{
	g_sdp_fired = 1;
	return 0;
}

static void test_sdp(void)
{
	SECTION("SDP: create/finalize/convert/parse round-trip/destroy");

	struct klvanc_packet_sdp_s *pkt = NULL;
	CHECK(klvanc_create_SDP(&pkt) == 0, "klvanc_create_SDP succeeds");
	CHECK(pkt != NULL, "packet allocated");
	if (!pkt)
		return;

	/* klvanc_create_SDP() leaves identifier at 0 (plain calloc); the
	   parser requires it to be the fixed OP-47 Subtitling Description
	   Packet magic 0x5115 (payload[0]==0x51, payload[1]==0x15) or it
	   rejects the packet outright -- set it explicitly, same as a real
	   caller constructing one from scratch would have to. */
	pkt->identifier = 0x5115;
	klvanc_finalize_SDP(pkt, 1);

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(klvanc_convert_SDP_to_words(pkt, &words, &wordCount) == 0, "convert_SDP_to_words succeeds");
	CHECK(words != NULL && wordCount > 0, "words populated");

	uint8_t *bytes = NULL;
	uint16_t byteCount = 0;
	CHECK(klvanc_convert_SDP_to_packetBytes(pkt, &bytes, &byteCount) == 0, "convert_SDP_to_packetBytes succeeds");
	CHECK(bytes != NULL && byteCount > 0, "bytes populated");
	free(bytes);

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .sdp = cb_sdp };
	ctx->callbacks = &cb;
	g_sdp_fired = 0;
	CHECK(klvanc_packet_parse(ctx, 15, words, wordCount) >= 0, "packet_parse succeeds on generated SDP line");
	CHECK(g_sdp_fired == 1, "SDP callback fired");

	free(words);
	klvanc_context_destroy(ctx);
	klvanc_destroy_SDP(pkt);
}

/* ------------------------------------------------------------------- */
/* SMPTE2038 packetizer + PES parser round trip                        */
/* ------------------------------------------------------------------- */
static struct klvanc_packet_header_s *g_captured_hdr;
static int cb_capture_all(void *uctx, struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr)
{
	/* Stash a copy -- hdr itself is freed by klvanc_packet_parse() right
	   after every callback for this frame returns. */
	klvanc_packet_copy(&g_captured_hdr, hdr);
	return 0;
}

static void test_smpte2038(void)
{
	SECTION("SMPTE2038: packetizer_alloc/begin/append/end + PES parse round-trip");

	struct klvanc_smpte2038_packetizer_s *pz = NULL;
	CHECK(klvanc_smpte2038_packetizer_alloc(&pz) == 0, "packetizer_alloc succeeds");
	CHECK(pz != NULL, "packetizer allocated");
	if (!pz)
		return;

	CHECK(klvanc_smpte2038_packetizer_begin(pz) == 0, "packetizer_begin succeeds");

	/* Capture a real klvanc_packet_header_s via the "all" callback (fired
	   for every successfully parsed packet, regardless of type) so
	   packetizer_append() gets a genuine header rather than a hand-rolled
	   one. */
	uint8_t afd_payload[] = { 0x00 };
	uint16_t *afd_words = NULL;
	uint16_t afd_wordCount = 0;
	CHECK(build_vanc_line(0x41, 0x05, afd_payload, sizeof(afd_payload), &afd_words, &afd_wordCount) == 0,
	      "build source AFD line for packetizing");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	struct klvanc_callbacks_s cb = { .all = cb_capture_all };
	ctx->callbacks = &cb;
	g_captured_hdr = NULL;
	CHECK(klvanc_packet_parse(ctx, 16, afd_words, afd_wordCount) >= 0, "packet_parse captures a header to packetize");
	CHECK(g_captured_hdr != NULL, "a real header was captured");

	if (g_captured_hdr) {
		int ret = klvanc_smpte2038_packetizer_append(pz, g_captured_hdr);
		CHECK(ret == 0, "packetizer_append succeeds with a real header");
	}

	CHECK(klvanc_smpte2038_packetizer_end(pz, 90000) == 0, "packetizer_end succeeds");
	CHECK(pz->bufused > 0, "packetizer produced a nonzero-length PES buffer");

	/* packetizer_end() fills in a full PES section header (start code
	   prefix, stream_id, etc -- KLVANC_SMPTE2038_PACKETIZER_BUFFER_RESET_
	   OFFSET = 14 bytes reserved for it at the front of the buffer), so
	   the round-trip parser here is _parse_pes_packet() (full section),
	   not _parse_pes_payload() (payload-only, for callers who split PES
	   framing out themselves, e.g. libavformat's mpegts demux). */
	struct klvanc_smpte2038_anc_data_packet_s *parsed = NULL;
	int ret = klvanc_smpte2038_parse_pes_packet(pz->buf, pz->bufused, &parsed);
	CHECK(ret == 0, "parse_pes_packet succeeds on the packetizer's own output");
	if (ret == 0 && parsed) {
		CHECK(parsed->lineCount == 1, "exactly one ANC line round-tripped");
		if (parsed->lineCount == 1) {
			/* DID/SDID/etc. are stored with parity bits (9:8) still
			   present -- see the doc comment on struct
			   klvanc_smpte2038_anc_data_line_s -- mask them off. */
			CHECK((parsed->lines[0].DID & 0x1ff) == 0x41, "round-tripped DID matches");
			CHECK((parsed->lines[0].SDID & 0x1ff) == 0x05, "round-tripped SDID matches");
			uint16_t *lwords = NULL;
			uint16_t lwordCount = 0;
			CHECK(klvanc_smpte2038_convert_line_to_words(&parsed->lines[0], &lwords, &lwordCount) == 0,
			      "convert_line_to_words succeeds");
			CHECK(lwords != NULL && lwordCount > 0, "line words populated");
			free(lwords);
		}
		klvanc_smpte2038_anc_data_packet_dump(parsed);
		klvanc_smpte2038_anc_data_packet_free(parsed);
	}

	free(g_captured_hdr);
	free(afd_words);
	klvanc_context_destroy(ctx);
	klvanc_smpte2038_packetizer_free(&pz);
	/* MODERATE finding (same pattern already documented for tools/
	   pes_extractor.c's pe_free() in the review): packetizer_free() takes
	   a T** -- implying, like iso13818_udp_receiver_free(), that it will
	   null the caller's pointer -- but never does `*ctx = NULL`. This
	   currently fails; flipping to PASS is the fix signal. */
	CHECK(pz == NULL, "packetizer_free nulls the caller's pointer (currently fails, see pe_free finding)");
}

/* ------------------------------------------------------------------- */
/* pixels: sanity checks for the v210/uyvy/nv20/y10 conversion helpers  */
/* ------------------------------------------------------------------- */
static void test_pixels(void)
{
	SECTION("pixels: v210/uyvy/nv20/y10 conversion helpers");

	/* One v210 32-bit group encodes 6 10-bit samples across 4 uint32_t
	   words -- use a minimal 4-word (1 group, width=6) buffer of a known,
	   non-zero pattern and confirm the unpack helpers run without
	   crashing and produce non-garbage (in-range 10-bit) output. */
	uint32_t v210[4] = { 0x3ff00000, 0x3ff3ff00, 0x000003ff, 0x3ff3ff3f };
	uint16_t y[6] = { 0 }, u[3] = { 0 }, v[3] = { 0 };
	klvanc_v210_planar_unpack_c(v210, y, u, v, 6);
	int inrange = 1;
	for (int i = 0; i < 6; i++)
		if (y[i] > 0x3ff)
			inrange = 0;
	CHECK(inrange, "v210_planar_unpack_c produces 10-bit-range luma samples");

	/* Requires dstSizeBytes >= width*6 (interleaved Y+UV output). */
	uint16_t nv20[18] = { 0 };
	int ret = klvanc_v210_line_to_nv20_c(v210, nv20, sizeof(nv20), 6);
	CHECK(ret == 0, "v210_line_to_nv20_c succeeds for a correctly-sized destination");
	ret = klvanc_v210_line_to_nv20_c(v210, nv20, 2 /* too small */, 6);
	CHECK(ret != 0, "v210_line_to_nv20_c rejects an undersized destination buffer");

	uint16_t uyvy[12] = { 0 };
	klvanc_v210_line_to_uyvy_c(v210, uyvy, 6);
	CHECK(1, "v210_line_to_uyvy_c completes without crashing");

	uint16_t y10src[8];
	for (int i = 0; i < 8; i++)
		y10src[i] = 0x200 + i;
	uint8_t v210dst[32] = { 0 };
	klvanc_y10_to_v210(y10src, v210dst, 8);
	CHECK(1, "y10_to_v210 completes without crashing");

	uint16_t uyvysrc[16];
	for (int i = 0; i < 16; i++)
		uyvysrc[i] = 0x200 + (i % 4);
	uint8_t v210dst2[64] = { 0 };
	klvanc_uyvy_to_v210(uyvysrc, v210dst2, 8);
	CHECK(1, "uyvy_to_v210 completes without crashing");
}

/* ------------------------------------------------------------------- */
/* lines: klvanc_line_create()/_free(), klvanc_generate_vanc_line()     */
/* ------------------------------------------------------------------- */
static void test_lines(void)
{
	SECTION("lines: klvanc_line_create()/klvanc_generate_vanc_line()/klvanc_line_free()");

	struct klvanc_line_s *line = klvanc_line_create(10);
	CHECK(line != NULL, "line_create succeeds");
	CHECK(line != NULL && line->line_number == 10, "line_number stored");
	CHECK(line != NULL && line->num_entries == 0, "a freshly created line has no entries");

	if (line) {
		struct klvanc_context_s *ctx;
		CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");

		uint16_t *out_buf = NULL;
		int out_len = 0;
		int ret = klvanc_generate_vanc_line(ctx, line, &out_buf, &out_len, 1920);
		CHECK(ret == 0, "generate_vanc_line succeeds on an empty line");
		if (out_buf)
			free(out_buf);

		klvanc_context_destroy(ctx);
		klvanc_line_free(line);
	}
}

/* ------------------------------------------------------------------- */
/* cache: enable/lookup/reset, plus a *direct*, non-destructive check   */
/* that a cache-line mutex is actually initialized (CRITICAL#cache).    */
/* ------------------------------------------------------------------- */
static void test_cache(void)
{
	SECTION("cache: enable_cache/lookup/update via packet_parse/reset");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");

	CHECK(klvanc_context_enable_cache(ctx) == 0, "enable_cache succeeds");

	uint8_t payload[] = { 0x00 };
	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	CHECK(build_vanc_line(0x41, 0x05, payload, sizeof(payload), &words, &wordCount) == 0, "build source AFD line");
	struct klvanc_callbacks_s cb = { 0 };
	ctx->callbacks = &cb;
	CHECK(klvanc_packet_parse(ctx, 20, words, wordCount) >= 0, "packet_parse (with cache enabled) succeeds");

	struct klvanc_cache_s *s = klvanc_cache_lookup(ctx, 0x41, 0x05);
	CHECK(s != NULL, "cache_lookup returns a non-NULL entry for the DID/SDID just seen");
	CHECK(s != NULL && s->activeCount > 0, "activeCount reflects the update we just performed");
	CHECK(klvanc_cache_lookup(NULL, 0x41, 0x05) == NULL, "cache_lookup rejects a NULL ctx");

	klvanc_cache_reset(ctx);
	if (s)
		CHECK(s->activeCount == 0, "cache_reset clears activeCount");

	free(words);
	klvanc_context_destroy(ctx);
}

#ifdef HAVE_LIBKLVANC_CACHE_MUTEX_TEST
/* Placeholder kept out of the default build path: see
   child_critical_cache_mutex_not_initialized() below for the real,
   process-isolated test of CRITICAL#cache. */
#endif

/* ===================================================================== */
/* CRITICAL / MODERATE REGRESSION TESTS (process-isolated)               */
/*                                                                        */
/* Every test in this section targets one specific, numbered finding     */
/* from the code review of this repository (commit d2bec17). None of     */
/* these bugs have been fixed yet -- these tests are expected to FAIL    */
/* right now. They exist so that when each bug is fixed, flipping to     */
/* PASS is the verification signal, the same way the equivalent process  */
/* worked for the sibling libklscte35 project earlier in this session.   */
/* ===================================================================== */

/* --- CRITICAL: core-cache.c / cache.h -- pthread_mutex_t objects are   */
/* calloc()'d but never pthread_mutex_init()'d. Confirmed on this        */
/* platform: pthread_mutex_lock() on such a mutex returns EINVAL, and    */
/* the library never checks that return value, so the cache's locking    */
/* is silently a complete no-op. This test calls the real public API     */
/* (enable_cache + lookup) and then directly locks/unlocks the resulting */
/* cache entry's mutex (one per did/sdid, guarding all 2048 of its lines */
/* -- see the comment on struct klvanc_cache_s in cache.h) -- exactly    */
/* the field klvanc_cache_update()/_reset() rely on -- and asserts the   */
/* lock actually succeeds. Not memory-unsafe by itself, so this one runs */
/* without isolation, but is kept in this section since it's a direct    */
/* regression test for a CRITICAL finding. */
static void test_critical_cache_mutex_not_initialized(void)
{
	SECTION("CRITICAL cache: pthread_mutex_t must be initialized before use");

	struct klvanc_context_s *ctx;
	CHECK(klvanc_context_create(&ctx) == 0, "context_create succeeds");
	CHECK(klvanc_context_enable_cache(ctx) == 0, "enable_cache succeeds");

	struct klvanc_cache_s *s = klvanc_cache_lookup(ctx, 0x41, 0x07);
	CHECK(s != NULL, "cache_lookup returns a valid entry");
	if (s) {
		int ret = pthread_mutex_lock(&s->mutex);
		CHECK(ret == 0, "locking a cache entry's mutex succeeds (got errno-style %d; "
		      "EINVAL means it was never pthread_mutex_init()'d)", ret);
		if (ret == 0)
			pthread_mutex_unlock(&s->mutex);
	}

	klvanc_context_destroy(ctx);
}

/* --- CRITICAL: core-packet-eia_708b.c:315-328 -- cc_count (0-31) only  */
/* checked against stream byte availability, not against cc[30]'s real  */
/* size. Setting it directly (as the convert/dump paths also trust it   */
/* unchecked) demonstrates the same unguarded array access without      */
/* needing to hand-craft a bit-packed CDP payload. */
static void child_critical_eia708b_cc_count_overflow(void)
{
	struct klvanc_packet_eia_708b_s *pkt = NULL;
	if (klvanc_create_eia708_cdp(&pkt) != 0)
		_exit(2);

	pkt->ccdata.cc_count = 31; /* KLVANC_MAX_CC_COUNT is 30: valid indices 0-29 */
	for (int i = 0; i < 31; i++)
		pkt->ccdata.cc[i % 30].cc_valid = 1; /* stay in-bounds while seeding */

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	klvanc_convert_EIA_708B_to_words(pkt, &words, &wordCount); /* reads cc[30] out of bounds */
	free(words);
	klvanc_destroy_eia708_cdp(pkt);
	_exit(0);
}

/* --- MODERATE: core-packet-kl_u64le_counter.c -- _to_words() has no    */
/* NULL check on pkt, unlike every sibling _to_words()/_to_packetBytes().*/
static void child_moderate_kl_u64le_null_pkt(void)
{
	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	klvanc_convert_KL_U64LE_COUNTER_to_words(NULL, &words, &wordCount);
	_exit(0); /* reaching here means it didn't crash -- i.e. the bug is fixed */
}

/* --- MODERATE (found while writing this suite, not in the original     */
/* review batch): core-packet-eia_708b.c klvanc_set_framerate_EIA_708B() */
/* computes gcd(num, den) and then unconditionally divides both inputs   */
/* by it; gcd(0, 0) == 0, so (0, 0) triggers an integer division by      */
/* zero. That's undefined behavior in C -- confirmed to silently         */
/* evaluate to 0 on this platform (ARM64, where integer division by      */
/* zero doesn't trap), but expected to raise SIGFPE on x86. Isolated so  */
/* this test is meaningful/safe on either architecture. */
static void child_moderate_eia708b_framerate_div_by_zero(void)
{
	struct klvanc_packet_eia_708b_s *pkt = NULL;
	if (klvanc_create_eia708_cdp(&pkt) != 0)
		_exit(2);

	klvanc_set_framerate_EIA_708B(pkt, 0, 0);

	klvanc_destroy_eia708_cdp(pkt);
	_exit(0); /* reaching here means it didn't crash -- i.e. the bug is fixed */
}

/* --- CRITICAL: core-packet-scte_104.c:577,579 -- descriptor_size - 5   */
/* underflows (unsigned) when a MO_PROPRIETARY_COMMAND_REQUEST_DATA op's */
/* wire data_length is 0-4, producing a ~4GB memcpy() into a 255-byte    */
/* struct field -- a wild heap write from a single non-fragmented SCTE-  */
/* 104 packet. Build the minimal MOM wire payload by hand (verified      */
/* against the exact byte offsets read by the parser). */
static void child_critical_scte104_proprietary_underflow(void)
{
	uint8_t mom[] = {
		0xff, 0xff,             /* rsvd */
		0x00, 0x10,             /* messageSize = 16 */
		0x00,                   /* protocol_version */
		0x00,                   /* AS_index */
		0x00,                   /* message_number */
		0x00, 0x00,             /* DPI_PID_index */
		0x00,                   /* SCTE35_protocol_version */
		0x00,                   /* timestamp: time_type = 0 (none) */
		0x01,                   /* num_ops = 1 */
		0x01, 0x0c,             /* opID = MO_PROPRIETARY_COMMAND_REQUEST_DATA (0x10c) */
		0x00, 0x00,             /* data_length = 0 -- triggers the underflow */
	};

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	if (build_vanc_line(0x41, 0x07, mom, sizeof(mom), &words, &wordCount) != 0)
		_exit(3);

	struct klvanc_context_s *ctx;
	if (klvanc_context_create(&ctx) != 0)
		_exit(4);
	struct klvanc_callbacks_s cb = { .scte_104 = cb_scte104 };
	ctx->callbacks = &cb;

	klvanc_packet_parse(ctx, 13, words, wordCount);

	free(words);
	klvanc_context_destroy(ctx);
	_exit(0); /* reaching here means it didn't crash -- i.e. the bug is fixed */
}

/* --- CRITICAL: core-packet-scte_104.c:453-464 -- parse_avail_request_  */
/* data() reads num_provider_avails (attacker-controlled, up to 255)     */
/* then loops reading 4 bytes/avail from a source buffer only            */
/* data_length bytes long. data_length=1 with num_provider_avails=255    */
/* reads ~1KB past a 1-byte heap allocation. */
static void child_critical_scte104_avail_overread(void)
{
	uint8_t mom[] = {
		0xff, 0xff, 0x00, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00,                   /* timestamp: none */
		0x01,                   /* num_ops = 1 */
		0x01, 0x0a,             /* opID = MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA (0x10a) */
		0x00, 0x01,             /* data_length = 1 */
		0xff,                   /* num_provider_avails = 255 (needs 255*4=1020 more bytes; only 0 follow) */
	};

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	if (build_vanc_line(0x41, 0x07, mom, sizeof(mom), &words, &wordCount) != 0)
		_exit(3);

	struct klvanc_context_s *ctx;
	if (klvanc_context_create(&ctx) != 0)
		_exit(4);
	struct klvanc_callbacks_s cb = { .scte_104 = cb_scte104 };
	ctx->callbacks = &cb;

	klvanc_packet_parse(ctx, 13, words, wordCount);

	free(words);
	klvanc_context_destroy(ctx);
	_exit(0);
}

/* --- CRITICAL: core-packet-scte_104.c:1531,1533 -- klvanc_SCTE_104_    */
/* Add_MOM_Op(): num_ops is `unsigned char`; the 256th call on one pkt   */
/* wraps it to 0, making ops[num_ops-1] == ops[-1] -- a negative-index   */
/* memset(). Also unchecked realloc(). Public API, used by libklscte35.  */
static void child_critical_scte104_add_mom_op_wraparound(void)
{
	struct klvanc_packet_scte_104_s *pkt = NULL;
	if (klvanc_alloc_SCTE_104(0xffff, &pkt) != 0)
		_exit(2);

	struct klvanc_multiple_operation_message_operation *op = NULL;
	int ret = 0;
	for (int i = 0; i < 256 && ret == 0; i++)
		ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);

	klvanc_free_SCTE_104(pkt);
	_exit(0); /* reaching here without corruption/crash means the bug is fixed */
}

/* --- CRITICAL: core-packet-smpte_2108_1.c:113-146 -- num_frames has no */
/* bound check against frames[MAX_S2108_1_FRAMES=10] before indexing --  */
/* 12 minimal (4-byte) unknown-type frames overflow the array. */
static void child_critical_smpte2108_1_num_frames_overflow(void)
{
	uint8_t payload[48];
	for (int i = 0; i < 12; i++) {
		payload[i * 4 + 0] = 0xEE; /* frame_type: not STATIC1/STATIC2 */
		payload[i * 4 + 1] = 0x02; /* frame_length = 2 */
		payload[i * 4 + 2] = 0x00; /* SEI type/length, skipped */
		payload[i * 4 + 3] = 0x00;
	}

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	if (build_vanc_line(0x41, 0x0c, payload, sizeof(payload), &words, &wordCount) != 0)
		_exit(3);

	struct klvanc_context_s *ctx;
	if (klvanc_context_create(&ctx) != 0)
		_exit(4);
	struct klvanc_callbacks_s cb = { .smpte_2108_1 = cb_smpte2108_1 };
	ctx->callbacks = &cb;

	klvanc_packet_parse(ctx, 14, words, wordCount);

	free(words);
	klvanc_context_destroy(ctx);
	_exit(0);
}

/* --- CRITICAL: core-packet-smpte_2108_1.c:143-145 -- frame_length &lt; 2 */
/* underflows `8 * (frame_length - 2)` to a huge unsigned bit count      */
/* passed to klbs_read_bits(), walking far past the payload buffer.      */
static void child_critical_smpte2108_1_frame_length_underflow(void)
{
	uint8_t payload[] = {
		0xEE, 0x00, /* frame_type: unknown, frame_length = 0 -- underflows */
		0x00, 0x00, /* SEI type/length, skipped */
	};

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	if (build_vanc_line(0x41, 0x0c, payload, sizeof(payload), &words, &wordCount) != 0)
		_exit(3);

	struct klvanc_context_s *ctx;
	if (klvanc_context_create(&ctx) != 0)
		_exit(4);
	struct klvanc_callbacks_s cb = { .smpte_2108_1 = cb_smpte2108_1 };
	ctx->callbacks = &cb;

	klvanc_packet_parse(ctx, 14, words, wordCount);

	free(words);
	klvanc_context_destroy(ctx);
	_exit(0);
}

/* --- MODERATE: core-packet-sdp.c:66,87 -- length==0 causes             */
/* payload[length-1] to evaluate as payload[-1] (negative index, UB).    */
static void child_moderate_sdp_length_zero(void)
{
	/* SDP payload's 3rd byte (index 2, per core-packet-sdp.c) is the
	   "length" field the parser reads unchecked. */
	uint8_t payload[] = { 0x00, 0x00, 0x00, 0x00, 0x00 };

	uint16_t *words = NULL;
	uint16_t wordCount = 0;
	if (build_vanc_line(0x43, 0x02, payload, sizeof(payload), &words, &wordCount) != 0)
		_exit(3);

	struct klvanc_context_s *ctx;
	if (klvanc_context_create(&ctx) != 0)
		_exit(4);
	struct klvanc_callbacks_s cb = { .sdp = cb_sdp };
	ctx->callbacks = &cb;

	klvanc_packet_parse(ctx, 15, words, wordCount);

	free(words);
	klvanc_context_destroy(ctx);
	_exit(0);
}

/* --- MODERATE: core-lines.c klvanc_line_free() -- no NULL check,       */
/* inconsistent with every sibling _free() function in the codebase.     */
static void child_moderate_lines_free_null(void)
{
	klvanc_line_free(NULL);
	_exit(0);
}

/* --- MODERATE (found while writing this suite, not in the original     */
/* review batch): smpte2038.c klvanc_smpte2038_packetizer_append() has   */
/* no NULL check on `pkt` and crashes immediately (confirmed via lldb:   */
/* EXC_BAD_ACCESS dereferencing a field of the NULL pointer) -- the same */
/* missing-NULL-check pattern flagged repeatedly elsewhere in this       */
/* review, just in a function nobody had exercised with NULL yet. */
static void child_moderate_smpte2038_packetizer_append_null(void)
{
	struct klvanc_smpte2038_packetizer_s *pz = NULL;
	if (klvanc_smpte2038_packetizer_alloc(&pz) != 0)
		_exit(2);
	klvanc_smpte2038_packetizer_begin(pz);

	klvanc_smpte2038_packetizer_append(pz, NULL);

	klvanc_smpte2038_packetizer_free(&pz);
	_exit(0); /* reaching here means it didn't crash -- i.e. the bug is fixed */
}

/* --- CRITICAL: tools/klringbuffer.c rb_write() -- inverted boundary    */
/* condition: a write of exactly the buffer's full capacity at a         */
/* non-zero `head` offset takes the "linear" memcpy branch instead of    */
/* wrapping, overrunning the allocation. */
static void child_critical_klringbuffer_rb_write_overflow(void)
{
	KLRingBuffer *rb = rb_new(64, 64); /* non-growing: size == size_max */
	if (!rb)
		_exit(2);

	char tmp[64];
	memset(tmp, 'A', sizeof(tmp));
	rb_write(rb, tmp, 64);   /* fill it completely: head=0, fill=64 */
	rb_read(rb, tmp, 32);    /* drain half: head=32, fill=32 */
	rb_write(rb, tmp, 32);   /* refill: head=32, fill=64 (buffer full, head!=0) */
	rb_read(rb, tmp, 64);    /* fully drain: head=32 (unchanged by read), fill=0 */

	/* Now: fill=0, head=32, size=64. A single write of exactly 64 bytes
	   (the full capacity) at this non-zero head is the exact trigger. */
	memset(tmp, 'B', sizeof(tmp));
	rb_write(rb, tmp, 64);

	rb_free(rb);
	_exit(0);
}

/* --- CRITICAL: tools/klringbuffer.c rb_grow() -- growing a buffer      */
/* whose valid data currently wraps around the physical end does not     */
/* relocate the wrapped segment, corrupting/losing data (and reading     */
/* whatever heap bytes now occupy the "new" region). Not a crash by      */
/* itself, so this checks data integrity rather than relying on a fault. */
static void child_critical_klringbuffer_rb_grow_corruption(void)
{
	KLRingBuffer *rb = rb_new(16, 1024); /* growable */
	if (!rb)
		_exit(2);

	char in[16], out[16];
	memset(in, 'X', sizeof(in));
	rb_write(rb, in, 16);   /* fill: head=0, fill=16 */
	rb_read(rb, out, 12);   /* drain most: head=12, fill=4 */

	/* The 4 remaining valid bytes are at data[12..15]. Write 4 more bytes
	   with a distinct pattern -- they must wrap to data[0..3]. */
	memset(in, 'Y', 4);
	rb_write(rb, in, 4);    /* head=12, fill=8, valid data wraps: [12..15]='X','[0..3]'='Y' */

	/* Force growth while data is wrapped. */
	rb_write(rb, in, 0);    /* no-op write, buffer state unchanged */
	/* Trigger rb_grow indirectly via a write that needs more room than
	   is currently free (fill=8, size=16, remain=8 -- write 9 to force growth). */
	char more[9];
	memset(more, 'Z', sizeof(more));
	rb_write(rb, more, 9);

	char result[8];
	rb_read(rb, result, 8); /* should read back 'X','X','X','X','Y','Y','Y','Y' */

	int ok = 1;
	for (int i = 0; i < 4; i++)
		if (result[i] != 'X')
			ok = 0;
	for (int i = 4; i < 8; i++)
		if (result[i] != 'Y')
			ok = 0;

	rb_free(rb);
	_exit(ok ? 0 : 5);
}

/* --- CRITICAL: tools/pes_extractor.c pe_processPacket() -- a fully     */
/* attacker-controlled TS adaptation_field_length byte (0-255) can push  */
/* `offset` past 188, underflowing `packet_size - offset` (computed in   */
/* int, used as size_t) to a huge value passed to rb_write(). */
static void pe_dummy_cb(void *cb_context, unsigned char *buf, int byteCount) { }

static void child_critical_pes_extractor_underflow(void)
{
	struct pes_extractor_s *pe = NULL;
	if (pe_alloc(&pe, NULL, pe_dummy_cb, 0x100) != 0)
		_exit(2);

	unsigned char pkt[188];
	memset(pkt, 0xff, sizeof(pkt));
	pkt[0] = 0x47;                  /* TS sync byte */
	pkt[1] = 0x40; pkt[2] = 0x00;    /* PUSI + PID = 0x100 */
	pkt[3] = 0x30;                   /* adaptation_field_control = 3 (adaptation + payload) */
	pkt[4] = 255;                    /* adaptation_field_length = 255 -- pushes offset past 188 */

	pe_push(pe, pkt, 1);

	pe_free(&pe);
	_exit(0);
}

static void test_critical_regressions(void)
{
	SECTION("Critical/moderate issue regressions (process-isolated)");

	run_isolated("CRITICAL#eia708b cc_count bounds", child_critical_eia708b_cc_count_overflow);
	run_isolated("MODERATE#eia708b set_framerate(0,0) division by zero", child_moderate_eia708b_framerate_div_by_zero);
	run_isolated("MODERATE#kl_u64le NULL pkt guard", child_moderate_kl_u64le_null_pkt);
	run_isolated("CRITICAL#scte104 proprietary_command data_length underflow", child_critical_scte104_proprietary_underflow);
	run_isolated("CRITICAL#scte104 avail_request_data over-read", child_critical_scte104_avail_overread);
	run_isolated("CRITICAL#scte104 Add_MOM_Op num_ops wraparound", child_critical_scte104_add_mom_op_wraparound);
	run_isolated("CRITICAL#smpte2108_1 num_frames bounds", child_critical_smpte2108_1_num_frames_overflow);
	run_isolated("CRITICAL#smpte2108_1 frame_length underflow", child_critical_smpte2108_1_frame_length_underflow);
	run_isolated("MODERATE#sdp length==0 negative index", child_moderate_sdp_length_zero);
	run_isolated("MODERATE#lines klvanc_line_free(NULL) guard", child_moderate_lines_free_null);
	run_isolated("MODERATE#smpte2038 packetizer_append(NULL) guard", child_moderate_smpte2038_packetizer_append_null);
	run_isolated("CRITICAL#klringbuffer rb_write full-buffer overflow", child_critical_klringbuffer_rb_write_overflow);
	run_isolated("CRITICAL#klringbuffer rb_grow wrapped-data corruption", child_critical_klringbuffer_rb_grow_corruption);
	run_isolated("CRITICAL#pes_extractor adaptation_field_length underflow", child_critical_pes_extractor_underflow);
}

int test_api_main(int argc, char *argv[])
{
	printf("libklvanc API test suite\n");

	test_core_context();
	test_core_checksum();
	test_core_did();
	test_core_lookup_by_type();
	test_core_sdi_create_payload();
	test_core_packet_parse_argcheck();
	test_core_packet_copy_free_save();
	test_core_packet_payload_append();

	test_afd();
	test_eia608();
	test_eia708b();
	test_kl_u64le_counter();
	test_scte104();
	test_smpte12_2();
	test_smpte2108_1();
	test_sdp();

	test_smpte2038();
	test_pixels();
	test_lines();
	test_cache();

	test_critical_cache_mutex_not_initialized();
	test_critical_regressions();

	printf("\n============================\n");
	printf("Total: %d passed, %d failed\n", g_pass, g_fail);
	printf("============================\n");

	return g_fail == 0 ? 0 : 1;
}
