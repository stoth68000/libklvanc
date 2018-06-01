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
 * @file	pixels.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	Common colorspace conversion functions for VANC
 */

#ifndef _PIXELS_H
#define _PIXELS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	const uint32_t * src - Brief description goes here.
 * @param[in]	uint16_t * y - Brief description goes here.
 * @param[in]	uint16_t * u - Brief description goes here.
 * @param[in]	uint16_t * v - Brief description goes here.
 * @param[in]	int width - Brief description goes here.
 */
void klvanc_v210_planar_unpack_c(const uint32_t * src, uint16_t * y, uint16_t * u, uint16_t * v, int width);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	const uint32_t * src - Brief description goes here.
 * @param[in]	uint16_t * dst - Brief description goes here.
 * @param[in]	int dstSizeBytes - Size of the dst buffer allocation.
 * @param[in]	int width - Brief description goes here.
 * @result 	0 - Success
 * @result 	< 0 - Error
 */
int klvanc_v210_line_to_nv20_c(const uint32_t * src, uint16_t * dst, int dstSizeBytes, int width);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t * src - Brief description goes here.
 * @param[in]	uint8_t * dst - Brief description goes here.
 * @param[in]	int lines - Brief description goes here.
 */
void klvanc_v210_downscale_line_c(uint16_t * src, uint8_t * dst, int lines);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	const uint32_t * src - Brief description goes here.
 * @param[in]	uint16_t * dst - Brief description goes here.
 * @param[in]	int width - Brief description goes here.
 */
void klvanc_v210_line_to_uyvy_c(const uint32_t * src, uint16_t * dst, int width);

/**
 * @brief	Convert Y10 buffer to V210
 * @param[in]	uint16_t * src - Array of 16-bit fields containing 10-bit Y values
 * @param[out]	uint8_t * dst - Destination containing resulting V210 video
 * @param[in]	int width - Number of Y pixels in src
 */
void klvanc_y10_to_v210(uint16_t *src, uint8_t *dst, int width);

/**
 * @brief	Convert UYVY buffer to V210
 * @param[in]	uint16_t * src - Array of 16-bit fields containing 10-bit YUV values
 * @param[out]	uint8_t * dst - Destination containing resulting V210 video
 * @param[in]	int width - Number of Y pixels in src
 */
void klvanc_uyvy_to_v210(uint16_t *src, uint8_t *dst, int width);

#ifdef __cplusplus
};
#endif

#endif /* _PIXELS_H */
