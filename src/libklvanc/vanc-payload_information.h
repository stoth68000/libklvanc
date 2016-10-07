/**
 * @file	vanc-payload_information.h
 * @author	Steven Toth <stoth@kernellabs.com>
 * @copyright	Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved.
 * @brief	TODO - Brief description goes here.
 */

#ifndef _VANC_PAYLOAD_INFORMATION_H
#define _VANC_PAYLOAD_INFORMATION_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

/**
 * @brief	TODO - Brief description goes here.
 */
enum payload_aspect_ratio_e
{
	ASPECT_UNDEFINED = 0,
	ASPECT_4x3,
	ASPECT_16x9,
};

/**
 * @brief	TODO - Brief description goes here.
 */
enum payload_afd_e
{
	AFD_UNDEFINED = 0,
	AFD_BOX_16x9_TOP,
	AFD_BOX_14x9_TOP,
	AFD_BOX_16x9_CENTER,
	AFD_FULL_FRAME,
	AFD_16x9_CENTER,
	AFD_14x9_CENTER,
	AFD_4x3_WITH_ALTERNATIVE_14x9_CENTER,
	AFD_16x9_WITH_ALTERNATIVE_14x9_CENTER,
	AFD_16x9_WITH_ALTERNATIVE_4x3_CENTER,
};

/**
 * @brief	TODO - Brief description goes here.
 */
struct packet_payload_information_s
{
	struct packet_header_s hdr;
	enum payload_aspect_ratio_e aspectRatio;
	enum payload_afd_e afd;
	unsigned short barDataValue[2];
	unsigned char barDataFlags;
};

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum payload_afd_e afd - Brief description goes here.
 */
const char *afd_to_string(enum payload_afd_e afd);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	enum payload_aspect_ratio_e ar - Brief description goes here.
 */
const char *aspectRatio_to_string(enum payload_aspect_ratio_e ar);

/**
 * @brief	TODO - Brief description goes here.
 * @param[in]	struct vanc_context_s *ctx, void *p - Brief description goes here.
 */
int dump_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_PAYLOAD_INFORMATION_H */
