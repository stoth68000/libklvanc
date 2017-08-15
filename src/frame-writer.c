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

#define LOCAL_DEBUG 0

extern int usleep(uint32_t usecs);

static void *fwr_writer_threadfunc(void *p)
{
	struct fwr_session_s *s = (struct fwr_session_s *)p;

	s->thread_terminate = 0;
	s->thread_running = 1;
	while (!s->thread_terminate) {

		usleep(100 * 1000);

		while (1 && !s->thread_terminate) {
			/* Wait on a timeout condition. */
			/* Write frames to disk. */
			/* Release frames. */
			void *frame;
			int type;
			if (fwr_writer_dequeue(s, &frame, &type) < 0)
				break;

			/* Do something with the data then clean up. */

			switch (type) {
			case FWR_FRAME_TIMING: /* timing */
				klvanc_timing_frame_write(s, (struct fwr_header_timing_s *)frame);
				klvanc_timing_frame_free(s, (struct fwr_header_timing_s *)frame);
				break;
			case FWR_FRAME_VIDEO:
				klvanc_video_frame_write(s, (struct fwr_header_video_s *)frame);
				klvanc_video_frame_free(s, (struct fwr_header_video_s *)frame);
				break;
			case FWR_FRAME_AUDIO:
				klvanc_pcm_frame_write(s, (struct fwr_header_audio_s *)frame);
				klvanc_pcm_frame_free(s, (struct fwr_header_audio_s *)frame);
				break;
			case FWR_FRAME_VANC:
				klvanc_vanc_frame_write(s, (struct fwr_header_vanc_s *)frame);
				klvanc_vanc_frame_free(s, (struct fwr_header_vanc_s *)frame);
				break;
			}

		}
	}
	s->thread_complete = 1;
	s->thread_running = 0;
	pthread_exit(0);
}

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

	s->writeMode = writeMode;
	if (writeMode) {
		xorg_list_init(&s->list);
		pthread_mutex_init(&s->listMutex, NULL);
		pthread_condattr_init(&s->condAttr);
		pthread_cond_init(&s->cond, &s->condAttr);

		pthread_create(&s->writerThreadId, 0, fwr_writer_threadfunc, s);
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

	if (fread(&f->footer, 1, fwr_header_audio_size_post, session->fh) != fwr_header_audio_size_post) {
		free(f->ptr);
		free(f);
		return -1;
	}

	if (f->footer != audio_v1_footer) {
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
	if (session->writeMode && session->thread_running) {
		session->thread_terminate = 1;
		while (!session->thread_complete)
			usleep(50 * 1000);
	}

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
int klvanc_timing_frame_create(struct fwr_session_s *session,
        uint32_t decklinkCaptureMode,
        struct fwr_header_timing_s **frame)
{
	struct fwr_header_timing_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

	f->sof = timing_v1_header;
	f->counter = session->counter++;
	gettimeofday(&f->ts1, NULL);
	f->decklinkCaptureMode = decklinkCaptureMode;
	f->eof = timing_v1_footer;

	*frame = f;
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

	if (frame->sof != timing_v1_header)
		return -1;

	if (frame->eof != timing_v1_footer)
		return -1;

	return 0;
}

void klvanc_timing_frame_free(struct fwr_session_s *session, struct fwr_header_timing_s *frame)
{
	free(frame);
}

/* -- */
int klvanc_video_frame_read(struct fwr_session_s *session, struct fwr_header_video_s **frame)
{
	struct fwr_header_video_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of header\n", __func__, fwr_header_video_size_pre);
#endif
	if (fread(f, 1, fwr_header_video_size_pre, session->fh) != fwr_header_video_size_pre) {
		free(f);
		return -1;
	}

	if (f->sof != video_v1_header) {
#if LOCAL_DEBUG
		printf("%s() f->sof = 0x%x\n", __func__, f->sof);
#endif
		return -1;
	}

	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of data\n", __func__, f->bufferLengthBytes);
#endif
	if (fread(f->ptr, 1, f->bufferLengthBytes, session->fh) != f->bufferLengthBytes) {
		free(f->ptr);
		free(f);
		return -1;
	}

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of footer\n", __func__, fwr_header_video_size_post);
#endif
	if (fread(&f->eof, 1, fwr_header_video_size_post, session->fh) != fwr_header_video_size_post) {
		free(f->ptr);
		free(f);
		return -1;
	}

	if (f->eof != video_v1_footer) {
#if LOCAL_DEBUG
		printf("%s() f->eof = 0x%x\n", __func__, f->eof);
#endif
		return -1;
	}

	*frame = f;
	return 0;
}

void klvanc_video_frame_free(struct fwr_session_s *session, struct fwr_header_video_s *frame)
{
	free(frame->ptr);
	free(frame);
}

void klvanc_video_file_close(struct fwr_session_s *session)
{
	if (session->fh) {
		fclose(session->fh);
		session->fh = NULL;
	}
	free(session);
}

int  klvanc_video_frame_create(struct fwr_session_s *session,
        uint32_t width, uint32_t height, uint32_t strideBytes,
        const uint8_t *buffer,
        struct fwr_header_video_s **frame)
{
	if (!buffer)
		return -1;

	struct fwr_header_video_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

        f->sof = video_v1_header;
	f->width = width;
	f->height = height;
	f->strideBytes = strideBytes;
	f->bufferLengthBytes = height * strideBytes;
	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

	memcpy(f->ptr, buffer, f->bufferLengthBytes);
        f->eof = video_v1_footer;

	*frame = f;
	return 0;
}

int klvanc_video_frame_write(struct fwr_session_s *session, struct fwr_header_video_s *frame)
{
	if (fwrite(frame, 1, fwr_header_video_size_pre, session->fh) != fwr_header_video_size_pre) {
		return -1;
	}

	if (fwrite(frame->ptr, 1, frame->bufferLengthBytes, session->fh) != frame->bufferLengthBytes) {
		return -1;
	}

	if (fwrite(&frame->eof, 1, fwr_header_video_size_post, session->fh) != fwr_header_video_size_post) {
		return -1;
	}

	return 0;
}

/* -- */
int klvanc_vanc_frame_read(struct fwr_session_s *session, struct fwr_header_vanc_s **frame)
{
	struct fwr_header_vanc_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of header\n", __func__, fwr_header_vanc_size_pre);
#endif
	if (fread(f, 1, fwr_header_vanc_size_pre, session->fh) != fwr_header_vanc_size_pre) {
		free(f);
		return -1;
	}

	if (f->sol != VANC_SOL_INDICATOR) {
#if LOCAL_DEBUG
		printf("%s() f->sol = 0x%x\n", __func__, f->sol);
#endif
		return -1;
	}

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of data\n", __func__, f->bufferLengthBytes);
#endif
	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

#if LOCAL_DEBUG
	printf("%s() read %d bytes of data\n", __func__, f->bufferLengthBytes);
#endif
	if (fread(f->ptr, 1, f->bufferLengthBytes, session->fh) != f->bufferLengthBytes) {
		free(f->ptr);
		free(f);
		return -1;
	}

#if LOCAL_DEBUG
	printf("%s() reading %d bytes of footer\n", __func__, fwr_header_vanc_size_post);
#endif
	if (fread(&f->eol, 1, fwr_header_vanc_size_post, session->fh) != fwr_header_vanc_size_post) {
		free(f->ptr);
		free(f);
		return -1;
	}

	if (f->eol != VANC_EOL_INDICATOR) {
#if LOCAL_DEBUG
		printf("%s() f->eol = 0x%x\n", __func__, f->eol);
#endif
		return -1;
	}

	*frame = f;
	return 0;
}

void klvanc_vanc_frame_free(struct fwr_session_s *session, struct fwr_header_vanc_s *frame)
{
	free(frame->ptr);
	free(frame);
}

int  klvanc_vanc_frame_create(struct fwr_session_s *session,
	uint32_t line,
        uint32_t width, uint32_t height, uint32_t strideBytes,
        const uint8_t *buffer,
        struct fwr_header_vanc_s **frame)
{
	if (!buffer)
		return -1;

	struct fwr_header_vanc_s *f = malloc(sizeof(*f));
	if (!f)
		return -1;

        f->sol = VANC_SOL_INDICATOR;
	f->line = line;
	f->width = width;
	f->height = height;
	f->strideBytes = strideBytes;
	f->bufferLengthBytes = strideBytes;
	f->ptr = malloc(f->bufferLengthBytes);
	if (!f->ptr) {
		free(f);
		return -1;
	}

	memcpy(f->ptr, buffer, f->bufferLengthBytes);
        f->eol = VANC_EOL_INDICATOR;

	*frame = f;
	return 0;
}

int klvanc_vanc_frame_write(struct fwr_session_s *session, struct fwr_header_vanc_s *frame)
{
	if (fwrite(frame, 1, fwr_header_vanc_size_pre, session->fh) != fwr_header_vanc_size_pre) {
		return -1;
	}

	if (fwrite(frame->ptr, 1, frame->bufferLengthBytes, session->fh) != frame->bufferLengthBytes) {
		return -1;
	}

	if (fwrite(&frame->eol, 1, fwr_header_vanc_size_post, session->fh) != fwr_header_vanc_size_post) {
		return -1;
	}

	return 0;
}

int klvanc_pcm_frame_peek(struct fwr_session_s *session, uint32_t *header)
{
	size_t r = fread(header, 1, sizeof(*header), session->fh);
	fseek(session->fh, -r, SEEK_CUR);
	if (r != sizeof(*header))
		return -1;

	return 0;
}

int fwr_writer_enqueue(struct fwr_session_s *session, void *ptr, int type)
{
	struct fwr_writer_node_s *n = malloc(sizeof(*n));
	if (!n)
		return -1;

	n->ptr = ptr;
	n->type = type;

	pthread_mutex_lock(&session->listMutex);
	xorg_list_append(&n->list, &session->list);
	pthread_mutex_unlock(&session->listMutex);

	pthread_cond_broadcast(&session->cond);
	
	return 0;
}

int fwr_writer_dequeue(struct fwr_session_s *session, void **ptr, int *type)
{
	int ret = -1;

	struct fwr_writer_node_s *n = NULL;
	pthread_mutex_lock(&session->listMutex);
	if (!xorg_list_is_empty(&session->list)) {
		n = xorg_list_first_entry(&session->list, struct fwr_writer_node_s, list);

		*ptr = n->ptr;
		*type = n->type;
		xorg_list_del(&n->list);
		ret = 0;

	}
	pthread_mutex_unlock(&session->listMutex);

	return ret;
}

