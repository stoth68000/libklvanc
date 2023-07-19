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
 * @copyright	Copyright (c) 2016-2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	VANC Headers and packet structure
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
enum klvanc_packet_type_e
{
	VANC_TYPE_UNDEFINED = 0,
	VANC_TYPE_AFD,
	VANC_TYPE_EIA_708B,
	VANC_TYPE_EIA_608,
	VANC_TYPE_SCTE_104,
	VANC_TYPE_KL_UINT64_COUNTER,
	VANC_TYPE_SDP,
	VANC_TYPE_SMPTE_S12_2,
	VANC_TYPE_SMPTE_S2108_1,
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_header_s
{
	enum klvanc_packet_type_e	type;
	unsigned short		adf[3];
	unsigned short		did;
	unsigned short		dbnsdid;
	unsigned short		checksum;
#define LIBKLVANC_PACKET_MAX_PAYLOAD (16384)
	unsigned short		payload[LIBKLVANC_PACKET_MAX_PAYLOAD];
	unsigned short		payloadLengthWords;
	unsigned int 		checksumValid;
	unsigned int		lineNr; 		/**< The vanc in this header came from line.... */
	unsigned short		raw[LIBKLVANC_PACKET_MAX_PAYLOAD];
	unsigned int 		rawLengthWords;
	unsigned short		horizontalOffset;	/**< Horizontal word where the ADF was detected. */
};

/**
 * @brief SMPTE 291-1-2011 Section 6.3
 * "An ancillary data packet with a DID word value equal to 80h may be deleted by any equipment
 * during a subsequent processing cycle (see Annex C). The occupied ancillary data space, however,
 * shall remain contiguous as defined in Section 7.3.
 * Note: Designers of equipment are advised that, in 8-bit systems, ancillary data packets with
 * DID words in the range of 80h – 83h all are considered to be marked for deletion."
 */
#define klvanc_packetType1(pkt) (((pkt)->did >= 0x80) && ((pkt)->did <= 0x83))

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_PACKETS_H */
