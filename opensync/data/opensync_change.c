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

#include "opensync-data.h"
#include "opensync_change_internals.h"

OSyncChange *osync_change_new(OSyncError **error)
{
	OSyncChange *change = osync_try_malloc0(sizeof(OSyncChange), error);
	if (!change)
		return NULL;
		
	change->ref_count = 1;
	
	return change;
}

void osync_change_ref(OSyncChange *change)
{
	osync_assert(change);
	
	g_atomic_int_inc(&(change->ref_count));
}

void osync_change_unref(OSyncChange *change)
{
	osync_assert(change);
	
	if (g_atomic_int_dec_and_test(&(change->ref_count))) {
		if (change->data)
			osync_data_unref(change->data);
			
		if (change->uid)
			g_free(change->uid);
		
		if (change->hash)
			g_free(change->hash);
		
		g_free(change);
	}
}

/*! @brief Gets the changetype of a change
 * 
 * @param change The change
 * @returns The changetype
 * 
 */
OSyncChangeType osync_change_get_changetype(OSyncChange *change)
{
	osync_assert(change);
	return change->changetype;
}

/*! @brief Sets the changetype of a change
 * 
 * @param change The change
 * @param type The changetype to set
 * 
 */
void osync_change_set_changetype(OSyncChange *change, OSyncChangeType type)
{
	osync_assert(change);
	change->changetype = type;
}

/*! @brief Sets the hash of a change that is used to decide wether a change is new, modifed etc
 * 
 * @param change The change
 * @param hash The hash to set
 * 
 */
void osync_change_set_hash(OSyncChange *change, const char *hash)
{
	osync_assert(change);
	if (change->hash)
		g_free(change->hash);
	change->hash = g_strdup(hash);
}

/*! @brief Gets the hash of a change
 * 
 * @param change The change
 * @returns The hash
 * 
 */
const char *osync_change_get_hash(OSyncChange *change)
{
	osync_assert(change);
	return change->hash;
}

/*! @brief Sets the uid of a change
 * 
 * @param change The change
 * @param uid The uid to set
 * 
 */
void osync_change_set_uid(OSyncChange *change, const char *uid)
{
	osync_assert(change);
	if (change->uid)
		g_free(change->uid);
	change->uid = g_strdup(uid);
}

/*! @brief Gets the uid of a change
 * 
 * @param change The change
 * @returns The uid
 * 
 */
const char *osync_change_get_uid(OSyncChange *change)
{
	osync_assert(change);
	return change->uid;
}

void osync_change_set_data(OSyncChange *change, OSyncData *data)
{
	osync_assert(change);
	if (change->data)
		osync_data_unref(change->data);
	change->data = data;
	osync_data_ref(data);
}

OSyncData *osync_change_get_data(OSyncChange *change)
{
	osync_assert(change);
	return change->data;
}

#if 0
/** Get the name of the source object type of a change 
 * 
 * @param change The change
 * @returns The name of the source object type
 **/
const char *osync_change_get_sourceobjtype(OSyncChange *change)
{
	g_assert(change);
	return change->sourceobjtype;
}

/** Get the name of the inital format of a change 
 * 
 * @param change The change
 * @returns The name of the inital format
 **/
OSyncObjFormat *osync_change_get_initial_objformat(OSyncChange *change)
{
	g_assert(change);
	if (change->initial_format)
		return change->initial_format;
	
	if (!change->initial_format)
		return NULL;
	
	osync_assert_msg(change->conv_env, "The conv env of the change must be set by calling member_set or conv_env_set");
	change->initial_format = osync_conv_find_objformat(change->conv_env, change->initial_format_name);
	return change->initial_format;
}

/*@}*/

/**
 * @defgroup OSyncChange OpenSync Change
 * @ingroup OSyncPublic
 * @brief The public API of the OSyncChange
 * 
 */
/*@{*/

/*! @brief Spawns a new change object
 * 
 * @returns Newly allocated change object
 * 
 */
OSyncChange *osync_change_new(void)
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);
	
	OSyncChange *change = g_malloc0(sizeof(OSyncChange));
	change->refcount = 1;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;
}

/*! @brief Frees a change
 * 
 * @param change The change to free
 * 
 */
void osync_change_free(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);
	g_assert(change);
	
	//FIXME cleanly release the change!
	g_free(change);

	osync_trace(TRACE_EXIT, "%s");
}

/*! @brief Frees the data of a change
 * 
 * This frees the data of a change but does not free
 * the change itself
 * 
 * @param change The change of which to free the data
 * 
 */
void osync_change_free_data(OSyncChange *change)
{
	g_assert(change);
	g_assert(osync_change_get_objformat(change));
	if (!osync_change_get_objformat(change)->destroy_func)
		osync_debug("OSCONV", 1, "Memory leak: can't free data of type %s", osync_change_get_objformat(change)->name);
	else {
		osync_debug("OSCONV", 4, "Freeing data of type %s", osync_change_get_objformat(change)->name);
		osync_change_get_objformat(change)->destroy_func(change->data, change->size);
	}
	change->data = NULL;
	change->size = 0;
	//FIXME Set format to NULL here?
}

