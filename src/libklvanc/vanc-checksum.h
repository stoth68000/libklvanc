/**
 * @file	vanc-checksum.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t *words - Brief description goes here.
 * @param[in]	int wordCount - Brief description goes here.
 */
uint16_t vanc_checksum_calculate(uint16_t *words, int wordCount);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t *words - Brief description goes here.
 * @param[in]	int wordCount - Brief description goes here.
 */
int vanc_checksum_is_valid(uint16_t *words, int wordCount);

