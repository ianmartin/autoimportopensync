#include <opensync/opensync_list.h>

//Specify any structs etc here.
typedef struct plugin_environment {
	OSyncList *sink_envs; // sink_environment
} plugin_environment;

typedef struct sink_environment {
	OSyncObjTypeSink *sink;
	//If you need a hashtable:
	OSyncHashTable *hashtable;
	//More stuff you need goes here
} sink_environment;

