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
#include <regex.h>

#include "url.h"

static int url_argsplit(char *arg, char **tag, char **value)
{
	if (!arg)
		return -1;

	/* Dup the string as we may want to modify it */
	char *tmp = calloc(1, 256);
	strcpy(tmp, arg);
	char *str = tmp;

	char *p = strsep(&str, "=");
	if (!p || !str) {
		free(tmp);
		return -2;
	}
	*tag = strdup(p);
	if (*tag == NULL) {
		free(tmp);
		return -1;
	}

	p = strsep(&str, "=");
	/* Strip any trailing args */
	for (int i = 0; i < strlen(p); i++) {
		if ((*(p + i) == '?') || (*(p + i) == '&')) {
			*(p + i) = 0;
			break;
		}
	}
	*value = strdup(p);
	if (*value == NULL) {
		free(tmp);
		return -1;
	}

	free(tmp);
	return 0;
}

static int regex_match(const char *str, const char *pattern)
{
	int ret = -1;
        regex_t rex;
	if (regcomp(&rex, pattern, REG_EXTENDED | REG_NOSUB | REG_ICASE) == 0) {

		if (regexec(&rex, str, 0, 0, 0) == REG_NOMATCH) {
			ret = -1;
		} else {
			ret = 0;
		}
	}

	regfree(&rex);
	return ret;
}

void url_print(struct url_opts_s *url)
{
	printf("url = %s\n", url->url);
	printf("\tprotocol = %s\n", url->protocol);
	printf("\tprotocol_type = %d [ %s ]\n",
		url->protocol_type,
		url->protocol_type == P_UDP ? "P_UDP" :
		url->protocol_type == P_RTP ? "P_RTP" : "UNDEFINED"
		);
	printf("\thostname = %s\n", url->hostname);
	printf("\thas_ipaddress = %d\n", url->has_ipaddress);
	if (url->has_port) {
		printf("\thas_port = %d\n", url->has_port);
		printf("\t\tport = %d\n", url->port);
	}
	if (url->has_ifname) {
		printf("\thas_ifname = %d\n", url->has_ifname);
		printf("\t\tifname = %s\n", url->ifname);
	}
#if 0
	if (url->has_buffersize) {
		printf("\thas_buffersize = %d\n", url->has_buffersize);
		printf("\t\tbuffersize = %d\n", url->buffersize);
	}
#endif
	if (url->has_fifosize) {
		printf("\thas_fifosize = %d\n", url->has_fifosize);
		printf("\t\tfifosize = %d\n", url->fifosize);
	}
}

static int has_url_argname(const char *url, const char *argname)
{
	char tmp[256];
	sprintf(tmp, "\\?%s=", argname);
	if (regex_match(url, tmp) < 0) {
		tmp[1] = '&';
		if (regex_match(url, tmp) < 0) {
			return 0;
		}
	}
	return 1;
}

int url_parse(const char *url, struct url_opts_s **result)
{
	int ret;
	int has_args = 0;
	struct url_opts_s *opts = calloc(1, sizeof(*opts));
	strncpy(opts->url, url, sizeof(opts->url) - 1);

	/* Check the protocol */
	if (regex_match(url, "^(udp|rtp)://") < 0) {
		ret = -1;
		goto err;
	}

	if (has_url_argname(url, "ifname") == 0) {
		opts->has_ifname = 0;
	} else {
		opts->has_ifname = 1;
		has_args++;
	}

#if 0
	if (has_url_argname(url, "buffersize") == 0) {
		opts->has_buffersize = 0;
	} else
		opts->has_buffersize = 1;
#endif

	if (has_url_argname(url, "fifosize") == 0) {
		opts->has_fifosize = 0;
	} else {
		opts->has_fifosize = 1;
		has_args++;
	}

	/* Validate the basic shape of the URL */
	if (regex_match(url, "://[0-9a-zA-Z].*:[0-9]*") < 0) {
		free(opts);
		return -1;
	}

        /* Clone the string into a tmp buffer as strsep will want to modify it */
        char tmp[256];
        memset(tmp, 0, sizeof(tmp));
        strncpy(tmp, url, sizeof(tmp) - 1);

        char *str = &tmp[0];
        char *p = strsep(&str, ":");
        if (!p || !str) {
		free(opts);
                return -1;
	}

        strncpy(opts->protocol, p, sizeof(opts->protocol) - 1);
	if (strcasecmp(opts->protocol, "udp") == 0)
		opts->protocol_type = P_UDP;
	else
	if (strcasecmp(opts->protocol, "rtp") == 0)
		opts->protocol_type = P_RTP;

        /* Skip the "//" after the : */
        if ((*(str + 0) == '/') && *(str + 1) == '/')
            str += 2;

        /* ip address */
        p = strsep(&str, ":");
        if (!p) {
		/* No hostname */
		free(opts);
                return -2;
	}

	if (regex_match(p, "^\[0-9].*.$") == 0)
		opts->has_ipaddress = 1;

        strncpy(opts->hostname, p, sizeof(opts->hostname) - 1);

        /* ip port */
        p = strsep(&str, ":");
        if (!p) {
		/* No port */
		free(opts);
                return -2;
	}

        opts->port = atoi(p);
	if (opts->port <= 65535)
		opts->has_port = 1;

	char tmp2[256];
	memset(tmp2, 0, sizeof(tmp));
	strncpy(tmp2, url, sizeof(tmp2) - 1);
	char *str2 = &tmp2[0];

	char *q = strsep(&str2, "?&");
	while (q && has_args) {

		q = strsep(&str2, "?&");
		if (!q)
			break;

		char *tag = 0, *value = 0;
		if (url_argsplit(q, &tag, &value) < 0) {
			printf("split error\n");
		}
                //printf("tag = [%s] = [%s]\n", tag, value);
		if (tag) {
			if (strcasecmp(tag, "ifname") == 0)
				strncpy(opts->ifname, value, sizeof(opts->ifname) - 1);
			else
#if 0
			if (strcasecmp(tag, "buffersize") == 0)
				opts->buffersize = atoi(value);
			else
#endif
			if (strcasecmp(tag, "fifosize") == 0)
				opts->fifosize = atoi(value);
			else {
				fprintf(stderr, "Unknown tag [%s], aborting.\n", tag);
				free(tag);
				free(value);
				ret = -1;
				goto err;
			}
			free(tag);
		}
		if (value)
			free(value);
	}

	/* Success */
	ret = 0;

	*result = opts;
	return ret;
err:
	*result = 0;
	free(opts);
        return ret;
}

void url_free(struct url_opts_s *arg)
{
        free(arg);
}

