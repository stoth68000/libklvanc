/*
 * Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved
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
 * @file	vanc-kl_u64le_counter.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	VANC counter library used for diagnostics/debugging
 */

#ifndef _VANC_KL_U64LE_COUNTER_H
#define _VANC_KL_U64LE_COUNTER_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_kl_u64le_counter_s
{
	struct klvanc_packet_header_s hdr;
	uint64_t counter;
};

/**
 * @brief	Create a KL counter packet
 * @param[out]	struct klvanc_packet_kl_u64le_counter_s **pkt - Pointer to newly created packet
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_create_KL_U64LE_COUNTER(struct klvanc_packet_kl_u64le_counter_s **pkt);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 * @return	0 - Success
 * @return	< 0 - Error
 */
int klvanc_dump_KL_U64LE_COUNTER(struct klvanc_context_s *ctx, void *p);

/**
 * @brief	Convert type struct klvanc_packet_kl_u64le_counter_s into a more traditional line of\n
 *              vanc words, so that we may push out as VANC data.
 *              On success, caller MUST free the resulting *words array.
 * @param[in]	struct klvanc_packet_kl_u64le_counter_s *pkt - A KL counter VANC entry
 * @param[out]	uint16_t **words - An array of words representing a fully formed vanc line.
 * @param[out]	uint16_t *wordCount - Number of words in the array.
 * @return        0 - Success
 * @return      < 0 - Error
 * @return      -ENOMEM - Not enough memory to satisfy request
 */
int klvanc_convert_KL_U64LE_COUNTER_to_words(struct klvanc_packet_kl_u64le_counter_s *pkt,
					     uint16_t **words, uint16_t *wordCount);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_KL_U64LE_COUNTER_H */
