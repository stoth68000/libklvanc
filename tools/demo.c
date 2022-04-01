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
	if (klvanc_dump_AFD(ctx, pkt) < 0)
		fprintf(stderr, "Failed to dump AFD packet");

	return 0;
}

static int cb_EIA_708B(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_708b_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	if (klvanc_dump_EIA_708B(ctx, pkt) < 0)
		fprintf(stderr, "Failed to dump CEA-708 packet");

	return 0;
}

static int cb_EIA_608(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_608_s *pkt)
{
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	if (klvanc_dump_EIA_608(ctx, pkt) < 0)
		fprintf(stderr, "Failed to dump EIA-608 packet");

	return 0;
}

static int cb_SCTE_104(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
	int ret = -1;
	printf("%s:%s()\n", __FILE__, __func__);

	/* Have the library display some debug */
	printf("Asking libklvanc to dump a struct\n");
	if (klvanc_dump_SCTE_104(ctx, pkt) < 0)
		fprintf(stderr, "Failed to dump SCTE-104 packet");

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

static int test_package_of_umid_data(struct klvanc_context_s *ctx)
{
/*
AD - A private descriptor tagged with Sencores letters. We don't know its format.
>> hdr->type   = 0
>>  ->adf      = 0x0000/0x03ff/0x03ff
>>  ->did/sdid = 0x44 / 0x44 [Undefined Undefined] via SDI line 17
>>  ->h_offset = 0
>>  ->checksum = 0x0214 (VALID)
>>  ->payloadLengthWords = 64
>>  ->payload  = 06 0a 2b 34 01 01 01 05 01 01 0d 43 33 00 00 00 e6 8a 1e 00 43 62 05 80 08 00 46 02 01 20 08 a4 87 20 a3 07 43 62 05 80 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
*/
	unsigned short arr[] = {
		0x000,
		0x3ff,
		0x3ff,
		0x244,
		0x144,

		/* This is an extended UMID, its 64 bytes.
		 * basic UMIDs are 32 bytes. See RP223-2003 page 4.
		 */
		0x140, /* DC */

		0x06, 0x0a, 0x2b, 0x34, 0x01, 0x01, 0x01, 0x05,
		0x01, 0x01, 0x0d, 0x43, 0x33, 0x00, 0x00, 0x00,
		0xe6, 0x8a, 0x1e, 0x00, 0x43, 0x62, 0x05, 0x80,
		0x08, 0x00, 0x46, 0x02, 0x01, 0x20, 0x08, 0xa4,
		0x87, 0x20, 0xa3, 0x07, 0x43, 0x62, 0x05, 0x80,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

		0x214, /* Checksum is valid */
	};

	/* report that this was from line 17, informational only. */
	int ret = klvanc_packet_parse(ctx, 17, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

static int test_program_description_data(struct klvanc_context_s *ctx)
{
	/* This carries various ATSC descriptors and we'll likely add sort for
	 * various descriptors over time. FOr now, we don't support ANY
	 * 0x61/0x61 messages, but we'll collect them here for reference.
	 */

	/* as per RP0207-2005:
	 * "Descriptors are uniquely identified by their descriptor tag. Any of the descriptors
	 *  listed in table 6-25 of ATSC A/65, as well as ATSC A/53 and ISO/IEC 13818-1, may be
	 *  transported as described in this practice. Equipment that is intended to comply with
	 *  this practice shall support those shown in the following table."
	 */
/*
AD - A private descriptor tagged with Sencores letters. We don't know its format.
>> hdr->type   = 0
>>  ->adf      = 0x0000/0x03ff/0x03ff
>>  ->did/sdid = 0x62 / 0x01 [Undefined Undefined] via SDI line 10
>>  ->h_offset = 0
>>  ->checksum = 0x024e (VALID)
>>  ->payloadLengthWords = 26
>>  ->payload  = ad 18 53 45 4e 31 01 57 53 45 45 20 54 56 00 00 00 00 00
>> 00 00 00 01 f0 04 01
*/

	unsigned short arr[] = {
		0x000,
		0x3ff,
		0x3ff,
		0x162,
		0x101,
		0x11A, /* DC */
		0x0ad, /* Payload Field = "ATSC private information descriptor" */
		0x018, /* Payload length */
		0x053,
		0x045,
		0x04e,
		0x031,
		0x001,
		0x057,
		0x053,
		0x045,
		0x045,
		0x020,
		0x054,
		0x056,
		0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000, 0x000,
		0x001,
		0x0f0,
		0x004,
		0x001,
		0x24e, /* Checksum is valid */
	};

	/* report that this was from line 13, informational only. */
	int ret = klvanc_packet_parse(ctx, 13, arr, sizeof(arr) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

/*
AD - A private descriptor tagged with Sencores letters. We don't know its format.
hdr->type   = 0
 ->adf      = 0x0000/0x03ff/0x03ff
 ->did/sdid = 0x62 / 0x01 [Undefined Undefined] via SDI line 10
 ->h_offset = 0
 ->checksum = 0x018c (VALID)
 ->payloadLengthWords = 23
 ->payload  = ad 15 53 45 4e 31 04 00 00 00 00 00 00 00 00 00 00 00 00 00 00 01 34 
*/

	unsigned short arr2[] = {
		0x000,
		0x3ff,
		0x3ff,
		0x162,
		0x101,
		0x117, /* DC */
		0x0ad, /* Payload Field = "ATSC private information descriptor" */
		0x015, /* Payload length */
		0x053,
		0x045,
		0x04e,
		0x031,
		0x004,
		0x000, 0x000, 0x000, 0x000,
		0x000, 0x000, 0x000, 0x000,
		0x000, 0x000, 0x000, 0x000,
		0x000, 0x000,
		0x001,
		0x034,
		0x18c, /* Checksum is valid */
	};

	/* report that this was from line 13, informational only. */
	ret = klvanc_packet_parse(ctx, 13, arr2, sizeof(arr2) / (sizeof(unsigned short)));
	if (ret < 0)
		return ret;

	return 0;
}

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

static unsigned char eia608_entry[] = {
	0x00, 0x00, 0x03, 0xff, 0x03, 0xff, 0x01, 0x61, 0x01, 0x02, 0x02, 0x03,
	0x02, 0x95, 0x01, 0x80, 0x01, 0x80, 0x01, 0xfb, 0x02, 0x00, 0x00, 0x40,
	0x02, 0x00
};
static int test_EIA_608(struct klvanc_context_s *ctx)
{

	printf("\nParsing a new CEA-608 VANC packet......\n");
	uint16_t *arr = malloc(sizeof(eia608_entry) / 2 * sizeof(uint16_t));
	if (arr == NULL)
		return -1;

	for (int i = 0; i < (sizeof(eia608_entry) / 2); i++) {
		arr[i] = eia608_entry[i * 2] << 8 | eia608_entry[i * 2 + 1];
	}

	printf("Original Input\n");
	for (int i = 0; i < (sizeof(eia608_entry) / 2); i++) {
		printf("%04x ", arr[i]);
	}
	printf("\n");

	int ret = klvanc_packet_parse(ctx, 13, arr, sizeof(eia608_entry) / sizeof(unsigned short));
	free(arr);

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

static int test_multimessage(struct klvanc_context_s *ctx)
{
	/* A fragmented SCTE104 multimessage. */

	/* These came from the metadata group.
	 * They'll need to be endian flipped before processing.
	 */
	uint16_t arr[] = {
		/* Msg 1 */
		0x0000, 0xff03, 0xff03, 0x4102, 0x0701, 0xff02, 0x0c02, 0xff02, // | ......A.........
		0xff02, 0x0101, 0x0e01, 0x0002, 0x0002, 0x0902, 0x0002, 0x0002, // | ................
		0x0002, 0x0002, 0x0502, 0x0101, 0x0401, 0x0002, 0x0201, 0x0f02, // | ................
		0xa002, 0x0101, 0x0b01, 0x0002, 0x2301, 0x0701, 0xb802, 0x4202, // | ........#.....B.
		0xb901, 0x0002, 0x0002, 0x0002, 0x0101, 0x1102, 0x5502, 0x4b02, // | ............U.K.
		0x5201, 0x4102, 0x4901, 0x4e02, 0x4501, 0x4802, 0x4501, 0x4c01, // | R.A.I.N.E.H.E.L.
		0x5002, 0x3002, 0x3401, 0x5701, 0x5002, 0x5602, 0x4901, 0x3502, // | P.0.4.W.P.V.I.5.
		0x0101, 0x0101, 0x0002, 0x0002, 0x0101, 0x0101, 0x0101, 0x0302, // | ................
		0x0101, 0x0b01, 0x0002, 0x4301, 0x0701, 0x8202, 0xab01, 0x6902, // | ......C.......i.
		0x0002, 0x0002, 0x0002, 0x0101, 0x3101, 0x5002, 0x4301, 0x5201, // | ........1.P.C.R.
		0x3101, 0x5f02, 0x3002, 0x3302, 0x3002, 0x3502, 0x3201, 0x3201, // | 1._.0.3.0.5.2.2.
		0x3002, 0x3502, 0x3502, 0x3801, 0x5701, 0x5002, 0x5602, 0x4901, // | 0.5.5.8.W.P.V.I.
		0x4102, 0x4301, 0x5401, 0x4901, 0x4f01, 0x4e02, 0x4e02, 0x4501, // | A.C.T.I.O.N.N.E.
		0x5701, 0x5302, 0x5302, 0x4102, 0x5401, 0x5502, 0x5201, 0x4402, // | W.S.S.A.T.U.R.D.
		0x4102, 0x5902, 0x4d02, 0x4f01, 0x5201, 0x4e02, 0x4901, 0x4e02, // | A.Y.M.O.R.N.I.N.
		0x4702, 0x4102, 0x5401, 0x3602, 0x4102, 0x4d02, 0x1102, 0x0101, // | G.A.T.6.A.M.....
		0x0101, 0x0002, 0x0002, 0x0101, 0x0101, 0x0101, 0x0302, 0x0101, // | ................
		0x0b01, 0x0002, 0x4301, 0x0701, 0xb901, 0xb202, 0x1601, 0x0002, // | ....C...........
		0x0002, 0x0002, 0x0101, 0x3101, 0x5002, 0x4301, 0x5201, 0x3101, // | ......1.P.C.R.1.
		0x5f02, 0x3002, 0x3302, 0x3002, 0x3502, 0x3201, 0x3201, 0x3002, // | _.0.3.0.5.2.2.0.
		0x3602, 0x3502, 0x3801, 0x5701, 0x5002, 0x5602, 0x4901, 0x4102, // | 6.5.8.W.P.V.I.A.
		0x4301, 0x5401, 0x4901, 0x4f01, 0x4e02, 0x4e02, 0x4501, 0x5701, // | C.T.I.O.N.N.E.W.
		0x5302, 0x5302, 0x4102, 0x5401, 0x5502, 0x5201, 0x4402, 0x4102, // | S.S.A.T.U.R.D.A.
		0x5902, 0x4d02, 0x4f01, 0x5201, 0x4e02, 0x4901, 0x4e02, 0x4702, // | Y.M.O.R.N.I.N.G.
		0x4102, 0x5401, 0x3701, 0x4102, 0x4d02, 0x1001, 0x0101, 0x0101, // | A.T.7.A.M.......
		0x0002, 0x0002, 0x0101, 0x0101, 0x0101, 0x0302, 0x0101, 0x0b01, // | ................
		0x0002, 0x4301, 0x0701, 0xb901, 0xb202, 0x1702, 0x0002, 0x0201, // | ..C.............
		0x6201, 0x0101, 0x3101, 0x5002, 0x4301, 0x5201, 0x3101, 0x5f02, // | b...1.P.C.R.1._.
		0x3002, 0x3302, 0x3002, 0x3502, 0x3201, 0x3201, 0x3002, 0x3602, // | 0.3.0.5.2.2.0.6.
		0x3502, 0x3801, 0x5701, 0x5002, 0x5602, 0x4901, 0x4102, 0x4301, // | 5.8.W.P.V.I.A.C.
		0x5401, 0x4901, 0x4f01, 0x4e02, 0x4e02, 0x4501, 0x5701, 0x5302, // | T.I.O.N.N.E.W.S.
		0x5302, 0x4102, 0x5401, 0x5502, 0x5201, 0x4402, 0x4102, 0x5902, // | S.A.T.U.R.D.A.Y.
		0x4d02, 0x4f01, 0x5201, 0x4e02, 0x4901, 0xd502,                 // | M.O.R.N.I...

		/* Msg 2 */
		0x0000, 0xff03, 0xff03, 0x4102, 0x0701, 0x1102, 0x0a02, 0x4e02, // | ......A.......N.
		0x4702, 0x4102, 0x5401, 0x3701, 0x4102, 0x4d02, 0x2001, 0x0101, // | G.A.T.7.A.M. ...
		0x0101, 0x0002, 0x0002, 0x0101, 0x0101, 0x0101, 0x0302, 0x7a01, // | ..............z.
	};

	printf("\nParsing a new SCTE104 Multi-message VANC packet......\n");
	uint16_t *b = malloc(sizeof(arr) + (16 * 2));
	if (b == NULL)
		return -1;

	for (int i = 0; i < (sizeof(arr) / sizeof(uint16_t)); i++) {
		b[i] = arr[i] << 8 | arr[i] >> 8;
	}

	printf("Converted Input\n");
	for (int i = 0; i < (sizeof(arr) / 2); i++) {
		printf("%04x ", b[i]);
	}
	printf("\n");

	int ret = klvanc_packet_parse(ctx, 13, b, sizeof(arr) / sizeof(uint16_t));
	free(b);
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

	ret = test_EIA_608(ctx);
	if (ret < 0)
		fprintf(stderr, "EIA_608 failed to parse\n");

	ret = test_scte_104(ctx);
	if (ret < 0)
		fprintf(stderr, "SCTE-104 failed to parse\n");

	ret = test_checksum();
	if (ret < 0)
		fprintf(stderr, "Checksum calculation failed\n");

	ret = test_program_description_data(ctx);
	if (ret < 0)
		fprintf(stderr, "Program Description Data failed\n");

	ret = test_package_of_umid_data(ctx);
	if (ret < 0)
		fprintf(stderr, "Package of UMID Data failed\n");

	ret = test_multimessage(ctx);
	if (ret < 0)
		fprintf(stderr, "Multi-messages failed\n");

	klvanc_context_destroy(ctx);
	printf("Library destroyed.\n");

	return 0;
}
