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

#include <opensync/opensync-data.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>

static OSyncChange *psyncContactCreate(PSyncEnv *env, PSyncEntry *entry, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, env, entry, error);
	PSyncDatabase *db = entry->db;

	OSyncChange *change = osync_change_new(error);
	if (!change)
		goto error;
	
	OSyncData *data = osync_data_new(NULL, 0, env->contact_format, error);
	if (!data)
		goto error_free_change;
	
	osync_change_set_data(change, data);
		
	char *uid = g_strdup_printf("uid-AddressDB-%ld", entry->id);
	osync_change_set_uid(change, uid);
	g_free(uid);
	
	if ((entry->attr &  dlpRecAttrDeleted) || (entry->attr & dlpRecAttrArchived)) {
		if ((entry->attr & dlpRecAttrArchived)) {
			osync_trace(TRACE_INTERNAL, "Archieved");
		}
		//we have a deleted record
		osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
	} else {
		/* Create the object data */
		PSyncContactEntry *contact = osync_try_malloc0(sizeof(PSyncContactEntry), error);
		if (!contact)
			goto error_free_change;
		contact->codepage = g_strdup(db->env->codepage);
		
		osync_trace(TRACE_INTERNAL, "Starting to unpack entry %i", db->size);
#ifdef OLD_PILOT_LINK
		unpack_Address(&(contact->address), entry->buffer, db->size);
#else
		unpack_Address(&(contact->address), entry->buffer, address_v1);
#endif
	    const char *catname = psyncDBCategoryFromId(entry->db, entry->category, NULL);
	    if (catname)
			contact->categories = g_list_append(contact->categories, g_strdup(catname));
		
		osync_data_set_data(data, (void *)contact, sizeof(PSyncContactEntry));
		
		if (entry->attr & dlpRecAttrDirty)  {
			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);
		} else {
			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_UNKNOWN);
		}
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, change);
	return change;

error_free_change:
	osync_change_unref(change);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
	return NULL;
}

static void psyncContactGetChanges(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	OSyncError *error = NULL;
	PSyncEnv *env = (PSyncEnv *)data;
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
	
	osync_trace(TRACE_INTERNAL, "Opening conduit");
	dlp_OpenConduit(env->socket);
	
	if (!(db = psyncDBOpen(env, "AddressDB", &error)))
		goto error;
	
	if (osync_objtype_sink_get_slowsync(sink) == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		
		int n;
		for (n = 0; (entry = psyncDBGetNthEntry(db, n, &error)); n++) {
			osync_trace(TRACE_INTERNAL, "Got record with id %ld", entry->id);
			
			OSyncChange *change = psyncContactCreate(env, entry, &error);
			if (!change)
				goto error;
			
			if (osync_change_get_data(change) == NULL)
				continue;

			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_ADDED);
			osync_context_report_change(ctx, change);
		}
	} else {
		while ((entry = psyncDBGetNextModified(db, &error))) {
			OSyncChange *change = psyncContactCreate(env, entry, &error);
			if (!change)
				goto error;
			
			osync_context_report_change(ctx, change);
		}
	}
				
	if (osync_error_is_set(&error))
		goto error_close_db;
	
	psyncDBClose(db);
	
	osync_context_report_success(ctx);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error_close_db:
	psyncDBClose(db);
error:
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_context_report_osyncerror(ctx, error);
	osync_error_unref(&error);
}

/*
		if (osync_error_get_type(&error) == OSYNC_ERROR_FILE_NOT_FOUND) {
			if ((dbCreated == FALSE) && !strcmp(database, "DatebookDB")) {
				//Unlock our Mutex so the palm does not die will the messagebox is open
				dbCreated = TRUE;
				if (dlp_CreateDB(env->socket, 1684108389, 1145132097, 0, 8, 0, "DatebookDB", &dbhandle) < 0) {
					dlp_AddSyncLogEntry(env->socket, "Unable to create Calendar.\n");
					osync_error_set(&error, OSYNC_ERROR_FILE_NOT_FOUND, "Unable to create Calendar");
					goto error;
				}
				env->database = dbhandle;
				dlp_AddSyncLogEntry(env->socket, "Created Calendar.\n");
				osync_trace(TRACE_INTERNAL, "Created Calendar.");
			}
		} else {*/


