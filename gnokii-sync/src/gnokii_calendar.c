/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"

gn_calnote_list calendar_list;	// keeps "position" of every event

/* The function generate a event uid string for calendar notes.
 * "gnokii-calendar-<memory location>"
 *
 * Returns: event uid 
 */
char *gnokii_calendar_memory_uid(int location) {

	osync_trace(TRACE_ENTRY, "%s()", __func__); 

	char *uid = NULL;

	uid = g_strdup_printf("gnokii-calendar-%i", location);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, uid);
	return uid; 
}

/* The function gets from a event uid string the memory location.
 *
 * Returns: memory location of a event
 * ReturnVal: -1 on error
 * ReturnVal: < 0 (memory location)
 */
int gnokii_calendar_get_memorylocation(const char *uid) {
	
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, uid);

	int memlocation;

	if (sscanf(uid, "gnokii-calendar-%u", &memlocation) == EOF) {
		osync_trace(TRACE_EXIT_ERROR, "%s: cannot get memory location from uid string.", __func__);

		return -1;
	}

	osync_trace(TRACE_EXIT, "%s: %i", __func__, memlocation);
	
	return memlocation;
}

/* The function gets from the (global) calendar_list the position of the entry.
 * This is needed to delete or modify an event, because gnokii
 * delete by position no by memory location (UID).
 *
 * Returns: Position of the Calendar note
 * ReturnVal:  -1 on error
 * ReturnVal: < 0 (position)
 */
int gnokii_calendar_get_position(int memlocation) {

	osync_trace(TRACE_ENTRY, "%s(%i)", __func__, memlocation);

	int i;

	for (i=0; calendar_list.location[i]; i++) {
		osync_trace(TRACE_INTERNAL, "calendar_list.location[%i] -> %i", i, calendar_list.location[i]);
			
		if (calendar_list.location[i] == memlocation) {
			osync_trace(TRACE_EXIT, "%s: %i", __func__, i + 1);
			return i + 1;
		}
	}

	osync_trace(TRACE_EXIT_ERROR, "%s: cannot remember position of given uid!", __func__);
	return -1; 
}

/* The function generates a simple hash of the given event entry.
 * 
 * Returns: Hash of the event.
 */ 
char *gnokii_calendar_hash(gn_calnote *calnote) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, calnote);
	
	char *tmp = NULL; 
	GString *hash = g_string_new("");	

	if (calnote->type) {
		tmp = g_strdup_printf("%i", calnote->type);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}

	if (calnote->time.year) {
		tmp = g_strdup_printf("%i%i%i.%i%i%i", 
					calnote->time.year,
					calnote->time.month,
					calnote->time.day,
					calnote->time.hour,
					calnote->time.minute,
					calnote->time.second);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}

	if (calnote->end_time.year) {
		tmp = g_strdup_printf("%i%i%i.%i%i%i",
					calnote->end_time.year,
					calnote->end_time.month,
					calnote->end_time.day,
					calnote->end_time.hour,
					calnote->end_time.minute,
					calnote->end_time.second);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}


	if (calnote->alarm.enabled) {
		tmp = g_strdup_printf("%i%i.%i%i%i.%i%i%i",
					calnote->alarm.enabled,
					calnote->alarm.tone,
					calnote->alarm.timestamp.year,
					calnote->alarm.timestamp.month,
					calnote->alarm.timestamp.day,
					calnote->alarm.timestamp.hour,
					calnote->alarm.timestamp.minute,
					calnote->alarm.timestamp.second);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}

	if (calnote->text) {
		hash =  g_string_append(hash, calnote->text);
	}

	if (calnote->type == GN_CALNOTE_CALL) {
		hash = g_string_append(hash, calnote->phone_number);
	}

	if (calnote->mlocation) {
		hash = g_string_append(hash, calnote->mlocation);
	}

	if (calnote->recurrence) {
		tmp = g_strdup_printf("%i", calnote->recurrence);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}
	
	osync_trace(TRACE_SENSITIVE, "HASH LINE: %s", hash->str);

	tmp = g_strdup_printf("%u", g_str_hash(hash->str));

	g_string_free(hash, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);

	return tmp;
}

/* The function get the Nth (@parm: pos) calendar entry of the cellphone.
 *
 * Returns: full filled calendar note
 * ReturnVal: gn_calnote*
 * ReturnVal: NULL on error or when no entries are left on cellphone.
 */
