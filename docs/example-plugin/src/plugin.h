//Specify any structs etc here.

typedef struct plugin_environment {
	OSyncObjFormat *objformat;
	//If you need a hashtable:
	OSyncHashTable *hashtable;
	//More stuff you need goes here
} plugin_environment;
