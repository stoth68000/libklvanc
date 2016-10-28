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
 * @file	vanc-packets.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _VANC_PACKETS_H
#define _VANC_PACKETS_H

#include <sys/types.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
enum packet_type_e
{
	VANC_TYPE_UNDEFINED = 0,
	VANC_TYPE_PAYLOAD_INFORMATION,
	VANC_TYPE_EIA_708B,
	VANC_TYPE_EIA_608,
	VANC_TYPE_SCTE_104,
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_header_s
{
	enum packet_type_e	type;
	unsigned short		adf[3];
	unsigned short		did;
	unsigned short		dbnsdid;
	unsigned short		checksum;
	unsigned short		payload[16384];
	unsigned short		payloadLengthWords;
	unsigned int 		checksumValid;
	unsigned int		lineNr; 		/**< The vanc in this header came from line.... */
	unsigned short		raw[16384];
	unsigned int 		rawLengthWords;
	unsigned short		horizontalOffset;	/**< Horizontal word where the ADF was detected. */
};

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_PACKETS_H */
