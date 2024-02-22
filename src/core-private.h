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
#include "klbitstream_readwriter.h"

#define getPrivate(ctx) ((struct vanc_context_private_s *)ctx->priv)
#define sanitizeWord(word) ((word) & 0xff)

#define KLAPI_OK 0

#define VALIDATE(ctx) \
 if (!ctx) return -EINVAL;

/* core-packet-afd.c */
int dump_AFD(struct klvanc_context_s *ctx, void *p);
int parse_AFD(struct klvanc_context_s *ctx,
	      struct klvanc_packet_header_s *hdr, void **pp);

/* core-packet-eia_708b.c */
int dump_EIA_708B(struct klvanc_context_s *ctx, void *p);
int parse_EIA_708B(struct klvanc_context_s *ctx,
		   struct klvanc_packet_header_s *hdr, void **pp);

/* core-packet-eia_608.c */
int dump_EIA_608(struct klvanc_context_s *ctx, void *p);
int parse_EIA_608(struct klvanc_context_s *ctx,
		  struct klvanc_packet_header_s *hdr, void **pp);

/* core-packet-scte_104.c */
int dump_SCTE_104(struct klvanc_context_s *ctx, void *p);
int parse_SCTE_104(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr,
		   void **pp);
void cleanup_SCTE_104(struct klvanc_context_s *ctx);

/* core-packet-kl_u64le_counter.c */
int dump_KL_U64LE_COUNTER(struct klvanc_context_s *ctx, void *p);
int parse_KL_U64LE_COUNTER(struct klvanc_context_s *ctx,
			   struct klvanc_packet_header_s *hdr, void **pp);

void klvanc_dump_packet_console(struct klvanc_context_s *ctx,
				struct klvanc_packet_header_s *hdr);

/* core-packet-sdp.c */
int dump_SDP(struct klvanc_context_s *ctx, void *p);
int parse_SDP(struct klvanc_context_s *ctx,
            struct klvanc_packet_header_s *hdr, void **pp);

/* core-packet-smpte_12_2.c */
int dump_SMPTE_12_2(struct klvanc_context_s *ctx, void *p);
int parse_SMPTE_12_2(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr,
		     void **pp);

/* core-packet-smpte_2108_1.c */
int dump_SMPTE_2108_1(struct klvanc_context_s *ctx, void *p);
int parse_SMPTE_2108_1(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *hdr,
		       void **pp);


/* We don't expect anything outside of the VANC framework to need toascii
 * call these, so we'll keep them private / internal calls.
 */
extern int  klvanc_cache_alloc(struct klvanc_context_s *ctx);
extern void klvanc_cache_free(struct klvanc_context_s *ctx);
extern int  klvanc_cache_update(struct klvanc_context_s *ctx,
				struct klvanc_packet_header_s *pkt);

/* Logging Macros */
#define PRINT_ERR(...) if (ctx->log_cb) ctx->log_cb(NULL, LIBKLVANC_LOGLEVEL_ERR, __VA_ARGS__);
#define PRINT_DEBUG(...) if (ctx->log_cb) ctx->log_cb(NULL, LIBKLVANC_LOGLEVEL_DEBUG, __VA_ARGS__);
#define PRINT_DEBUG_MEMBER_INT(m) if (ctx->log_cb) ctx->log_cb(NULL, LIBKLVANC_LOGLEVEL_DEBUG, " %s = 0x%x\n", #m, m);
#define PRINT_DEBUG_MEMBER_INTI(m, n) if (ctx->log_cb) ctx->log_cb(NULL, LIBKLVANC_LOGLEVEL_DEBUG, "%*c%s = 0x%x\n", n, ' ', #m, m);
#define PRINT_DEBUG_MEMBER_INT64(m) if (ctx->log_cb) ctx->log_cb(NULL, LIBKLVANC_LOGLEVEL_DEBUG, " %s = 0x%" PRIx64 "\n", #m, m);

#endif
