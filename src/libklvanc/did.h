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
