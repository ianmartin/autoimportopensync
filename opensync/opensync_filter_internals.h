
/**
 * @defgroup OSyncFilterPrivate OpenSync Filter Internals
 * @ingroup OSyncPrivate
 * @brief Private api of the filter system
 * 
 */
/*@{*/


/*! @brief Represents a filter to filter changes */
struct OSyncFilter {
	OSyncGroup *group;
	long long int sourcememberid;
	long long int destmemberid;
	char *sourceobjtype;
	char *destobjtype;
	char *detectobjtype;
	OSyncFilterAction action;
	OSyncFilterFunction hook;
	char *function_name;
	char *config;
};

/*! @brief Represents a custom filter that can be used to call hooks */
struct OSyncCustomFilter {
	char *name;
	char *objtype;
	char *format;
	OSyncFilterFunction hook;
};

OSyncFilter *osync_filter_new(void);
void osync_filter_register(OSyncGroup *group, OSyncFilter *filter);
void osync_filter_update_hook(OSyncFilter *filter, OSyncGroup *group, const char *function_name);

/*@}*/
