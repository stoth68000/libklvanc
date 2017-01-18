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
#include <zlib.h>
#if HAVE_CURSES_H
#include <curses.h>
#endif
#include <libgen.h>
#include <signal.h>
#include <libklvanc/vanc.h>
#include <libklvanc/smpte2038.h>

#include "hexdump.h"
#include "version.h"
#include "DeckLinkAPI.h"
#include "ts_packetizer.h"

#define WIDE 80

class DeckLinkCaptureDelegate : public IDeckLinkInputCallback
{
public:
	DeckLinkCaptureDelegate();
	~DeckLinkCaptureDelegate();

	virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID iid, LPVOID * ppv) { return E_NOINTERFACE; }
	virtual ULONG STDMETHODCALLTYPE AddRef(void);
	virtual ULONG STDMETHODCALLTYPE Release(void);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFormatChanged(BMDVideoInputFormatChangedEvents, IDeckLinkDisplayMode *, BMDDetectedVideoInputFormatFlags);
	virtual HRESULT STDMETHODCALLTYPE VideoInputFrameArrived(IDeckLinkVideoInputFrame *, IDeckLinkAudioInputPacket *); 

private:
	ULONG m_refCount;
	pthread_mutex_t m_mutex;
};

#define RELEASE_IF_NOT_NULL(obj) \
        if (obj != NULL) { \
                obj->Release(); \
                obj = NULL; \
        }

/*
48693539 Mode:  9 HD 1080i 59.94    Mode:  6 HD 1080p 29.97  

Decklink Hardware supported modes:
[decklink @ 0x25da300] Mode:  0 NTSC                     6e747363 [ntsc]
[decklink @ 0x25da300] Mode:  1 NTSC 23.98               6e743233 [nt23]
[decklink @ 0x25da300] Mode:  2 PAL                      70616c20 [pal ]
[decklink @ 0x25da300] Mode:  3 HD 1080p 23.98           32337073 [23ps]
[decklink @ 0x25da300] Mode:  4 HD 1080p 24              32347073 [24ps]
[decklink @ 0x25da300] Mode:  5 HD 1080p 25              48703235 [Hp25]
[decklink @ 0x25da300] Mode:  6 HD 1080p 29.97           48703239 [Hp29]
[decklink @ 0x25da300] Mode:  7 HD 1080p 30              48703330 [Hp30]
[decklink @ 0x25da300] Mode:  8 HD 1080i 50              48693530 [Hi50]
[decklink @ 0x25da300] Mode:  9 HD 1080i 59.94           48693539 [Hi59]
[decklink @ 0x25da300] Mode: 10 HD 1080i 60              48693630 [Hi60]
[decklink @ 0x25da300] Mode: 11 HD 720p 50               68703530 [hp50]
[decklink @ 0x25da300] Mode: 12 HD 720p 59.94            68703539 [hp59]
[decklink @ 0x25da300] Mode: 13 HD 720p 60               68703630 [hp60]
*/

static struct vanc_context_s *vanchdl;
static pthread_mutex_t sleepMutex;
static pthread_cond_t sleepCond;
static int videoOutputFile = -1;
static int audioOutputFile = -1;
static int vancOutputFile = -1;
static int g_showStartupMemory = 0;
static int g_verbose = 0;
static unsigned int g_linenr = 0;

/* SMPTE 2038 */
static int g_packetizeSMPTE2038 = 0;
static int g_packetizePID = 0;
static struct smpte2038_packetizer_s *smpte2038_ctx = 0;
static uint8_t g_cc = 0;
/* END:SMPTE 2038 */

static IDeckLink *deckLink;
static IDeckLinkInput *deckLinkInput;
static IDeckLinkDisplayModeIterator *displayModeIterator;

static BMDTimecodeFormat g_timecodeFormat = 0;
static int g_videoModeIndex = -1;
static uint32_t g_audioChannels = 2;
static uint32_t g_audioSampleDepth = 16;
static const char *g_videoOutputFilename = NULL;
static const char *g_audioOutputFilename = NULL;
static const char *g_vancOutputFilename = NULL;
static const char *g_vancInputFilename = NULL;
static int g_maxFrames = -1;
static int g_shutdown = 0;
static int g_monitor_reset = 0;
static int g_monitor_mode = 0;
static int g_no_signal = 1;
static BMDDisplayMode g_detected_mode_id = 0;
static BMDDisplayMode g_requested_mode_id = 0;

static unsigned long audioFrameCount = 0;
static struct frameTime_s {
	unsigned long long lastTime;
	unsigned long long frameCount;
	unsigned long long remoteFrameCount;
} frameTimes[2];

#if HAVE_CURSES_H
pthread_t threadId;
struct vanc_monitor_line_s
{
	int      active;
	uint64_t count;
	pthread_mutex_t mutex;
	struct   packet_header_s *pkt;
};

struct vanc_monitor_s
{
	uint32_t activeCount;
	struct vanc_monitor_line_s lines[2048];
	uint32_t did, sdid;
	const char *desc, *spec;
	struct timeval lastUpdated;
	int hasCursor;
	int expandUI;
};
static struct vanc_monitor_s *selected = 0;

/* A static array of structs, for did/sdid rapid lookups.
 * Where DD and SD range from 00.FFF.
 */
//static struct vanc_monitor_s monitorLines[0x10000];
static struct vanc_monitor_s *monitorLines;
#define vanc_monitor_stat_lookup(didnr, sdidnr) &monitorLines[ (((didnr) << 8) | (sdidnr)) ]

static void cursor_expand_all()
{
	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (!e->activeCount)
				continue;
			e->expandUI = 1;
		}
	}
}

static void cursor_expand()
{
	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (!e->activeCount)
				continue;

			if (e->hasCursor == 1) {
				if (e->expandUI)
					e->expandUI = 0;
				else
					e->expandUI = 1;
				return;
			}
		}
	}
}

