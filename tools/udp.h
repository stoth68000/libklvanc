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

#ifndef ISO13818_H
#define ISO13818_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#define SOCKET int
#endif
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*tsudp_receiver_callback)(void *userContext, unsigned char *buf, int byteCount);
struct iso13818_udp_receiver_s {
	SOCKET skt;

	struct sockaddr_in sin;
	unsigned int ip_port;
	char ip_addr[32];
	int stripRTPHeader;

	unsigned char *rxbuffer;
	unsigned int rxbuffer_size;

	pthread_t threadId;
	int thread_running;
	int thread_terminate;
	int thread_complete;

	tsudp_receiver_callback cb;
	void *userContext;

	/* Debug dumping to disk */
	pthread_mutex_t fh_mutex;
	FILE *fh;
};

int iso13818_udp_receiver_alloc(struct iso13818_udp_receiver_s **p,
        unsigned int socket_buffer_size,
        const char *ip_addr,
        unsigned short ip_port,
        tsudp_receiver_callback cb,
        void *userContext,
	int stripRTPHeader);
void iso13818_udp_receiver_free(struct iso13818_udp_receiver_s **p);
ssize_t iso13818_udp_receiver_read(struct iso13818_udp_receiver_s *ctx, unsigned char *buf, unsigned int byteCount);
int iso13818_udp_receiver_thread_start(struct iso13818_udp_receiver_s *ctx);

/* Add or remove a specific network interface from the receiver, if its a multicast address */
int  iso13818_udp_receiver_join_multicast(struct iso13818_udp_receiver_s *p, char *ifname);
int  iso13818_udp_receiver_drop_multicast(struct iso13818_udp_receiver_s *p, char *ifname);

#ifdef __cplusplus
};
#endif
#endif /* ISO13818_H */
