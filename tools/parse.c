/* Copyright (c) 2014-2016 Kernel Labs Inc. */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <assert.h>
#include <libgen.h>
#include <signal.h>
#include <stdbool.h>
#include <libklvanc/vanc.h>

#include "hexdump.h"
#include "version.h"

static struct klvanc_context_s *vanchdl;
FILE *vancOutputFile = NULL;
static int g_verbose = 0;
static int g_saveVanc = 0;
static unsigned int g_frameCount = 0;
static unsigned int g_lastLine = 0;
static unsigned int g_vancEntryCount = 0;

/* Filtering */
static int g_filterMatch = 0;
static int g_filtermatchCount = 0;
uint16_t g_filter_did = 0;
uint16_t g_filter_sdid = 0;


static const char *g_vancOutputFilename = NULL;
static const char *g_vancInputFilename = NULL;

static void convert_colorspace_and_parse_vanc(unsigned char *buf, unsigned int uiWidth, unsigned int lineNr)
{
	/* Convert the vanc line from V210 to CrCB422, then vanc parse it */

	/* We need two kinds of type pointers into the source vbi buffer */
	/* TODO: What the hell is this, two ptrs? */
	const uint32_t *src = (const uint32_t *)buf;

	/* Convert Blackmagic pixel format to nv20.
	 * src pointer gets mangled during conversion, hence we need its own
	 * ptr instead of passing vbiBufferPtr */
	uint16_t decoded_words[16384];
	memset(&decoded_words[0], 0, sizeof(decoded_words));
	uint16_t *p_anc = decoded_words;
	if (klvanc_v210_line_to_nv20_c(src, p_anc, sizeof(decoded_words), (uiWidth / 6) * 6) < 0)
		return;

	int ret = klvanc_packet_parse(vanchdl, lineNr, decoded_words, sizeof(decoded_words) / (sizeof(unsigned short)));
	if (ret < 0) {
		/* No VANC on this line */
	}
}

#define VANC_SOL_INDICATOR 0xEFBEADDE
#define VANC_EOL_INDICATOR 0xEDFEADDE
static int AnalyzeVANC(const char *fn)
{
	FILE *fh = fopen(fn, "rb");
	if (!fh) {
		fprintf(stderr, "Unable to open [%s]\n", fn);
		return -1;
	}

	fseek(fh, 0, SEEK_END);
	fprintf(stdout, "Analyzing VANC file [%s] length %lu bytes\n", fn, ftell(fh));
	fseek(fh, 0, SEEK_SET);

	unsigned int uiSOL;
	unsigned int uiLine;
	unsigned int uiWidth;
	unsigned int uiHeight;
	unsigned int uiStride;
	unsigned int uiEOL;
	unsigned int maxbuflen = 16384;
	unsigned char *buf = (unsigned char *)malloc(maxbuflen);
	size_t ret;

	while (!feof(fh)) {

		/* Warning: Balance these reads with the file writes in processVANC */
		ret = fread(&uiSOL, 1, sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}
		ret = fread(&uiLine, 1, sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}
		ret = fread(&uiWidth, 1, sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}
		ret = fread(&uiHeight, 1, sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}
		ret = fread(&uiStride, 1,sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}

		memset(buf, 0, maxbuflen);
		if (uiStride > maxbuflen) {
			fprintf(stderr, "Invalid stride specified: %d\n", uiStride);
			break;
		}

		ret = fread(buf, 1, uiStride, fh);
		if (ret < uiStride) {
			fprintf(stderr, "Premature end of file\n");
		}

		assert(uiStride < maxbuflen);
		ret = fread(&uiEOL, 1, sizeof(unsigned int), fh);
		if (ret < sizeof(unsigned int)) {
			fprintf(stderr, "Premature end of file\n");
			break;
		}

		if (uiLine <= g_lastLine) {
			g_frameCount++;
		}
		g_lastLine = uiLine;

#if 0
		fprintf(stdout, "Line: %04d SOL: %x EOL: %x ", uiLine, uiSOL, uiEOL);
		fprintf(stdout, "Width: %d Height: %d Stride: %d ", uiWidth, uiHeight, uiStride);
#endif
		if (uiSOL != VANC_SOL_INDICATOR) {
			fprintf(stdout, " SOL corrupt\n");
			break;
		} else if (uiEOL != VANC_EOL_INDICATOR) {
			fprintf(stdout, " EOL corrupt\n");
			break;
		} else {
#if 0
			fprintf(stdout, "\n");
#endif
		}

		if (g_verbose > 1)
			hexdump(buf, uiStride, 64);

		g_filterMatch = 0;
		convert_colorspace_and_parse_vanc(buf, uiStride, uiLine);
		if (g_filterMatch) {
			g_filtermatchCount++;
			/* Line matched filter criteria, so do something with it */
			if (vancOutputFile) {
				fwrite(&uiSOL, sizeof(unsigned int), 1, vancOutputFile);
				fwrite(&uiLine, sizeof(unsigned int), 1, vancOutputFile);
				fwrite(&uiWidth, sizeof(unsigned int), 1, vancOutputFile);
				fwrite(&uiHeight, sizeof(unsigned int), 1, vancOutputFile);
				fwrite(&uiStride, sizeof(unsigned int), 1, vancOutputFile);
				fwrite(buf, uiStride, 1, vancOutputFile);
				fwrite(&uiEOL, sizeof(unsigned int), 1, vancOutputFile);
				fflush(vancOutputFile);
			}
		}
	}

	free(buf);
	fclose(fh);

	if (g_filter_did || g_filter_sdid) {
		if (g_filtermatchCount == 0)
			fprintf(stderr, "Filtering requested but no matching records found\n");
		else
			fprintf(stderr, "Filtering returned %d entries\n", g_filtermatchCount);
	}

	fprintf(stderr, "Frames processed: %d frames\n", g_frameCount);

	return 0;
}