static void cursor_down()
{
	struct vanc_monitor_s *def = 0;
	struct vanc_monitor_s *prev = 0;

	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (!e->activeCount)
				continue;

			def = e;
			if (e->hasCursor == 1 && !prev) {
				prev = e;
			} else
			if (!e->hasCursor && prev) {
				prev->hasCursor = 0;
				e->hasCursor = 1;
				return;
			}
		}
	}

	if (def)
		def->hasCursor = 1;
}

static void cursor_up()
{
	struct vanc_monitor_s *def = 0;
	struct vanc_monitor_s *prev = 0;

	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (!e->activeCount)
				continue;

			def = e;
			if (e->hasCursor == 0) {
				prev = e;
			} else
			if (e->hasCursor && prev) {
				prev->hasCursor = 1;
				e->hasCursor = 0;
				return;
			}
		}
	}

	if (def)
		def->hasCursor = 0;
}


static void vanc_monitor_stats_reset()
{
	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			e->activeCount = 0;

			for (int l = 0; l < 2048; l++) {
				struct vanc_monitor_line_s *line = &e->lines[ l ];
				if (!line->active)
					continue;

				line->active = 0;
				line->count = 0;

				pthread_mutex_lock(&line->mutex);
				if (line->pkt) {
					vanc_packet_free(line->pkt);
					line->pkt = 0;
				}
				pthread_mutex_unlock(&line->mutex);
			}
		}
	}
}

static void vanc_monitor_stats_dump_curses()
{
	int linecount = 0;
	int headLineColor = 1;
	int cursorColor = 5;

	char head_c[160];
	if (g_no_signal)
		sprintf(head_c, "NO SIGNAL");
	else
	if (g_requested_mode_id == g_detected_mode_id)
		sprintf(head_c, "SIGNAL LOCKED");
	else {
		//sprintf(head_c, "CHECK SIGNAL SETTINGS %x == %x", g_detected_mode_id, g_requested_mode_id);
		sprintf(head_c, "CHECK SIGNAL SETTINGS");
		headLineColor = 4;
	}

	char head_a[160];
	sprintf(head_a, " DID / SDID  DESCRIPTION");

	char head_b[160];
	int blen = (WIDE - 5) - (strlen(head_a) + strlen(head_c));
	memset(head_b, 0x20, sizeof(head_b));
	head_b[blen] = 0;

	attron(COLOR_PAIR(headLineColor));
	mvprintw(linecount++, 0, "%s%s%s", head_a, head_b, head_c);
        attroff(COLOR_PAIR(headLineColor));

	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (e->activeCount == 0)
				continue;

			{
				if (e->hasCursor)
					attron(COLOR_PAIR(cursorColor));
				char t[80];
				sprintf(t, "  %02x / %02x    %s [%s] ", e->did, e->sdid, e->desc, e->spec);
				mvprintw(linecount++, 0, "%-75s", t);
				if (e->hasCursor)
					attroff(COLOR_PAIR(cursorColor));
			}

			for (int l = 0; l < 2048; l++) {
				struct vanc_monitor_line_s *line = &e->lines[ l ];
				if (!line->active)
					continue;

				pthread_mutex_lock(&line->mutex);
				struct packet_header_s *pkt = line->pkt;

				mvprintw(linecount++, 13, "line #%d count #%lu horizontal offset word #%d", l, line->count,
					pkt->horizontalOffset);

				if (e->expandUI)
				{
					mvprintw(linecount++, 13, "data length: 0x%x (%d)",
						pkt->payloadLengthWords,
						pkt->payloadLengthWords);

					char p[256] = { 0 };
					int cnt = 0;
					for (int w = 0; w < pkt->payloadLengthWords; w++) {
						sprintf(p + strlen(p), "%02x ", (pkt->payload[w]) & 0xff);
						if (++cnt == 16 || (w + 1) == pkt->payloadLengthWords) {
							cnt = 0;
							if (w == 15 || (pkt->payloadLengthWords < 15))
								mvprintw(linecount++, 13, "  -> %s", p);
							else
								mvprintw(linecount++, 13, "     %s", p);
							p[0] = 0;
						}
					}
					mvprintw(linecount++, 13, "checksum %03x (%s)",
						pkt->checksum,
						pkt->checksumValid ? "VALID" : "INVALID");
				}
				pthread_mutex_unlock(&line->mutex);
			}

			linecount++;
		}
	}

	attron(COLOR_PAIR(2));
        mvprintw(linecount++, 0, "q)uit r)eset e)xpand E)xpand all");
	attroff(COLOR_PAIR(2));

	char tail_c[160];
	time_t now = time(0);
	sprintf(tail_c, "%s", ctime(&now));

	char tail_a[160];
	sprintf(tail_a, "KLVANC_CAPTURE");

	char tail_b[160];
	blen = (WIDE - 4) - (strlen(tail_a) + strlen(tail_c));
	memset(tail_b, 0x20, sizeof(tail_b));
	tail_b[blen] = 0;

	attron(COLOR_PAIR(1));
	mvprintw(linecount++, 0, "%s%s%s", tail_a, tail_b, tail_c);
        attroff(COLOR_PAIR(1));
}

static void vanc_monitor_stats_dump()
{
	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_monitor_s *e = vanc_monitor_stat_lookup(d, s);
			if (e->activeCount == 0)
				continue;

			printf("%02x/%02x: %s [%s] ", e->did, e->sdid, e->desc, e->spec);
			for (int l = 0; l < 2048; l++) {
				if (e->lines[l].active)
					printf("%d(%" PRIu64 ") ", l, e->lines[l].count);
			}
			printf("\n");
		}
	}
}

static void vanc_monitor_stat_alloc()
{
	monitorLines = (struct vanc_monitor_s *)calloc(0x10000, sizeof(struct vanc_monitor_s));
}

static void vanc_monitor_stat_free()
{
	free(monitorLines);
}

