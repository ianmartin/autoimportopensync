typedef enum OSyncFilterAction {
	OSYNC_FILTER_IGNORE = 0,
	OSYNC_FILTER_ALLOW = 1,
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

typedef osync_bool (* OSyncFilterFunction)(OSyncMember *, OSyncChange *);

osync_bool osync_filter_change_allowed(OSyncMember *destmember, OSyncChange *change);
OSyncFilter *osync_filter_add(OSyncGroup *group, OSyncMember *sourcemember, OSyncMember *destmember, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action);
void osync_filter_remove(OSyncGroup *group, OSyncFilter *filter);
