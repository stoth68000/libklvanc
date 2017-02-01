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

#include <libklvanc/vanc.h>

#include "core-private.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int vanc_context_dump(struct vanc_context_s *ctx)
{
	VALIDATE(ctx);
	struct vanc_context_private_s *prv = getPrivate(ctx);

	printf("ctx %p\n", (void *)ctx);
	printf("->private %p\n", (void *)prv);

	vanc_buffer_dump_list(&prv->listOfItems);

	return KLAPI_OK;
}

int vanc_context_create(struct vanc_context_s **ctx)
{
	int ret = KLAPI_OK;

	struct vanc_context_s *p = calloc(1, sizeof(struct vanc_context_s));
	if (!p)
		return -ENOMEM;

	p->priv = calloc(1, sizeof(struct vanc_context_private_s));
	if (!p->priv) {
		free(p);
		return -ENOMEM;
	}

	/* If we fail to parse a vanc message, don't report more than one of those per second. */
	klrestricted_code_path_block_initialize(&p->rcp_failedToDecode, 1, 1, 1000);

	struct vanc_context_private_s *prv = getPrivate(p);
	pthread_mutex_init(&prv->listlock, NULL);

	/* Lock the mutex, init the list. Here for cut/paste purposes */
	pthread_mutex_lock(&prv->listlock);
	xorg_list_init(&prv->listOfItems);

	/* TODO: ST: DId I add these from a template? Do we even use these buffers? */
	
	/* Create some buffers, put them on a list */
	for (unsigned int i = 0; i < 4; i++) {
		struct buffer_s *buf;
		ret = vanc_buffer_alloc(p, &buf, i);
		if (ret < 0) {
			fprintf(stderr, "Unable to allocate buffer, enough free ram?\n");
			break;
		}
		/* append to tail (vs _add() which inserts to head) */
		xorg_list_append(&buf->list, &prv->listOfItems);
	}
	pthread_mutex_unlock(&prv->listlock);

	if (ret == KLAPI_OK)
		*ctx = p;

	return ret;
}

int vanc_context_destroy(struct vanc_context_s *ctx)
{
	VALIDATE(ctx);
	struct vanc_context_private_s *prv = getPrivate(ctx);
	struct buffer_s *buf;

	vanc_cache_free(ctx);

	/* Free any allocated buffers */
	pthread_mutex_lock(&prv->listlock);
	while (xorg_list_is_empty(&prv->listOfItems) == 0) {
		buf = xorg_list_first_entry(&prv->listOfItems, struct buffer_s, list);
		xorg_list_del(&buf->list);
		vanc_buffer_free(buf);
	}
	pthread_mutex_unlock(&prv->listlock);

	memset(ctx->priv, 0, sizeof(struct vanc_context_private_s));
	free(ctx->priv);

	memset(ctx, 0, sizeof(*ctx));
	free(ctx);

	return KLAPI_OK;
}

int vanc_context_enable_cache(struct vanc_context_s *ctx)
{
	if (vanc_cache_alloc(ctx) < 0) {
		fprintf(stderr, "Unable to allocate vanc cache, enough free ram? Will continue.\n");
		return -1;
	}

	return 0;
}

