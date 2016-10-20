/*
 * Copyright (c) 2016 Kernel Labs Inc. All Rights Reserved
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

#ifndef ISO13818_URL_H
#define ISO13818_URL_H

#ifdef __cplusplus
extern "C" {
#endif

/* udp://hostname:port/?ifname=eth0&fifosize=1048576 */
struct url_opts_s
{
        char url[256];

        char protocol[16];
        enum protocol_e {
                P_UNDEFINED = 0,
                P_UDP,
                P_RTP
        } protocol_type;

	/* Whether we think hostname contains a pure ip address */
        int has_ipaddress;
        char hostname[64];

	/* 1-65535 */
        int has_port;
        int port;

#if 0
        int has_buffersize;
        int buffersize;
#endif

	/* socket buffer rx/tx size (bytes) */
        int has_fifosize;
        int fifosize;

        int has_ifname;
        char ifname[16];
};

void url_print(struct url_opts_s *url);
int  url_parse(const char *url, struct url_opts_s **result);
void url_free(struct url_opts_s *arg);

#ifdef __cplusplus
};
#endif

#endif /* ISO13818_URL_H */
