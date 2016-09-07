/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_H
#define _VANC_H

#include <stdint.h>
#include <sys/errno.h>
#include <sys/errno.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A series of callbacks used by the VANC library to inform user applications
 * that certain messages have been fully decoded.
 */
struct packet_payload_information_s;
struct packet_eia_708b_s;
struct packet_eia_608_s;
struct vanc_context_s;
struct vanc_callbacks_s
{
	int (*payload_information)(void *user_context, struct vanc_context_s *, struct packet_payload_information_s *);
	int (*eia_708b)(void *user_context, struct vanc_context_s *, struct packet_eia_708b_s *);
	int (*eia_608)(void *user_context, struct vanc_context_s *, struct packet_eia_608_s *);
};

/* Application specific context, the library allocates and stores user specific instance
 * information.
 */
struct vanc_context_s
{
	int verbose;
	struct vanc_callbacks_s *callbacks;
	void *callback_context;

	/* Internal use by the library */
	void *priv;
};

/* Create or destroy some basic application/library context */
int vanc_context_create(struct vanc_context_s **ctx);
int vanc_context_destroy(struct vanc_context_s *ctx);
int vanc_context_dump(struct vanc_context_s *ctx);

/* Parse a line of payload, trigger callbacks as necessary. lineNr is passed around and only
 * used for reporting purposes, so we can figure out which line this came from in different
 * callbacks and various parts of the reporting stack.
 */
int vanc_packet_parse(struct vanc_context_s *ctx, unsigned int lineNr, unsigned short *arr, unsigned int len);
void klvanc_dump_words_console(uint16_t *vanc, int maxlen, unsigned int linenr, int onlyvalid);

#include <libklvanc/vanc-module1.h>
#include <libklvanc/vanc-payload_information.h>
#include <libklvanc/vanc-eia_708b.h>
#include <libklvanc/vanc-eia_608.h>
#include <libklvanc/did.h>
#include <libklvanc/pixels.h>

#ifdef __cplusplus
};
#endif

#endif /* _VANC_H */
