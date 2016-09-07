/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_PAYLOAD_INFORMATION_H
#define _VANC_PAYLOAD_INFORMATION_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

enum payload_aspect_ratio_e
{
	ASPECT_UNDEFINED = 0,
	ASPECT_4x3,
	ASPECT_16x9,
};

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

struct packet_payload_information_s
{
	struct packet_header_s hdr;
	enum payload_aspect_ratio_e aspectRatio;
	enum payload_afd_e afd;
	unsigned short barDataValue[2];
	unsigned char barDataFlags;
};

const char *afd_to_string(enum payload_afd_e afd);
const char *aspectRatio_to_string(enum payload_aspect_ratio_e ar);
int dump_PAYLOAD_INFORMATION(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_PAYLOAD_INFORMATION_H */
