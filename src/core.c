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

/* Default logging implementation just writes to stderr */
static void vanc_default_logger(void *ctx, int level, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int klvanc_context_dump(struct klvanc_context_s *ctx)
{
	VALIDATE(ctx);

	printf("ctx %p\n", (void *)ctx);

	return KLAPI_OK;
}

int klvanc_context_create(struct klvanc_context_s **ctx)
{
	int ret = KLAPI_OK;

	struct klvanc_context_s *p = calloc(1, sizeof(struct klvanc_context_s));
	if (!p)
		return -ENOMEM;

	/* If we fail to parse a vanc message, don't report more than one of those per second. */
	klrestricted_code_path_block_initialize(&p->rcp_failedToDecode, 1, 1, 60 * 1000);

	if (ret == KLAPI_OK)
		*ctx = p;

	/* Set the default logger */
	(*ctx)->log_cb = vanc_default_logger;

	return ret;
}

int klvanc_context_destroy(struct klvanc_context_s *ctx)
{
	VALIDATE(ctx);

	klvanc_cache_free(ctx);

	cleanup_SCTE_104(ctx);

	memset(ctx, 0, sizeof(*ctx));
	free(ctx);

	return KLAPI_OK;
}

int klvanc_context_enable_cache(struct klvanc_context_s *ctx)
{
	if (klvanc_cache_alloc(ctx) < 0) {
		fprintf(stderr, "Unable to allocate vanc cache, enough free ram? Will continue.\n");
		return -1;
	}

	return 0;
}

