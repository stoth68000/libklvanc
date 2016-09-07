/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_SCTE_104_H
#define _VANC_SCTE_104_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

struct packet_scte_104_s
{
	struct packet_header_s hdr;
	int nr;
};

int dump_SCTE_104(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_SCTE_104_H */
