#include "opensync.h"
#include "opensync_internals.h"

void osync_filter_register(OSyncGroup *group, OSyncFilter *filter)
{
	g_assert(group);
	group->filters = g_list_append(group->filters, filter);
}

OSyncFilter *_osync_filter_add_ids(OSyncGroup *group, long long int sourcememberid, long long int destmemberid, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action)
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
	return filter;
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
OSyncFilter *osync_filter_add(OSyncGroup *group, OSyncMember *sourcemember, OSyncMember *destmember, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action)
{
	long long int sourcememberid = 0;
	long long int destmemberid = 0;
	if (sourcemember)
		sourcememberid = sourcemember->id;
	if (destmember)
		destmemberid = destmember->id;
	return _osync_filter_add_ids(group, sourcememberid, destmemberid, sourceobjtype, destobjtype, detectobjtype, action);
}

void osync_filter_remove(OSyncGroup *group, OSyncFilter *filter)
{
	g_assert(group);
	group->filters = g_list_remove(group->filters, filter);
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
void osync_filter_add_complete(OSyncGroup *group, long long int sourcememberid, long long int destmemberid, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterFunction hook)
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
	for (f = member->group->filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		if (!filter->destmemberid || filter->destmemberid == member->id)
			ret = g_list_append(ret, filter);
	}
	return ret;
}

OSyncFilterAction osync_filter_invoke(OSyncFilter *filter, OSyncChange *change, OSyncMember *destmember)
{
	g_assert(filter);
	g_assert(change);
	osync_debug("OSFLT", 0, "Starting to invoke filter for change %s", change->uid);
	if (filter->sourcememberid && change->sourcemember && filter->sourcememberid != change->sourcemember->id)
		return OSYNC_FILTER_IGNORE;
	if (filter->destmemberid && filter->destmemberid != destmember->id)
		return OSYNC_FILTER_IGNORE;
	if (filter->sourceobjtype && strcmp(filter->sourceobjtype, change->sourceobjtype))
		return OSYNC_FILTER_IGNORE;
	if (filter->destobjtype && strcmp(filter->destobjtype, change->destobjtype))
		return OSYNC_FILTER_IGNORE;
	if (filter->detectobjtype) {
		if (!change->is_detected) {
			//Detect change
			//FIXME can we do that? can we just detect the change or will it break something?
		}
		if (!change->objtype)
			return OSYNC_FILTER_IGNORE;
		if (strcmp(filter->detectobjtype, change->objtype->name))
			return OSYNC_FILTER_IGNORE;
	}
	
	osync_debug("OSFLT", 0, "Change %s passed the filter!", change->uid);
	//We passed the filter. Now we can return the action
	if (!filter->hook)
		return filter->action;
	
	//What exactly do we need to pass to the hook?
	return filter->hook(change->member, change);		
}

osync_bool osync_filter_change_allowed(OSyncMember *destmember, OSyncChange *change)
{
	GList *filters = _osync_filter_find(destmember);
	GList *f = NULL;
	int ret = TRUE;
	osync_debug("OSFLT", 0, "Checking if change %s is allowed for member %lli. Filters to invoke: %i", change->uid, destmember->id, g_list_length(filters));
	for (f = filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		OSyncFilterAction action = osync_filter_invoke(filter, change, destmember);
		if (action == OSYNC_FILTER_ALLOW)
			ret = TRUE;
		if (action == OSYNC_FILTER_DENY)
			ret = FALSE;
	}
	g_list_free(filters);
	return ret;
}
