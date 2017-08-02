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

/* A tool to inspect audio buffers from blackmagic hardware, analyze channels start + span,
 * detect SMPTE337 preable headers and inform the caller. We're a helper framework than
 * can be used to extract AC3 bitstream audio from SDI, or other bitstream codecs for that
 * matter.
 */

#ifndef _SMPTE337_DETECTOR_H 
#define _SMPTE337_DETECTOR_H 

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <libklvanc/klringbuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct smpte337_detector_s;

typedef void (*smpte337_detector_callback)(void *user_context,
	struct smpte337_detector_s *ctx, 
	uint8_t datamode, uint8_t datatype, uint32_t payload_bitCount,
	uint8_t *payload);

struct smpte337_detector_s
{
	KLRingBuffer *rb;

	smpte337_detector_callback cb;
	void *cbContext;

	/*  0. The framework should attempt to determine the wordlength,
	 *     looking for the specific syncword1/2 patterns before
	 *     committing any data to the ring buffers.
	 * 16. Detected operating on 16bit words.
	 * 20. Detected operating on 20bit words.
	 * 24. Detected operating on 24bit words.
	 */
	uint32_t wordLength;
};

struct smpte337_detector_s *smpte337_detector_alloc(smpte337_detector_callback cb, void *cbContext);

void smpte337_detector_free(struct smpte337_detector_s *ctx);

size_t smpte337_detector_write(struct smpte337_detector_s *ctx, uint8_t *buf,
	uint32_t audioFrames, uint32_t sampleDepth, uint32_t channelsPerFrame, uint32_t frameStrideBytes,
	uint32_t spanCount);

#ifdef __cplusplus
};
#endif

#endif /* _SMPTE337_DETECTOR_H */
