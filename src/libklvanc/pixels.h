/**
 * @file	pixels.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#include <stdint.h>

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
 * @param[in]	uint32_t * src - Brief description goes here.
 * @param[in]	uint16_t * dst - Brief description goes here.
 * @param[in]	int width - Brief description goes here.
 */
void klvanc_v210_line_to_uyvy_c(uint32_t * src, uint16_t * dst, int width);