static int pkt_filtered(struct klvanc_packet_header_s *pkt)
{
	if (g_filter_did > 0) {
		if (g_filter_sdid > 0) {
			/* Filtering by both DID and SDID */
			if (g_filter_did == pkt->did && g_filter_sdid == pkt->dbnsdid) {
				return 1;
			}
		} else {
			/* only filtering by DID */
			if (g_filter_did == pkt->did) {
				return 1;
			}
		}
	} else {
		/* No filtering configured, so everything matches */
		return 1;
	}
	return 0;
}

/* CALLBACKS for message notification */
static int cb_AFD(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_afd_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		if (klvanc_dump_AFD(ctx, pkt) < 0)
			fprintf(stderr, "Failed to dump AFD packet");
	}

	return 0;
}

static int cb_EIA_708B(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_eia_708b_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		if (klvanc_dump_EIA_708B(ctx, pkt) < 0)
			fprintf(stderr, "Failed to dump CEA-708 packet");
	}

	return 0;
}

static int cb_EIA_608(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_eia_608_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		if (klvanc_dump_EIA_608(ctx, pkt) < 0)
			fprintf(stderr, "Failed to dump EIA-608 packet");
	}

	return 0;
}

static int cb_SCTE_104(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_scte_104_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		if (klvanc_dump_SCTE_104(ctx, pkt) < 0)
			fprintf(stderr, "Failed to dump SCTE-104 packet");
	}
	return 0;
}

static int cb_SMPTE_12_2(void *callback_context, struct klvanc_context_s *ctx,
			 struct klvanc_packet_smpte_12_2_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		klvanc_dump_SMPTE_12_2(ctx, pkt);
	}
	return 0;
}