gn_calnote *gnokii_calendar_get_calnote(int pos, gn_data *caldata, struct gn_statemachine *state, gn_error error) {

	osync_trace(TRACE_ENTRY, "%s(%i, %i)", __func__, pos, error);
	
	// will be destroyed by destroy_gnokii_event() of gnokii-format 
	gn_calnote *calnote = (gn_calnote *) malloc(sizeof(gn_calnote));

	// keep our new calnote clean :)
	memset(calnote, 0, sizeof(gn_calnote));
	
	// next location/calnote
	calnote->location = pos;

	caldata->calnote = calnote;

	// get the nth (pos) entry of the cellphone. 
	error = gn_sm_functions(GN_OP_GetCalendarNote, caldata, state);
	
	// check if the location is empty and return NULL
	if (error == GN_ERR_EMPTYLOCATION) {
		osync_trace(TRACE_EXIT, "%s: no calendar note left.", __func__);
		g_free(calnote);
		return NULL;
	}

	// check if there were any other errors
	if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): error while query the phone - gnokii: %s", __func__, gn_error_print(error));
		g_free(calnote);
		return NULL;
	}

	osync_trace(TRACE_EXIT, "%s:%p", __func__, calnote);
	return calnote; 
}

/* This function writes event entries on the cellphone which need a correct 
 * filled calendar note.
 *
 * TODO: check for correct calnote before writing it...(?)
 *
 * Return: bool 
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_calendar_write_calnote(gn_calnote *calnote, struct gn_statemachine *state) {

	osync_trace(TRACE_ENTRY, "%s", __func__);

        gn_error error = GN_ERR_NONE;
        gn_data* data = (gn_data *) malloc(sizeof(gn_data));

        gn_data_clear(data);

	calnote->location = 0;

        data->calnote = calnote;

	osync_trace(TRACE_SENSITIVE, "calnote->location: %i\n"
			            "calnote->text: %s\n"
				    "calnote->type: %i\n"
				    "calnote->time: %04i.%02i.%02i-%02i:%02i:%02i\n" 
				    "calnote->end_time: %04i.%02i.%02i-%02i:%02i:%02i\n"
				    "calnote->alarm.enabled: %i\n"
				    "calnote->alarm.tone: %i\n"
				    "calnote->alarm.timestamp: %04i.%02i.%02i-%02i:%02i:%02i\n"
				    "calnote->mlocation: %s\n"
				    "calnote->phone_number: %s\n"
				    "calnote->recurrence: %i",
			calnote->location, calnote->text, calnote->type,

			calnote->time.year, calnote->time.month, 
			calnote->time.day, calnote->time.hour, 
			calnote->time.minute, calnote->time.second,

			calnote->end_time.year, calnote->end_time.month, 
			calnote->end_time.day, calnote->end_time.hour,
			calnote->end_time.minute, calnote->end_time.second,

			calnote->alarm.enabled, calnote->alarm.tone,
			calnote->alarm.timestamp.year, calnote->alarm.timestamp.month,
			calnote->alarm.timestamp.day, calnote->alarm.timestamp.hour,
			calnote->alarm.timestamp.minute, calnote->alarm.timestamp.second,

			calnote->mlocation, calnote->phone_number,

			calnote->recurrence);


        error = gn_sm_functions(GN_OP_WriteCalendarNote, data, state);

        if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): Couldn't write calnote: %s\n", __func__, gn_error_print(error));
		g_free(data);
		return FALSE;
	}

	g_free(data);

	osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

/* This function delete an event on the cellphone.
 * gnokii_calendar_get_position() have to be used to get the position.
 * Because calendar notes get deleted by position in the cellphone (sorted by 
 * time) and _NOT_ by memory location!
 *
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */ 
osync_bool gnokii_calendar_delete_calnote(const char *uid, struct gn_statemachine *state) {

	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, uid);

	gn_error error = GN_ERR_NONE;
	gn_calnote *calnote = (gn_calnote *) malloc(sizeof(gn_calnote));
	gn_data *data = (gn_data *) malloc(sizeof(gn_data));

	// get position
	int mem_location = gnokii_calendar_get_memorylocation(uid);
	int position = gnokii_calendar_get_position(mem_location);

	gn_data_clear(data);

	// set position (not memory location! )to delete
	calnote->location = position; 
	data->calnote = calnote;
	data->calnote_list = &calendar_list;

	error = gn_sm_functions(GN_OP_DeleteCalendarNote, data, state);
	if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): Couldn't delete calnote: %s\n", __func__, gn_error_print(error));
		return FALSE;
	}

	g_free(calnote);
	g_free(data);
	
	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/* The function get all event entries with gnokii_calendar_get_calnote() and checks for
 * changes by comparing old hash with new hash....
 */

