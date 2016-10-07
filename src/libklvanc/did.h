/**
 * @file	did.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	Lookup functions that translate did/sdid into printable strings.
 */

#include <stdint.h>

/**
 * @brief	For a given did, sdid, lookup a printable user description.
 * @param[in]	uint16_t did - Brief description goes here.
 * @param[in]	uint16_t sdid - Brief description goes here.
 * @return	Success - User facing printable string.
 * @return	Error - NULL
 */
const char * klvanc_didLookupDescription(uint16_t did, uint16_t sdid);

/**
 * @brief	For a given did, sdid, lookup the printable specification name.
 * @param[in]	uint16_t did - Brief description goes here.
 * @param[in]	uint16_t sdid - Brief description goes here.
 * @return	Success - User facing printable string.
 * @return	Error - NULL
 */
const char * klvanc_didLookupSpecification(uint16_t did, uint16_t sdid);
