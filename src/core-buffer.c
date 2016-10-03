/* By design, non of these funcs use the mutex, caller handles locking. */

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vanc_buffer_dump(struct buffer_s *buf)
{
	VALIDATE(buf);
	printf("  buf %p nr = %d ctx = %p\n", (void *)buf, buf->nr, (void *)buf->ctx);
	return KLAPI_OK;
}

/* Demonstrate safe list traversal, this could support a direct list delete() */
int vanc_buffer_dump_list(struct xorg_list *head)
{
	VALIDATE(head);
	struct buffer_s *iterator = NULL, *next;
	xorg_list_for_each_entry_safe(iterator, next, head, list) {
		vanc_buffer_dump(iterator);
	}

	return KLAPI_OK;
}

int vanc_buffer_alloc(struct vanc_context_s *ctx, struct buffer_s **pp, unsigned int nr)
{
	VALIDATE(ctx);
	VALIDATE(pp);

	struct buffer_s *buf;
	buf = calloc(1, sizeof(struct buffer_s));
	if (buf == 0)
		return -ENOMEM;

	buf->ctx = ctx;
	buf->nr = nr;

	*pp = buf;
	return KLAPI_OK;
}

int vanc_buffer_free(struct buffer_s *buf)
{
	VALIDATE(buf);

	/* The caller is responsible for removing the buffer form any list. */
	memset(buf, 0, sizeof(*buf));

	return KLAPI_OK;
}

int vanc_buffer_reset(struct buffer_s *buf)
{
	VALIDATE(buf);

	/* Perform any buffer wipes, reset any vars as your library needs */

	return KLAPI_OK;
}

