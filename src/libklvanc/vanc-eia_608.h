/**
 * @file	vanc-eia_608.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _VANC_EIA_608_H
#define _VANC_EIA_608_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_eia_608_s
{
	struct packet_header_s hdr;
	int nr;
	unsigned char payload[3];

	/* Parsed */
	int marker_bits;
	int cc_valid;
	int cc_type;
	unsigned char cc_data_1;
	unsigned char cc_data_2;
};

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 */
int dump_EIA_608(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_EIA_608_H */
