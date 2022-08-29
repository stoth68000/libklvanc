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
#include <pthread.h>
#include <libgen.h>
#include <fcntl.h>
#include <getopt.h>
#include <signal.h>
#include <sys/time.h>
#include <libklvanc/vanc.h>
#include "udp.h"
#include "url.h"
#include "pes_extractor.h"
#include "version.h"
#include "hexdump.h"

#define DEFAULT_FIFOSIZE 1048576
#define DEFAULT_PID 0x80

static struct app_context_s
{
	int verbose;
	int running;
	char *input_url;
	struct url_opts_s *i_url;
	unsigned int pid;
	int pes_packets_found;
	int vanc_packets_found;
	char *decode_types;

	struct iso13818_udp_receiver_s *udprx;
	struct pes_extractor_s *pe;
	struct klvanc_context_s *vanchdl;
} app_context;

static struct app_context_s *ctx = &app_context;

/* When the PES extractor has depacketized a PES packet of data, we're
 * called with the entire PES array. Parse it, dump it to console.
 * We're called from the thread context of whoever calls pe_push().
 */
static pes_extractor_callback pes_cb(void *cb_context, uint8_t *buf, int byteCount)
{
	/* Warning: we're shadowing the global ctx at this point. */
	struct app_context_s *ctx = cb_context;
	if (ctx->verbose) {
		printf("%s()\n", __func__);
		if (ctx->verbose > 1)
			hexdump(buf, byteCount, 16);
	}

	/* Parse the PES section, like any other tool might. */
	struct klvanc_smpte2038_anc_data_packet_s *pkt = 0;
	klvanc_smpte2038_parse_pes_packet(buf, byteCount, &pkt);
	if (pkt) {
		ctx->pes_packets_found++;

		/* Dump the entire message in english to console, handy for debugging. */
		klvanc_smpte2038_anc_data_packet_dump(pkt);

		/* For fun, convert all SMPTE2038 ANC Lines into raw VANC, then parse
		 * it using the standard VANC library facilities.
		 */
		printf("SMPTE2038 message has %d line(s), displaying...\n", pkt->lineCount);
		for (int i = 0; i < pkt->lineCount; i++) {
			struct klvanc_smpte2038_anc_data_line_s *l = &pkt->lines[i];

			uint16_t *words;
			uint16_t wordCount;
			if (klvanc_smpte2038_convert_line_to_words(l, &words, &wordCount) < 0)
				break;

			if (ctx->verbose > 1) {
				printf("LineEntry[%d]: ", i);
				for (int j = 0; j < wordCount; j++)
					printf("%03x ", words[j]);
				printf("\n\n");
			}

			/* Heck, why don't we attempt to parse the vanc? */
			if (klvanc_packet_parse(ctx->vanchdl, l->line_number, words, wordCount) < 0) {
			}

			free(words); /* Caller must free the resource */

			ctx->vanc_packets_found++;
		}

		/* Don't forget to free the parsed SMPTE2038 packet */
		klvanc_smpte2038_anc_data_packet_free(pkt);
	}
	else
		fprintf(stderr, "Error parsing packet\n");

	/* TODO: Push the vanc into the VANC processor */

	return 0;
}

/* We're called with blocks of UDP data */
static tsudp_receiver_callback udp_cb(void *userContext, uint8_t *buf, int byteCount)
{
	struct app_context_s *ctx = userContext;

	if (ctx->verbose) {
		printf("%s() pushing %d bytes\n", __func__, byteCount);
		if (ctx->verbose > 1)
			hexdump(buf, 188, 16);
	}
	pe_push(ctx->pe, buf, byteCount / 188);
	return 0;
}

static void signal_handler(int signum)
{
	ctx->running = 0;
}

/* FIXME: Maybe move this into the library itself? */
static struct decode_types {
	const char *name;
	enum klvanc_packet_type_e type;
} valid_decode_types[] = {
	{ "cea608", VANC_TYPE_EIA_608 },
	{ "cea708", VANC_TYPE_EIA_708B },
	{ "afd", VANC_TYPE_AFD },
	{ "atc", VANC_TYPE_SMPTE_S12_2 },
	{ "klcounter", VANC_TYPE_KL_UINT64_COUNTER },
	{ "scte104", VANC_TYPE_SCTE_104 },
	{ "sdp", VANC_TYPE_SDP },
};

static int _usage(const char *progname, int status)
{
	fprintf(stderr, COPYRIGHT "\n");
	fprintf(stderr, "Detect and capture SMPTE2038 VANC frames from a UDP transport stream.\n");
	fprintf(stderr, "Usage: %s [OPTIONS]\n"
		"    -i <udp url. Eg. udp://224.0.0.1:5000>\n"
		"    -P <pid 0xNNNN> VANC PID to process (def: 0x%x)\n"
		"    -v Increase verbose level\n"
		"    -t <vanc_types> enable VANC dumping (e.g. 'cea708,scte104')\n"
		"       valid types are: all",
	basename((char *)progname),
	DEFAULT_PID
	);
	for (int j = 0; j < (sizeof(valid_decode_types) / sizeof(struct decode_types)); j++) {
		fprintf(stderr, ",%s", valid_decode_types[j].name);
	}
	fprintf(stderr, "\n");

	exit(status);
}

static int cb_AFD(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_afd_s *pkt)
{
	return klvanc_dump_AFD(ctx, pkt);
}

static int cb_EIA_708B(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_708b_s *pkt)
{
	return klvanc_dump_EIA_708B(ctx, pkt);
}

