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
#include <libgen.h>

/* External tool hooks */
extern int demo_main(int argc, char *argv[]);
extern int parse_main(int argc, char *argv[]);
extern int smpte2038_main(int argc, char *argv[]);
extern int scte104_main(int argc, char *argv[]);
extern int genscte104_main(int argc, char *argv[]);
extern int gensmpte2038_main(int argc, char *argv[]);
extern int eia708_main(int argc, char *argv[]);
extern int smpte12_2_main(int argc, char *argv[]);
extern int afd_main(int argc, char *argv[]);

typedef int (*func_ptr)(int, char *argv[]);

int main(int argc, char *argv[])
{
	struct app_s {
		char *name;
		func_ptr func;
	} apps[] = {
		{ "klvanc_util",		demo_main, },
		{ "klvanc_parse",		parse_main, },
		{ "klvanc_smpte2038",		smpte2038_main, },
		{ "klvanc_scte104",		scte104_main, },
		{ "klvanc_eia708",		eia708_main, },
		{ "klvanc_genscte104",		genscte104_main, },
		{ "klvanc_gensmpte2038",	gensmpte2038_main, },
		{ "klvanc_smpte12_2",		smpte12_2_main, },
		{ "klvanc_afd",			afd_main, },
		{ 0, 0 },
	};
	char *appname = basename(argv[0]);

	int i = 0;
	struct app_s *app = &apps[i++];
	while (app->name) {
		if (strcmp(appname, app->name) == 0)
			return app->func(argc, argv);

		app = &apps[i++];
	}

	printf("No application called %s, aborting.\n", appname);
	i = 0;
	app = &apps[i++];
	while (app->name) {
		printf("%s ", app->name);
		app = &apps[i++];
	}

	return 1;
}