/*! @brief Resets a change
 * 
 * Resets the information about changetype, hash, data etc.
 * 
 * @param change The change to reset
 * 
 */
void osync_change_reset(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);
	
	if (change->hash)
		g_free(change->hash);
	change->hash = NULL;
	//FIXME Release data
	change->data = NULL;
	change->size = 0;
	change->has_data = FALSE;
	change->changetype = CHANGE_UNKNOWN;
	//change->sourceobjtype = NULL;
	//change->destobjtype = NULL;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief This will save a change into the database
 * 
 * @param change The change to save
 * @param save_format Wether to save the format or not.
 * @param error A pointer to a error struct
 * @returns TRUE if save was successful, FALSE otherwise
 * 
 */
osync_bool osync_change_save(OSyncChange *change, osync_bool save_format, OSyncError **error)
{
	if (!change->changes_db)
		change->changes_db = change->member->group->changes_db;
	return osync_db_save_change(change, save_format, error);
}

/*! @brief This will delete a change from the database
 * 
 * @param change The change to delete
 * @param error A pointer to a error struct
 * @returns TRUE if deletion was successful, FALSE otherwise
 * 
 */
osync_bool osync_change_delete(OSyncChange *change, OSyncError **error)
{
	return osync_db_delete_change(change, error);
}

/*! @brief This will load the changes from the database
 * 
 * This opens the change database and returns an array with
 * the changes. The changes have to be freed later. The database has
 * to be closed with a call to osync_changes_close()
 * 
 * @param group The group for which to load the changes
 * @param changes An pointer to an array in which to store the changes
 * @param error A pointer to a error struct
 * @returns TRUE if load was successful, FALSE otherwise
 * 
 */
osync_bool osync_changes_load(OSyncGroup *group, OSyncChange ***changes, OSyncError **error)
{
	return osync_db_open_changes(group, changes, error);
}

/*! @brief Closes the change database
 * 
 * @param group The group for which to close the database
 * 
 */
void osync_changes_close(OSyncGroup *group)
{
	osync_db_close_changes(group);
}

/*! @brief Gets the member which reported a change
 * 
 * @param change The change
 * @returns The member of the change
 * 
 */
OSyncMember *osync_change_get_member(OSyncChange *change)
{
	g_assert(change);
	return change->member;
}

/*! @brief Sets the member of a change
 * 
 * @param change The change
 * @param member The member of the change
 * 
 */
void osync_change_set_member(OSyncChange *change, OSyncMember *member)
{
	g_assert(change);
	change->member = member;
	change->conv_env = member->group->conv_env;
}

/*! @brief Sets the conversion environment of a change
 * 
 * @param change The change
 * @param env The conversion environment
 * 
 */
void osync_change_set_conv_env(OSyncChange *change, OSyncFormatEnv *env)
{
	g_assert(change);
	change->conv_env = env;
}

/*! @brief Gets the object type of a change
 * 
 * @param change The change
 * @returns The object type
 * 
 */
OSyncObjType *osync_change_get_objtype(OSyncChange *change)
{
	g_assert(change);
	
	if (change->objtype)
		return change->objtype;
	
	if (!change->objtype_name) {
		OSyncObjFormat *format = osync_change_get_objformat(change);
		if (!format)
			return NULL;
		change->objtype = format->objtype;
		return format->objtype;
	}
	
	osync_assert_msg(change->conv_env, "The conv env of the change must be set by calling member_set or conv_env_set");
	change->objtype = osync_conv_find_objtype(change->conv_env, change->objtype_name);
	return change->objtype;
}

/*! @brief Sets the object type of a change
 * 
 * @param change The change
 * @param type The object type
 * 
 */
void osync_change_set_objtype(OSyncChange *change, OSyncObjType *type)
{
	g_assert(change);
	change->objtype = type;
}

/*! @brief Sets the object type of a change from the name
 * 
 * @param change The change
 * @param name The object type name
 * 
 */
void osync_change_set_objtype_string(OSyncChange *change, const char *name)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, change, name);
	
	g_assert(change);
	if (change->objtype_name)
		g_free(change->objtype_name);
	change->objtype_name = g_strdup(name);
	//Invalidate the previous object type
	change->objtype = NULL;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Gets the object format of a change
 * 
 * @param change The change
 * @returns The object format
 * 
 */
OSyncObjFormat *osync_change_get_objformat(OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, change);
	g_assert(change);
	
	if (change->format) {
		osync_trace(TRACE_EXIT, "%s: %p", __func__, change->format);
		return change->format;
	}
	
	if (!change->format_name) {
		osync_trace(TRACE_EXIT, "%s: No name yet", __func__);
		return NULL;
	}
	
	osync_assert_msg(change->conv_env, "The conv env of the change must be set by calling member_set or conv_env_set");
	change->format = osync_conv_find_objformat(change->conv_env, change->format_name);
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change->format);
	return change->format;
}

/*! @brief Sets the object format of a change
 * 
 * @param change The change
 * @param objformat The object format
 * 
 */
