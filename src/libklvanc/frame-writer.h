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

/**
 * @file	playout.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _FRAME_WRITER_H
#define _FRAME_WRITER_H

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif  

#define audio_v1_header 0x0100EDFE
#define audio_v1_footer 0x0100ADDE

struct fwr_header_audio_s
{
	uint32_t header;            /* See audio_v1_header */
	struct timeval ts;
	uint32_t channelCount;      /* 2-16 */
	uint32_t frameCount;        /* Number of samples per channel, in this buffer. */
	uint32_t sampleDepth;       /* 16 or 32 */
	uint32_t bufferLengthBytes; /* Size of the complete audio buffer, for all channels and framcounts. */
	uint8_t  *ptr;              /* Audio data */
	uint32_t footer;            /* See audio_v1_footer */
} __attribute__((packed));
#define fwr_header_audio_size_pre  (sizeof(struct fwr_header_audio_s) - sizeof(uint32_t) - sizeof(uint8_t *))
#define fwr_header_audio_size_post (sizeof(uint32_t))

struct fwr_session_s
{
	FILE *fh;
	int type; /* 1 = PCM_AUDIO. */

	uint64_t counter;
};

int klvanc_pcm_frame_peek(struct fwr_session_s *session, uint32_t *header);

int  klvanc_pcm_file_open(const char *filename, int writeMode, struct fwr_session_s **session);
int  klvanc_pcm_frame_read(struct fwr_session_s *session, struct fwr_header_audio_s **frame);
void klvanc_pcm_frame_free(struct fwr_session_s *session, struct fwr_header_audio_s *frame);
void klvanc_pcm_file_close(struct fwr_session_s *session);

int  klvanc_pcm_frame_create(struct fwr_session_s *session,
	uint32_t frameCount, uint32_t sampleDepth, uint32_t channelCount,
	const uint8_t *buffer,
	struct fwr_header_audio_s **frame);

int klvanc_pcm_frame_write(struct fwr_session_s *session, struct fwr_header_audio_s *frame);

/* -- */

#define timing_v1_header 0xC0DEADDE
#define timing_v1_footer 0xC0DEADDF
struct fwr_header_timing_s
{
	uint32_t       sof;
	uint64_t       counter;
	struct timeval ts1;
	uint32_t       decklinkCaptureMode;
	uint32_t       eof;
} __attribute__((packed));

int  klvanc_timing_frame_set(struct fwr_session_s *session,
	uint32_t decklinkCaptureMode,
	struct fwr_header_timing_s *frame);

int klvanc_timing_frame_write(struct fwr_session_s *session, struct fwr_header_timing_s *frame);
int klvanc_timing_frame_read(struct fwr_session_s *session, struct fwr_header_timing_s *frame);


#define video_v1_header 0xDFBEADDE
#define video_v1_footer 0xDFFEADDE
struct fwr_header_video_s
{
	uint32_t sof;
	uint32_t width;
	uint32_t height;
	uint32_t strideBytes;
	uint32_t bufferLengthBytes;
	uint8_t  *ptr;              /* Video and VANC data */
	uint32_t eof;

} __attribute__((packed));
#define fwr_header_video_size_pre  (sizeof(struct fwr_header_video_s) - sizeof(uint32_t) - sizeof(uint8_t *))
#define fwr_header_video_size_post (sizeof(uint32_t))

int  klvanc_video_file_open(const char *filename, int writeMode, struct fwr_session_s **session);
int  klvanc_video_frame_read(struct fwr_session_s *session, struct fwr_header_video_s **frame);
void klvanc_video_frame_free(struct fwr_session_s *session, struct fwr_header_video_s *frame);
void klvanc_video_file_close(struct fwr_session_s *session);

int  klvanc_video_frame_create(struct fwr_session_s *session,
	uint32_t width, uint32_t height, uint32_t stride,
	const uint8_t *buffer,
	struct fwr_header_video_s **frame);

int klvanc_video_frame_write(struct fwr_session_s *session, struct fwr_header_video_s *frame);

/* -- */
#define VANC_SOL_INDICATOR 0xEFBEADDE
#define VANC_EOL_INDICATOR 0xEDFEADDE
struct fwr_header_vanc_s
{
	uint32_t sol;
	uint32_t line;
	uint32_t width;
	uint32_t height;
	uint32_t strideBytes;
	uint32_t bufferLengthBytes;
	uint8_t  *ptr;              /* Video and VANC data */
	uint32_t eol;
} __attribute__((packed));
#define fwr_header_vanc_size_pre  (sizeof(struct fwr_header_vanc_s) - sizeof(uint32_t) - sizeof(uint8_t *))
#define fwr_header_vanc_size_post (sizeof(uint32_t))

int  klvanc_vanc_frame_read(struct fwr_session_s *session, struct fwr_header_vanc_s **frame);
void klvanc_vanc_frame_free(struct fwr_session_s *session, struct fwr_header_vanc_s *frame);

int  klvanc_vanc_frame_create(struct fwr_session_s *session,
	uint32_t line,
	uint32_t width, uint32_t height, uint32_t stride,
	const uint8_t *buffer,
	struct fwr_header_vanc_s **frame);

int klvanc_vanc_frame_write(struct fwr_session_s *session, struct fwr_header_vanc_s *frame);

__inline__ int fwr_timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
     /* Perform the carry for the later subtraction by updating y. */
     if (x->tv_usec < y->tv_usec)
     {
         int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
         y->tv_usec -= 1000000 * nsec;
         y->tv_sec += nsec;
     }
     if (x->tv_usec - y->tv_usec > 1000000)
     {
         int nsec = (x->tv_usec - y->tv_usec) / 1000000;
         y->tv_usec += 1000000 * nsec;
         y->tv_sec -= nsec;
     }

     /* Compute the time remaining to wait. tv_usec is certainly positive. */
     result->tv_sec = x->tv_sec - y->tv_sec;
     result->tv_usec = x->tv_usec - y->tv_usec;

     /* Return 1 if result is negative. */
     return x->tv_sec < y->tv_sec;
}

#ifdef __cplusplus
};
#endif  

#endif /* _FRAME_WRITER_H */
