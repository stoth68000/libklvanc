/*
 * Copyright (c) 2026 Kernel Labs Inc. All Rights Reserved.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#ifndef _VANC_SMPTE_12_3_H
#define _VANC_SMPTE_12_3_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Decoded SMPTE ST 12-3 high-frame-rate ancillary time code. */
struct klvanc_packet_smpte_12_3_s
{
	struct klvanc_packet_header_s hdr;
	unsigned int dbb1;
	unsigned int dbb2;
	unsigned int bitstream_number;
	unsigned int super_frame_rate;
	unsigned int frames_per_super_frame;
	unsigned int super_frames;
	unsigned int seconds;
	unsigned int minutes;
	unsigned int hours;
	unsigned int drop_frame;
	unsigned int sub_frame_1;
	unsigned int sub_frame_2;
	unsigned int sub_frame_3;
	unsigned int sub_frame_4;
	unsigned int sub_frame_5;
	unsigned int frame_identifier;
	unsigned int frames;
};

int klvanc_dump_SMPTE_12_3(struct klvanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_SMPTE_12_3_H */
