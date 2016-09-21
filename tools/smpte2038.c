/* Copyright (c) 2016 Kernel Labs Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <libgen.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <libklvanc/vanc.h>
#include "udp.h"
#include "url.h"

#include "version.h"

#define DEFAULT_FIFOSIZE 1048576

static struct app_context_s
{
	int running;
	char *input_url;
	struct url_opts_s *i_url;

	struct iso13818_udp_receiver_s *udprx;
} app_context;

static struct app_context_s *ctx = &app_context;

static void hexdump(unsigned char *buf, unsigned int len, int bytesPerRow /* Typically 16 */)
{
        for (unsigned int i = 0; i < len; i++)
                printf("%02x%s", buf[i], ((i + 1) % bytesPerRow) ? " " : "\n");
        printf("\n");
}

#include "bitstream.h"
static void smpte2038_generate_sample_708B_packet()
{

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
	wBitStream *bs  = bitstream_new();

	uint8_t buf[8192] = { 0 };
	bitstream_attach(bs, buf, sizeof(buf));

	/* PES Header - Bug: bitstream can't write 32bit values */
	bitstream_write_bits(bs, 1, 24);	/* packet_start_code_prefix */
	bitstream_write_bits(bs, 0xBD, 8);	/* stream_id */
	bitstream_write_bits(bs, 0, 16);	/* PES_packet_length */
	bitstream_write_bits(bs, 2, 2);		/* '10' fixed value */
	bitstream_write_bits(bs, 0, 2);		/* PES_scrambling_control (not scrambled) */
	bitstream_write_bits(bs, 0, 1);		/* PES_priority */
	bitstream_write_bits(bs, 1, 1);		/* data_alignment_indicator (aligned) */
	bitstream_write_bits(bs, 0, 1);		/* copyright (not-copyright) */
	bitstream_write_bits(bs, 0, 1);		/* original-or-copy (copy) */
	bitstream_write_bits(bs, 2, 2);		/* PTS_DTS_flags (PTS Present) */
	bitstream_write_bits(bs, 0, 1);		/* ESCR_flag (not present) */
	bitstream_write_bits(bs, 0, 1);		/* ES_RATE_flag (not present) */
	bitstream_write_bits(bs, 0, 1);		/* DSM_TRICK_MODE_flag (not present) */
	bitstream_write_bits(bs, 0, 1);		/* additional_copy_info_flag (not present) */
	bitstream_write_bits(bs, 0, 1);		/* PES_CRC_flag (not present) */
	bitstream_write_bits(bs, 0, 1);		/* PES_EXTENSION_flag (not present) */
	bitstream_write_bits(bs, 5, 8);		/* PES_HEADER_DATA_length */
	bitstream_write_bits(bs, 2, 4);		/* '0010' fixed value */

	uint64_t pts = 0;
	bitstream_write_bits(bs, (pts >> 30), 3);	/* PTS[32:30] */
	bitstream_write_bits(bs, 1, 1);			/* marker_bit */
	bitstream_write_bits(bs, (pts >> 15) & 0xefff, 15);	/* PTS[29:15] */
	bitstream_write_bits(bs, 1, 1);			/* marker_bit */
	bitstream_write_bits(bs, (pts & 0xefff), 15);		/* PTS[14:0] */
	bitstream_write_bits(bs, 1, 1);			/* marker_bit */

	int lineCount = 1;
	for (int i = 0; i < lineCount; i++) {
		/* VANC Payload */
		bitstream_write_bits(bs, 0, 6);		/* fixed value '000000' */
		bitstream_write_bits(bs, 0, 1);		/* c_not_y_channel_flag (HD luminance) */
		bitstream_write_bits(bs, 9, 11);	/* line_number (9) */
		bitstream_write_bits(bs, 0, 12);	/* horizonal_offset (0 words from SAV) */
		bitstream_write_bits(bs, arr[3], 10);	/* DID */
		bitstream_write_bits(bs, arr[4], 10);	/* SDID */
		bitstream_write_bits(bs, arr[5], 10);	/* data_count */
		for (int i = 6; i < (sizeof(arr) / sizeof(unsigned short)); i++) {
			/* This data_count AND checksum */
			bitstream_write_bits(bs, arr[i], 10);
		}

		/* Byte alignment stuffing */
		while (bs->position % 8) {
			bitstream_write_bits(bs, 1, 1);	/* Stuffing byte */
		}
	}

	/* Flush the remaining bits out into buffer, else we lose up to
	 * the last 32 bits that are cached in the bitstream implementation.
	 */
	bitstream_flush(bs);

	/* Now updated the PES packet length */
	int len = bitstream_getlength_bytes(bs) - 6;
	bs->buffer[4] = (len >> 8) & 0xff;
	bs->buffer[5] = len & 0xff;

	hexdump(buf, bitstream_getlength_bytes(bs), 16);
	bitstream_file_save(bs, "/tmp/bitstream-scte2038-EIA708B.raw");

	/* Now, also produce a transport packet, for pid 0x80 */
	uint8_t section[8192];
	memset(section, 0xff, sizeof(section));
	memcpy(section, bs->buffer, bitstream_getlength_bytes(bs));

	/* Create a sample TS packet, on pid 0x88 */
	uint8_t pkt[188];
	memset(pkt, 0xff, sizeof(pkt));
	pkt[0] = 0x47;
	pkt[1] = 0x40;
	pkt[2] = 0x88;
	pkt[3] = 0x10;
	memcpy(&pkt[4], bs->buffer, bitstream_getlength_bytes(bs));
	FILE *fh = fopen("/tmp/bitstream-scte2038-EIA708B.ts", "wb");
	if (fh) {
		fwrite(pkt, sizeof(pkt), 1, fh);
		fclose(fh);
	}
	hexdump(pkt, sizeof(pkt), 16);

	bitstream_free(bs);

	struct smpte2038_anc_data_packet_s *result = 0;
	smpte2038_parse_section(section, bitstream_getlength_bytes(bs), &result);
	smpte2038_smpte2038_anc_data_packet_dump(result);
}