static int cb_all(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_header_s *pkt)
{
	if (pkt_filtered(pkt)) {
		g_filterMatch = 1;

		if (g_saveVanc > 0) {
			char tmpfname[256];
			snprintf(tmpfname, sizeof(tmpfname), "%d.vancentry", g_vancEntryCount);
			FILE *fd = fopen(tmpfname, "w");
			if (fd) {
				for (int i = 0; i < pkt->payloadLengthWords + 7; i++) {
					unsigned char out[2];
					out[0] = pkt->raw[i] >> 8;
					out[1] = pkt->raw[i] & 0xff;
					fwrite(out, 2, 1, fd);
				}
				fclose(fd);
			}
		}
		g_vancEntryCount++;
	}
	return 0;
}

static int cb_SDP(void *callback_context, struct klvanc_context_s *ctx,
		struct klvanc_packet_sdp_s *pkt)
{
	/* Have the library display some debug */
	if (pkt_filtered(&pkt->hdr)) {
		klvanc_dump_SDP(ctx, pkt);
	}
	return 0;
}

static struct klvanc_callbacks_s callbacks =
{
	.afd			= cb_AFD,
	.eia_708b               = cb_EIA_708B,
	.eia_608                = cb_EIA_608,
	.scte_104               = cb_SCTE_104,
	.all                    = cb_all,
	.kl_i64le_counter       = NULL,
	.sdp                    = cb_SDP,
	.smpte_12_2             = cb_SMPTE_12_2,
};

/* END - CALLBACKS for message notification */

static int usage(const char *progname, int status)
{
	fprintf(stderr, COPYRIGHT "\n");
	fprintf(stderr, "Capture decklink SDI payload, capture vanc, analyze vanc.\n");
	fprintf(stderr, "Usage: %s -m <mode id> [OPTIONS]\n", basename((char *)progname));

	fprintf(stderr,
		"    -I <filename>   Interpret and display input VANC filename (See -V)\n"
		"    -o <filename>   Saved filtered lines to an output file\n"
		"    -v              Increase level of verbosity (def: 0)\n"
		"    -d <did>        Filter by DID\n"
		"    -s <sdid>       Filter by SDID\n"
		"\n"
		"Parse a file and output all SCTE-104 entries:\n"
		"    %s -I foo.vanc -d 0x41 -s 0x07\n\n"
		"Parse a file and save to a file the filtered set:\n"
		"    %s -I foo.vanc -d 0x41 -s 0x07 -o output.vanc\n\n",
		basename((char *)progname),
		basename((char *)progname)
		);

	exit(status);
}

int parse_main(int argc, char *argv[])
{
	int ch;
	bool wantHelp = false;

	while ((ch = getopt(argc, argv, "?hf:o:p:vxI:d:s:")) != -1) {
		switch (ch) {
		case 'o':
			g_vancOutputFilename = optarg;
			break;
		case 'I':
			g_vancInputFilename = optarg;
			break;
		case 'd':
			g_filter_did = strtoul(optarg, NULL, 0);;
			break;
		case 's':
			g_filter_sdid = strtoul(optarg, NULL, 0);;
			break;
		case 'v':
			g_verbose++;
			break;
		case 'x':
			g_saveVanc++;
			break;
		case '?':
		case 'h':
			wantHelp = true;
		}
	}

	if (wantHelp) {
		usage(argv[0], 0);
		goto bail;
	}

	if (klvanc_context_create(&vanchdl) < 0) {
		fprintf(stderr, "Error initializing library context\n");
		exit(1);
	}

	vanchdl->verbose = g_verbose;
	vanchdl->callbacks = &callbacks;

	if (g_vancOutputFilename != NULL) {
		vancOutputFile = fopen(g_vancOutputFilename, "w");
		if (vancOutputFile == NULL) {
			fprintf(stderr, "Could not open vanc output file \"%s\"\n", g_vancOutputFilename);
			goto bail;
		}
		fprintf(stderr, "Opened file for output: %s\n", g_vancOutputFilename);
	}

	if (g_vancInputFilename != NULL) {
		return AnalyzeVANC(g_vancInputFilename);
	}

	klvanc_context_destroy(vanchdl);

bail:

	if (vancOutputFile != NULL)
		fclose(vancOutputFile);

	return 0;
}
