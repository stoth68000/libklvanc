/**
 * @file	vanc-packets.h
 * @author	Copyright Kernel Labs Inc 2014-2016. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

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
};

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_PACKETS_H */
