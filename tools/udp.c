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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>
#include "udp.h"

/* Compilation issues on centos, trouble headers won't include
 *  * even with reasonable #defines.
 *   */
#ifndef NI_MAXHOST
#define NI_MAXHOST 1025
#endif
#ifndef NI_NUMERICHOST
#define NI_NUMERICHOST 0x01
#endif

/* UDP Receiver ... */

static int modifyMulticastInterfaces(int skt, struct sockaddr_in *sin, char *ipaddr, unsigned short port, int option, char *ifname)
{
	/* Setup multicast on all IPV4 network interfaces, IPV6 interfaces are ignored */
	struct ifaddrs *addrs;
	int result = getifaddrs(&addrs);
	int didModify = 0;
	if (result >= 0) {
		const struct ifaddrs *cursor = addrs;
		while (cursor != NULL) {
			if (cursor->ifa_addr && (cursor->ifa_flags & IFF_BROADCAST) && (cursor->ifa_flags & IFF_UP) &&
				(cursor->ifa_addr->sa_family == AF_INET)) {

				char host[NI_MAXHOST];

				int r = getnameinfo(cursor->ifa_addr,
					cursor->ifa_addr->sa_family == AF_INET ? sizeof(struct sockaddr_in) : sizeof(struct sockaddr_in6),
					host, NI_MAXHOST,
					NULL, 0, NI_NUMERICHOST);
				if (r == 0) {
#if 0
					printf("ifa_name:%s flags:%x host:%s\n", cursor->ifa_name, cursor->ifa_flags, host);
#endif
					int shouldModify = 1;

					if (ifname && (strcasecmp(ifname, cursor->ifa_name) != 0))
						shouldModify = 0;

					if (shouldModify) {

						/* Enable multicast reception on this specific interface */
						/* join multicast group */
						struct ip_mreq mreq;
						mreq.imr_multiaddr.s_addr = sin->sin_addr.s_addr;
						mreq.imr_interface.s_addr = ((struct sockaddr_in *)cursor->ifa_addr)->sin_addr.s_addr;
						if (setsockopt(skt, IPPROTO_IP, option, (void *)&mreq, sizeof(mreq)) < 0) {
							fprintf(stderr, "%s() cannot %s multicast group %s on iface %s\n", __func__, ipaddr,
								option == IP_ADD_MEMBERSHIP ? "join" : "leave",
								cursor->ifa_name
								);
							freeifaddrs(addrs);
							return -1;
						} else {
							didModify++;
							fprintf(stderr, "%s() %s multicast group %s ok on iface %s\n", __func__, ipaddr,
								option == IP_ADD_MEMBERSHIP ? "join" : "leave",
								cursor->ifa_name
								);
						}
					}
				}
			}
			cursor = cursor->ifa_next;
		}
		freeifaddrs(addrs);
	}

	if (didModify)
		return 0; /* Success */
	else
		return -1;
}

