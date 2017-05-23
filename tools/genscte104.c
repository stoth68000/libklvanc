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

static int testcase_1(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = 0x02;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 300;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->tier_data.tier_data = 0x123;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_2(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = 0x02;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 300;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_DTMF_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->dtmf_data.pre_roll_time = 200; /* 20 Seconds */
	op->dtmf_data.dtmf_length = 3;
	op->dtmf_data.dtmf_char[0] = '0';
	op->dtmf_data.dtmf_char[1] = '4';
	op->dtmf_data.dtmf_char[2] = '2';

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_3(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = 0x02;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 300;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_AVAIL_DESCRIPTOR_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->avail_descriptor_data.num_provider_avails = 1;
	op->avail_descriptor_data.provider_avail_id[0] = 999;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_4(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = 0x02;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 300;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_SEGMENTATION_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	struct segmentation_descriptor_request_data *seg = &op->segmentation_data;
	seg->event_id = 1234;
	seg->event_cancel_indicator = 0;
	seg->delivery_not_restricted_flag = 1;
	seg->web_delivery_allowed_flag = 1;
	seg->no_regional_blackout_flag = 0;
	seg->archive_allowed_flag = 1;
	seg->device_restrictions = 0;
	seg->duration = 350;
	seg->upid_type = 1;
	seg->upid_length = 3;
	seg->upid[0] = 1;
	seg->upid[1] = 2;
	seg->upid[2] = 3;

	seg->type_id = 3;
	seg->segment_num = 1;
	seg->segments_expected = 2;
	seg->duration_extension_frames = 59;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_5(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = 0x02;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 300;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIME_DESCRIPTOR, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->time_data.TAI_seconds = 1490808516; /* Wed Mar 29 13:28:36 EDT 2017 */
	op->time_data.TAI_ns = 500000000;
	op->time_data.UTC_offset = 500; /* Nonsensical value? */

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_6(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_NULL_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIME_DESCRIPTOR, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->time_data.TAI_seconds = 1490808516; /* Wed Mar 29 13:28:36 EDT 2017 */
	op->time_data.TAI_ns = 500000000;
	op->time_data.UTC_offset = 500; /* Nonsensical value? */

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_7(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_NULL_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_DESCRIPTOR_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->descriptor_data.descriptor_count = 1;
	op->descriptor_data.total_length = 7;
	op->descriptor_data.descriptor_bytes[0] = 0x0b;
	op->descriptor_data.descriptor_bytes[1] = 0x05;
	op->descriptor_data.descriptor_bytes[2] = 0x54;
	op->descriptor_data.descriptor_bytes[3] = 0x56;
	op->descriptor_data.descriptor_bytes[4] = 0x43;
	op->descriptor_data.descriptor_bytes[5] = 0x54;
	op->descriptor_data.descriptor_bytes[6] = 0x11;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_8(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_NULL_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_PROPRIETARY_COMMAND_REQUEST_DATA,
					 &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->proprietary_data.proprietary_id = 0x5522aa11;
	op->proprietary_data.proprietary_command = 0x22;
	op->proprietary_data.data_length = 7;
	op->proprietary_data.proprietary_data[0] = 0x0b;
	op->proprietary_data.proprietary_data[1] = 0x05;
	op->proprietary_data.proprietary_data[2] = 0x54;
	op->proprietary_data.proprietary_data[3] = 0x56;
	op->proprietary_data.proprietary_data[4] = 0x43;
	op->proprietary_data.proprietary_data[5] = 0x54;
	op->proprietary_data.proprietary_data[6] = 0x11;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_9(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = SPLICEEND_IMMEDIATE;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 0;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 0;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->tier_data.tier_data = 0x123;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_10(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = SPLICESTART_NORMAL;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 5000;
	op->sr_data.brk_duration = 280;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 1;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->tier_data.tier_data = 0x123;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_11(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = SPLICEEND_NORMAL;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 5000;
	op->sr_data.brk_duration = 0;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 0;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->tier_data.tier_data = 0x123;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

static int testcase_12(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount)
{

	struct packet_scte_104_s *pkt;
	struct multiple_operation_message_operation *op;
	int ret;

	ret = alloc_SCTE_104(0xffff, &pkt);
	if (ret != 0)
		return -1;

	ret =  klvanc_SCTE_104_Add_MOM_Op(pkt, MO_SPLICE_REQUEST_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	op->sr_data.splice_insert_type = SPLICE_CANCEL;
	op->sr_data.splice_event_id = 0x1234;
	op->sr_data.unique_program_id = 0x4567;
	op->sr_data.pre_roll_time = 0;
	op->sr_data.brk_duration = 0;
	op->sr_data.avail_num = 1;
	op->sr_data.avails_expected = 2;
	op->sr_data.auto_return_flag = 0;

	ret = klvanc_SCTE_104_Add_MOM_Op(pkt, MO_INSERT_TIER_DATA, &op);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}
	op->tier_data.tier_data = 0x123;

	ret = dump_SCTE_104(ctx, pkt);
	if (ret != 0) {
		free_SCTE_104(pkt);
		return -1;
	}

	ret = convert_SCTE_104_to_words(pkt, words, wordCount);
	if (ret != 0) {
		fprintf(stderr, "Failed to convert 104 to words: %d\n", ret);
		free_SCTE_104(pkt);
		return -1;
	}

	free_SCTE_104(pkt);
	return 0;
}

struct testcase {
	const char *name;
	int (*test)(struct vanc_context_s *ctx, uint16_t **words, uint16_t *wordCount);
};

struct testcase testcases[] = {
	{ "Splice immediate + Insert Tier Data", testcase_1 },
	{ "Splice immediate + DTMF", testcase_2 },
	{ "Splice immediate + Insert Avail", testcase_3 },
	{ "Splice immediate + Insert Segmentation", testcase_4 },
	{ "Splice Immediate + Insert Time Descriptor", testcase_5 },
	{ "Splice Null + Insert Time Descriptor", testcase_6 },
	{ "Splice Null + Insert Descriptor", testcase_7 },
	{ "Splice Null + Proprietary command", testcase_8 },
	{ "SpliceEnd Immediate", testcase_9 },
	{ "SpliceStart Normal", testcase_10 },
	{ "SpliceEnd Normal", testcase_11 },
	{ "Splice Cancel", testcase_12 },

};
#define NUM_TESTCASES sizeof(testcases) / sizeof(struct testcase)

int genscte104_main(int argc, char *argv[])
{
	struct vanc_context_s *ctx;
	uint16_t *words;
	uint16_t wordCount;

	int ret;

	if (vanc_context_create(&ctx) < 0) {
		fprintf(stderr, "Error initializing library context\n");
		exit(1);
	}
	ctx->verbose = 1;


	for (int i = 0; i < NUM_TESTCASES; i++) {
		printf("Running test case %d: %s\n", i, testcases[i].name);
		ret = testcases[i].test(ctx, &words, &wordCount);
		if (ret != 0)
			exit(1);

		printf("Final Output\n");
		for (int i = 0; i < wordCount; i++) {
			printf("%02x %02x ", words[i] >> 8, words[i] & 0xff);
		}
		printf("\n");
		free(words);
	}

	vanc_context_destroy(ctx);
	return 0;
}
