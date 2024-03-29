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
#include <inttypes.h>
#include <unistd.h>
#include <string.h>
#include <libgen.h>
#include <fcntl.h>
#include <getopt.h>
#include "klbitstream_readwriter.h"
#include "ts_packetizer.h"
#include "version.h"
#include "hexdump.h"

#define DEFAULT_PID 0x80

static struct app_context_s
{
	int verbose;
	unsigned int pid;
} app_context;

static struct app_context_s *ctx = &app_context;

/* Create a PES array containing 8 lines of VANC data.
 */
static void smpte2038_generate_sample_708B_packet(struct app_context_s *ctx)
{
	/* STEP 1. Generate some useful VANC packed into a PES frame. */

	/* This is a fully formed 708B VANC message. We'll bury
	 * this inside a SMPTE2038 wrapper and prepare a TS packet
	 * that could be used for sample/test data.
	 */
	unsigned short arr[] = {
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
		0x1b4, /* Checksum */
	};

	/* See smpte 2038-2008 - Page 5, Table 2 for description. */
	struct klbs_context_s *bs  = klbs_alloc();

	uint8_t buf[8192] = { 0 };
	klbs_write_set_buffer(bs, buf, sizeof(buf));

	/* PES Header - Bug: bitstream can't write 32bit values */
	klbs_write_bits(bs, 1, 24);		/* packet_start_code_prefix */
	klbs_write_bits(bs, 0xBD, 8);		/* stream_id */
	klbs_write_bits(bs, 0, 16);		/* PES_packet_length */
	klbs_write_bits(bs, 2, 2);		/* '10' fixed value */
	klbs_write_bits(bs, 0, 2);		/* PES_scrambling_control (not scrambled) */
	klbs_write_bits(bs, 0, 1);		/* PES_priority */
	klbs_write_bits(bs, 1, 1);		/* data_alignment_indicator (aligned) */
	klbs_write_bits(bs, 0, 1);		/* copyright (not-copyright) */
	klbs_write_bits(bs, 0, 1);		/* original-or-copy (copy) */
	klbs_write_bits(bs, 2, 2);		/* PTS_DTS_flags (PTS Present) */
	klbs_write_bits(bs, 0, 1);		/* ESCR_flag (not present) */
	klbs_write_bits(bs, 0, 1);		/* ES_RATE_flag (not present) */
	klbs_write_bits(bs, 0, 1);		/* DSM_TRICK_MODE_flag (not present) */
	klbs_write_bits(bs, 0, 1);		/* additional_copy_info_flag (not present) */
	klbs_write_bits(bs, 0, 1);		/* PES_CRC_flag (not present) */
	klbs_write_bits(bs, 0, 1);		/* PES_EXTENSION_flag (not present) */
	klbs_write_bits(bs, 5, 8);		/* PES_HEADER_DATA_length */
	klbs_write_bits(bs, 2, 4);		/* '0010' fixed value */

	uint64_t pts = 0;
	klbs_write_bits(bs, (pts >> 30), 3);	/* PTS[32:30] */
	klbs_write_bits(bs, 1, 1);			/* marker_bit */
	klbs_write_bits(bs, (pts >> 15) & 0xefff, 15);	/* PTS[29:15] */
	klbs_write_bits(bs, 1, 1);			/* marker_bit */
	klbs_write_bits(bs, (pts & 0xefff), 15);		/* PTS[14:0] */
	klbs_write_bits(bs, 1, 1);			/* marker_bit */

	int lineCount = 4;
	for (int i = 0; i < lineCount; i++) {
		/* VANC Payload */
		klbs_write_bits(bs, 0, 6);	/* fixed value '000000' */
		klbs_write_bits(bs, 0, 1);	/* c_not_y_channel_flag (HD luminance) */
		klbs_write_bits(bs, 10 + i, 11);	/* line_number (9) */
		klbs_write_bits(bs, 0, 12);	/* horizonal_offset (0 words from SAV) */
		klbs_write_bits(bs, arr[3], 10);	/* DID */
		klbs_write_bits(bs, arr[4], 10);	/* SDID */
		klbs_write_bits(bs, arr[5], 10);	/* data_count */
		for (int i = 6; i < (sizeof(arr) / sizeof(unsigned short)); i++) {
			/* This data_count AND checksum */
			klbs_write_bits(bs, arr[i], 10);
		}

		/* Byte alignment stuffing */
		while (bs->reg_used > 0) {
			klbs_write_bits(bs, 1, 1);	/* Stuffing byte */
		}
	}

	/* Flush the remaining bits out into buffer, else we lose up to
	 * the last 7 bits that are cached in the bitstream implementation.
	 */
	klbs_write_buffer_complete(bs);

	/* Now updated the PES packet length */
	int len = klbs_get_byte_count(bs) - 6;
	klbs_get_buffer(bs)[4] = (len >> 8) & 0xff;
	klbs_get_buffer(bs)[5] = len & 0xff;

	/* STEP 2. Do something useful with the PES, now that its fully assembled. */

	/* The PES is ready. Save a file copy. */
	printf("%s() We've constructed a fake PES, here it is:\n", __func__);
	hexdump(buf, klbs_get_byte_count(bs), 16);
	klbs_save(bs, "/tmp/bitstream-scte2038-EIA708B.raw");

	/* STEP 3. Maybe we should packetize the PES into TS packets. */

	uint8_t section[8192];
	int section_length = klbs_get_byte_count(bs);
	memset(section, 0xff, sizeof(section));
	memcpy(section, klbs_get_buffer(bs), section_length);

	uint8_t *pkts = 0;
	uint32_t packetCount = 0;
	uint8_t cc = 0;
	ts_packetizer(section, section_length, &pkts, &packetCount, 188, &cc, ctx->pid);
	for (uint32_t i = 0; i < packetCount; i++) {
		printf("%s() We've constructed some TS packets, here they are:\n", __func__);
		hexdump(pkts + (i * 188), 188, 16);
	}

	/* Write all the TS packets to a temp file. */
	FILE *fh = fopen("/tmp/bitstream-scte2038-EIA708B.ts", "wb");
	if (fh) {
		fwrite(pkts, packetCount, 188, fh);
		fclose(fh);
	}

	free(pkts); /* Results from the packetizer have to be caller freed. */

	klbs_free(bs);
}

static int _usage(const char *progname, int status)
{
	fprintf(stderr, COPYRIGHT "\n");
	fprintf(stderr, "Generate a SMPTE2038 stream containing 708 VANC\n");
	fprintf(stderr, "Usage: %s [OPTIONS]\n"
		"    -P <pid 0xNNNN> VANC PID to generate to (def: 0x%x)\n"
		"    -h This help page\n",
		basename((char *)progname),
		DEFAULT_PID
		);
	exit(status);
}

static int _main(int argc, char *argv[])
{
	int opt;
	ctx->pid = DEFAULT_PID;

	while ((opt = getopt(argc, argv, "?h:P")) != -1) {
		switch (opt) {
                case 'P':
                        if ((sscanf(optarg, "0x%x", &ctx->pid) != 1) || (ctx->pid > 0x1fff))
				_usage(argv[0], 1);
                        break;
		case '?':
		case 'h':
			_usage(argv[0], 0);
		}
	}

	smpte2038_generate_sample_708B_packet(ctx);
	exit(0);
}

int gensmpte2038_main(int argc, char *argv[])
{
	return _main(argc, argv);
}
