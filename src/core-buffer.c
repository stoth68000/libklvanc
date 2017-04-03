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
	free(buf);

	return KLAPI_OK;
}

int vanc_buffer_reset(struct buffer_s *buf)
{
	VALIDATE(buf);

	/* Perform any buffer wipes, reset any vars as your library needs */
	memset(buf, 0, sizeof(*buf));

	return KLAPI_OK;
}

