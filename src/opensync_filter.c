#include "opensync"
#include "opensync_internals.h"

typedef enum OSyncFilterAction {
	OSYNC_FILTER_IGNORE = 0,
	OSYNC_FILTER_ALLOW = 1,
	OSYNC_FILTER_DENY = 2
} OSyncFilterAction;

typedef struct OSyncFilter {
	OSyncGroup *group;
	long long int sourcememberid;
	long long int destmemberid;
	const char *sourceobjtype;
	const char *destobjtype;
	const char *detectobjtype;
	OSyncFilterAction allow;
	OSyncFilterFunction hook;
} OSyncFilter;

void osync_filter_register(OSyncGroup *group, OSyncFilter *filter)
{
	g_assert(group);
	group->filters = g_list_append(group->filters, filter);
}

/**
 * @defgroup OSyncEnvAPI OpenSync Environment
 * @ingroup OSyncPublic
 * @brief The public API of opensync
 * 
 * This gives you an insight in the public API of opensync.
 * 
 */
/*@{*/

OSyncFilter *osync_filter_new(void)
{
	OSyncFilter *filter = g_malloc0(sizeof(OSyncFilter));
	g_assert(filter);
	return filter;
}


/*! @brief Register a new filter
 * 
 * @param group For which group to register the filter
 * @param sourcememberid The id of the member reporting the object. 0 for any
 * @param destmemberid The id of the member receiving the object. 0 for any
 * @param sourceobjtype The objtype as reported by the member without detection. NULL for any
 * @param destobjtype The objtype as about being saved by the member without detection. NULL for any
 * @param detectobjtype The objtype as detected. NULL for ignore
 * @param allow Set to TRUE if this filter should allow the object, To false if it should deny
 * 
 */
void osync_filter_add(OSyncGroup *group, long long int sourcememberid, long long int destmemberid, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action)
{
	OSyncFilter *filter = osync_filter_new();
	filter->group = group;
	filter->sourcememberid = sourcememberid;
	filter->destmemberid = destmemberid;
	filter->sourceobjtype = g_strdup(sourceobjtype);
	filter->destobjtype = g_strdup(destobjtype);
	filter->detectobjtype = g_strdup(detectobjtype);
	filter->action = action;
	
	osync_filter_register(group, filter);
}

/*! @brief Register a new filter
 * 
 * @param sourcememberid The id of the member reporting the object. 0 for any
 * @param destmemberid The id of the member receiving the object. 0 for any
 * @param sourceobjtype The objtype as reported by the member without detection. NULL for any
 * @param detectobjtype The objtype as detected. NULL for any
 * @param hook The filter function to call to decide if to filter the object.
 * 
 */
void osync_filter_add_complete(long long int sourcememberid, long long int destmemberid, const char *sourceobjtype, const char *detectobjtype, OSyncFilterFunction hook)
{
	OSyncFilter *filter = osync_filter_new();
	filter->group = group;
	filter->sourcememberid = sourcememberid;
	filter->destmemberid = destmemberid;
	filter->sourceobjtype = g_strdup(sourceobjtype);
	filter->destobjtype = g_strdup(destobjtype);
	filter->detectobjtype = g_strdup(detectobjtype);
	filter->hook = hook;
	
	osync_filter_register(group, filter);
}

GList *_osync_filter_find(OSyncMember *member)
{
	GList *f = NULL;
	GList *ret = NULL;
	for (f = destmember->group->filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		if (filter->destmemberid = destmember->id)
			ret = g_list_append(ret, filter);
	}
	return ret;
}

OSyncFilterAction osync_filter_invoke(OSyncFilter *filter, OSyncChange *change)
{
	//Need to add to change: destobjtype sourceobjtype detected
	
	g_assert(filter);
	g_assert(change);
	if (filter->sourcememberid && filter->sourcememberid != change->sourcemember->id)
		return OSYNC_FILTER_IGNORED;
	//destmemberid already checked by filter_find
	if (filter->sourceobjtype && strcmp(filter->sourceobjtype, change->sourceobjtype))
		return OSYNC_FILTER_IGNORED;
	if (filter->destobjtype && strcmp(filter->destobjtype, change->destobjtype))
		return OSYNC_FILTER_IGNORED;
	if (filter->detectobjtype) {
		if (!change->is_detected) {
			//Detect change
			//FIXME can we do that? can we just detect the change or will it break something?
		}
		if (strcmp(filter->detectobjtype, change->objtype))
			return OSYNC_FILTER_IGNORED;
	}
	
	//We passed the filter. Now we can return the action
	if (!filter->hook)
		return filter->action;
	
	//What exactly do we need to pass to the hook?
	return filter->hook(change->member, change);		
}

osync_bool osync_filter_change_allowed(OSyncMember *destmember, OSyncChange *change)
{
	GList *filters = _osync_filter_find(destmember);
	GList *f;
	int ret;
	for (f = filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		OSyncFilterAction action = osync_filter_invoke(member, change);
		if (action == OSYNC_FILTER_ALLOW)
			ret = TRUE;
		if (action == OSYNC_FILTER_DENY)
			ret = FALSE;
	}
	g_list_free(filters);
	return ret;
}
