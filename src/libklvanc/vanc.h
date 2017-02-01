/*
 * Copyright (c) 2016-2017 Kernel Labs Inc. All Rights Reserved
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

/**
 * @file	vanc.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	Parse VANC lines, interpret data, call user callbacks with populated structures.\n
 *              Callers allocate a usage context, assign callbacks to this context and then feed\n
 *              VANC data into the library. VANC messages that are understood are parse, converted\n
 *              into structs and if the user has registered a callback for that specific message type\n
 *              then the structure is passed via the callback.\n\n
 *              Callbacks should not attempt to modify or release the callback structs, releasing of\n
 *              memory allocats is automatically taken care of by this library.
 */

#ifndef _VANC_H
#define _VANC_H

#include <stdint.h>
#include <sys/errno.h>
#include <sys/errno.h>
#include <libklvanc/klrestricted_code_path.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A series of callbacks used by the VANC library to inform user applications
 * that certain messages have been fully decoded.
 */

/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_header_s;

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
	int (*all)(void *user_context, struct vanc_context_s *, struct packet_header_s *);
};

struct vanc_cache_s;

/**
 * @brief       Application specific context, the library allocates and stores user specific instance
 *		        information.
 */
struct vanc_context_s
{
	int verbose;
	struct vanc_callbacks_s *callbacks;
	void *callback_context;

	/* Internal use by the library */
	void *priv;
	struct klrestricted_code_path_block_s rcp_failedToDecode;

	/* Optional: A cache of VANC lines we've detected in the stream.
	 * see vanc_context_enable_monitor().
	 * A static array of structs, for did/sdid rapid lookups.
	 * Where DD and SD range from 00..FF.
	 * This contains an array of structs, each containing a set of lines,
	 * optimized for update/query. The structures are typically used by
	 * applications that want to keep tabs on what messages have been
	 * seen in the stream, per line. Its important to understand that
	 * for every message, we cache it, and the same message (same line)
	 * overwrites our previous cached message.
	 */
	struct vanc_cache_s *cacheLines;
};

/**
 * @brief	Create or destroy some basic application/library context.\n
 *              The context is considered private and is likely to change.
 * @param[out]	struct vanc_context_s **ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int vanc_context_create(struct vanc_context_s **ctx);

/**
 * @brief	Deallocate and destroy a context. See vanc_context_create()
 * @param[in]	struct vanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int vanc_context_destroy(struct vanc_context_s *ctx);

/**
 * @brief	Generate user visible context details to the console.
 * @param[in]	struct vanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int vanc_context_dump(struct vanc_context_s *ctx);

/**
 * @brief	Parse a line of payload, trigger callbacks as necessary. lineNr is passed around and only\n
 *		used for reporting purposes, so we can figure out which line this came from in different\n
 *		callbacks and various parts of the reporting stack.
 * @param[in]	struct vanc_context_s *ctx - Context.
 * @param[in]	unsigned int lineNr - SDI line number the array data came from. Used for information / tracking purposes only.
 * @param[in]	unsigned short *words - Array of SDI words (10bit) that the caller wants parsed.
 * @param[in]	unsigned int wordCount - Number of words in array.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int vanc_packet_parse(struct vanc_context_s *ctx, unsigned int lineNr, unsigned short *words, unsigned int wordCount);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t *array - Array of SDI words (10bit) that the caller wants parsed.
 * @param[in]	int len - Number of words in array.
 * @param[in]	unsigned int linenr - SDI line number the array data came from. Used for information / tracking purposes only.
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
#include <libklvanc/cache.h>

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
 * @return      0 - Success
 * @return      < 0 - Error
 */
int vanc_sdi_create_payload(uint8_t sdid, uint8_t did,
	const uint8_t *src, uint16_t srcByteCount,
	uint16_t **dst, uint16_t *dstWordCount,
	uint32_t bitDepth);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum packet_type_e type
 * @return	TODO.
 */
const char *vanc_lookupDescriptionByType(enum packet_type_e type);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum packet_type_e type
 * @return	TODO.
 */
const char *vanc_lookupSpecificationByType(enum packet_type_e type);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct packet_header_s **dst
 * @param[in]	struct packet_header_s *src
 * @return	TODO.
 */
int vanc_packet_copy(struct packet_header_s **dst, struct packet_header_s *src);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct packet_header_s *src
 */
void vanc_packet_free(struct packet_header_s *src);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_H */
