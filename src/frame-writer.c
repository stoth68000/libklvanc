/*
 * Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved
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
#include <libklvanc/frame-writer.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#if 0
struct fwr_header_audio_s
{
        uint32_t header;            /* See audio_v1_header */
        uint32_t channelCount;      /* 2-16 */
        uint32_t frameCount;        /* Number of samples per channel, in this buffer. */
        uint32_t sampleDepth;       /* 16 or 32 */
        uint32_t bufferLengthBytes; /* Size of the complete audio buffer, for all channels and framcounts. */
        /* .... data .... */        /* Audio data */
	uint8_t *ptr;
        uint32_t footer;            /* See audio_v1_footer */
} __attribute__((packed));

struct fw_session_s
{
        FILE *fh;
        int type; /* 1 = PCM_AUDIO. */
};
#endif

int klvanc_pcm_file_open(const char *filename, int writeMode, struct fwr_session_s **session)
{
	struct fwr_session_s *s = calloc(1, sizeof(*s));
	if (!s)
		return -1;

	if (writeMode)
		s->fh = fopen(filename, "wb");
	else
		s->fh = fopen(filename, "rb");

	if (!s->fh) {
		free(s);
		return -1;
	}

	*session = s;
	return 0;
}

int klvanc_pcm_frame_read(struct fwr_session_s *session, struct fwr_header_audio_s **frame)
{
	struct fwr_header_audio_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

	if (fread(f, 1, fwr_header_audio_size_pre, session->fh) != fwr_header_audio_size_pre) {
		free(f);
		return -1;
	}

	if (f->header != audio_v1_header) {
		return -1;
	}

	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

	if (fread(f->ptr, 1, f->bufferLengthBytes, session->fh) != f->bufferLengthBytes) {
		free(f->ptr);
		free(f);
		return -1;
	}

	if (fread(f + fwr_header_audio_size_post, 1, fwr_header_audio_size_post, session->fh) != fwr_header_audio_size_post) {
		free(f->ptr);
		free(f);
		return -1;
	}

	*frame = f;
	return 0;
}

void klvanc_pcm_frame_free(struct fwr_session_s *session, struct fwr_header_audio_s *frame)
{
	free(frame->ptr);
	free(frame);
}

void klvanc_pcm_file_close(struct fwr_session_s *session)
{
	if (session->fh) {
		fclose(session->fh);
		session->fh = NULL;
	}
	free(session);
}

int  klvanc_pcm_frame_create(struct fwr_session_s *session,
        uint32_t frameCount, uint32_t sampleDepth, uint32_t channelCount,
        const uint8_t *buffer,
        struct fwr_header_audio_s **frame)
{
	if (!buffer)
		return -1;

	struct fwr_header_audio_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

        f->header = audio_v1_header;
	f->channelCount = channelCount;
	f->frameCount = frameCount;
	f->sampleDepth = sampleDepth;
	f->bufferLengthBytes = f->frameCount * f->channelCount * (f->sampleDepth / 4);
	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

	memcpy(f->ptr, buffer, f->bufferLengthBytes);
        f->footer = audio_v1_footer;

	*frame = f;
	return 0;
}

int klvanc_pcm_frame_write(struct fwr_session_s *session, struct fwr_header_audio_s *frame)
{
	if (fwrite(frame, 1, fwr_header_audio_size_pre, session->fh) != fwr_header_audio_size_pre) {
		return -1;
	}
	if (fwrite(frame->ptr, 1, frame->bufferLengthBytes, session->fh) != frame->bufferLengthBytes) {
		return -1;
	}

	if (fwrite(&frame->footer, 1, fwr_header_audio_size_post, session->fh) != fwr_header_audio_size_post) {
		return -1;
	}

	return 0;
}

/* -- */
int klvanc_timing_frame_set(struct fwr_session_s *session,
        uint32_t decklinkCaptureMode,
        struct fwr_header_timing_s *frame)
{
	frame->sof = timing_v1_header;
	frame->counter = session->counter++;
	gettimeofday(&frame->ts1, NULL);
	frame->decklinkCaptureMode = decklinkCaptureMode;
	frame->eof = timing_v1_footer;
	return 0;
}

int klvanc_timing_frame_write(struct fwr_session_s *session, struct fwr_header_timing_s *frame)
{
	if (fwrite(frame, 1, sizeof(*frame), session->fh) != sizeof(*frame)) {
		return -1;
	}

	return 0;
}

int klvanc_timing_frame_read(struct fwr_session_s *session, struct fwr_header_timing_s *frame)
{
	if (fread(frame, 1, sizeof(*frame), session->fh) != sizeof(*frame)) {
		return -1;
	}

	return 0;
}

