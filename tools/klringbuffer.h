/* Copyright Kernel Labs Inc, 2016 */

#ifndef KLRINGBUFFER_H
#define KLRINGBUFFER_H

/* Based on: https://gist.github.com/jsimmons/609674 */

/* A copy on write/read ring buffer, with the ability
 * to dynamically grow the buffer up to a user defined
 * maximum. Shrink buffer when its empty.
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

#define KLRINGBUFFER_STATUS(rb) \
        printf("rb.size = %zu rb.remain = %zu rb.used = %zu\n", \
		rb_size(rb), rb_remain(rb), rb_used(rb)); \

typedef struct
{
	unsigned char *data;
	size_t size;
	size_t size_max;
	size_t size_initial;
	size_t head;
	size_t fill;
} KLRingBuffer;

KLRingBuffer *rb_new(size_t size, size_t size_max);

static inline bool rb_is_empty(KLRingBuffer *buf)
{
    return buf->fill == 0;
}

static inline bool rb_is_full(KLRingBuffer *buf)
{
    return buf->fill == buf->size;
}

static inline size_t rb_size(KLRingBuffer *buf)
{
    return buf->size;
}

static inline size_t rb_used(KLRingBuffer *buf)
{
    return buf->fill;
}

static inline size_t rb_remain(KLRingBuffer *buf)
{
    return buf->size - buf->fill;
}

static inline void rb_empty(KLRingBuffer *buf)
{
    buf->head = buf->fill = 0;
}

size_t rb_write(KLRingBuffer *buf, const char *from, size_t bytes);
#if 0
char *rb_write_pointer(KLRingBuffer *buf, size_t *writable);
void rb_write_commit(KLRingBuffer *buf, size_t bytes);
#endif

size_t rb_read(KLRingBuffer *buf, char *to, size_t bytes);
size_t rb_peek(KLRingBuffer *buf, char *to, size_t bytes);

#if 0
const char *rb_read_pointer(KLRingBuffer *buf, size_t offset, size_t *readable);
void rb_read_commit(KLRingBuffer *buf, size_t bytes);
void rb_stream(KLRingBuffer *from, KLRingBuffer *to, size_t bytes);
#endif

void rb_free(KLRingBuffer *buf);

void rb_fwrite(KLRingBuffer *buf, FILE *fh);

#endif /* KLRINGBUFFER_H */
