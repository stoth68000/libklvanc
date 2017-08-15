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
 * @file	frame-writer.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	Helper functions to read/write raw video/pcm/vanc from/to disk.
 */

/* Design
 * Make, serialize and de-serialize four kinds of c structs to from disk.
 * Deal with copying frames in into structs, deal with storing timing data,
 * flush all of these frames to disk in a background thread, so that
 * we don't disturb the decklink FrameArrival timing callback.
 */

#ifndef _FRAME_WRITER_H
#define _FRAME_WRITER_H

#include <stdio.h>
#include <stdint.h>
#include <pthread.h>
#include "xorg-list.h"

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

	int writeMode;

	pthread_mutex_t listMutex;
	struct xorg_list list;
	pthread_cond_t cond;
	pthread_condattr_t condAttr;

	pthread_t writerThreadId;
	int thread_running;
	int thread_terminate;
	int thread_complete;
};

#define FWR_FRAME_TIMING 1
#define FWR_FRAME_VIDEO  2
#define FWR_FRAME_AUDIO  3
#define FWR_FRAME_VANC   4

struct fwr_writer_node_s
{
	struct xorg_list list;

	/* FWR_FRAME_... */
	int type;
	void *ptr;
};

/**
 * @brief       Create a session context and prepare to read or write audio, video, vanc from/to disk.
 *              The resource should be ultimately free'd using fwr_session_file_close().
 * @param[in]   const char *filename - File to be read or written. Files are automatically overwritten.
 * @param[in]   int writeMode - 0 if reading, 1 if writing.
 * @param[out]  struct fwr_session_s **session - newly created session object.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_session_file_open(const char *filename, int writeMode, struct fwr_session_s **session);

/**
 * @brief       Free a previously created session and close any file resourses.
 *              Callers may no longer use the session object.
 * @param[in]   struct fwr_session_s *session - session object.
 */
void fwr_session_file_close(struct fwr_session_s *session);

/**
 * @brief       Peek ahead in the READ session, look for the next header code, allowing parsers
 *              to call the most approrpiate fwr_n_read() method.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[out]  uint32_t *header - A header code, Eg. audio_v1_header or video_v1_header
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_session_frame_peek(struct fwr_session_s *session, uint32_t *header);

/**
 * @brief       From the READ session, allocate and populate an audio structure from the current file pointer.
 *              The caller must release the frame with a call to fwr_pcm_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[out]  struct fwr_header_audio_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_pcm_frame_read(struct fwr_session_s *session, struct fwr_header_audio_s **frame);

/**
 * @brief       Destroy and release any resources related to the frame object.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_audio_s *frame - Object to be released.
 */
void fwr_pcm_frame_free(struct fwr_session_s *session, struct fwr_header_audio_s *frame);

/**
 * @brief       Allocate memory and populate the audio frame with the user supplied parameters.
 *              The caller must release the frame with a call to fwr_pcm_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   uint32_t frameCount - Number of samples (per channel) present in this buffer.
 * @param[in]   uint32_t sampleDepth - Typically 16 or 32.
 * @param[in]   uint32_t channelCount - 2 to 16.
 * @param[in]   uint8_t *buffer - A buffer of data expected to be channels * frameCount * (depth / 8) long.
 * @param[out]  struct fwr_header_audio_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_pcm_frame_create(struct fwr_session_s *session,
	uint32_t frameCount, uint32_t sampleDepth, uint32_t channelCount,
	const uint8_t *buffer,
	struct fwr_header_audio_s **frame);

/*
 * @brief       For a WRITE session, flush the contents of the audio frame to disk.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_audio_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_pcm_frame_write(struct fwr_session_s *session, struct fwr_header_audio_s *frame);

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

/**
 * @brief       Allocate memory and populate the timing frame with the user supplied parameters.
 *              The caller must release the frame with a call to fwr_timing_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   uint32_t decklinkCaptureMode - A four digit code that represents width/height/framerate/scan-line.
 * @param[out]  struct fwr_header_timing_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_timing_frame_create(struct fwr_session_s *session,
	uint32_t decklinkCaptureMode,
	struct fwr_header_timing_s **frame);

/*
 * @brief       For a WRITE session, flush the contents of the timing frame to disk.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_timing_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_timing_frame_write(struct fwr_session_s *session, struct fwr_header_timing_s *frame);

/**
 * @brief       From the READ session, allocate and populate a timing structure from the current file pointer.
 *              The caller must release the frame with a call to fwr_timing_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[out]  struct fwr_header_timing_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_timing_frame_read(struct fwr_session_s *session, struct fwr_header_timing_s *frame);

/**
 * @brief       Destroy and release any resources related to the frame object.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_timing_s *frame - Object to be released.
 */
