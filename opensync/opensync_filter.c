/*
 * libopensync - A synchronization framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include "opensync.h"
#include "opensync_internals.h"

/**
 * @defgroup OSyncFilterPrivate OpenSync Filter Internals
 * @ingroup OSyncPrivate
 * @brief Private api of the filter system
 * 
 */
/*@{*/

#ifndef DOXYGEN_SHOULD_SKIP_THIS
OSyncFilter *_osync_filter_add_ids(OSyncGroup *group, long long int sourcememberid, long long int destmemberid, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action, const char *function_name)
{
	OSyncFilter *filter = osync_filter_new();
	filter->group = group;
	filter->sourcememberid = sourcememberid;
	filter->destmemberid = destmemberid;
	filter->sourceobjtype = g_strdup(sourceobjtype);
	filter->destobjtype = g_strdup(destobjtype);
	filter->detectobjtype = g_strdup(detectobjtype);
	filter->action = action;
	
	if (function_name) {
		osync_filter_update_hook(filter, group, function_name);
	}
	
	osync_filter_register(group, filter);
	return filter;
}

void osync_filter_update_hook(OSyncFilter *filter, OSyncGroup *group, const char *function_name)
{
	g_assert(filter);
	g_assert(group);
	g_assert(function_name);
	
	OSyncFilterFunction hook = NULL;
	GList *f;
	for (f = group->conv_env->filter_functions; f; f = f->next) {
		OSyncCustomFilter *custom = f->data;
		if (!strcmp(custom->name, function_name)) 
			hook = custom->hook;
	}
	if (!hook) {
		printf("Unable to add custom filter, hook not found!\n");
		return;
	}
	filter->hook = hook;
	filter->function_name = g_strdup(function_name);
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
	osync_debug("OSFLT", 3, "Starting to invoke filter for change %s", change->uid);
	if (filter->sourcememberid && change->sourcemember && filter->sourcememberid != change->sourcemember->id)
		return OSYNC_FILTER_IGNORE;
	if (filter->destmemberid && filter->destmemberid != destmember->id)
		return OSYNC_FILTER_IGNORE;
	if (filter->sourceobjtype && strcmp(filter->sourceobjtype, change->sourceobjtype))
		return OSYNC_FILTER_IGNORE;
	if (filter->destobjtype && change->destobjtype && strcmp(filter->destobjtype, change->destobjtype))
		return OSYNC_FILTER_IGNORE;
	if (filter->detectobjtype) {
		OSyncError *error = NULL;
		OSyncObjType *objtype = osync_change_detect_objtype_full(osync_member_get_format_env(destmember), change, &error);
		if (!objtype) {
			osync_error_free(&error);
			return OSYNC_FILTER_IGNORE;
		}
		if (strcmp(filter->detectobjtype, objtype->name))
			return OSYNC_FILTER_IGNORE;
	}
	
	osync_debug("OSFLT", 3, "Change %s passed the filter!", change->uid);
	//We passed the filter. Now we can return the action
	if (!filter->hook)
		return filter->action;
	
	//What exactly do we need to pass to the hook?
	return filter->hook(change, filter->config);		
}

osync_bool osync_filter_change_allowed(OSyncMember *destmember, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "osync_filter_change_allowed(%p, %p)", destmember, change);
	GList *filters = _osync_filter_find(destmember);
	GList *f = NULL;
	int ret = TRUE;
	osync_debug("OSFLT", 3, "Checking if change %s is allowed for member %lli. Filters to invoke: %i", change->uid, destmember->id, g_list_length(filters));
	for (f = filters; f; f = f->next) {
		OSyncFilter *filter = f->data;
		OSyncFilterAction action = osync_filter_invoke(filter, change, destmember);
		if (action == OSYNC_FILTER_ALLOW)
			ret = TRUE;
		if (action == OSYNC_FILTER_DENY)
			ret = FALSE;
	}
	g_list_free(filters);
	osync_trace(TRACE_EXIT, "osync_filter_change_allowed: %s", ret ? "TRUE" : "FALSE");
	return ret;
}

const char *osync_filter_get_sourceobjtype(OSyncFilter *filter)
{
	return filter->sourceobjtype;
}

const char *osync_filter_get_destobjtype(OSyncFilter *filter)
{
	return filter->destobjtype;
}

const char *osync_filter_get_detectobjtype(OSyncFilter *filter)
{
	return filter->detectobjtype;
}

OSyncFilterAction osync_filter_get_action(OSyncFilter *filter)
{
	return filter->action;
}

OSyncMember *osync_filter_get_sourcemember(OSyncFilter *filter)
{
	return osync_member_from_id(filter->group, filter->sourcememberid);
}

OSyncMember *osync_filter_get_destmember(OSyncFilter *filter)
{
	return osync_member_from_id(filter->group, filter->destmemberid);
}
#endif

/*@}*/

/**
 * @defgroup OSyncFilterAPI OpenSync Filter
 * @ingroup OSyncPublic
 * @brief Allows filtering of changes and applying hooks to changes as they pass through opensync
 * 
 */
