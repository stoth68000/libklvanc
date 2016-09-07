/* Copyright Kernel Labs Inc 2015-2016. All Rights Reserved. */

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

	struct vanc_context_private_s *prv = getPrivate(p);
	pthread_mutex_init(&prv->listlock, NULL);

	/* Lock the mutex, init the list. Here for cut/paste purposes */
	pthread_mutex_lock(&prv->listlock);
	xorg_list_init(&prv->listOfItems);

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
