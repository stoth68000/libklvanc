/**
 * @file	vanc-checksum.h
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

