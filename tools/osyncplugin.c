#include "opensync.h"
#include "opensync_internals.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

static void usage (char *name, int ecode)
{
  fprintf (stderr, "Usage: %s <pluginname>\n", name);
  fprintf (stderr, "--config <filename>\tSet the config file to use\n");
  exit (ecode);
}

int main (int argc, char *argv[])
{
	int i;
	char *pluginname = NULL;
	char *configfile = NULL;
	OSyncError *error = NULL;
	if (argc <= 2)
		usage (argv[0], 1);

	pluginname = argv[1];
	for (i = 2; i < argc; i++) {
		char *arg = argv[i];
		if (!strcmp (arg, "--config")) {
			configfile = argv[i + 1];
			i++;
			if (!configfile)
				usage (argv[0], 1);
		} else if (!strcmp (arg, "--help")) {
			usage (argv[0], 0);
		} else if (!strcmp (arg, "--")) {
			break;
		} else if (arg[0] == '-') {
			usage (argv[0], 1);
		} else {
			usage (argv[0], 1);
		}
	}
	
	OSyncEnv *osync = osync_env_new();
	
	if (!osync_env_initialize(osync, &error)) {
		printf("Unable to initialize environment: %s\n", error->message);
		osync_error_free(&error);
		return 1;
	}
	
	OSyncPlugin *plugin = osync_env_find_plugin(osync, pluginname);
	if (!plugin) {
		printf("Unable to find plugin with name \"%s\"\n", pluginname);
		return 1;
	}
	
	return 0;
}
