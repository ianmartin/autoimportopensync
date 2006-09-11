
/*! @brief Represents a filter to filter changes 
 * @ingroup OSyncFilterPrivate
 **/
struct OSyncFilter {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
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
#endif
};

/*! @brief Represents a custom filter that can be used to call hooks
 * @ingroup OSyncFilterPrivate
 **/
struct OSyncCustomFilter {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	char *name;
	char *objtype;
	char *format;
	OSyncFilterFunction hook;
#endif
};

OSyncFilter *osync_filter_new(void);
void osync_filter_register(OSyncGroup *group, OSyncFilter *filter);
void osync_filter_update_hook(OSyncFilter *filter, OSyncGroup *group, const char *function_name);