static int vanc_monitor_stat_update(struct packet_header_s *pkt)
{
	if (pkt->did > 0xff)
		return -1;
	if (pkt->dbnsdid > 0xff)
		return -1;
	if (pkt->lineNr >= 2048)
		return -1;

	struct vanc_monitor_s *s = vanc_monitor_stat_lookup(pkt->did, pkt->dbnsdid);
	if (s->activeCount == 0) {
		s->did = pkt->did;
		s->sdid = pkt->dbnsdid;
		s->desc = vanc_lookupDescriptionByType(pkt->type);
		s->spec = vanc_lookupSpecificationByType(pkt->type);
	}
	gettimeofday(&s->lastUpdated, NULL);

	struct vanc_monitor_line_s *line = &s->lines[ pkt->lineNr ];
	line->active = 1;
	s->activeCount++;

	pthread_mutex_lock(&line->mutex);
	if (line->pkt) {
		vanc_packet_free(line->pkt);
		line->pkt = 0;
	}
	vanc_packet_copy(&line->pkt, pkt);
	pthread_mutex_unlock(&line->mutex);

	line->count++;

	if (selected == 0) {
		selected = s;
		selected->hasCursor = 1;
	}

	return 0;
}

static void signal_handler(int signum);
static void *thread_func_input(void *p)
{
	while (!g_shutdown) {
		int ch = getch();
		if (ch == 'q') {
			signal_handler(1);
			break;
		}
		if (ch == 'r')
			g_monitor_reset = 1;
		if (ch == 'e')
			cursor_expand();
		if (ch == 'E')
			cursor_expand_all();
		if (ch == 0x1b) {
			ch = getch();

			/* Cursor keys */
			if (ch == 0x5b) {
				ch = getch();
				if (ch == 0x41) {
					/* Up arrow */
					cursor_up();
				} else
				if (ch == 0x42) {
					/* Down arrow */
					cursor_down();
				} else
				if (ch == 0x43) {
					/* Right arrow */
				} else
				if (ch == 0x44) {
					/* Left arrow */
				}
			}
		}
	}
	return 0;
}

static void *thread_func_draw(void *p)
{
	noecho();
	curs_set(0);
	start_color();
	init_pair(1, COLOR_WHITE, COLOR_BLUE);
	init_pair(2, COLOR_CYAN, COLOR_BLACK);
	init_pair(3, COLOR_RED, COLOR_BLACK);
	init_pair(4, COLOR_RED, COLOR_BLUE);
	init_pair(5, COLOR_BLACK, COLOR_WHITE);

	while (!g_shutdown) {
		if (g_monitor_reset) {
			g_monitor_reset = 0;
			vanc_monitor_stats_reset();
		}

		clear();
		vanc_monitor_stats_dump_curses();

		refresh();
		usleep(100 * 1000);
	}

	return 0;
}
#endif /* HAVE_CURSES_H */

static void signal_handler(int signum)
{
	pthread_cond_signal(&sleepCond);
	g_shutdown = 1;
}

static void showMemory(FILE * fd)
{
	char fn[64];
	char s[80];
	sprintf(fn, "/proc/%d/statm", getpid());

	FILE *fh = fopen(fn, "rb");
	if (!fh)
		return;

	memset(s, 0, sizeof(s));
	size_t wlen = fread(s, 1, sizeof(s), fh);
	fclose(fh);

	if (wlen > 0) {
		fprintf(fd, "%s: %s", fn, s);
	}
}

static unsigned long long msecsX10()
{
	unsigned long long elapsedMs;

	struct timeval now;
	gettimeofday(&now, 0);

	elapsedMs = (now.tv_sec * 10000.0);	/* sec to ms */
	elapsedMs += (now.tv_usec / 100.0);	/* us to ms */

	return elapsedMs;
}

static char g_mode[5];		/* Racey */
static const char *display_mode_to_string(BMDDisplayMode m)
{
	g_mode[4] = 0;
	g_mode[3] = m;
	g_mode[2] = m >> 8;
	g_mode[1] = m >> 16;
	g_mode[0] = m >> 24;

	return &g_mode[0];
}

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

	int ret = vanc_packet_parse(vanchdl, lineNr, decoded_words, sizeof(decoded_words) / (sizeof(unsigned short)));
	if (ret < 0) {
		/* No VANC on this line */
	}
}

#define VANC_SOL_INDICATOR 0xEFBEADDE
#define VANC_EOL_INDICATOR 0xEDFEADDE
#define TS_OUTPUT_NAME "/tmp/smpte2038-sample.ts"
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

	while (!feof(fh)) {

		/* Warning: Balance these reads with the file writes in processVANC */
		fread(&uiSOL, sizeof(unsigned int), 1, fh);
		fread(&uiLine, sizeof(unsigned int), 1, fh);
		fread(&uiWidth, sizeof(unsigned int), 1, fh);
		fread(&uiHeight, sizeof(unsigned int), 1, fh);
		fread(&uiStride, sizeof(unsigned int), 1, fh);
		memset(buf, 0, maxbuflen);
		fread(buf, uiStride, 1, fh);
		assert(uiStride < maxbuflen);
		fread(&uiEOL, sizeof(unsigned int), 1, fh);

		if (g_linenr && g_linenr != uiLine)
			continue;

		fprintf(stdout, "Line: %04d SOL: %x EOL: %x ", uiLine, uiSOL, uiEOL);
		fprintf(stdout, "Width: %d Height: %d Stride: %d ", uiWidth, uiHeight, uiStride);
		if (uiSOL != VANC_SOL_INDICATOR)
			fprintf(stdout, " SOL corrupt ");
		if (uiEOL != VANC_EOL_INDICATOR)
			fprintf(stdout, " EOL corrupt ");

		fprintf(stdout, "\n");

		if (g_verbose > 1)
			hexdump(buf, uiStride, 64);

		if (uiLine == 1 && g_packetizeSMPTE2038) {
			if (smpte2038_packetizer_end(smpte2038_ctx, 0) == 0) {
				printf("%s() PES buffer is complete\n", __func__);

				uint8_t *pkts = 0;
				uint32_t packetCount = 0;
				if (ts_packetizer(smpte2038_ctx->buf, smpte2038_ctx->bufused, &pkts,
					&packetCount, 188, &g_cc, g_packetizePID) == 0) {
					FILE *fh = fopen(TS_OUTPUT_NAME, "a+");
					if (fh) {
						if (g_verbose) {
							printf("Writing %d SMPTE2038 TS packet(s) to %s\n",
								packetCount, TS_OUTPUT_NAME);
						}
						fwrite(pkts, packetCount, 188, fh);
						fclose(fh);
					}
					free(pkts);
				}
			}
			smpte2038_packetizer_begin(smpte2038_ctx);
		}
		convert_colorspace_and_parse_vanc(buf, uiStride, uiLine);
	}

	free(buf);
	fclose(fh);

	return 0;
}