int iso13818_udp_receiver_alloc(struct iso13818_udp_receiver_s **p,
	unsigned int socket_buffer_size,
	const char *ip_addr,
	unsigned short ip_port,
	tsudp_receiver_callback cb,
	void *userContext,
	int stripRTPHeader)
{
	if (!ip_addr)
		return -1;

	struct iso13818_udp_receiver_s *ctx = (struct iso13818_udp_receiver_s *)calloc(1, sizeof(*ctx));
	if (!ctx)
		return -ENOMEM;

	ctx->ip_port = ip_port;
	ctx->rxbuffer_size = 2048;
	ctx->threadId = 0;
	ctx->thread_running = ctx->thread_terminate = ctx->thread_complete = 0;
	strncpy(ctx->ip_addr, ip_addr, sizeof(ctx->ip_addr) - 1);
	ctx->cb = cb;
	ctx->userContext = userContext;
	ctx->stripRTPHeader = stripRTPHeader;

	/* Create the UDP discover socket */
	ctx->skt = socket(AF_INET, SOCK_DGRAM, 0);
	if (ctx->skt < 0) {
		free(ctx);
		return -1;
	}

	int n = socket_buffer_size;
	if (setsockopt(ctx->skt, SOL_SOCKET, SO_RCVBUF, &n, sizeof(n)) == -1) {
		perror("so_rcvbuf");
		close(ctx->skt);
		free(ctx);
		return -1;
	}

	int reuse = 1;
	if (setsockopt(ctx->skt, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
		close(ctx->skt);
		free(ctx);
		return -1;
	}

	ctx->sin.sin_family = AF_INET;
	ctx->sin.sin_port = htons(ctx->ip_port);
	ctx->sin.sin_addr.s_addr = inet_addr(ctx->ip_addr);
	if (bind(ctx->skt, (struct sockaddr *)&ctx->sin, sizeof(ctx->sin)) < 0) {
		perror("bind");
		close(ctx->skt);
		free(ctx);
		return -1;
	}

	if (IN_MULTICAST(ntohl(ctx->sin.sin_addr.s_addr))) {
		//modifyMulticastInterfaces(ctx->skt, &ctx->sin, ctx->ip_addr, ctx->ip_port, IP_ADD_MEMBERSHIP, 0);
	}

	/* Non-blocking required */
	int fl = fcntl(ctx->skt, F_GETFL, 0);
	if (fcntl(ctx->skt, F_SETFL, fl | O_NONBLOCK) < 0) {
		perror("fcntl");
		close(ctx->skt);
		free(ctx);
		return -1;
	}

	ctx->rxbuffer = malloc(ctx->rxbuffer_size);
	if (!ctx->rxbuffer) {
		close(ctx->skt);
		free(ctx);
		return -1;
	}

	pthread_mutex_init(&ctx->fh_mutex, NULL);
	*p = ctx;
	return 0;
}

void iso13818_udp_receiver_free(struct iso13818_udp_receiver_s **p)
{
	if (!p || !*p)
		return;

	struct iso13818_udp_receiver_s *ctx = (struct iso13818_udp_receiver_s *)*p;

	ctx->thread_terminate = 1;
	if (ctx->threadId) {
		/* Join rather than busy-poll on thread_complete: reading
		   thread_running here race against the new thread's very first
		   statement (thread_running = 1), so a free() that lands between
		   pthread_create() returning and that first store would see
		   thread_running == 0 and skip waiting entirely, then proceed to
		   close the socket and free ctx while the thread is still
		   starting up and about to touch it -- a use-after-free. Joining
		   on threadId (which is set synchronously by thread_start()
		   before this can run) doesn't have that race. */
		pthread_join(ctx->threadId, NULL);
	}

	if (ctx->skt != -1) {
		if (IN_MULTICAST(ntohl(ctx->sin.sin_addr.s_addr)))
			modifyMulticastInterfaces(ctx->skt, &ctx->sin, ctx->ip_addr, ctx->ip_port, IP_DROP_MEMBERSHIP, 0);
		close(ctx->skt);
	}

	pthread_mutex_destroy(&ctx->fh_mutex);
	free(ctx->rxbuffer);
	free(ctx);
	*p = 0;
}

int iso13818_udp_receiver_join_multicast(struct iso13818_udp_receiver_s *ctx, char *ifname)
{
	/* assert() compiles out entirely under NDEBUG, silently turning a
	   missing-argument bug into a NULL deref instead of a controlled
	   error -- these are trust-boundary checks, not internal invariants. */
	if (!ctx || !ifname)
		return -1;
	if (!IN_MULTICAST(ntohl(ctx->sin.sin_addr.s_addr)))
		return -1;

	return modifyMulticastInterfaces(ctx->skt, &ctx->sin, ctx->ip_addr, ctx->ip_port, IP_ADD_MEMBERSHIP, ifname);
}

int iso13818_udp_receiver_drop_multicast(struct iso13818_udp_receiver_s *ctx, char *ifname)
{
	if (!ctx || !ifname)
		return -1;
	if (!IN_MULTICAST(ntohl(ctx->sin.sin_addr.s_addr)))
		return -1;

	return modifyMulticastInterfaces(ctx->skt, &ctx->sin, ctx->ip_addr, ctx->ip_port, IP_DROP_MEMBERSHIP, ifname);
}

static void *udp_receiver_threadfunc(void *p)
{
	struct iso13818_udp_receiver_s *ctx = (struct iso13818_udp_receiver_s *)p;

	ctx->thread_running = 1;
	while (!ctx->thread_terminate) {
		struct pollfd fds = { .fd = ctx->skt, .events = POLLIN, .revents = 0 };

		/* Wait for up to 250ms on a timeout based on socket activity */
		int ret = poll(&fds, 1, 250);
		if (ret == 0) {
			/* Timeout */
			continue;
		}

		if (ret < 0) {
			/* Unspecified error */
			continue;
		}

		/* Ret > 0, meaning our FD returned data is available. */

		/* Push the arbitrary buffer of bytes, output is fully aligned
		 * packets via the tool_realign_callback callback, which are
		 * then pushed directly into the core.
		 */
		/* MSG_TRUNC makes recv() return the full datagram length even when
		   it exceeds rxbuffer_size, so truncation (a datagram larger than
		   our buffer, silently dropping bytes we'd otherwise process as
		   if complete) can be detected instead of processing partial
		   packet data unknowingly. */
		ssize_t rxbytes = recv(ctx->skt, ctx->rxbuffer, ctx->rxbuffer_size, MSG_TRUNC);
		if (rxbytes < 0) {
			continue;
		}
		if ((size_t)rxbytes > ctx->rxbuffer_size) {
			fprintf(stderr, "%s() truncated datagram of %zd bytes exceeds rxbuffer_size %u, dropping\n",
				__func__, rxbytes, ctx->rxbuffer_size);
			continue;
		}
		if (rxbytes && ctx->cb && !ctx->stripRTPHeader) {
			ctx->cb(ctx->userContext, ctx->rxbuffer, rxbytes);
		}
		else
		if (rxbytes && ctx->cb && ctx->stripRTPHeader) {
			/* Some implementations pad the trailer of the packet with
			 * dummy bytes, we don't want to pass these along.
			 * Hint: Ceton does, silicondust doesn't */
			int bytes = ((rxbytes - 12) / 188) * 188;
			ctx->cb(ctx->userContext, ctx->rxbuffer + 12, bytes);
		}

	}
	ctx->thread_complete = 1;
	ctx->thread_running = 0;
	pthread_exit(0);
}

int iso13818_udp_receiver_thread_start(struct iso13818_udp_receiver_s *ctx)
{
	if (!ctx || ctx->threadId != 0)
		return -1;
	return pthread_create(&ctx->threadId, 0, udp_receiver_threadfunc, ctx);
}

/* UDP Transmitter ... */
