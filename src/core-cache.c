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

#define KLVANC_CACHE_ENTRIES 0x10000

int klvanc_cache_alloc(struct klvanc_context_s *ctx)
{
	struct klvanc_cache_s *lines = calloc(KLVANC_CACHE_ENTRIES, sizeof(struct klvanc_cache_s));
	if (!lines)
		return -1;

	/* Every entry's mutex must be pthread_mutex_init()'d before use --
	   calloc() only zero-fills the memory, which is not a valid
	   pthread_mutex_t on every platform (confirmed: pthread_mutex_lock()
	   on a zeroed-but-never-initialized mutex returns EINVAL rather than
	   actually locking anything). If any individual init fails partway
	   through, unwind the ones already initialized rather than leaving a
	   partially-initialized array behind. */
	int i;
	for (i = 0; i < KLVANC_CACHE_ENTRIES; i++) {
		if (pthread_mutex_init(&lines[i].mutex, NULL) != 0)
			break;
	}
	if (i != KLVANC_CACHE_ENTRIES) {
		for (int j = 0; j < i; j++)
			pthread_mutex_destroy(&lines[j].mutex);
		free(lines);
		return -1;
	}

	ctx->cacheLines = lines;
	return 0;
}

void klvanc_cache_free(struct klvanc_context_s *ctx)
{
	if (!ctx->cacheLines)
		return;

	/* Free any cached lines otherwise we'll memory leak. */
	klvanc_cache_reset(ctx);

	for (int i = 0; i < KLVANC_CACHE_ENTRIES; i++)
		pthread_mutex_destroy(&ctx->cacheLines[i].mutex);

	free(ctx->cacheLines);
	ctx->cacheLines = 0;
}

struct klvanc_cache_s * klvanc_cache_lookup(struct klvanc_context_s *ctx, uint8_t didnr, uint8_t sdidnr)
{
    if (!ctx)
        return NULL;
    if (!ctx->cacheLines)
        return NULL;

    return &ctx->cacheLines[ (((didnr) << 8) | (sdidnr)) ];
};

int klvanc_cache_update(struct klvanc_context_s *ctx, struct klvanc_packet_header_s *pkt)
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

	struct klvanc_cache_s *s = klvanc_cache_lookup(ctx, pkt->did, pkt->dbnsdid);

	/* Every field touched below (including each line's active/count/pkt)
	   is shared, mutable state that klvanc_cache_lookup() hands a raw
	   pointer to -- take the entry's mutex for the whole update, not
	   just the pkt copy/free, so a concurrent reader/updater never
	   observes a half-updated entry. */
	pthread_mutex_lock(&s->mutex);

	if (s->activeCount == 0) {
		s->did = pkt->did;
		s->sdid = pkt->dbnsdid;
		s->desc = klvanc_didLookupDescription(pkt->did, pkt->dbnsdid);
		s->spec = klvanc_didLookupSpecification(pkt->did, pkt->dbnsdid);
	}
	gettimeofday(&s->lastUpdated, NULL);

	struct klvanc_cache_line_s *line = &s->lines[ pkt->lineNr ];
	line->active = 1;
	s->activeCount++;

	if (line->pkt) {
		klvanc_packet_free(line->pkt);
		line->pkt = 0;
	}
	klvanc_packet_copy(&line->pkt, pkt);
	line->count++;

	pthread_mutex_unlock(&s->mutex);

	return 0;
}

void klvanc_cache_reset(struct klvanc_context_s *ctx)
{
	if (!ctx)
		return;
	if (!ctx->cacheLines)
		return;

	for (int d = 0; d <= 0xff; d++) {
		for (int s = 0; s <= 0xff; s++) {
			struct klvanc_cache_s *e = klvanc_cache_lookup(ctx, d, s);

			pthread_mutex_lock(&e->mutex);

			if (e->activeCount == 0) {
				pthread_mutex_unlock(&e->mutex);
				continue;
			}
			e->activeCount = 0;

			for (int l = 0; l < 2048; l++) {
				struct klvanc_cache_line_s *line = &e->lines[ l ];
				if (!line->active)
					continue;

				line->active = 0;
				line->count = 0;

				if (line->pkt) {
					klvanc_packet_free(line->pkt);
					line->pkt = 0;
				}
			}

			pthread_mutex_unlock(&e->mutex);
		}
	}
}
