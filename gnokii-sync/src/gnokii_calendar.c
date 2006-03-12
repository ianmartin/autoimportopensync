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
	
	char *tmp = g_strup(""); 

	if (calnote->type)
		tmp = g_strdup_printf("%s-%i", tmp, calnote->type);

	if (calnote->time.year)
		tmp = g_strdup_printf("%s-%i%i%i.%i%i%i", 
					tmp,
					calnote->time.year,
					calnote->time.month,
					calnote->time.day,
					calnote->time.hour,
					calnote->time.minute,
					calnote->time.second);

	
/* 
 * TODO: gnokii can not write (at the moment (gnokii version 0.6.12) the end_time on cellphones.
 * See README for BUGS/KNOWN PROBLEMS.
 * We ignore this hash .. otherwise entries will disappear in PIMs :(
 
   
	if (calnote->end_time.year)
		tmp = g_strdup_printf("%s-%i%i%i.%i%i%i",
					tmp,
					calnote->end_time.year,
					calnote->end_time.month,
					calnote->end_time.day,
					calnote->end_time.hour,
					calnote->end_time.minute,
					calnote->end_time.second);

*/

	if (calnote->alarm.enabled)
		tmp = g_strdup_printf("%s-%i%i.%i%i%i.%i%i%i",
					tmp,
					calnote->alarm.enabled,
					calnote->alarm.tone,
					calnote->alarm.timestamp.year,
					calnote->alarm.timestamp.month,
					calnote->alarm.timestamp.day,
					calnote->alarm.timestamp.hour,
					calnote->alarm.timestamp.minute,
					calnote->alarm.timestamp.second);

	if (calnote->text)
		tmp = g_strdup_printf("%s-%s", tmp, calnote->text);

	if (calnote->type == GN_CALNOTE_CALL)
		tmp = g_strdup_printf("%s-%s", tmp, calnote->phone_number);

/* 
 * TODO: gnokii can not write (at the moment (gnokii version 0.6.12) the meeting location (mlocation) on cellphones.
 * See README for BUGS/KNOWN PROBLEMS.   
 */
	if (calnote->mlocation)
		tmp = g_strdup_printf("%s-%s", tmp, calnote->mlocation);

	if (calnote->recurrence)
		tmp = g_strdup_printf("%s-%i", tmp, calnote->recurrence);
	
#ifndef HIDE_SENSITIVE	
	osync_trace(TRACE_INTERNAL, "HASH LINE: %s", tmp);
#endif	
	tmp = g_strdup_printf("%u", g_str_hash(tmp));

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
		return NULL;
	}

	// check if there were any other errors
	if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): error while query the phone - gnokii: %s",	gn_error_print(error));
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

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, calnote);

        gn_error error = GN_ERR_NONE;
        gn_data* data = (gn_data *) malloc(sizeof(gn_data));

        gn_data_clear(data);

	calnote->location = 0;

        data->calnote = calnote;

#ifndef HIDE_SENSITIVE	
	osync_trace(TRACE_INTERNAL, "calnote->location: %i\n"
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
#endif	

        error = gn_sm_functions(GN_OP_WriteCalendarNote, data, state);

        if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): Couldn't write calnote: %s\n", __func__, gn_error_print(error));
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
 *
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_calendar_get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	int i = 0;
	char *hash = NULL;
	char *uid = NULL;
	gn_error error_note = GN_ERR_NONE; 	// gnokii error messages
	gn_calnote *calnote = NULL;

	gn_data	*caldata = (gn_data *) malloc(sizeof(gn_data));

	memset(&calendar_list, 0, sizeof(gn_calnote_list));
	memset(caldata, 0, sizeof(gn_data));

	caldata->calnote_list = &calendar_list; 

	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	// check for slowsync and prepare the "event" hashtable if needed
	if (osync_member_get_slow_sync(env->member, "event") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		osync_hashtable_set_slow_sync(env->hashtable, "event");
	}

	// get all notes 
	for (i=1; error_note == GN_ERR_NONE; i++) {

		calnote = gnokii_calendar_get_calnote(i, caldata, env->state, error_note);
		
		if (calnote == NULL)
			break;

		OSyncChange *change = osync_change_new();

		// prepare UID with gnokii-calendar-<memory location>
		uid = g_strdup_printf ("gnokii-calendar-%i", calnote->location);
		osync_change_set_uid(change, uid);

		// get hash of calnote
		hash = gnokii_calendar_hash(calnote);
		osync_change_set_hash(change, hash);	

		osync_change_set_objformat_string(change, "gnokii-event"); 
		osync_change_set_objtype_string(change, "event");

		osync_change_set_data(change, (void *)calnote, sizeof(gn_calnote), TRUE);
	
		if (osync_hashtable_detect_change(env->hashtable, change)) {
			osync_trace(TRACE_INTERNAL, "Position: %i Needs to be reported (!= hash)", calnote->location);
			osync_context_report_change(ctx, change);
			osync_hashtable_update_hash(env->hashtable, change);
		}	

	}

	osync_debug("GNOKII", 2, "number of calendar notes: %i", i - 1);

	osync_hashtable_report_deleted(env->hashtable, ctx, "event");

	g_free(hash);
	g_free(uid);
	g_free(caldata);

	osync_trace(TRACE_EXIT, "%s", __func__);
	return TRUE;
}

