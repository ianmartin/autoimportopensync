//Specify any structs etc here.

typedef struct plugin_environment {
	OSyncMember *member;
	//If you need a hashtable:
	OSyncHashTable *hashtable;
	//More stuff you need goes here
} plugin_environment;