#define COMPRESS 0
#if COMPRESS
static int cdstlen = 16384;
static uint8_t *cdstbuf = 0;
#endif
#define DECOMPRESS 0
#if DECOMPRESS
static int ddstlen = 16384;
static uint8_t *ddstbuf = 0;
#endif
static void ProcessVANC(IDeckLinkVideoInputFrame * frame)
{
	IDeckLinkVideoFrameAncillary *vanc;
	if (frame->GetAncillaryData(&vanc) != S_OK)
		return;

	if (g_packetizeSMPTE2038)
		smpte2038_packetizer_begin(smpte2038_ctx);

	BMDDisplayMode dm = vanc->GetDisplayMode();
	BMDPixelFormat pf = vanc->GetPixelFormat();

	unsigned int uiStride = frame->GetRowBytes();
	unsigned int uiWidth = frame->GetWidth();
	unsigned int uiHeight = frame->GetHeight();
	unsigned int uiLine;
	unsigned int uiSOL = VANC_SOL_INDICATOR;
	unsigned int uiEOL = VANC_EOL_INDICATOR;
	int written = 0;
	for (unsigned int i = 0; i < uiHeight; i++) {
		uint8_t *buf;
		int ret = vanc->GetBufferForVerticalBlankingLine(i, (void **)&buf);
		if (ret != S_OK)
			continue;

		uiLine = i;

		/* Process the line colorspace, hand-off to the vanc library for parsing
		 * and prepare to receive callbacks.
		 */
		convert_colorspace_and_parse_vanc(buf, uiWidth, uiLine);

		if (vancOutputFile >= 0) {
			/* Warning: Balance these writes with the file reads in AnalyzeVANC */
			write(vancOutputFile, &uiSOL, sizeof(unsigned int));
			write(vancOutputFile, &uiLine, sizeof(unsigned int));
			write(vancOutputFile, &uiWidth, sizeof(unsigned int));
			write(vancOutputFile, &uiHeight, sizeof(unsigned int));
			write(vancOutputFile, &uiStride, sizeof(unsigned int));
			write(vancOutputFile, buf, uiStride);
#if COMPRESS
			if (cdstbuf == 0)
				cdstbuf = (uint8_t *)malloc(cdstlen);

			/* Pack metadata into the pre-compress buffer */
			int z = 0;
			*(buf + z++) = uiLine >> 8;
			*(buf + z++) = uiLine;
			*(buf + z++) = uiWidth >> 8;
			*(buf + z++) = uiWidth;
			*(buf + z++) = uiHeight >> 8;
			*(buf + z++) = uiHeight;
			*(buf + z++) = uiStride >> 8;
			*(buf + z++) = uiStride;

			z_stream zInfo = { 0 };
			zInfo.total_out = zInfo.avail_out = cdstlen;
			zInfo.next_in = (uint8_t *)buf + z;
			zInfo.total_in = zInfo.avail_in = z + uiStride;
			zInfo.next_out = cdstbuf;
			memcpy(buf + z, buf, uiStride);

			int nErr = deflateInit(&zInfo, Z_DEFAULT_COMPRESSION);
			unsigned int compressLength = 0;
			if (nErr == Z_OK ) {
				nErr = deflate(&zInfo, Z_FINISH);
				if (nErr == Z_STREAM_END) {
					compressLength = zInfo.total_out;
					write(vancOutputFile, &compressLength, sizeof(unsigned int));
					write(vancOutputFile, cdstbuf, compressLength);
					if (g_verbose > 1)
						printf("Compressed %d bytes\n", compressLength);
				} else {
					fprintf(stderr, "Failed to compress payload\n");
				}
			}
			deflateEnd(&zInfo);
#endif
#if DECOMPRESS
			/* Decompress and verify */
			if (ddstbuf == 0)
				ddstbuf = (uint8_t *)malloc(ddstlen);

			z_stream dzInfo = { 0 };
			dzInfo.total_in = dzInfo.avail_in = compressLength;
			dzInfo.total_out = dzInfo.avail_out = ddstlen;
			dzInfo.next_in = (uint8_t *)cdstbuf;
			dzInfo.next_out = ddstbuf;

			nErr = inflateInit(&dzInfo);
			if (nErr == Z_OK) {
				nErr = inflate(&dzInfo, Z_FINISH);
				if (nErr == Z_STREAM_END) {
					if (memcmp(buf, ddstbuf, dzInfo.total_out) == 0) {
						/* Success */
					} else
						fprintf(stderr, "Decompress validation failed\n");
				} else
					fprintf(stderr, "Inflate error, %d\n", nErr);
			} else
				fprintf(stderr, "Decompress error, %d\n", nErr);
			inflateEnd(&dzInfo);
#endif
			write(vancOutputFile, &uiEOL, sizeof(unsigned int));

			written++;
		}

	}

	if (g_packetizeSMPTE2038) {
		BMDTimeValue stream_time;
		BMDTimeValue frame_duration;
		frame->GetStreamTime(&stream_time, &frame_duration, 90000);
		if (smpte2038_packetizer_end(smpte2038_ctx, stream_time) == 0) {
			printf("%s() PES buffer is complete\n", __func__);
		}
	}

	if (g_verbose) {
		fprintf(stdout, "PixelFormat %x [%s] DisplayMode [%s] Wrote %d [potential] VANC lines\n",
			pf,
			pf == bmdFormat8BitYUV ? "bmdFormat8BitYUV" :
			pf == bmdFormat10BitYUV ? "bmdFormat10BitYUV" :
			pf == bmdFormat8BitARGB ? "bmdFormat8BitARGB" :
			pf == bmdFormat8BitBGRA ? "bmdFormat8BitBGRA" :
			pf == bmdFormat10BitRGB ? "bmdFormat10BitRGB" : "undefined",
			display_mode_to_string(dm), written);
	}

	vanc->Release();

#if COMPRESS
	if (cdstbuf) {
		free(cdstbuf);
		cdstbuf = 0;
	}
#endif
#if DECOMPRESS
	if (ddstbuf) {
		free(ddstbuf);
		ddstbuf = 0;
	}
#endif
	return;
}

