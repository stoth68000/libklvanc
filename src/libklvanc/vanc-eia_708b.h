/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_EIA_708B_H
#define _VANC_EIA_708B_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

struct packet_eia_708b_s
{
	struct packet_header_s hdr;
	int nr;
};

int dump_EIA_708B(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_EIA_708B_H */