/*@{*/

/** @brief Registers a filter with a group
 * 
 * @param group The group in which to register the filter
 * @param filter The filter to register
 **/
void osync_filter_register(OSyncGroup *group, OSyncFilter *filter)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, filter);
	g_assert(group);
	group->filters = g_list_append(group->filters, filter);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Creates a new filter
 * 
 * @returns A newly allocated filter
 **/
OSyncFilter *osync_filter_new(void)
{
	osync_trace(TRACE_ENTRY, "%s(void)", __func__);
	OSyncFilter *filter = g_malloc0(sizeof(OSyncFilter));
	g_assert(filter);
	osync_trace(TRACE_EXIT, "%s: %p", __func__, filter);
	return filter;
}

/** @brief Frees a filter
 * 
 * @param filter The filter to free
 **/
void osync_filter_free(OSyncFilter *filter)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, filter);
	g_assert(filter);
	if (filter->sourceobjtype)
		g_free(filter->sourceobjtype);
	if (filter->destobjtype)
		g_free(filter->destobjtype);
	if (filter->detectobjtype)
		g_free(filter->detectobjtype);
	
	g_free(filter);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Register a new filter
 * 
 * @param group For which group to register the filter
 * @param sourcemember The member reporting the object. NULL for any
 * @param destmember The member receiving the object. NULL for any
 * @param sourceobjtype The objtype as reported by the member without detection. NULL for any
 * @param destobjtype The objtype as about being saved by the member without detection. NULL for any
 * @param detectobjtype The objtype as detected. NULL for ignore
 * @param action Set this to the action the filter should return for the object
 * @returns The new added Filter
 */
OSyncFilter *osync_filter_add(OSyncGroup *group, OSyncMember *sourcemember, OSyncMember *destmember, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, OSyncFilterAction action)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p:%lli, %p:%lli, %s, %s, %s, %i)", __func__, group, \
		sourcemember, sourcemember ? sourcemember->id : 0, \
		destmember, destmember ? destmember->id : 0, \
		sourceobjtype, destobjtype, detectobjtype, action);
		
	long long int sourcememberid = 0;
	long long int destmemberid = 0;
	if (sourcemember)
		sourcememberid = sourcemember->id;
	if (destmember)
		destmemberid = destmember->id;
	
	OSyncFilter *filter = _osync_filter_add_ids(group, sourcememberid, destmemberid, sourceobjtype, destobjtype, detectobjtype, action, NULL);
	osync_trace(TRACE_EXIT, "%s: %p", __func__, filter);
	return filter;
}

/*! @brief Removes a filter from a group
 * 
 * @param group The group to remove from
 * @param filter The filter to remove
 **/
void osync_filter_remove(OSyncGroup *group, OSyncFilter *filter)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, group, filter);
	g_assert(group);
	group->filters = g_list_remove(group->filters, filter);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Register a new custom filter
 * 
 * @param group The group that should store the filter
 * @param sourcemember The member reporting the object. NULL for any
 * @param destmember The member receiving the object. NULL for any
 * @param sourceobjtype The objtype as reported by the member without detection. NULL for any
 * @param destobjtype The object type has it is being added on the target. NULL for any
 * @param detectobjtype The objtype as detected. NULL for any
 * @param function_name The filter function to call to decide if to filter the object.
 * @returns The new added Filter
 * 
 */
OSyncFilter *osync_filter_add_custom(OSyncGroup *group, OSyncMember *sourcemember, OSyncMember *destmember, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, const char *function_name)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p:%lli, %p:%lli, %s, %s, %s, %s)", __func__, group, \
		sourcemember, sourcemember ? sourcemember->id : 0, \
		destmember, destmember ? destmember->id : 0, \
		sourceobjtype, destobjtype, detectobjtype, function_name);
	long long int sourcememberid = 0;
	long long int destmemberid = 0;
	if (sourcemember)
		sourcememberid = sourcemember->id;
	if (destmember)
		destmemberid = destmember->id;

	OSyncFilter *filter = _osync_filter_add_ids(group, sourcememberid, destmemberid, sourceobjtype, destobjtype, detectobjtype, OSYNC_FILTER_IGNORE, function_name);
	osync_trace(TRACE_EXIT, "%s: %p", __func__, filter);
	return filter;
}

/** @brief Sets the config for a filter
 * 
 * Config must be a null-terminated string
 * 
 * @param filter The filter
 * @param config The new config for this filter
 **/
void osync_filter_set_config(OSyncFilter *filter, const char *config)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, filter, config);
	g_assert(filter);
	if (filter->config)
		g_free(filter->config);
	filter->config = g_strdup(config);
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/** @brief Gets the config of a filter
 * 
 * @param filter The filter
 * @returns The config of this filter
 **/
const char *osync_filter_get_config(OSyncFilter *filter)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, filter);
	g_assert(filter);
	osync_trace(TRACE_EXIT, "%s: %s", __func__, filter->config);
	return filter->config;
}

/*@}*/
