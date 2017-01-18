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
 * @file	klrestricted_code_path.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved.
 * @brief	Generic code to limit the running of certain code to N times per second.
 *          Primary use case is to prevent errors from spamming system logs.
 *          User allocates a small context, context contains max latency.__msfr_align
 *          Users asks whether its permitted to execute the code block, function determines answer.
 *          First occurence is always allowed to execute.
 *
 * USAGE during initialization, allow messages only every 1000ms
 * struct klrestricted_code_path_block_s global_mypathXYZ;
 * klrestricted_code_path_block_initialize(&global_mypathXYZ, <uniqueid>, 1, 1000);
 *
 * USAGE during runtime:
 * if (klrestricted_code_path_block_execute(&global_mypathXYZ))
 * {
 *    fprintf(stderr, "I logged a high volume message, but only once per second!\n");
 * }
 */

#ifndef _KLRESTRICTED_CODE_PATH_H
#define _KLRESTRICTED_CODE_PATH_H

#include <stdint.h>
#include <string.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct klrestricted_code_path_block_s
{
	int enableChecking;
	int id;
	int minimumIntervalMs;

	struct timeval lastExecuteTime;
	uint64_t countBlockEntered;
	uint64_t countBlockAvoided;
};

__inline__ static int klrcp_timeval_subtract(struct timeval *result, struct timeval *x, struct timeval *y)
{
     /* Perform the carry for the later subtraction by updating y. */
     if (x->tv_usec < y->tv_usec)
     {
         int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
         y->tv_usec -= 1000000 * nsec;
         y->tv_sec += nsec;
     }
     if (x->tv_usec - y->tv_usec > 1000000)
     {
         int nsec = (x->tv_usec - y->tv_usec) / 1000000;
         y->tv_usec += 1000000 * nsec;
         y->tv_sec -= nsec;
     }

     /* Compute the time remaining to wait. tv_usec is certainly positive. */
     result->tv_sec = x->tv_sec - y->tv_sec;
     result->tv_usec = x->tv_usec - y->tv_usec;

     /* Return 1 if result is negative. */
     return x->tv_sec < y->tv_sec;
}

__inline__ static uint64_t klrcp_timediff_to_msecs(struct timeval *tv)
{
        return (tv->tv_sec * 1000) + (tv->tv_usec / 1000);
}

__inline__ void klrestricted_code_path_block_initialize(struct klrestricted_code_path_block_s *blk, int id,
	int enableChecking, int minimumIntervalMs)
{
	memset(blk, 0, sizeof(*blk));
	blk->id = id;
	blk->enableChecking = enableChecking;
	blk->minimumIntervalMs = minimumIntervalMs;
};

__inline__ int klrestricted_code_path_block_execute(struct klrestricted_code_path_block_s *blk)
{
	if (blk->enableChecking == 0) {
		blk->countBlockEntered++;
		return 1;
	}

	struct timeval now, diff;
	gettimeofday(&now, 0);
	klrcp_timeval_subtract(&diff, &now, &blk->lastExecuteTime);
	if (klrcp_timediff_to_msecs(&diff) < blk->minimumIntervalMs) {
		blk->countBlockAvoided++;
		return 0;
	}

	blk->lastExecuteTime = now;
	blk->countBlockEntered++;
	return 1;
}

#ifdef __cplusplus
};
#endif

#endif /* _KLRESTRICTED_CODE_PATH_H */
