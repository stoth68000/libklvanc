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
 * @file	cache.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	VANC Caching functionality
 */

#ifndef _VANC_CACHE_H
#define _VANC_CACHE_H

#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif  

struct klvanc_cache_line_s
{
	int             active;
	uint64_t        count;
	pthread_mutex_t mutex;
	struct klvanc_packet_header_s *pkt;
};

struct klvanc_cache_s
{
	uint32_t       did, sdid;
	const char    *desc, *spec;
	struct timeval lastUpdated;
	int            hasCursor;
	int            expandUI;
	int            save;
	uint32_t       activeCount;
	struct klvanc_cache_line_s lines[2048];
};

/**
 * @brief	    Begin caching and summarizing VANC payload, useful when you want to
 *              query what VANC messages, and how many you seen on what lines.
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int klvanc_context_enable_cache(struct klvanc_context_s *ctx);

/**
 * @brief	    When caching and summarizing VANC payload is enabled, use this to reset any
 *              internal counters, line counts and restart the stats collection process.
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
void klvanc_cache_reset(struct klvanc_context_s *ctx);

/**
 * @brief	    When caching and summarizing VANC payload is enabled, lookup any statistics
 *              related to didnr and sdidnr.
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
struct klvanc_cache_s * klvanc_cache_lookup(struct klvanc_context_s *ctx, uint8_t didnr, uint8_t sdidnr);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_CACHE_H */
