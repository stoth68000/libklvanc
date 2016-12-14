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
 * @brief	Routines for managing VANC lines containing one or more VANC packets
 */

#ifndef _VANC_LINE_H
#define _VANC_LINE_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif  

#define MAX_VANC_LINES   64
#define MAX_VANC_ENTRIES 16

struct vanc_entry_s
{
        int h_offset;
        uint16_t *payload;
        int pixel_width;
};

/**
 * @brief	Represents a VANC line prior to serialization
 */
struct vanc_line_s
{
	int line_number;
	struct vanc_entry_s *p_entries[MAX_VANC_ENTRIES];
	int num_entries;
};

/**
 * @brief	Represents a group of VANC lines (e.g. perhaps corresponding to a video frame)
 */
    
struct vanc_line_set_s
{
	int num_lines;
	struct vanc_line_s *lines[MAX_VANC_LINES];
};

struct vanc_line_s *vanc_line_create(int line_number);
void vanc_line_free(struct vanc_line_s *line);
void vanc_line_insert(struct vanc_line_set_s *vanc_lines, uint16_t *pixels,
		      int pixel_width, int line_number, int horizontal_offset);
void generate_vanc_line(struct vanc_line_s *line, uint16_t **out_buf, int *out_len, int line_pixel_width);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_LINES_H */