void osync_change_set_objformat(OSyncChange *change, OSyncObjFormat *objformat)
{
	g_assert(change);
	change->format = objformat;
	if (objformat)
		change->objtype = objformat->objtype;
	else
		change->objtype = NULL;
}

/*! @brief Sets the object format of a change from the name
 * 
 * @param change The change
 * @param name The object format name
 * 
 */
void osync_change_set_objformat_string(OSyncChange *change, const char *name)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s)", __func__, change, name);
	
	g_assert(change);
	if (change->format_name)
		g_free(change->format_name);
	change->format_name = g_strdup(name);
	//Invalidate the previous format
	change->format = NULL;
	
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/*! @brief Sets the data of a change
 * 
 * @param change The change
 * @param data The data to set
 * @param size The size of the data to set
 * @param has_data Set this to TRUE of this already is the complete data
 * 
 */
void osync_change_set_data(OSyncChange *change, char *data, int size, osync_bool has_data)
{
	change->data = data;
	change->size = size;
	change->has_data = has_data;
}

/*! @brief Returns wether the complete data already has been set
 * 
 * @param change The change
 * @returns TRUE if the change has the complete data set
 * 
 */
osync_bool osync_change_has_data(OSyncChange *change)
{
	g_assert(change);
	return change->has_data;
}

/*! @brief Gets the data of a change
 * 
 * @param change The change
 * @returns The data
 * 
 */
char *osync_change_get_data(OSyncChange *change)
{
	g_assert(change);
	return change->data;
}

/*! @brief Gets the size of the data of a change
 * 
 * @param change The change
 * @returns The size of the data
 * 
 */
int osync_change_get_datasize(OSyncChange *change)
{
	g_assert(change);
	return change->size;
}

/*! @brief Gets the mappingid of a change
 * 
 * @param change The change
 * @returns The mappingid of the data
 * 
 */
long long int osync_change_get_mappingid(OSyncChange *change)
{
	g_assert(change);
	return change->mappingid;
}

/*! @brief Sets the mappingid of a change
 * 
 * @param change The change
 * @param mappingid The mappingid to set
 * 
 */
void osync_change_set_mappingid(OSyncChange *change, long long int mappingid)
{
	g_assert(change);
	change->mappingid = mappingid;
}

/*! @brief Gets data that can be used privately by the engine
 * 
 * @param change The change
 * @returns The data of the engine
 * 
 */
void *osync_change_get_engine_data(OSyncChange *change)
{
	g_assert(change);
	return change->engine_data;
}

/*! @brief Sets the data of the engine
 * 
 * @param change The change
 * @param engine_data The data
 * 
 */
void osync_change_set_engine_data(OSyncChange *change, void *engine_data)
{
	g_assert(change);
	change->engine_data = engine_data;
}

/*! @brief Gets the id of the change which is always unique
 * 
 * @param change The change
 * @returns The id
 * 
 */
long long int osync_change_get_id(OSyncChange *change)
{
	g_assert(change);
	return change->id;
}

/*! @brief Updated one change from another change
 * 
 * This function can be used to "merge" the information from 2
 * changes into one. The uid, hash, data of the target change
 * are overwriten by those of the source change if they are not set already
 * on the target. The data of the source change is copied.
 * 
 * @param source The source change
 * @param target The target change
 * 
 */
void osync_change_update(OSyncChange *source, OSyncChange *target)
{
	osync_trace(TRACE_ENTRY, "osync_change_update(%p, %p)", source, target);
	//FIXME free stuff
	g_assert(source);
	g_assert(target);
	if (!target->uid)
		target->uid = g_strdup(source->uid);
	target->hash = g_strdup(source->hash);
	
	OSyncError *error = NULL;
	if (!osync_change_copy_data(source, target, &error)) {
		osync_trace(TRACE_INTERNAL, "unable to copy change: %s", osync_error_print(&error));
		osync_error_unref(&error);
	}
	
	target->has_data = source->has_data;
	target->changetype = source->changetype;
	if (source->format)
		target->format = osync_change_get_objformat(source);
	if (source->objtype) {
		target->objtype = osync_change_get_objtype(source);
		target->sourceobjtype = g_strdup(osync_change_get_objtype(source)->name);
	}
	
	target->changes_db = source->changes_db;
	
	osync_trace(TRACE_EXIT, "osync_change_update");
}


/*! @brief Duplicates the uid of the change
 * 
 * This will call the duplicate function of a format.
 * This is used if a uid is not unique.
 * 
 * @param change The change to duplicate
 * @returns TRUE if the uid was duplicated successful
 * 
 */
osync_bool osync_change_duplicate(OSyncChange *change)
{
	g_assert(change);
	OSyncObjFormat *format = osync_change_get_objformat(change);
	osync_debug("OSCONV", 3, "Duplicating change %s with format %s\n", change->uid, format->name);
	if (!format || !format->duplicate_func)
		return FALSE;
	format->duplicate_func(change);
	return TRUE;
}

#endif

/*@}*/
