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

void klvanc_dump_packet_console(struct vanc_context_s *ctx, struct packet_header_s *hdr);
#endif