void fwr_timing_frame_free(struct fwr_session_s *session, struct fwr_header_timing_s *frame);

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

/**
 * @brief       From the READ session, allocate and populate a video structure from the current file pointer.
 *              The caller must release the frame with a call to fwr_video_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[out]  struct fwr_header_video_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_video_frame_read(struct fwr_session_s *session, struct fwr_header_video_s **frame);

/**
 * @brief       Destroy and release any resources related to the frame object.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_video_s *frame - Object to be released.
 */
void fwr_video_frame_free(struct fwr_session_s *session, struct fwr_header_video_s *frame);

/**
 * @brief       Allocate memory and populate the video frame with the user supplied parameters.
 *              The caller must release the frame with a call to fwr_video_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   uint32_t width - Eg. 1280
 * @param[in]   uint32_t height - Eg. 720
 * @param[in]   uint32_t stride - Per line, mesaure in bytes.
 * @param[in]   uint8_t *buffer - a buffer of height * stride bytes.
 * @param[out]  struct fwr_header_video_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_video_frame_create(struct fwr_session_s *session,
	uint32_t width, uint32_t height, uint32_t stride,
	const uint8_t *buffer,
	struct fwr_header_video_s **frame);

/*
 * @brief       For a WRITE session, flush the contents of the video frame to disk.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_video_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_video_frame_write(struct fwr_session_s *session, struct fwr_header_video_s *frame);

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

/**
 * @brief       From the READ session, allocate and populate a vanc structure from the current file pointer.
 *              The caller must release the frame with a call to fwr_vanc_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[out]  struct fwr_header_vanc_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_vanc_frame_read(struct fwr_session_s *session, struct fwr_header_vanc_s **frame);

/**
 * @brief       Destroy and release any resources related to the frame object.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_vanc_s *frame - Object to be released.
 */
void fwr_vanc_frame_free(struct fwr_session_s *session, struct fwr_header_vanc_s *frame);

/**
 * @brief       Allocate memory and populate the vanc frame with the user supplied parameters.
 *              The caller must release the frame with a call to fwr_vanc_frame_free().
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   uint32_t line - Video line the vanc came from, Eg. 11
 * @param[in]   uint32_t width - Eg. 1280
 * @param[in]   uint32_t height - Eg. 720
 * @param[in]   uint32_t stride - Line length expressed in bytes.
 * @param[in]   uint8_t *buffer - a buffer of stride bytes.
 * @param[out]  struct fwr_header_vanc_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int  fwr_vanc_frame_create(struct fwr_session_s *session,
	uint32_t line,
	uint32_t width, uint32_t height, uint32_t stride,
	const uint8_t *buffer,
	struct fwr_header_vanc_s **frame);

/*
 * @brief       For a WRITE session, flush the contents of the vanc frame to disk.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   struct fwr_header_vanc_s **frame - Freshly allocated frame of data.
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_vanc_frame_write(struct fwr_session_s *session, struct fwr_header_vanc_s *frame);

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

/**
 * @brief       Place a previously allocate from onto a deferred queue.
 *              A thread will dequeue the frame at some later point in time and serialize
 *              out the frame to disk using the fwr_write_n_() calls.
 *              The thread will automatically release via fwr_n_frame_free()
 *              when the allocation is no longer required, the caller MUST NOT
 *              release the frame manually.
 * @param[in]   struct fwr_session_s *session - session object.
 * @param[in]   void *ptr - Pointer reference to various frame types.
 * @param[in]   int type - The struct type we're passing in the pointer, Eg. FWR_FRAME_VIDEO
 * @return        0 - Success
 * @return      < 0 - Error
 */
int fwr_writer_enqueue(struct fwr_session_s *session, void *ptr, int type);

#ifdef __cplusplus
};
#endif  

#endif /* _FRAME_WRITER_H */