void gnokii_calendar_get_changes(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, plugindata, info, ctx);

	int i = 0;
	char *hash = NULL;
	char *uid = NULL;
	gn_error error_note = GN_ERR_NONE; 	// gnokii error messages
	gn_calnote *calnote = NULL;

	
	OSyncError *error = NULL;


        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);

	gn_data	*caldata = (gn_data *) malloc(sizeof(gn_data));

	memset(&calendar_list, 0, sizeof(gn_calnote_list));
	memset(caldata, 0, sizeof(gn_data));

	caldata->calnote_list = &calendar_list; 

        gnokii_sinkenv *sinkenv = osync_objtype_sink_get_userdata(sink);
	gnokii_environment *env = (gnokii_environment *) plugindata;

	// reset reports internal hashtable list, to discover deleted entries
	osync_hashtable_reset_reports(sinkenv->hashtable);

	// check for slowsync and prepare the "event" hashtable if needed
	if (osync_objtype_sink_get_slowsync(sink)) {		
		osync_trace(TRACE_INTERNAL, "slow sync");
		assert(sinkenv->hashtable);
		if (!osync_hashtable_slowsync(sinkenv->hashtable, &error)) {
			osync_context_report_osyncerror(ctx, error);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
			return;
		}
	}

	// get all notes 
	for (i=1; error_note == GN_ERR_NONE; i++) {

		calnote = gnokii_calendar_get_calnote(i, caldata, env->state, error_note);
		
		if (calnote == NULL)
			break;

		// prepare UID with gnokii-calendar-<memory location>
		uid = g_strdup_printf ("gnokii-calendar-%i", calnote->location);
		osync_hashtable_report(sinkenv->hashtable, uid);

		hash = gnokii_calendar_hash(calnote);
		OSyncChangeType type = osync_hashtable_get_changetype(sinkenv->hashtable, uid, hash);

		if (type == OSYNC_CHANGE_TYPE_UNMODIFIED) {
			g_free(hash);
			g_free(uid);
			g_free(calnote);
			continue;
		}

		osync_hashtable_update_hash(sinkenv->hashtable, type, uid, hash);

		OSyncChange *change = osync_change_new(&error);

		osync_change_set_uid(change, uid);
		osync_change_set_hash(change, hash);	
		osync_change_set_changetype(change, type);

		// set data
		osync_trace(TRACE_INTERNAL, "objformat: %p", sinkenv->objformat);
		OSyncData *odata = osync_data_new((char *) calnote, sizeof(gn_calnote), sinkenv->objformat, &error);
		if (!odata) {
			osync_change_unref(change);
			osync_context_report_osyncwarning(ctx, error);
			osync_error_unref(&error);
			g_free(hash);
			g_free(uid);
			g_free(calnote);
			continue;
		}

		osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
		osync_change_set_data(change, odata);
		osync_data_unref(odata);

		osync_context_report_change(ctx, change);

		osync_trace(TRACE_INTERNAL, "Change: %p", change);

		osync_change_unref(change);

		g_free(hash);
		g_free(uid);
	}

	osync_trace(TRACE_INTERNAL, "number of calendar notes: %i", i - 1);

	assert(sinkenv->hashtable);
        char **uids = osync_hashtable_get_deleted(sinkenv->hashtable);
        for (i = 0; uids[i]; i++) {
                OSyncChange *change = osync_change_new(&error);
                if (!change) {
                        g_free(uids[i]);
                        osync_context_report_osyncwarning(ctx, error);
                        osync_error_unref(&error);
                        continue;
                }

                osync_change_set_uid(change, uids[i]);
                osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);

                OSyncData *odata = osync_data_new(NULL, 0, sinkenv->objformat, &error);
                if (!odata) {
                        g_free(uids[i]);
                        osync_change_unref(change);
                        osync_context_report_osyncwarning(ctx, error);
                        osync_error_unref(&error);
                        continue;
                }

                osync_data_set_objtype(odata, osync_objtype_sink_get_name(sink));
                osync_change_set_data(change, odata);
                osync_data_unref(odata);

                osync_context_report_change(ctx, change);

                osync_hashtable_update_hash(sinkenv->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), NULL);

                osync_change_unref(change);
                g_free(uids[i]);
        }
        g_free(uids);


	g_free(caldata);

	osync_context_report_success(ctx);

	osync_trace(TRACE_EXIT, "%s", __func__);
}

