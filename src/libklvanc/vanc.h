/**
 * @file	vanc.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

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
/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_payload_information_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct packet_eia_708b_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct packet_eia_608_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct packet_scte_104_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct vanc_context_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct vanc_callbacks_s
{
	int (*payload_information)(void *user_context, struct vanc_context_s *, struct packet_payload_information_s *);
	int (*eia_708b)(void *user_context, struct vanc_context_s *, struct packet_eia_708b_s *);
	int (*eia_608)(void *user_context, struct vanc_context_s *, struct packet_eia_608_s *);
	int (*scte_104)(void *user_context, struct vanc_context_s *, struct packet_scte_104_s *);
};

/**
 * @brief       Application specific context, the library allocates and stores user specific instance
 *		information.
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
/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s **ctx - Brief description goes here.
 */
int vanc_context_create(struct vanc_context_s **ctx);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx - Brief description goes here.
 */
int vanc_context_destroy(struct vanc_context_s *ctx);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx - Brief description goes here.
 */
int vanc_context_dump(struct vanc_context_s *ctx);

/**
 * @brief	Parse a line of payload, trigger callbacks as necessary. lineNr is passed around and only
 *		used for reporting purposes, so we can figure out which line this came from in different
 *		callbacks and various parts of the reporting stack.
 * @param[in]	struct vanc_context_s *ctx - Brief description goes here.
 * @param[in]	unsigned int lineNr - Brief description goes here.
 * @param[in]	unsigned short *arr - Brief description goes here.
 * @param[in]	unsigned int len - Brief description goes here.
 */
int vanc_packet_parse(struct vanc_context_s *ctx, unsigned int lineNr, unsigned short *arr, unsigned int len);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t *vanc - Brief description goes here.
 * @param[in]	int maxlen - Brief description goes here.
 * @param[in]	unsigned int linenr - Brief description goes here.
 * @param[in]	int onlyvalid - Brief description goes here.
 */
void vanc_dump_words_console(uint16_t *vanc, int maxlen, unsigned int linenr, int onlyvalid);

#include <libklvanc/vanc-module1.h>
#include <libklvanc/vanc-payload_information.h>
#include <libklvanc/vanc-eia_708b.h>
#include <libklvanc/vanc-eia_608.h>
#include <libklvanc/vanc-scte_104.h>
#include <libklvanc/did.h>
#include <libklvanc/pixels.h>
#include <libklvanc/vanc-checksum.h>
#include <libklvanc/smpte2038.h>

/**
 * @brief	Take an array of payload, create a fully formed VANC message.
 *		bitDepth of 10 is the only valid input value.
 *		did: 0x41, sdid: 0x07 = SCTE104
 *		generateParity = 1/0
 * @param[in]	uint8_t sdid, uint8_t did - Brief description goes here.
 * @param[in]	const uint8_t *src - Brief description goes here.
 * @param[in]	uint16_t srcByteCount - Brief description goes here.
 * @param[in]	uint16_t **dst - Brief description goes here.
 * @param[in]	uint16_t *dstWordCount - Brief description goes here.
 * @param[in]	uint32_t bitDepth - Brief description goes here.
 */
int vanc_sdi_create_payload(uint8_t sdid, uint8_t did,
	const uint8_t *src, uint16_t srcByteCount,
	uint16_t **dst, uint16_t *dstWordCount,
	uint32_t bitDepth);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_H */
