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
};

int  klvanc_pcm_file_open(const char *filename, int writeMode, struct fwr_session_s **session);
int  klvanc_pcm_frame_read(struct fwr_session_s *session, struct fwr_header_audio_s **frame);
void klvanc_pcm_frame_free(struct fwr_session_s *session, struct fwr_header_audio_s *frame);
void klvanc_pcm_file_close(struct fwr_session_s *session);

int  klvanc_pcm_frame_create(struct fwr_session_s *session,
	uint32_t frameCount, uint32_t sampleDepth, uint32_t channelCount,
	const uint8_t *buffer,
	struct fwr_header_audio_s **frame);

int klvanc_pcm_frame_write(struct fwr_session_s *session, struct fwr_header_audio_s *frame);

#ifdef __cplusplus
};
#endif  

#endif /* _FRAME_WRITER_H */