/* The function commit changes of calendar entries to the cellphone.
 * 
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_calendar_commit(OSyncContext *ctx, OSyncChange *change) {

	osync_trace(TRACE_ENTRY, "%s() (%p, %p)", __func__, ctx, change);

	OSyncError *error = NULL;
	gn_calnote *calnote = NULL;
	char *uid = NULL;
	char *hash = NULL;
	
	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	// Get changed calendar note
	calnote = (gn_calnote *) osync_change_get_data(change);

	// Check for type of changes
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			// Delete the change

			if (!gnokii_calendar_delete_calnote(osync_change_get_uid(change), env->state)) {

				osync_error_update(&error, "Unable to delete event.");
				goto error;
			}
			
			break;
		case CHANGE_ADDED:

			// Add the change
			if (!gnokii_calendar_write_calnote(calnote, env->state)) {
				osync_error_update(&error, "Unable to write event.");
				goto error;
			}

			// memory location is known after note was written
			uid = gnokii_calendar_memory_uid(calnote->location);
			osync_change_set_uid(change, uid);

			// generate and set hash of entry
			hash = gnokii_calendar_hash(calnote);
			osync_change_set_hash(change, hash);

			break;
		case CHANGE_MODIFIED:
			// Modify the change

			// libgnokii can not modify - delete the old calnote and add a new note
			if (!gnokii_calendar_delete_calnote(osync_change_get_uid(change), env->state)) {
				osync_error_update(&error, "Unable to modify (delete) event.");
				goto error;
			}

			if (!gnokii_calendar_write_calnote(calnote, env->state)) {
				osync_error_update(&error, "Unable to modify (write) event.");
				goto error;
			}

			/////////// WORKAROUND for "dirty" modify ///////////

			// fake a delete change to remove the old hash
			OSyncChange *delete_change = osync_change_new();

			// the old uid will be set for this "fake" change
			osync_change_set_uid(delete_change, osync_change_get_uid(change));
			osync_change_set_changetype(delete_change, CHANGE_DELETED);
			osync_hashtable_update_hash(env->hashtable, delete_change);

			// update the old UID with the followed UID for the hashtable
			// - is needed for new memory location
			uid = gnokii_calendar_memory_uid(calnote->location);
			osync_change_set_uid(change, uid);

			// set modified changetype for calendar entry 
			osync_change_set_changetype(change, CHANGE_MODIFIED);

			// set hash for the modified calendar entry
			hash = gnokii_calendar_hash(calnote);
			osync_change_set_hash(change, hash);

			osync_trace(TRACE_INTERNAL, "New UID: %s (%s) changetype: %i", 
					osync_change_get_uid(change), hash,
					osync_change_get_changetype(change));

			////////// END OF WORKAROUND ////////////////

			break;
		default:
			osync_trace(TRACE_INTERNAL, "Unknown change type...");
			break;
	}
	
	// answer the call
	osync_context_report_success(ctx);

	// update hashtable
	osync_hashtable_update_hash(env->hashtable, change);

	// free uid and hash
	g_free(uid);
	g_free(hash);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	return FALSE;
}

