/* Copyright Kernel Labs Inc 2014, 2015, 2016 */

#include <stdio.h>
#include <string.h>
#include <libgen.h>

/* External tool hooks */
extern int demo_main(int argc, char *argv[]);
extern int capture_main(int argc, char *argv[]);

typedef int (*func_ptr)(int, char *argv[]);

int main(int argc, char *argv[])
{
	struct app_s {
		char *name;
		func_ptr func;
	} apps[] = {
		{ "klvanc_util",		demo_main, },
		{ "klvanc_capture",		capture_main, },
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
