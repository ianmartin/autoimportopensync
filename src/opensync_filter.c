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

void osync_filter_register(OSyncGroup *group, OSyncFilter *filter)
{
	g_assert(group);
	group->filters = g_list_append(group->filters, filter);
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

/**
 * @defgroup OSyncFilterAPI OpenSync Filter
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


void osync_filter_free(OSyncFilter *filter)
{
	g_assert(filter);
	if (filter->sourceobjtype)
		g_free(filter->sourceobjtype);
	if (filter->destobjtype)
		g_free(filter->destobjtype);
	if (filter->detectobjtype)
		g_free(filter->detectobjtype);
	
	g_free(filter);
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
	return _osync_filter_add_ids(group, sourcememberid, destmemberid, sourceobjtype, destobjtype, detectobjtype, action, NULL);
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
OSyncFilter *osync_filter_add_custom(OSyncGroup *group, OSyncMember *sourcemember, OSyncMember *destmember, const char *sourceobjtype, const char *destobjtype, const char *detectobjtype, const char *function_name)
{
	long long int sourcememberid = 0;
	long long int destmemberid = 0;
	if (sourcemember)
		sourcememberid = sourcemember->id;
	if (destmember)
		destmemberid = destmember->id;
	return _osync_filter_add_ids(group, sourcememberid, destmemberid, sourceobjtype, destobjtype, detectobjtype, OSYNC_FILTER_IGNORE, function_name);
}

void osync_filter_set_config(OSyncFilter *filter, const char *config)
{
	g_assert(filter);
	filter->config = g_strdup(config);
}

const char *osync_filter_get_config(OSyncFilter *filter)
{
	g_assert(filter);
	return filter->config;
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
		printf("Starting to do detect objtype filtering on change %s looking for %s\n", change->uid, filter->detectobjtype);
		printf("right now it is of format %s and objtype %s\n", change->format ? change->format->name : "none", change->objtype ? change->objtype->name : "None");
		OSyncObjType *objtype = osync_change_detect_objtype_full(osync_member_get_format_env(destmember), change, NULL);
		printf("change %s objtype is %p name %s\n", change->uid, objtype, objtype ? objtype->name : "None");
		if (!objtype)
			return OSYNC_FILTER_IGNORE;
		if (strcmp(filter->detectobjtype, objtype->name))
			return OSYNC_FILTER_IGNORE;
	}
	
	osync_debug("OSFLT", 0, "Change %s passed the filter!", change->uid);
	//We passed the filter. Now we can return the action
	if (!filter->hook)
		return filter->action;
	
	//What exactly do we need to pass to the hook?
	return filter->hook(change, filter->config);		
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
