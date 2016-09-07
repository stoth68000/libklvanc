/* Copyright Kernel Labs Inc 2014-2016. All Rights Reserved. */

#ifndef _VANC_EIA_608_H
#define _VANC_EIA_608_H

#include <libklvanc/vanc-packets.h>

#ifdef __cplusplus
extern "C" {
#endif  

struct packet_eia_608_s
{
	struct packet_header_s hdr;
	int nr;
	unsigned char payload[3];

	/* Parsed */
	int marker_bits;
	int cc_valid;
	int cc_type;
	unsigned char cc_data_1;
	unsigned char cc_data_2;
};

int dump_EIA_608(struct vanc_context_s *ctx, void *p);

#ifdef __cplusplus
};
#endif  

#endif /* _VANC_EIA_608_H */
