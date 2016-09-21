/* Copyright Kernel Labs Inc 2015, 2016 */

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
