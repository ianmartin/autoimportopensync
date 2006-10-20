/*
 * libopensync-palm-plugin - A palm plugin for opensync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
 * 
 */

#include "palm_sync.h"
#include "palm_format.h"

static OSyncChange *psyncTodoCreate(PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, entry, error);
	PSyncDatabase *db = entry->db;
	
	OSyncChange *change = osync_change_new();
	if (!change)
		goto error;
	
	/* Set the uid */
	char *uid = g_strdup_printf("uid-ToDoDB-%ld", entry->id);
	osync_change_set_uid(change, uid);
	g_free(uid);
	
	/* Set the format */
	osync_change_set_objformat_string(change, "palm-todo");
	
	if ((entry->attr &  dlpRecAttrDeleted) || (entry->attr & dlpRecAttrArchived)) {
		if ((entry->attr & dlpRecAttrArchived)) {
			osync_trace(TRACE_INTERNAL, "Archieved");
		}
		//we have a deleted record
		osync_change_set_changetype(change, CHANGE_DELETED);
	} else {
		/* Create the object data */
		PSyncTodoEntry *todo = osync_try_malloc0(sizeof(PSyncTodoEntry), error);
		if (!todo)
			goto error_free_change;
		todo->codepage = g_strdup(db->env->codepage);
		
		osync_trace(TRACE_INTERNAL, "Starting to unpack entry %i", db->size);
		
#ifdef OLD_PILOT_LINK
		unpack_ToDo(&(todo->todo), entry->buffer, db->size);
#else
		unpack_ToDo(&(todo->todo), entry->buffer, todo_v1);
#endif

		
	    const char *catname = psyncDBCategoryFromId(entry->db, entry->category, NULL);
	    if (catname)
			todo->categories = g_list_append(todo->categories, g_strdup(catname));
			
		osync_change_set_data(change, (void *)todo, sizeof(PSyncTodoEntry), TRUE);
		
		if (entry->attr & dlpRecAttrDirty) {
			osync_change_set_changetype(change, CHANGE_MODIFIED);
		} else {
			osync_change_set_changetype(change, CHANGE_UNKNOWN);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;
	
error_free_change:
	osync_change_free(change);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

osync_bool psyncTodoGetChangeInfo(OSyncContext *ctx, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, error);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	
	if (!(db = psyncDBOpen(env, "ToDoDB", error)))
		goto error;
	
	if (osync_member_get_slow_sync(env->member, "todo") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		
		int n;
		for (n = 0; (entry = psyncDBGetNthEntry(db, n, error)); n++) {
			if (osync_error_is_set(error))
				goto error;
				
			if (entry == NULL)
				continue;

			osync_trace(TRACE_INTERNAL, "Got all recored with id %ld", entry->id);
			
			OSyncChange *change = psyncTodoCreate(entry, error);
			if (!change)
				goto error;
			
			if (osync_change_get_data(change) == NULL)
				continue;
			
			osync_change_set_changetype(change, CHANGE_ADDED);
			osync_context_report_change(ctx, change);
		}
	} else {
		while ((entry = psyncDBGetNextModified(db, error))) {
			if (osync_error_is_set(error))
				goto error;
			
			if (entry == NULL)
				continue;

			OSyncChange *change = psyncTodoCreate(entry, error);
			if (!change)
				goto error;
			
			osync_context_report_change(ctx, change);
		}
	}
	
	if (osync_error_is_set(error))
		goto error_close_db;
	
	psyncDBClose(db);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error_close_db:
	psyncDBClose(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}

osync_bool psyncTodoCommit(OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, ctx, change);
	PSyncEnv *env = (PSyncEnv *)osync_context_get_plugin_data(ctx);
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	PSyncTodoEntry *todo = NULL;
	OSyncError *error = NULL;
	unsigned long id = 0;

	//open the DB
	if (!(db = psyncDBOpen(env, "ToDoDB", &error)))
		goto error;
	
	todo = (PSyncTodoEntry *)osync_change_get_data(change);
			
	switch (osync_change_get_changetype(change)) {
		case CHANGE_MODIFIED:
			//Get the id
			id = psyncUidGetID(osync_change_get_uid(change), &error);
			if (!id)
				goto error;

			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
#ifdef OLD_PILOT_LINK
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, 0xffff);
#else
			entry->buffer = pi_buffer_new(65536);
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, todo_v1);
#endif
	
			if (entry->size < 0) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error packing todo");
				goto error;
			}
			if (!psyncDBWrite(db, entry, &error))
				goto error;
				
			break;
		case CHANGE_ADDED:
			//Add a new entry
			osync_trace(TRACE_INTERNAL, "Find category");
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			GList *c = NULL;
			for (c = todo->categories; c; c = c->next) {
				osync_trace(TRACE_INTERNAL, "searching category %s\n", c->data);
				entry->category = psyncDBCategoryToId(db, c->data, NULL);
				if (entry->category != 0) {
					osync_trace(TRACE_INTERNAL, "Found category %i\n", entry->category);
					break;
				}
			}
			
			osync_trace(TRACE_INTERNAL, "Adding new entry");
			

#ifdef OLD_PILOT_LINK
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, 0xffff);
#else
			entry->buffer = pi_buffer_new(65536);
			entry->size = pack_ToDo(&(todo->todo), entry->buffer, todo_v1);
#endif
	
			if (entry->size < 0) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Error packing todo");
				goto error;
			}
			if (!psyncDBAdd(db, entry, &id, &error))
				goto error;
			
			//Make the new uid
			char *uid = g_strdup_printf("uid-ToDoDB-%ld", id);
			osync_change_set_uid(change, uid);
			g_free(uid);
			break;
		case CHANGE_DELETED:
			id = psyncUidGetID(osync_change_get_uid(change), &error);
			if (!id)
				goto error;
		
			if (!psyncDBDelete(db, id, &error))
				goto error;
				
			break;
		default:
			osync_error_set(&error, OSYNC_ERROR_GENERIC, "Wrong change type");
			goto error;
	}
	
	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}
