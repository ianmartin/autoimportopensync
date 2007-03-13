#ifndef _OPENSYNC_GROUP_INTERNALS_H_
#define _OPENSYNC_GROUP_INTERNALS_H_

/*! @brief Represent a group of members that should be synchronized */
struct OSyncGroup {
	/** The name of the group */
	char *name;
	/** The members of the group */
	GList *members;
	/** The path, where the configuration resides */
	char *configdir;
	/** The last time this group was synchronized successfully */
	time_t last_sync;
	/** The lock file of the group */
#ifdef _WIN32
	HANDLE lock_fd;
#else
	int lock_fd;
#endif
	/** The filters of this group */
	GList *filters;
	/** The defined resolution for this group */
	OSyncConflictResolution conflict_resolution;
	/** The winning side if the select resolution is choosen */
	int conflict_winner;
	
	int ref_count;
};

#endif /*_OPENSYNC_GROUP_INTERNALS_H_*/