static tsudp_receiver_callback udp_cb(void *userContext, unsigned char *buf, int byteCount)
{
//	struct app_context_s *ctx = userContext;
	return 0;
}

static void signal_handler(int signum)
{
	ctx->running = 0;
}

static int _usage(const char *progname, int status)
{
	fprintf(stderr, COPYRIGHT "\n");
	fprintf(stderr, "Detect and capture SMPTE2038 VANC frames from a UDP transport stream.\n");
	fprintf(stderr, "Usage: %s [OPTIONS]\n"
		"    -i <udp url. Eg. udp://224.0.0.1:5000>\n"
		"    -g generate sample SMPTE2038 stream and parse it.\n",
	basename((char *)progname));

	exit(status);
}

static int _main(int argc, char *argv[])
{
	int opt;
	int exitStatus = 0;
	ctx->running = 1;

	while ((opt = getopt(argc, argv, "?ghi:")) != -1) {
		switch (opt) {
		case 'g':
			smpte2038_generate_sample_708B_packet();
			exit(0);
			break;
		case 'i':
			ctx->input_url = optarg;
			if (url_parse(ctx->input_url, &ctx->i_url) < 0) {
				_usage(argv[0], 1);
                        }
			break;
		case '?':
		case 'h':
			_usage(argv[0], 0);
		}
	}

	if (ctx->input_url == NULL) {
		fprintf(stderr, "Missing mandatory -i option\n");
		_usage(argv[0], 1);
	}

        int fs = DEFAULT_FIFOSIZE;
	if (ctx->i_url->has_fifosize)
		fs = ctx->i_url->fifosize;

	if (iso13818_udp_receiver_alloc(&ctx->udprx, fs,
		ctx->i_url->hostname, ctx->i_url->port, (tsudp_receiver_callback)udp_cb, ctx, 0) < 0) {
		fprintf(stderr, "Unable to allocate a UDP Receiver for %s:%d\n",
		ctx->i_url->hostname, ctx->i_url->port);
		goto no_mem;
	}

	/* Add a multicast NIC if reqd. */
	if (ctx->i_url->has_ifname) {
		iso13818_udp_receiver_join_multicast(ctx->udprx, ctx->i_url->ifname);
	}

	signal(SIGINT, signal_handler);

	iso13818_udp_receiver_thread_start(ctx->udprx);
	while (ctx->running) {
		usleep(100 * 1000);
	}
	iso13818_udp_receiver_free(&ctx->udprx);

no_mem:
	return exitStatus;
}

int smpte2038_main(int argc, char *argv[])
{
	return _main(argc, argv);
}
