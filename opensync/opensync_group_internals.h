#ifndef _OPENSYNC_GROUP_INTERNALS_H_
#define _OPENSYNC_GROUP_INTERNALS_H_

/*! @brief Represent a group of members that should be synchronized */
struct OSyncGroup {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	char *name;
	GList *members;
	char *configdir;
	OSyncEnv *env;
	OSyncFormatEnv *conv_env;
	void *data;
	long long int id;
	int lock_fd;
	GList *filters;
	char *changes_path;
	OSyncDB *changes_db;
#endif
};

#endif //_OPENSYNC_GROUP_INTERNALS_H_
