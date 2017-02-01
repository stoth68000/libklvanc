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

/* Not for inclusion by user applications */

#ifndef vanc_PRIVATE_H
#define vanc_PRIVATE_H

#include <sys/types.h>
#include <sys/errno.h>

/* We'll have a mutex and a list of items */
#include <pthread.h>
#include "xorg-list.h"

#define getPrivate(ctx) ((struct vanc_context_private_s *)ctx->priv)
#define sanitizeWord(word) ((word) & 0xff)

#define KLAPI_OK 0

#define VALIDATE(ctx) \
 if (!ctx) return -EINVAL;

struct buffer_s
{
	struct vanc_context_s *ctx;
	struct xorg_list list;
	unsigned int nr;
};

/* Application specific context the library allocates */
struct vanc_context_private_s
{
	/* Private internal vars here */
	pthread_mutex_t listlock;
	struct xorg_list listOfItems;
};

int vanc_buffer_alloc(struct vanc_context_s *ctx, struct buffer_s **buf, unsigned int nr);
int vanc_buffer_free(struct buffer_s *buf);
int vanc_buffer_reset(struct buffer_s *buf);
int vanc_buffer_dump(struct buffer_s *buf);
int vanc_buffer_dump_list(struct xorg_list *head);

/* core-packet-payload_information.c */
int dump_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, void *p);
int parse_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp);

/* core-packet-eia_708b.c */
int dump_EIA_708B(struct vanc_context_s *ctx, void *p);
int parse_EIA_708B(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp);

/* core-packet-eia_608.c */
int dump_EIA_608(struct vanc_context_s *ctx, void *p);
int parse_EIA_608(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp);

/* core-packet-eia_608.c */
int dump_SCTE_104(struct vanc_context_s *ctx, void *p);
int parse_SCTE_104(struct vanc_context_s *ctx, struct packet_header_s *hdr, void **pp);

void klvanc_dump_packet_console(struct vanc_context_s *ctx, struct packet_header_s *hdr);

/* We don't expect anything outside of the VANC framework to need toascii
 * call these, so we'll keep them private / internal calls.
 */
extern int  vanc_cache_alloc(struct vanc_context_s *ctx);
extern void vanc_cache_free(struct vanc_context_s *ctx);
extern int  vanc_cache_update(struct vanc_context_s *ctx, struct packet_header_s *pkt);

#endif