/* The function commit changes of calendar entries to the cellphone. */
void gnokii_calendar_commit_change(void *plugindata, OSyncPluginInfo *info, OSyncContext *ctx, OSyncChange *change) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %p)", __func__, plugindata, info, change, ctx);

	OSyncError *error = NULL;
	gn_calnote *calnote = NULL;
	char *uid = NULL;
	char *hash = NULL;
	char *buf = NULL;
	
        OSyncObjTypeSink *sink = osync_plugin_info_get_sink(info);
        gnokii_sinkenv *sinkenv = osync_objtype_sink_get_userdata(sink);
	gnokii_environment *env = (gnokii_environment *) plugindata;

	// Get changed calendar note
	OSyncData *data = osync_change_get_data(change);
	osync_data_get_data(data, &buf, NULL);
	calnote = (gn_calnote *) buf;

	// Check for type of changes
	switch (osync_change_get_changetype(change)) {
		case OSYNC_CHANGE_TYPE_DELETED:
			// Delete the change

			if (!gnokii_calendar_delete_calnote(osync_change_get_uid(change), env->state)) {

				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete event.");
				goto error;
			}
			
			break;
		case OSYNC_CHANGE_TYPE_ADDED:

			// Add the change
			if (!gnokii_calendar_write_calnote(calnote, env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to write event.");
				goto error;
			}

			// memory location is known after note was written
			uid = gnokii_calendar_memory_uid(calnote->location);
			osync_change_set_uid(change, uid);
			g_free(uid);

			// generate and set hash of entry
			hash = gnokii_calendar_hash(calnote);
			osync_change_set_hash(change, hash);
			g_free(hash);

			break;
		case OSYNC_CHANGE_TYPE_MODIFIED:
			// Modify the change

			// libgnokii can not modify - delete the old calnote and add a new note
			if (!gnokii_calendar_delete_calnote(osync_change_get_uid(change), env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to modify (delete) event.");
				goto error;
			}

			if (!gnokii_calendar_write_calnote(calnote, env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to modify (write) event.");
				goto error;
			}

			/////////// WORKAROUND for "dirty" modify ///////////
			/*

			// fake a delete change to remove the old hash
			OSyncChange *delete_change = osync_change_new(&error);

			// the old uid will be set for this "fake" change
			osync_change_set_uid(delete_change, osync_change_get_uid(change));
			osync_change_set_changetype(delete_change, OSYNC_CHANGE_TYPE_DELETED);
			osync_hashtable_update_hash(sinkenv->hashtable, OSYNC_CHANGE_TYPE_DELETED, osync_change_get_uid(change), delete_change);
			*/

			osync_hashtable_delete(sinkenv->hashtable, osync_change_get_uid(change));

			// update the old UID with the followed UID for the hashtable
			// - is needed for new memory location
			uid = gnokii_calendar_memory_uid(calnote->location);
			osync_change_set_uid(change, uid);


			// set modified changetype for calendar entry 
			osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_MODIFIED);

			// set hash for the modified calendar entry
			hash = gnokii_calendar_hash(calnote);
			osync_change_set_hash(change, hash);

			osync_hashtable_write(sinkenv->hashtable, uid, hash);

			g_free(hash);
			g_free(uid);

			/*
			osync_trace(TRACE_INTERNAL, "New CHANGE: %p UID: %s (%s) changetype: %i", 
					change,
					osync_change_get_uid(change), hash,
					osync_change_get_changetype(change));
			*/		

			////////// END OF WORKAROUND ////////////////

			break;
		default:
			osync_trace(TRACE_INTERNAL, "Unknown change type...");
			break;
	}
	
	osync_context_report_success(ctx);
			       
	// update hashtable
	osync_hashtable_update_hash(sinkenv->hashtable, osync_change_get_changetype(change), osync_change_get_uid(change), osync_change_get_hash(change));

	osync_trace(TRACE_EXIT, "%s", __func__);
	return;

error:
	osync_context_report_osyncerror(ctx, error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_unref(&error);
}

