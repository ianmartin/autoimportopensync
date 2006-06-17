
/*! @brief A member of a group which represent a single device */
struct OSyncMember {
	long long int id;
	char *configdir;
	char *configdata;
	int configsize;
	OSyncGroup *group;

	char *pluginname;
	
	//For the filters
	GList *objtypes;
	GList *filters;
	int ref_count;
};