static int cb_EIA_608(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_eia_608_s *pkt)
{
	return klvanc_dump_EIA_608(ctx, pkt);
}

static int cb_SCTE_104(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_scte_104_s *pkt)
{
	return klvanc_dump_SCTE_104(ctx, pkt);
}

static int cb_SMPTE_12M(void *callback_context, struct klvanc_context_s *ctx, struct klvanc_packet_smpte_12_2_s *pkt)
{
	return klvanc_dump_SMPTE_12_2(ctx, pkt);
}

static int cb_KL_UINT64_COUNTER(void *callback_context, struct klvanc_context_s *ctx,
				struct klvanc_packet_kl_u64le_counter_s *pkt)
{
	return klvanc_dump_KL_U64LE_COUNTER(ctx, pkt);
}

static int cb_SDP(void *callback_context, struct klvanc_context_s *ctx,
		  struct klvanc_packet_sdp_s *pkt)
{
	return klvanc_dump_SDP(ctx, pkt);
}

static int enable_logging(struct klvanc_callbacks_s *callbacks, enum klvanc_packet_type_e type)
{
	switch(type) {
	case VANC_TYPE_EIA_608:
		callbacks->eia_608 = cb_EIA_608;
		break;
	case VANC_TYPE_EIA_708B:
		callbacks->eia_708b = cb_EIA_708B;
		break;
	case VANC_TYPE_SCTE_104:
		callbacks->scte_104 = cb_SCTE_104;
		break;
	case VANC_TYPE_AFD:
		callbacks->afd = cb_AFD;
		break;
	case VANC_TYPE_SMPTE_S12_2:
		callbacks->smpte_12_2 = cb_SMPTE_12M;
		break;
	case VANC_TYPE_KL_UINT64_COUNTER:
		callbacks->kl_i64le_counter = cb_KL_UINT64_COUNTER;
		break;
	case VANC_TYPE_SDP:
		callbacks->sdp = cb_SDP;
		break;
	default:
		return -1;
	}
	return 0;
}

static int _main(int argc, char *argv[])
{
	int opt;
	int exitStatus = 0;
	ctx->running = 1;
	ctx->pid = DEFAULT_PID;
	ctx->verbose = 0;
	char *dtype;
	enum {
		IT_UDP = 0,
		IT_FILE
	} inputType = IT_UDP;
	static struct klvanc_callbacks_s callbacks;

	while ((opt = getopt(argc, argv, "?hi:P:vt:")) != -1) {
		switch (opt) {
		case 'i':
			ctx->input_url = optarg;
			if (url_parse(ctx->input_url, &ctx->i_url) < 0) {
				/* NOT a valid URL, assume its a file */
				if (access(optarg, R_OK) != 0)
					_usage(argv[0], 0);

				inputType = IT_FILE;
			} else
				inputType = IT_UDP;
			break;
                case 'P':
                        if ((sscanf(optarg, "0x%x", &ctx->pid) != 1) || (ctx->pid > 0x1fff))
				_usage(argv[0], 1);
                        break;
		case 'v':
			ctx->verbose++;
			break;
		case 't':
			ctx->decode_types = optarg;
			while( (dtype = strsep(&ctx->decode_types, ",")) != NULL ) {
				int found = 0;
				if (strcmp("all", dtype) == 0) {
					for (int j = 0; j < (sizeof(valid_decode_types) / sizeof(struct decode_types)); j++)
						enable_logging(&callbacks, valid_decode_types[j].type);
					found = 1;
					break;
				}
				for (int j = 0; j < (sizeof(valid_decode_types) / sizeof(struct decode_types)); j++)
				{
					if (strcmp(valid_decode_types[j].name, dtype) == 0) {
						if (enable_logging(&callbacks, valid_decode_types[j].type) == 0)
							found = 1;
					}
				}
				if (found == 0)
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

	pe_alloc(&ctx->pe, ctx, (pes_extractor_callback)pes_cb, ctx->pid);
	signal(SIGINT, signal_handler);

	if (klvanc_context_create(&ctx->vanchdl) < 0) {
		fprintf(stderr, "Error initializing klvanc library context\n");
		exit(1);
	}
	ctx->vanchdl->verbose = 1;

	/* Define callbacks which dump out the structures */
	ctx->vanchdl->callbacks = &callbacks;

	if (inputType == IT_UDP) {
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

		/* Start UDP receive and wait for CTRL-C */
		iso13818_udp_receiver_thread_start(ctx->udprx);
		while (ctx->running) {
			usleep(100 * 1000);
		}

		/* Shutdown */
		iso13818_udp_receiver_free(&ctx->udprx);
	} else
	if (inputType == IT_FILE) {
		FILE *fh = fopen(ctx->input_url, "rb");
		if (fh) {

			uint8_t pkt[188];
			while (!feof(fh) && ctx->running) {
				if (fread(pkt, 188, 1, fh) != 1)
					break;

				udp_cb(ctx, pkt, sizeof(pkt));
			}
			fclose(fh);
		}

	}

	pe_free(&ctx->pe);

no_mem:

	/* Print summary */
	printf("Total PES packets found: %d\n", ctx->pes_packets_found);
	printf("Total VANC packets found: %d\n", ctx->vanc_packets_found);
	printf("Total VANC checksum failures: %d\n", ctx->vanchdl->checksum_failures);

	klvanc_context_destroy(ctx->vanchdl);
	return exitStatus;
}

int smpte2038_main(int argc, char *argv[])
{
	return _main(argc, argv);
}
