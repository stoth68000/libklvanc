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

#include "klringbuffer.h"

KLRingBuffer *rb_new(size_t size, size_t size_max)
{
	if ((size == 0) || (size > size_max))
		return 0;

	KLRingBuffer *buf = malloc(sizeof(*buf));
	if (!buf)
		return 0;

	buf->data = malloc(size);
	if (!buf->data) {
		free(buf);
		return 0;
	}

	buf->size = size;
	buf->size_initial = size;
	buf->head = buf->fill = 0;
	buf->size_max = size_max;

	return buf;
}

static int rb_grow(KLRingBuffer *buf, size_t increment)
{
	if (!buf)
		return -1;

	if ((rb_size(buf) + increment) > buf->size_max)
		return -2;

	buf->data = realloc(buf->data, buf->size + increment);
	buf->size += increment;
	return 0;
}

static void rb_shrink_reset(KLRingBuffer *buf)
{
	buf->data = realloc(buf->data, buf->size_initial);
	buf->size = buf->size_initial;
	buf->head = buf->fill = 0;
}

static inline void advance_tail(KLRingBuffer *buf, size_t bytes)
{
	buf->fill += bytes;
}

size_t rb_write(KLRingBuffer *buf, const char *from, size_t bytes)
{
	assert(buf);
	assert(from);

	if (bytes > rb_remain(buf)) {
		if (rb_grow(buf, bytes * 128) < 0)
			return 0;
	}

	unsigned char *tail = buf->data + ((buf->head + buf->fill) % buf->size);
	unsigned char *write_end = buf->data + ((buf->head + buf->fill + bytes) % buf->size);

	if (tail <= write_end) {
		memcpy(tail, from, bytes);
	} else {
		unsigned char *end = buf->data + buf->size;
        
		size_t first_write = end - tail;
		memcpy(tail, from, first_write);
        
		size_t second_write = bytes - first_write;
		memcpy(buf->data, from + first_write, second_write);
	}

	advance_tail(buf, bytes);
	return bytes;
}

#if 0
char *rb_write_pointer(KLRingBuffer *buf, size_t *writable)
{
    if(rb_is_full(buf))
    {
        *writable = 0;
        return NULL;
    }

    char *head = buf->data + buf->head;
    char *tail = buf->data + ((buf->head + buf->fill) % buf->size);

    if(tail < head)
    {
        *writable = head - tail;
    }
    else
    {
        char *end = buf->data + buf->size;
        *writable = end - tail;
    }

    return tail;
}

void rb_write_commit(KLRingBuffer *buf, size_t bytes)
{
    assert(bytes <= rb_remain(buf));
    advance_tail(buf, bytes);
}
#endif

static inline void advance_head(KLRingBuffer *buf, size_t bytes)
{
	buf->head = (buf->head + bytes) % buf->size;
	buf->fill -= bytes;
}

static size_t rb_reader(KLRingBuffer *buf, char *to, size_t bytes, int advance_read_head)
{
	assert(buf);
	assert(to);

	if (bytes > rb_used(buf))
		bytes = rb_used(buf);

	if (bytes == 0)
		return 0;

	unsigned char *head = buf->data + buf->head;
	unsigned char *end_read = buf->data + ((buf->head + bytes) % buf->size);

	if (end_read <= head) {
		unsigned char *end = buf->data + buf->size;

		size_t first_read = end - head;
		memcpy(to, head, first_read);

		size_t second_read = bytes - first_read;
		memcpy(to + first_read, buf->data, second_read);
	} else {
		memcpy(to, head, bytes);
	}

	if (advance_read_head)
		advance_head(buf, bytes); 

	/* When the buffer is empty its a good time to
	 * free any prior large allocations.
	 */
	if ((rb_used(buf) == 0) && (buf->size > buf->size_initial))
		rb_shrink_reset(buf);

	return bytes;
}

size_t rb_read(KLRingBuffer *buf, char *to, size_t bytes)
{
	return rb_reader(buf, to, bytes, 1); /* Advance read head */
}

size_t rb_peek(KLRingBuffer *buf, char *to, size_t bytes)
{
	return rb_reader(buf, to, bytes, 0); /* Don't Advance read head */
}

#if 0
const char *rb_read_pointer(KLRingBuffer *buf, size_t offset, size_t *readable)
{
    if(rb_is_empty(buf))
    {
        *readable = 0;
        return NULL;
    }

    char *head = buf->data + buf->head + offset;
    char *tail = buf->data + ((buf->head + offset + buf->fill) % buf->size);

    if(tail <= head)
    {
        char *end = buf->data + buf->size;
        *readable = end - head;
    }
    else
    {
        *readable = tail - head;
    }

    return head;
}

void rb_read_commit(KLRingBuffer *buf, size_t bytes)
{
    assert(rb_used(buf) >= bytes);
    advance_head(buf, bytes);
}

void rb_stream(KLRingBuffer *from, KLRingBuffer *to, size_t bytes)
{
    assert(rb_used(from) <= bytes);
    assert(rb_remain(to) >= bytes);

    size_t copied = 0;
    while(copied < bytes)
    {
        size_t can_read;
        const char *from_ptr = rb_read_pointer(from, copied, &can_read);

        size_t copied_this_read = 0;
        
        while(copied_this_read < can_read)
        {
            size_t can_write;
            char *to_ptr = rb_write_pointer(to, &can_write);

            size_t write = (can_read > can_write) ? can_write : can_read;
            memcpy(to_ptr, from_ptr, write);

            copied_this_read += write;
        }

        copied += copied_this_read;
    }

    advance_tail(to, copied);
}
#endif

void rb_free(KLRingBuffer *buf)
{
	assert(buf);
	if (buf) {
		free(buf->data);
		free(buf);
	}
}

void rb_fwrite(KLRingBuffer *buf, FILE *fh)
{
	if (rb_is_empty(buf))
		return;

	unsigned char head[4] = { 'H', 'E', 'A', 'D' };
	fwrite(&head[0], 1, sizeof(head), fh);

	unsigned int rb_len = rb_used(buf);
	unsigned char hdrlen[4] = {
		(rb_len >> 24) & 0xff,
		(rb_len >> 16) & 0xff,
		(rb_len >>  8) & 0xff,
		(rb_len >>  0) & 0xff
	};
	fwrite(&hdrlen[0], 1, sizeof(hdrlen), fh);

	unsigned char b[8192];
	size_t len = 1;
	while (len) {
		len = rb_read(buf, (char *)&b[0], sizeof(b));
		if (len)
			fwrite(&b[0], 1, len, fh);
	}

	unsigned char tail[4] = { 'T', 'A', 'I', 'L' };
	fwrite(&tail[0], 1, sizeof(tail), fh);
}

