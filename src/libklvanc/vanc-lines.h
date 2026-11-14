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

/**
 * @file	vanc-lines.h
 * @author	Devin Heitmueller <dheitmueller@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	Routines for managing VANC lines containing one or more VANC packets.
 *
 *              These routines allow a caller to take VANC packets and assemble a frame
 *              composed of a group of VANC lines.  This includes:
 *              1.  Ensuring no VANC packets with a given line/offset will overlap (moving VANC packets if necessary)
 *              2.  Ensuring there are no gaps between VANC packets
 *              3.  Ensuring the sum of all VANC packets does not overflow the line length
 *              4.  Ensuring there are no illegal values in the VANC payload at a physical level
 */

#ifndef _KLVANC_LINE_H
#define _KLVANC_LINE_H

#include <stdint.h>
#include <sys/types.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif  

#define KLVANC_MAX_VANC_LINES   64
#define KLVANC_MAX_VANC_ENTRIES 16

struct klvanc_entry_s
{
        int h_offset;
        uint16_t *payload;
        int pixel_width;
};

/**
 * @brief	Represents a VANC line prior to serialization
 */
struct klvanc_line_s
{
	int line_number;
	struct klvanc_entry_s *p_entries[KLVANC_MAX_VANC_ENTRIES];
	int num_entries;
};

/**
 * @brief	Represents a group of VANC lines (e.g. perhaps corresponding to a video frame)
 */
    
struct klvanc_line_set_s
{
	int num_lines;
	struct klvanc_line_s *lines[KLVANC_MAX_VANC_LINES];
};

/**
 * @brief	Create a VANC line
 *
 * @param[in]	int line_number - line number corresponding to where the line
 *              placed within the field
 * @return      struct klvanc_line_s containing the line or NULL on error
 */
struct klvanc_line_s *klvanc_line_create(int line_number);

/**
 * @brief	Free a previously created klvanc_line_s structure
 * @param[in]	struct klvanc_line_s *line - pointer to the line to be deleted.
 *              This will also deallocate any VANC packets previously inserted into the line
 */
void klvanc_line_free(struct klvanc_line_s *line);

/**
 * @brief	Insert a VANC packet into a VANC frame
 *
 * @param[in]	struct klvanc_line_set_s *klvanc_lines - pointer to set of VANC lines to insert into
 * @param[in]	uint16_t *pixels - Pointer to array of pixels to insert.  For 10-bit pixels, these
 *              should be 10-bit values inserted into 16-bit padded fields.
 * @param[in]	int pixel_width - width of [pixels] argument, measured in number of samples
 * @param[in]	int line_number - line number this VANC packet should appear on in the
 *              resulting VANC frame
 * @param[in]	int horizontal_offset - offset into active video area the packet should ideally
 *              be inserted, measured in pixels
 *
 * @return      0 - Success
 * @return      -ENOMEM - insufficient memory to store the VANC packet
 */
int klvanc_line_insert(struct klvanc_context_s *ctx, struct klvanc_line_set_s *vanc_lines,
		       uint16_t *pixels, int pixel_width, int line_number, int horizontal_offset);

/**
 * @brief	Generate pixel array representing a fully formed VANC line.  This
 *              function will take in a klvanc_line_s, format the VANC entries to ensure there
 *              are no gaps or overlapping packets, and create a final pixel array which
 *              can be colorspace converted and output over SDI.
 *
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @param[in]	struct klvanc_line_s *line - the VANC line to operate on
 * @param[out]	uint16_t **out_buf - a pointer to the buffer the function will output into.  The
 *              memory for the buffer will be allocated by the function, and the caller will need
 *              to call free() to deallocate the resulting buffer.  For 10-bit video, the array
 *              will contain 10-bit samples in 16-bit values.  Bit packing or Colorspace conversion
 *              to other formats (such as V210) is the responsibility of the caller.
 * @param[out]	int *out_len - the size of the resulting out_buf buffer.  This parameter allows
 *              the caller to know how large out_buf is to avoid buffer overflows.  This value may
 *              be shorter than the pixel width of the overall line.  Measured in number of 16-bit
 *              samples (i.e. not bytes).
 * @param[in]	int line_pixel_width - Size of the line the VANC will ultimately be inserted into,
 *              measured in number of samples.  The function will not create an out_buf buffer which
 *              is larger than this value, nor will it insert VANC packets which exceed the line
 *              width specified.
 * @return      0 - Success
 * @return      -ENOMEM - insufficient memory to store the VANC packet
 */
int klvanc_generate_vanc_line(struct klvanc_context_s *ctx, struct klvanc_line_s *line,
			      uint16_t **out_buf, int *out_len, int line_pixel_width);

/**
 * @brief	Generate byte array representing a fully formed VANC line.  This
 *              function will take in a klvanc_line_s, format the VANC entries to ensure there
 *              are no gaps or overlapping packets, and create a final byte array which
 *              can be directly output over SDI in packed v210 format.
 *
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @param[in]	struct klvanc_line_s *line - the VANC line to operate on
 * @param[out]	uint8_t *out_buf - a pointer to the buffer the function will output into.  The
 *              memory for the output buffer should be allocated by the caller (or in the case
 *              of working with Blackmagic cards, pass the buffer returned from
 *              vanc->GetBufferForVerticalBlankingLine().  Note that this function will determine
 *              whether to do HD interleaving (inserting into the Y region only) or SD
 *              interleaving (using both Y and UV regions) based on the value provided via the
 *              line_pixel_width argument.
 * @param[in]	int line_pixel_width - Size of the output buffer, measured in number of samples.
 *              When working with Blackmagic cards, this value will typically be the line width
 *              (e.g. 1920 for 1080i video), but there are special exceptions for certain 4K
 *              cards so review the Blackmagic SDK documentation for details.  This function
 *              will it insert VANC packets which exceed the line width specified.
 * @return      0 - Success
 * @return      -ENOMEM - insufficient memory to store the VANC packet
 */
int klvanc_generate_vanc_line_v210(struct klvanc_context_s *ctx, struct klvanc_line_s *line,
				   uint8_t *out_buf, int line_pixel_width);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_LINES_H */
