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

int dump_EIA_708B(struct vanc_context_s *ctx, void *p)
{
	struct packet_eia_708b_s *pkt = p;

	if (ctx->verbose)
		printf("%s() %p\n", __func__, (void *)pkt);

	return KLAPI_OK;
}

int parse_EIA_708B(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp)
{
	if (ctx->verbose)
		printf("%s()\n", __func__);

	struct packet_eia_708b_s *pkt = calloc(1, sizeof(*pkt));
	if (!pkt)
		return -ENOMEM;

	memcpy(&pkt->hdr, hdr, sizeof(*hdr));

	if (ctx->callbacks && ctx->callbacks->eia_708b)
		ctx->callbacks->eia_708b(ctx->callback_context, ctx, pkt);

	*pp = pkt;
	return KLAPI_OK;
}

