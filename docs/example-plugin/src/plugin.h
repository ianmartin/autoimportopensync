#include <glib.h>

//Specify any structs etc here.
typedef struct plugin_environment {
	GList *sink_envs; // sink_environment
} plugin_environment;

typedef struct sink_environment {
	OSyncObjTypeSink *sink;
	OSyncObjFormat *objformat;
	//If you need a hashtable:
	OSyncHashTable *hashtable;
	//More stuff you need goes here
} sink_environment;

typedef struct format_data {
	char *data;
} format_data;

