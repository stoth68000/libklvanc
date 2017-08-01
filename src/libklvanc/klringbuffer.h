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

#ifndef KLRINGBUFFER_H
#define KLRINGBUFFER_H

/* Based on: https://gist.github.com/jsimmons/609674 */

/* A copy on write/read ring buffer, with the ability
 * to dynamically grow the buffer up to a user defined
 * maximum. Shrink buffer when its empty, ring WILL truncate
 * data and flag an overflow condition.
 * Absolutely not thread safe. User needs to implement their
 * own locking mechanism if this is important. see
 * rb_new_threadsafe() for a new mutex based implementation.
 */

/* KL Modifications for return values (read/write) so we
 * can track the number of bytes transferred.
 * Modifications to support dynamic growing of the
 * circular buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <pthread.h>

typedef struct
{
	/* Private, don't modify, inspect or rely on the contents. */
	pthread_mutex_t mutex;
	int usingMutex;

	unsigned char *data;
	size_t size;
	size_t size_max;
	size_t size_initial;
	size_t head;
	size_t fill;
} KLRingBuffer;

/**
 * @brief       Allocate a new object, with an initial and maximum growth size. Note
 *              that this ring is NOT threadsafe, use the _new_threadsafe() func if
 *              multiple threads are expected to modify the buffer.
 * @param[in]   size_t size - Initial size of buffer in bytes.
 * @param[in]   size_t size_max - Maximum allowable growable size in bytes.
 * @return      pointer to object, or NULL on error.
 */
KLRingBuffer *rb_new(size_t size, size_t size_max);

/**
 * @brief       Allocate a new object, with an initial and maximum growth size. Note
 *              that this ring is threadsafe, and may be safely used between multiple
 *              concurrent threads.
 * @param[in]   size_t size - Initial size of buffer in bytes.
 * @param[in]   size_t size_max - Maximum allowable growable size in bytes.
 * @return      pointer to object, or NULL on error.
 */
KLRingBuffer *rb_new_threadsafe(size_t size, size_t size_max);

/**
 * @brief       Check for presence of data in the rin buffer.
 * @param[in]   KLRingBuffer *buf - Object.
 * @return      Returns TRUE if empty.
 */
bool rb_is_empty(KLRingBuffer *buf);

/**
 * @brief       Check if the ring has grown to reach is maximum allowable size.
 * @param[in]   KLRingBuffer *buf - Object.
 * @return      Returns TRUE if full.
 */
bool rb_is_full(KLRingBuffer *buf);

/**
 * @brief       Return total number of bytes written to the ring, regardless of allocated size.
 * @param[in]   KLRingBuffer *buf - Object.
 * @return      TODO.
 */
size_t rb_used(KLRingBuffer *buf);

/**
 * @brief       Drain the contents of the ring, this is significantly more efficient than
 *              draining through rb_read().
 * @param[in]	KLRingBuffer *buf - Brief description goes here.
 */
void rb_empty(KLRingBuffer *buf);

/**
 * @brief       The amount of unused free space available, assuming the ring is allowed
 *              to grow to its upper limit.
 * @param[in]   KLRingBuffer *buf - Object.
 * @return	size in bytes of free space.
 */
size_t rb_unused(KLRingBuffer *buf);

/**
 * @brief       Write data into the ring, growing it if necessary, returning the number of bytes written.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]	const char *from - Source data to (mem)copy into the ring.
 * @param[in]	size_t bytes - Number of bytes to copy.
 * @param[out]	int didOverflow - True if the write caused the buffer to truncate data
 *              due to the ring reaching is maximum allowable size. This indicates data was lost.
 * @return	Number of bytes written.
 */
size_t rb_write_with_state(KLRingBuffer *buf, const char *from, size_t bytes, int *didOverflow);

/**
 * @brief       (Deprecated) Write data into the ring, growing it if necessary,
 *              returning the number of bytes written.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]	const char *from - Source data to (mem)copy into the ring.
 * @param[in]	size_t bytes - Number of bytes to copy.
 * @return	Number of bytes written.
 */
__attribute__((deprecated))
size_t rb_write(KLRingBuffer *buf, const char *from, size_t bytes);

/**
 * @brief       Read data from the ring, draining it, returning the number of bytes read.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]	char *to - Destination buffer when data will me (mem)copied to.
 * @param[in]	size_t bytes - Number of bytes to copy.
 * @return	Number of bytes written.
 */
size_t rb_read(KLRingBuffer *buf, char *to, size_t bytes);
size_t rb_read_alloc(KLRingBuffer *buf, char **to, size_t bytes);

/**
 * @brief       Read data from the ring, without draining it, returning the number of bytes read.
 *              Original data will remain in the ring for later use.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]	char *to - Destination buffer when data will me (mem)copied to.
 * @param[in]	size_t bytes - Number of bytes to copy.
 * @return	Number of bytes written.
 */
size_t rb_peek(KLRingBuffer *buf, char *to, size_t bytes);

/**
 * @brief       Write the entire contents of the ring, draining it, to file. A debug helper func.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]   FILE *fh - A file already opened for write access.
 */
void rb_fwrite(KLRingBuffer *buf, FILE *fh);

/**
 * @brief       Destroy/release all resources related to this object.
 * @param[in]   KLRingBuffer *buf - Object.
 */
void rb_free(KLRingBuffer *buf);

/**
 * @brief       TODO - Brief description goes here.
 * @param[in]   KLRingBuffer *buf - Object.
 * @param[in]	size_t bytes - Number of bytes to discard.
 */
void rb_discard(KLRingBuffer *buf, size_t bytes);

#endif /* KLRINGBUFFER_H */