static void psyncContactCommit(void *data, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, data, info, ctx, change);
	PSyncEnv *env = (PSyncEnv *)data;
	PSyncDatabase *db = NULL;
	PSyncEntry *entry = NULL;
	PSyncContactEntry *contact = NULL;
	OSyncError *error = NULL;
	unsigned long id = 0;

	//open the DB
	if (!(db = psyncDBOpen(env, "AddressDB", &error)))
		goto error;
	
	contact = (PSyncContactEntry *)osync_change_get_data(change);
			
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_MODIFIED:
			//Get the id
			id = psyncUidGetID(osync_change_get_uid(change), &error);
			if (!id)
				goto error;
			
			PSyncEntry *orig_entry = psyncDBGetEntryByID(db, id, &error);
			if (!orig_entry)
				goto error;
			
			PSyncContactEntry *orig_contact = osync_try_malloc0(sizeof(PSyncContactEntry), &error);
			if (!orig_contact)
				goto error;
		
#ifdef OLD_PILOT_LINK
			unpack_Address(&(orig_contact->address), orig_entry->buffer, db->size);
#else
			unpack_Address(&(orig_contact->address), orig_entry->buffer, address_v1);
#endif
	
			if ((orig_contact->address.showPhone) > 4)
				orig_contact->address.showPhone = 0;
			contact->address.showPhone = orig_contact->address.showPhone;

			g_free(orig_entry);
			g_free(orig_contact);
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
#ifdef OLD_PILOT_LINK
			entry->size = pack_Address(&(contact->address), entry->buffer, 0xffff);
#else
			entry->buffer = pi_buffer_new(65536);
			entry->size = pack_Address(&(contact->address), entry->buffer, address_v1);
#endif
			if (!psyncDBWrite(db, entry, &error))
				goto error;
				
			break;
		case OSYNC_CHANGE_TYPE_ADDED:
			//Add a new entry
			osync_trace(TRACE_INTERNAL, "Find category");
			
			entry = osync_try_malloc0(sizeof(PSyncEntry), &error);
			if (!entry)
				goto error;
			entry->id = id;
			
			GList *c = NULL;
			for (c = contact->categories; c; c = c->next) {
				osync_trace(TRACE_INTERNAL, "searching category %s\n", c->data);
				entry->category = psyncDBCategoryToId(db, c->data, NULL);
				if (entry->category != 0) {
					osync_trace(TRACE_INTERNAL, "Found category %i\n", entry->category);
					break;
				}
			}
			
			osync_trace(TRACE_INTERNAL, "Adding new entry");
			
#ifdef OLD_PILOT_LINK
			entry->size = pack_Address(&(contact->address), entry->buffer, 0xffff);
#else
			entry->buffer = pi_buffer_new(65536);
			entry->size = pack_Address(&(contact->address), entry->buffer, address_v1);
#endif
			
			if (!psyncDBAdd(db, entry, &id, &error))
				goto error;
			
			//Make the new uid
			char *uid = g_strdup_printf("uid-AddressDB-%ld", id);
			osync_change_set_uid(change, uid);
			g_free(uid);
			break;
		case OSYNC_CHANGE_TYPE_DELETED:
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
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

static void psyncContactSyncDone(void *data, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, data, info, ctx);
	PSyncEnv *env = (PSyncEnv *)data;

	PSyncDatabase *db = NULL;
	OSyncError *error = NULL;

	if ((db = psyncDBOpen(env, "AddressDB", &error))) {
		osync_trace(TRACE_INTERNAL, "Reseting Sync Flags for AddressDB");
		dlp_ResetSyncFlags(env->socket, db->handle);
		dlp_CleanUpDatabase(env->socket, db->handle);
		psyncDBClose(db);
	} else {
		osync_context_report_osyncerror(ctx, error);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
		osync_error_unref(&error);
		return;
	}

	osync_context_report_success(ctx);
	osync_trace(TRACE_EXIT, "%s", __func__);
	return;
}

osync_bool psyncContactInitialize(PSyncEnv *env, OSyncPluginInfo *info, OSyncError **error)
{
	OSyncFormatEnv *formatenv = osync_plugin_info_get_format_env(info);
	env->contact_format = osync_format_env_find_objformat(formatenv, "palm-format");
	
	env->contact_sink = osync_objtype_sink_new("contact", error);
	if (!env->contact_sink)
		return FALSE;
	
	osync_objtype_sink_add_objformat(env->contact_sink, "palm-format");
	
	OSyncObjTypeSinkFunctions functions;
	memset(&functions, 0, sizeof(functions));
	functions.get_changes = psyncContactGetChanges;
	functions.commit = psyncContactCommit;
	functions.sync_done = psyncContactSyncDone;
	
	/* We pass the OSyncFileDir object to the sink, so we dont have to look it up
	 * again once the functions are called */
	osync_objtype_sink_set_functions(env->contact_sink, functions, NULL);
	osync_plugin_info_add_objtype(info, env->contact_sink);
	
	return TRUE;
}
