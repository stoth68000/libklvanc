/*
 * Copyright (c) 2017 Kernel Labs Inc. All Rights Reserved
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

#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>

/* Maintain a static array of VANC messages, so that at any given time,
 * a user may ask "what message types have I seen on what lines?".
 */

int vanc_cache_alloc(struct vanc_context_s *ctx)
{
	ctx->cacheLines = calloc(0x10000, sizeof(struct vanc_cache_s));
    if (!ctx->cacheLines)
        return -1;

    return 0;
}

void vanc_cache_free(struct vanc_context_s *ctx)
{
    /* Free any cached lines otherwise we'll memory leak. */
    vanc_cache_reset(ctx);

    if (ctx->cacheLines) {
	    free(ctx->cacheLines);
        ctx->cacheLines = 0;
    }
}

struct vanc_cache_s * vanc_cache_lookup(struct vanc_context_s *ctx, uint8_t didnr, uint8_t sdidnr)
{
    if (!ctx)
        return NULL;
    if (!ctx->cacheLines)
        return NULL;

    return &ctx->cacheLines[ (((didnr) << 8) | (sdidnr)) ];
};

int vanc_cache_update(struct vanc_context_s *ctx, struct packet_header_s *pkt)
{
    if (!ctx)
        return -1;
    if (!ctx->cacheLines)
        return -1;
	if (pkt->did > 0xff)
		return -1;
	if (pkt->dbnsdid > 0xff)
		return -1;
	if (pkt->lineNr >= 2048)
		return -1;

	struct vanc_cache_s *s = vanc_cache_lookup(ctx, pkt->did, pkt->dbnsdid);
	if (s->activeCount == 0) {
		s->did = pkt->did;
		s->sdid = pkt->dbnsdid;
		s->desc = vanc_lookupDescriptionByType(pkt->type);
		s->spec = vanc_lookupSpecificationByType(pkt->type);
	}
	gettimeofday(&s->lastUpdated, NULL);

	struct vanc_cache_line_s *line = &s->lines[ pkt->lineNr ];
	line->active = 1;
	s->activeCount++;

	pthread_mutex_lock(&line->mutex);
	if (line->pkt) {
		vanc_packet_free(line->pkt);
		line->pkt = 0;
	}
	vanc_packet_copy(&line->pkt, pkt);
	pthread_mutex_unlock(&line->mutex);

	line->count++;

	return 0;
}

void vanc_cache_reset(struct vanc_context_s *ctx)
{
    if (!ctx)
        return;
    if (!ctx->cacheLines)
        return;

	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct vanc_cache_s *e = vanc_cache_lookup(ctx, d, s);
			e->activeCount = 0;

			for (int l = 0; l < 2048; l++) {
				struct vanc_cache_line_s *line = &e->lines[ l ];
				if (!line->active)
					continue;

				line->active = 0;
				line->count = 0;

				pthread_mutex_lock(&line->mutex);
				if (line->pkt) {
					vanc_packet_free(line->pkt);
					line->pkt = 0;
				}
				pthread_mutex_unlock(&line->mutex);
			}
		}
	}
}