DeckLinkCaptureDelegate::DeckLinkCaptureDelegate()
: m_refCount(0)
{
	pthread_mutex_init(&m_mutex, NULL);
}

DeckLinkCaptureDelegate::~DeckLinkCaptureDelegate()
{
	pthread_mutex_destroy(&m_mutex);
}

ULONG DeckLinkCaptureDelegate::AddRef(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount++;
	pthread_mutex_unlock(&m_mutex);

	return (ULONG) m_refCount;
}

ULONG DeckLinkCaptureDelegate::Release(void)
{
	pthread_mutex_lock(&m_mutex);
	m_refCount--;
	pthread_mutex_unlock(&m_mutex);

	if (m_refCount == 0) {
		delete this;
		return 0;
	}

	return (ULONG) m_refCount;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFrameArrived(IDeckLinkVideoInputFrame *videoFrame, IDeckLinkAudioInputPacket *audioFrame)
{
	if (g_shutdown == 1) {
		g_shutdown = 2;
		return S_OK;
	}
	if (g_shutdown == 2)
		return S_OK;

	IDeckLinkVideoFrame *rightEyeFrame = NULL;
	IDeckLinkVideoFrame3DExtensions *threeDExtensions = NULL;
	void *frameBytes;
	void *audioFrameBytes;
	struct frameTime_s *frameTime;

	if (g_showStartupMemory) {
		showMemory(stderr);
		g_showStartupMemory = 0;
	}
	// Handle Video Frame
	if (videoFrame) {
		frameTime = &frameTimes[0];

		static int didDrop = 0;
		unsigned long long t = msecsX10();
		double interval = t - frameTime->lastTime;
		interval /= 10;
		if (frameTime->lastTime && (frameTime->lastTime + 170) < t) {
			//printf("\nLost %f frames (no frame for %7.2f ms)\n", interval / 16.7, interval);
			didDrop = 1;
		} else if (didDrop) {
			//printf("\nCatchup %4.2f ms\n", interval);
			didDrop = 0;
		}
		frameTime->lastTime = t;

		// If 3D mode is enabled we retreive the 3D extensions interface which gives.
		// us access to the right eye frame by calling GetFrameForRightEye() .
		if ((videoFrame->QueryInterface(IID_IDeckLinkVideoFrame3DExtensions, (void **)&threeDExtensions) != S_OK)
		    || (threeDExtensions->GetFrameForRightEye(&rightEyeFrame) != S_OK)) {
			rightEyeFrame = NULL;
		}

		if (threeDExtensions)
			threeDExtensions->Release();

		if (videoFrame->GetFlags() & bmdFrameHasNoInputSource && !g_monitor_mode) {
			g_no_signal = 1;
			if (!g_monitor_mode) {
				fprintf(stdout, "Frame received (#%8llu) - No input signal detected (%7.2f ms)\n",
					frameTime->frameCount, interval);
			}
		} else {
			g_no_signal = 0;
			const char *timecodeString = NULL;
			if (g_timecodeFormat != 0) {
				IDeckLinkTimecode *timecode;
				if (videoFrame->
				    GetTimecode(g_timecodeFormat,
						&timecode) == S_OK) {
					timecode->GetString(&timecodeString);
				}
			}

			unsigned int currRFC = 0;
#if 0
			/* KL: Look for the framecount metadata, created by the KL signal generator. */
			unsigned char *pixelData;
			videoFrame->GetBytes((void **)&pixelData);
			if ((*(pixelData + 0) == 0xde) &&
			    (*(pixelData + 1) == 0xad) &&
			    (*(pixelData + 2) == 0xbe) &&
			    (*(pixelData + 3) == 0xef)) {

				unsigned char *p = pixelData + 4;

				unsigned char tag = 0;
				unsigned char taglen = 0;
				while (tag != 0xaa /* No more tags */ ) {
					tag = *p++;
					taglen = *p++;

					//fprintf(stdout, "tag %x len %x\n", tag, taglen);
					if (tag == 0x01 /* Frame counter */ ) {

						/* We need a null n the string end before we can convert it */
						unsigned char tmp[16];
						memset(tmp, 0, sizeof(tmp));
						memcpy(tmp, p, 10);

						currRFC =
						    atoi((const char *)tmp);
					}

					p += taglen;
				}

				//for (int c = 0; c < 18; c++)
				//      fprintf(stdout, "%02x ", *(pixelData + c));
				//fprintf(stdout, "\n");
			}
#endif

			int isBad = 0;
			if (frameTime->remoteFrameCount + 1 == currRFC)
				isBad = 0;

			if (g_verbose) {
				fprintf(stdout,
					"Frame received (#%10llu) [%s] - %s - Size: %li bytes (%7.2f ms) [remoteFrame: %d] ",
					frameTime->frameCount,
					timecodeString !=
					NULL ? timecodeString : "No timecode",
					rightEyeFrame !=
					NULL ? "Valid Frame (3D left/right)" :
					"Valid Frame",
					videoFrame->GetRowBytes() *
					videoFrame->GetHeight(), interval, currRFC);
			

				if (isBad) {
					fprintf(stdout, " %lld frames lost", currRFC - frameTime->remoteFrameCount);
					fprintf(stdout, "\n");
				} else
					fprintf(stdout, "\n");
			}

			frameTime->remoteFrameCount = currRFC;

			if (isBad)
				showMemory(stdout);

			if (timecodeString)
				free((void *)timecodeString);

			if (videoOutputFile != -1) {
				videoFrame->GetBytes(&frameBytes);
				write(videoOutputFile, frameBytes,
				      videoFrame->GetRowBytes() *
				      videoFrame->GetHeight());

				if (rightEyeFrame) {
					rightEyeFrame->GetBytes(&frameBytes);
					write(videoOutputFile, frameBytes,
					      videoFrame->GetRowBytes() *
					      videoFrame->GetHeight());
				}
			}
		}

		if (rightEyeFrame)
			rightEyeFrame->Release();

		frameTime->frameCount++;

		if (frameTime->frameCount == 100) {
			//usleep(1100 * 1000);
		}

		if (g_maxFrames > 0 && (int)frameTime->frameCount >= g_maxFrames) {
			pthread_cond_signal(&sleepCond);
		}
	}

	/* Video Ancillary data */
	if (videoFrame)
		ProcessVANC(videoFrame);

	// Handle Audio Frame
	if (audioFrame) {
		frameTime = &frameTimes[1];

		uint32_t sampleSize =
		    audioFrame->GetSampleFrameCount() * g_audioChannels *
		    (g_audioSampleDepth / 8);

		unsigned long long t = msecsX10();
		double interval = t - frameTime->lastTime;
		interval /= 10;

		if (g_verbose) {
			fprintf(stdout,
				"Audio received (#%10lu) - Size: %u sfc: %lu channels: %u depth: %u bytes  (%7.2f ms)\n",
				audioFrameCount,
				sampleSize,
				audioFrame->GetSampleFrameCount(),
				g_audioChannels,
				g_audioSampleDepth / 8,
				interval);
		}

		if (audioOutputFile != -1) {
			audioFrame->GetBytes(&audioFrameBytes);
			write(audioOutputFile, audioFrameBytes, sampleSize);
		}

		frameTime->frameCount++;
		frameTime->lastTime = t;
	}
	return S_OK;
}

HRESULT DeckLinkCaptureDelegate::VideoInputFormatChanged(BMDVideoInputFormatChangedEvents events, IDeckLinkDisplayMode * mode, BMDDetectedVideoInputFormatFlags)
{
	g_detected_mode_id = mode->GetDisplayMode();
	return S_OK;
}

/* CALLBACKS for message notification */
static int cb_PAYLOAD_INFORMATION(void *callback_context, struct vanc_context_s *ctx, struct packet_payload_information_s *pkt)
{
	/* Have the library display some debug */
	if (!g_monitor_mode)
		dump_PAYLOAD_INFORMATION(ctx, pkt);

	return 0;
}

static int cb_EIA_708B(void *callback_context, struct vanc_context_s *ctx, struct packet_eia_708b_s *pkt)
{
	/* Have the library display some debug */
	if (!g_monitor_mode)
		dump_EIA_708B(ctx, pkt);

	return 0;
}

static int cb_EIA_608(void *callback_context, struct vanc_context_s *ctx, struct packet_eia_608_s *pkt)
{
	/* Have the library display some debug */
	if (!g_monitor_mode)
		dump_EIA_608(ctx, pkt);

	return 0;
}

static int cb_SCTE_104(void *callback_context, struct vanc_context_s *ctx, struct packet_scte_104_s *pkt)
{
	/* Have the library display some debug */
	if (!g_monitor_mode)
		dump_SCTE_104(ctx, pkt);

	return 0;
}

static int cb_all(void *callback_context, struct vanc_context_s *ctx, struct packet_header_s *pkt)
{
#if HAVE_CURSES_H
	vanc_monitor_stat_update(pkt);
#endif

	if (g_packetizeSMPTE2038) {
		if (smpte2038_packetizer_append(smpte2038_ctx, pkt) < 0) {
		}
	}

	return 0;
}

static struct vanc_callbacks_s callbacks =
{
	.payload_information    = cb_PAYLOAD_INFORMATION,
	.eia_708b               = cb_EIA_708B,
	.eia_608                = cb_EIA_608,
	.scte_104               = cb_SCTE_104,
	.all                    = cb_all,
};

/* END - CALLBACKS for message notification */

static int usage(const char *progname, int status)
{
	fprintf(stderr, COPYRIGHT "\n");
	fprintf(stderr, "Capture decklink SDI payload, capture vanc, analyze vanc.\n");
	fprintf(stderr, "Usage: %s -m <mode id> [OPTIONS]\n"
		"\n" "    -m <mode id>:\n", basename((char *)progname));

	int displayModeCount = 0;
	IDeckLinkDisplayMode *displayMode;
	while (displayModeIterator->Next(&displayMode) == S_OK) {

		char *displayModeString = NULL;
		HRESULT result = displayMode->GetName((const char **)&displayModeString);
		if (result == S_OK) {
			BMDTimeValue frameRateDuration, frameRateScale;
			displayMode->GetFrameRate(&frameRateDuration, &frameRateScale);

			fprintf(stderr, "        %2d:  %-20s \t %li x %li \t %g FPS [0x%08x]\n",
				displayModeCount, displayModeString,
				displayMode->GetWidth(),
				displayMode->GetHeight(),
				(double)frameRateScale /
				(double)frameRateDuration,
				displayMode->GetDisplayMode());

			free(displayModeString);
			displayModeCount++;
		}

		displayMode->Release();
	}

	fprintf(stderr,
		"    -p <pixelformat>\n"
		"         0:   8 bit YUV (4:2:2) (default)\n"
		"         1:  10 bit YUV (4:2:2)\n"
		"         2:  10 bit RGB (4:4:4)\n"
		"    -t <format> Print timecode\n"
		"        rp188:  RP 188\n"
		"         vitc:  VITC\n"
		"       serial:  Serial Timecode\n"
		"    -f <filename>   raw video output filename\n"
		"    -a <filename>   raw audio output filanem\n"
		"    -V <filename>   raw vanc output filename\n"
		"    -I <filename>   Interpret and display input VANC filename (See -V)\n"
		"    -l <linenr>     During -I parse, process a specific line# (def: 0 all)\n"
		"    -c <channels>   Audio Channels (2, 8 or 16 - def: 2)\n"
		"    -s <depth>      Audio Sample Depth (16 or 32 - def: 16)\n"
		"    -n <frames>     Number of frames to capture (def: unlimited)\n"
		"    -v              Increase level of verbosity (def: 0)\n"
		"    -3              Capture Stereoscopic 3D (Requires 3D Hardware support)\n"
		"    -i <number>     Capture from input port (def: 0)\n"
		"    -P pid 0xNNNN   Packetsize all detected VANC into SMPTE2038 TS packets using pid.\n"
		"                    The packets are store in file %s\n"
		"    -M              During VANC capture, display a Curses onscreen UI.\n"
		"\n"
		"Capture and display all VANC messages and show line/msg counts in an interactive UI (1080i 59.94):\n"
		"    %s -m9 -p1 -M\n\n"
		"Capture video and/or audio to a file. Raw video and/or audio can be viewed with mplayer eg:\n"
		"\n"
		"    %s -m2 -n 50 -f video.raw -a audio.raw\n"
		"    mplayer video.raw -demuxer rawvideo -rawvideo pal:uyvy -audiofile audio.raw -audio-demuxer 20 -rawaudio rate=48000\n\n"
		"Capture then interpret 10bit VANC (or 8bit VANC wth -p0), from 1280x720p60\n"
		"    %s -m13 -p1 -V vanc.raw\n"
		"    %s          -I vanc.raw\n\n",
		TS_OUTPUT_NAME,
		basename((char *)progname),
		basename((char *)progname),
		basename((char *)progname),
		basename((char *)progname)
		);

	exit(status);
}

static int _main(int argc, char *argv[])
{
	IDeckLinkIterator *deckLinkIterator = CreateDeckLinkIteratorInstance();
	DeckLinkCaptureDelegate *delegate;
	IDeckLinkDisplayMode *displayMode;
	BMDVideoInputFlags inputFlags = bmdVideoInputEnableFormatDetection;
	BMDDisplayMode selectedDisplayMode = bmdModeNTSC;
	BMDPixelFormat pixelFormat = bmdFormat8BitYUV;
	int displayModeCount = 0;
	int exitStatus = 1;
	int ch;
	int portnr = 0;
	bool foundDisplayMode = false;
	bool wantHelp = false;
	HRESULT result;

	pthread_mutex_init(&sleepMutex, NULL);
	pthread_cond_init(&sleepCond, NULL);

	while ((ch = getopt(argc, argv, "?h3c:s:f:a:m:n:p:t:vV:I:i:l:PM")) != -1) {
		switch (ch) {
		case 'm':
			g_videoModeIndex = atoi(optarg);
			break;
		case 'c':
			g_audioChannels = atoi(optarg);
			if (g_audioChannels != 2 && g_audioChannels != 8 && g_audioChannels != 16) {
				fprintf(stderr, "Invalid argument: Audio Channels must be either 2, 8 or 16\n");
				goto bail;
			}
			break;
		case 's':
			g_audioSampleDepth = atoi(optarg);
			if (g_audioSampleDepth != 16 && g_audioSampleDepth != 32) {
				fprintf(stderr, "Invalid argument: Audio Sample Depth must be either 16 bits or 32 bits\n");
				goto bail;
			}
			break;
		case 'f':
			g_videoOutputFilename = optarg;
			break;
		case 'a':
			g_audioOutputFilename = optarg;
			break;
		case 'I':
			g_vancInputFilename = optarg;
			break;
		case 'i':
			portnr = atoi(optarg);
			break;
		case 'l':
			g_linenr = atoi(optarg);
			break;
		case 'V':
			g_vancOutputFilename = optarg;
			break;
		case 'n':
			g_maxFrames = atoi(optarg);
			break;
		case 'M':
			g_monitor_mode = 1;
			break;
		case 'v':
			g_verbose++;
			break;
		case '3':
			inputFlags |= bmdVideoInputDualStream3D;
			break;
		case 'p':
			switch (atoi(optarg)) {
			case 0:
				pixelFormat = bmdFormat8BitYUV;
				break;
			case 1:
				pixelFormat = bmdFormat10BitYUV;
				break;
			case 2:
				pixelFormat = bmdFormat10BitRGB;
				break;
			default:
				fprintf(stderr, "Invalid argument: Pixel format %d is not valid", atoi(optarg));
				goto bail;
			}
			break;
		case 't':
			if (!strcmp(optarg, "rp188"))
				g_timecodeFormat = bmdTimecodeRP188Any;
			else if (!strcmp(optarg, "vitc"))
				g_timecodeFormat = bmdTimecodeVITC;
			else if (!strcmp(optarg, "serial"))
				g_timecodeFormat = bmdTimecodeSerial;
			else {
				fprintf(stderr, "Invalid argument: Timecode format \"%s\" is invalid\n", optarg);
				goto bail;
			}
			break;
		case 'P':
			g_packetizeSMPTE2038 = 1;
			if ((sscanf(optarg, "0x%x", &g_packetizePID) != 1) || (g_packetizePID > 0x1fff)) {
				wantHelp = true;
			} else {
				/* Success */
			}
			break;
		case '?':
		case 'h':
			wantHelp = true;
		}
	}

 	if (g_packetizeSMPTE2038) {
		unlink(TS_OUTPUT_NAME);
		if (smpte2038_packetizer_alloc(&smpte2038_ctx) < 0) {
			fprintf(stderr, "Unable to allocate a SMPTE2038 context.\n");
			goto bail;
		}
	}

	if (!deckLinkIterator) {
		fprintf(stderr, "This application requires the DeckLink drivers installed.\n");
		goto bail;
	}

	for (int i = 0; i <= portnr; i++) {
		/* Connect to the nth DeckLink instance */
		result = deckLinkIterator->Next(&deckLink);
		if (result != S_OK) {
			fprintf(stderr, "No capture devices found.\n");
			goto bail;
		}
	}

	if (deckLink->QueryInterface(IID_IDeckLinkInput, (void **)&deckLinkInput) != S_OK) {
		fprintf(stderr, "No input capture devices found.\n");
		goto bail;
	}

	delegate = new DeckLinkCaptureDelegate();
	deckLinkInput->SetCallback(delegate);

	/* Obtain an IDeckLinkDisplayModeIterator to enumerate the display modes supported on output */
	result = deckLinkInput->GetDisplayModeIterator(&displayModeIterator);
	if (result != S_OK) {
		fprintf(stderr, "Could not obtain the video output display mode iterator - result = %08x\n", result);
		goto bail;
	}

	if (wantHelp) {
		usage(argv[0], 0);
	}

        if (vanc_context_create(&vanchdl) < 0) {
                fprintf(stderr, "Error initializing library context\n");
                exit(1);
        }

#if HAVE_CURSES_H
	vanc_monitor_stat_alloc();
#endif
       	vanchdl->verbose = g_verbose;
        vanchdl->callbacks = &callbacks;

	if (g_vancInputFilename != NULL) {
		return AnalyzeVANC(g_vancInputFilename);
	}

	if (g_videoModeIndex < 0) {
		fprintf(stderr, "No video mode specified\n");
		usage(argv[0], 0);
	}

	if (g_videoOutputFilename != NULL) {
		videoOutputFile = open(g_videoOutputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if (videoOutputFile < 0) {
			fprintf(stderr, "Could not open video output file \"%s\"\n", g_videoOutputFilename);
			goto bail;
		}
	}
	if (g_audioOutputFilename != NULL) {
		audioOutputFile = open(g_audioOutputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if (audioOutputFile < 0) {
			fprintf(stderr, "Could not open audio output file \"%s\"\n", g_audioOutputFilename);
			goto bail;
		}
	}

	if (g_vancOutputFilename != NULL) {
		vancOutputFile = open(g_vancOutputFilename, O_WRONLY | O_CREAT | O_TRUNC, 0664);
		if (vancOutputFile < 0) {
			fprintf(stderr, "Could not open vanc output file \"%s\"\n", g_vancOutputFilename);
			goto bail;
		}
	}

	while (displayModeIterator->Next(&displayMode) == S_OK) {
		if (g_videoModeIndex == displayModeCount) {

			foundDisplayMode = true;

			const char *displayModeName;
			displayMode->GetName(&displayModeName);
			selectedDisplayMode = displayMode->GetDisplayMode();
			g_detected_mode_id = displayMode->GetDisplayMode();
			g_requested_mode_id = displayMode->GetDisplayMode();

			BMDDisplayModeSupport result;
			deckLinkInput->DoesSupportVideoMode(selectedDisplayMode, pixelFormat, bmdVideoInputFlagDefault, &result, NULL);
			if (result == bmdDisplayModeNotSupported) {
				fprintf(stderr, "The display mode %s is not supported with the selected pixel format\n", displayModeName);
				goto bail;
			}

			if (inputFlags & bmdVideoInputDualStream3D) {
				if (!(displayMode->GetFlags() & bmdDisplayModeSupports3D)) {
					fprintf(stderr, "The display mode %s is not supported with 3D\n", displayModeName);
					goto bail;
				}
			}

			break;
		}
		displayModeCount++;
		displayMode->Release();
	}

	if (!foundDisplayMode) {
		fprintf(stderr, "Invalid mode %d specified\n", g_videoModeIndex);
		goto bail;
	}

	result = deckLinkInput->EnableVideoInput(selectedDisplayMode, pixelFormat, inputFlags);
	if (result != S_OK) {
		fprintf(stderr, "Failed to enable video input. Is another application using the card?\n");
		goto bail;
	}

	result = deckLinkInput->EnableAudioInput(bmdAudioSampleRate48kHz, g_audioSampleDepth, g_audioChannels);
	if (result != S_OK) {
		fprintf(stderr, "Failed to enable audio input. Is another application using the card?\n");
		goto bail;
	}

	result = deckLinkInput->StartStreams();
	if (result != S_OK) {
		fprintf(stderr, "Failed to start stream. Is another application using the card?\n");
		goto bail;
	}

	signal(SIGINT, signal_handler);

#if HAVE_CURSES_H
	if (g_monitor_mode) {
		initscr();
		pthread_create(&threadId, 0, thread_func_draw, NULL);
		pthread_create(&threadId, 0, thread_func_input, NULL);
	}
#endif

	/* All Okay. */
	exitStatus = 0;

	/* Block main thread until signal occurs */
	pthread_mutex_lock(&sleepMutex);
	pthread_cond_wait(&sleepCond, &sleepMutex);
	pthread_mutex_unlock(&sleepMutex);

	while (g_shutdown != 2)
		usleep(50 * 1000);

	fprintf(stdout, "Stopping Capture\n");
	result = deckLinkInput->StopStreams();
	if (result != S_OK) {
		fprintf(stderr, "Failed to start stream. Is another application using the card?\n");
	}

#if HAVE_CURSES_H
	vanc_monitor_stats_dump();
#endif
        vanc_context_destroy(vanchdl);
	smpte2038_packetizer_free(&smpte2038_ctx);

#if HAVE_CURSES_H
	vanc_monitor_stat_free();
	if (g_monitor_mode)
		endwin();
#endif

bail:

	if (videoOutputFile)
		close(videoOutputFile);
	if (audioOutputFile)
		close(audioOutputFile);
	if (vancOutputFile)
		close(vancOutputFile);

	RELEASE_IF_NOT_NULL(displayModeIterator);
	RELEASE_IF_NOT_NULL(deckLinkInput);
	RELEASE_IF_NOT_NULL(deckLink);
	RELEASE_IF_NOT_NULL(deckLinkIterator);

	return exitStatus;
}

extern "C" int capture_main(int argc, char *argv[])
{
	return _main(argc, argv);
}

