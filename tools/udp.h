/* Copyright Kernel Labs Inc 2015-2016. All Rights Reserved. */

#ifndef ISO13818_H
#define ISO13818_H

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*tsudp_receiver_callback)(void *userContext, unsigned char *buf, int byteCount);
struct iso13818_udp_receiver_s
{
	int skt;

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
