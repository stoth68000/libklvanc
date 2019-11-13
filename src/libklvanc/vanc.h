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
#include <stdarg.h>
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
struct klvanc_packet_header_s;

/**
 * @brief	TODO - Brief description goes here.
 */
struct klvanc_packet_afd_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_eia_708b_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_eia_608_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_scte_104_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_sdp_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_context_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_kl_u64le_counter_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_packet_smpte_12_2_s;

/**
 * @brief       TODO - Brief description goes here.
 */
struct klvanc_callbacks_s
{
	int (*afd)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_afd_s *);
	int (*eia_708b)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_eia_708b_s *);
	int (*eia_608)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_eia_608_s *);
	int (*scte_104)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_scte_104_s *);
	int (*all)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_header_s *);
	int (*kl_i64le_counter)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_kl_u64le_counter_s *);
	int (*sdp)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_sdp_s *);
	int (*smpte_12_2)(void *user_context, struct klvanc_context_s *, struct klvanc_packet_smpte_12_2_s *);
};

struct klvanc_cache_s;

/**
 * @brief       Application specific context, the library allocates and stores user specific instance
 *		        information.
 */
struct klvanc_context_s
{
	int verbose;
	int allow_bad_checksums; /*!< defaults to false. If true, any frames with bad CRCS are not ignored, they are passed via callback to application. */
	struct klvanc_callbacks_s *callbacks;
	void *callback_context;
	int warn_on_decode_failure; /*!< defaults to false. If true, framework will warn every 60 seconds when it discovered an unsupported DID. */

	void (*log_cb)(void *p, int level, const char *fmt, ...);

	/* Internal use by the library */
	void *priv;
	struct klrestricted_code_path_block_s rcp_failedToDecode;
	unsigned int checksum_failures;

	/* Optional: A cache of VANC lines we've detected in the stream.
	 * see klvanc_context_enable_monitor().
	 * A static array of structs, for did/sdid rapid lookups.
	 * Where DD and SD range from 00..FF.
	 * This contains an array of structs, each containing a set of lines,
	 * optimized for update/query. The structures are typically used by
	 * applications that want to keep tabs on what messages have been
	 * seen in the stream, per line. Its important to understand that
	 * for every message, we cache it, and the same message (same line)
	 * overwrites our previous cached message.
	 */
	struct klvanc_cache_s *cacheLines;
};

#define LIBKLVANC_LOGLEVEL_ERR 0
#define LIBKLVANC_LOGLEVEL_WARN 1
#define LIBKLVANC_LOGLEVEL_INFO 2
#define LIBKLVANC_LOGLEVEL_DEBUG 3

/**
 * @brief	Create or destroy some basic application/library context.\n
 *              The context is considered private and is likely to change.
 * @param[out]	struct klvanc_context_s **ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int klvanc_context_create(struct klvanc_context_s **ctx);

/**
 * @brief	Deallocate and destroy a context. See klvanc_context_create()
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int klvanc_context_destroy(struct klvanc_context_s *ctx);

/**
 * @brief	Generate user visible context details to the console.
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int klvanc_context_dump(struct klvanc_context_s *ctx);

/**
 * @brief	Parse a line of payload, trigger callbacks as necessary. lineNr is passed around and only\n
 *		used for reporting purposes, so we can figure out which line this came from in different\n
 *		callbacks and various parts of the reporting stack.
 * @param[in]	struct klvanc_context_s *ctx - Context.
 * @param[in]	unsigned int lineNr - SDI line number the array data came from. Used for information / tracking purposes only.
 * @param[in]	unsigned short *words - Array of SDI words (10bit) that the caller wants parsed.
 * @param[in]	unsigned int wordCount - Number of words in array.
 * @return      0 - Success
 * @return      < 0 - Error
 */
int klvanc_packet_parse(struct klvanc_context_s *ctx, unsigned int lineNr, unsigned short *words, unsigned int wordCount);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	uint16_t *array - Array of SDI words (10bit) that the caller wants parsed.
 * @param[in]	int len - Number of words in array.
 * @param[in]	unsigned int linenr - SDI line number the array data came from. Used for information / tracking purposes only.
 * @param[in]	int onlyvalid - Brief description goes here.
 */
void klvanc_dump_words_console(struct klvanc_context_s *ctx, uint16_t *vanc, int maxlen, unsigned int linenr, int onlyvalid);

#include <libklvanc/vanc-afd.h>
#include <libklvanc/vanc-eia_708b.h>
#include <libklvanc/vanc-eia_608.h>
#include <libklvanc/vanc-scte_104.h>
#include <libklvanc/vanc-smpte_12_2.h>
#include <libklvanc/did.h>
#include <libklvanc/pixels.h>
#include <libklvanc/vanc-checksum.h>
#include <libklvanc/smpte2038.h>
#include <libklvanc/cache.h>
#include <libklvanc/vanc-kl_u64le_counter.h>
#include <libklvanc/vanc-sdp.h>

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
int klvanc_sdi_create_payload(uint8_t sdid, uint8_t did,
	const uint8_t *src, uint16_t srcByteCount,
	uint16_t **dst, uint16_t *dstWordCount,
	uint32_t bitDepth);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum packet_type_e type
 * @return	TODO.
 */
const char *klvanc_lookupDescriptionByType(enum klvanc_packet_type_e type);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum packet_type_e type
 * @return	TODO.
 */
const char *klvanc_lookupSpecificationByType(enum klvanc_packet_type_e type);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct packet_header_s **dst
 * @param[in]	struct packet_header_s *src
 * @return	TODO.
 */
int klvanc_packet_copy(struct klvanc_packet_header_s **dst,
		       struct klvanc_packet_header_s *src);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct packet_header_s *src
 */
void klvanc_packet_free(struct klvanc_packet_header_s *src);

#ifdef __cplusplus
};
#endif

#endif /* _VANC_H */
