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

/* This function generate the uid for the contact entry.
 * UID looks like this:  gnokii-contact-<MEMORY TYPE>-<MEMORY LOCATION>  
 *             Example:  gnokii-contact-ME-9
 *
 * Returns: UID of the contact
 */
char *gnokii_contact_uid(gn_phonebook_entry *contact) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, contact);
	
	char *uid, *memory_type = NULL;

	if (contact->memory_type == GN_MT_ME)
		memory_type = g_strdup("ME");
	else if (contact->memory_type == GN_MT_SM)
		memory_type = g_strdup("SM");

	uid = g_strdup_printf("gnokii-contact-%s-%i", memory_type, contact->location);

	g_free(memory_type);
			
	osync_trace(TRACE_EXIT, "%s: %s", __func__, uid);
	return uid;
}

/* The function extract the memory location. (not the memory type!)
 * 
 * Returns: location 
 */ 
void gnokii_contact_memlocation(const char *uid, gn_phonebook_entry *contact) {
	
	int location = 0;
	char memtype[3];	    

	sscanf(uid, "gnokii-contact-%2s-%i", memtype, &location); 

	contact->location = location;
	contact->memory_type = gn_str2memory_type(memtype); 
}

/* The function generates a simple hash of the given contact entry.
 * 
 * Returns: Hash of the contact.
 */ 
char *gnokii_contact_hash(gn_phonebook_entry *contact) {

	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, contact);
	
	int i;
	char *tmp = NULL; 
	GString *hash = g_string_new("");

	if (contact->name) {
		hash = g_string_append(hash, contact->name);
	}

	if (contact->caller_group) {
		tmp = g_strdup_printf("%i", contact->caller_group);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}

	if (contact->date.year) {
		tmp = g_strdup_printf("%i%i%i.%i%i%i.%i",
				contact->date.year,
				contact->date.month,
				contact->date.day,
				contact->date.hour,
				contact->date.minute,
				contact->date.second,
				contact->date.timezone);
		hash = g_string_append(hash, tmp);
		g_free(tmp);
	}

	for (i=0; i < contact->subentries_count; i++) {
		tmp = g_strdup_printf("sub%i", i);
		hash = g_string_append(hash, tmp);
		g_free(tmp);

		if (contact->subentries[i].entry_type) {
			tmp = g_strdup_printf("%i", contact->subentries[i].entry_type);
			hash = g_string_append(hash, tmp);
			g_free(tmp);
		}

		if (contact->subentries[i].number_type) {
			tmp = g_strdup_printf("%i", contact->subentries[i].number_type);
			hash = g_string_append(hash, tmp);
			g_free(tmp);
		}

		if (contact->subentries[i].data.number) {
			hash = g_string_append(hash, contact->subentries[i].data.number);
		}
	}
	
	osync_trace(TRACE_SENSITIVE, "HASH LINE: %s", hash->str);

	tmp = g_strdup_printf("%u", g_str_hash(hash->str));

	g_string_free(hash, TRUE);

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);

	return tmp;
}

/* The function return a free location for a contact entry.
 *  
 * Returns: filled contact note with memory_type and location
 */
gn_phonebook_entry *gnokii_contact_freelocation(struct gn_statemachine *state) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, state);

	int memtype, i;
	gn_error error;
	gn_phonebook_entry *contact = (gn_phonebook_entry *) malloc(sizeof(gn_phonebook_entry));
	gn_data *data = (gn_data *) malloc(sizeof(gn_data));
	memset(data, 0, sizeof(gn_data));
	memset(contact, 0, sizeof(gn_phonebook_entry));

	for (memtype=0; memtype <= GN_MT_SM; memtype++) {
		contact->memory_type = memtype;

		for (i=1;;i++) {
			contact->location = i;
			data->phonebook_entry = contact;

			error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);

			if (error == GN_ERR_INVALIDMEMORYTYPE) {
				osync_trace(TRACE_INTERNAL, "gnokii contact error: %s, exiting loop.", gn_error_print(error));
				break;
			}

			if (error == GN_ERR_EMPTYLOCATION) {
				osync_trace(TRACE_EXIT, "%s(): memorty_type: %i location: %i counter: %i", __func__, contact->memory_type, contact->location, i);
				return contact;
			}

			if (error != GN_ERR_NONE)
				osync_trace(TRACE_INTERNAL, "gnokii error: %s\n", gn_error_print(error));
		}
	}

	// TODO set error and leave
	osync_trace(TRACE_EXIT, "%s(): NO FREE LOCATION!", __func__);
	return NULL;
}

/* The function get the Nth (@parm: pos) phonebook entry of the cellphone.
 *
 * Returns: full filled contact note
 * ReturnVal: gn_phonebook_entry*
 * ReturnVal: NULL on error or when no entries are left on cellphone.
 */
gn_phonebook_entry *gnokii_contact_read(int memory_type, int pos, gn_data *data, struct gn_statemachine *state, gn_error *error) {

	osync_trace(TRACE_ENTRY, "%s(%i, %i, %p, %p, %i)", __func__, memory_type, pos, data, state, error);
	
	// will be destroyed by destroy_gnokii_contact() of gnokii-format 
	gn_phonebook_entry *contact = (gn_phonebook_entry *) malloc(sizeof(gn_phonebook_entry));

	// keep our new contact clean :)
	memset(contact, 0, sizeof(gn_phonebook_entry));
	
	// next location/contact
	contact->location = pos;

	// memory type of phonebook entry
	contact->memory_type = memory_type;

	data->phonebook_entry = contact;

	// get the nth (pos) entry of the cellphone. 
	*error = gn_sm_functions(GN_OP_ReadPhonebook, data, state);

	// ignore empty locations
	if (*error == GN_ERR_EMPTYLOCATION) {
		g_free(contact);
		osync_trace(TRACE_EXIT, "%s: empty location", __func__);
		return NULL;
	}

	// check if there were any other errors
	if (*error != GN_ERR_NONE) {
		g_free(contact);
		osync_trace(TRACE_EXIT_ERROR, "%s(): error while query the phone - gnokii: %s",	__func__, gn_error_print(*error));
		return NULL;
	}

	osync_trace(TRACE_EXIT, "%s: Contact [%i][%i]", __func__, contact->memory_type, contact->location);
	return contact; 
}

/* This function writes contact entries on the cellphone which need a correct 
 * filled contact note.
 *
 * TODO: check for correct contact before writing it...(gn_phonebook_entry_sanitize()?)
 *
 * Return: bool 
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_contact_write(gn_phonebook_entry *contact, struct gn_statemachine *state) {

	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, contact, state);

	int i;
        gn_error error = GN_ERR_NONE;
        gn_data* data = (gn_data *) malloc(sizeof(gn_data));

        gn_data_clear(data);

	if (!contact->location) {
		gn_phonebook_entry *free_entry = gnokii_contact_freelocation(state);
		osync_trace(TRACE_INTERNAL, "Free location is %i at memtype: %i",
				free_entry->location, free_entry->memory_type);
		contact->location = free_entry->location;
		contact->memory_type = free_entry->memory_type;
		g_free(free_entry);
	}

	gn_phonebook_entry_sanitize(contact);
        data->phonebook_entry = contact;

	osync_trace(TRACE_SENSITIVE, "contact->location: %i\n"
				    "contact->empty: %i\n"
				    "contact->name: %s\n"
				    "contact->memory_type: %i\n"
				    "contact->caller_group: %i\n"
				    "contact->date: %04i-%02i-%02i %02i:%02i:%02i tz:%i\n"
				    "contact->subentries_count: %i\n",
			contact->location,
			contact->empty, contact->name,
			contact->memory_type,
			contact->caller_group, 
			
			contact->date.year, contact->date.month, contact->date.day,
			contact->date.hour, contact->date.minute, contact->date.second,
			contact->date.timezone,
			
			contact->subentries_count);

	for (i=0; i < contact->subentries_count; i++) { 

		osync_trace(TRACE_SENSITIVE, "subentry #%i Number: %s [Number TYpe: %i] [Entry Type: %i]",
				i,
				contact->subentries[i].data.number,
				contact->subentries[i].number_type,
				contact->subentries[i].entry_type);

	}

        error = gn_sm_functions(GN_OP_WritePhonebook, data, state);

        if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): Couldn't write contact: %s", __func__, gn_error_print(error));
		g_free(data);
		return FALSE;
	} else {
		osync_trace(TRACE_INTERNAL, "%s(): successfully written at %i on memory_type: %i", __func__, 
				contact->location, contact->memory_type); 
	}

	g_free(data);

	osync_trace(TRACE_EXIT, "%s", __func__);
        return TRUE;
}

/* This function delete an contact on the cellphone.
 * gnokii_contact_get_position() have to be used to get the position.
 * Because contact notes get deleted by position in the cellphone (sorted by 
 * time) and _NOT_ by memory location!
 *
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */ 
osync_bool gnokii_contact_delete(const char *uid, struct gn_statemachine *state) {

	osync_trace(TRACE_ENTRY, "%s(%s, %p)", __func__, uid, state);

//	int location = 0;
//	char memory_type_string[3];
	gn_error error = GN_ERR_NONE;

	gn_phonebook_entry *contact = (gn_phonebook_entry *) malloc(sizeof(gn_phonebook_entry));
	gn_data *data = (gn_data *) malloc(sizeof(gn_data));


	memset(contact, 0, sizeof(gn_phonebook_entry));

//	sscanf(uid, "gnokii-contact-%2s-%u", memory_type_string, &location);

	gnokii_contact_memlocation(uid, contact);
//	contact->memory_type = gn_str2memory_type(memory_type_string);
//	contact->location = location;
	contact->empty = 1;

	gn_data_clear(data);

	data->phonebook_entry = contact; 

	osync_trace(TRACE_INTERNAL, "Try to delete entry with Memory Type: %i at Location: %i\n", 
			contact->memory_type, contact->location);

	error = gn_sm_functions(GN_OP_DeletePhonebook, data, state);
	/* FIXME - don't check return code - return code is wrong - bug in libgnokii?
	if (error != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s(): Couldn't delete contact: %s\n", __func__, gn_error_print(error));
		return FALSE;
	}
	*/

	g_free(contact);
	g_free(data);
	
	osync_trace(TRACE_EXIT, "%s()", __func__);
	return TRUE;
}

/* The function get all contact entries with gnokii_contact_get_entry() and checks for
 * changes by comparing old hash with new hash....
 *
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_contact_get_changeinfo(OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, ctx);

	int location = 0, memtype, num_entries;
	char *hash = NULL;
	char *uid = NULL;
	gn_error gnokii_error = GN_ERR_NONE; 	// gnokii error messages
	gn_phonebook_entry *contact = NULL;
	gn_memory_status memstat;
	gn_data *data = (gn_data *) malloc(sizeof(gn_data));

	memset(data, 0, sizeof(gn_data));

	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	// check for slowsync and prepare the "contact" hashtable if needed
	if (osync_member_get_slow_sync(env->member, "contact") == TRUE) {
		osync_trace(TRACE_INTERNAL, "slow sync");
		osync_hashtable_set_slow_sync(env->hashtable, "contact");
	}

	// get all notes 
	for (memtype=0; memtype <= GN_MT_SM; memtype++) {

		location = 0;
		num_entries = 0;

		data->memory_status = &memstat;
		memstat.memory_type = memtype; 
		memstat.used = 0;
		gnokii_error = gn_sm_functions(GN_OP_GetMemoryStatus, data, env->state);
		if (gnokii_error != GN_ERR_NONE) {
			osync_trace(TRACE_EXIT_ERROR, "%s: gnokii memory stat error: %s (memory: %i)", __func__, 
					gn_error_print(gnokii_error), memtype);
			continue;
		}
		num_entries = memstat.used;

		osync_trace(TRACE_INTERNAL, "Memory Usage: Number of entries in MEM[%i]: %i", memtype, num_entries);

		while (num_entries > 0) {
			location++;

			gnokii_error = GN_ERR_NONE;
			contact = gnokii_contact_read(memtype, location, data, env->state, &gnokii_error);

			if ((gnokii_error == GN_ERR_NONE) && (contact != NULL)) {
				// Decrement the number of entries to fetch only if we got one
				num_entries--;
			} else { 
				// Check the various reasons why we did not get a good entry
				if (gnokii_error == GN_ERR_INVALIDMEMORYTYPE) {
					osync_trace(TRACE_INTERNAL, "gnokii contact error: %s, exiting loop.", gn_error_print(gnokii_error));
					break;
				}

				if (gnokii_error != GN_ERR_EMPTYLOCATION) {
	       				osync_trace(TRACE_INTERNAL, "gnokii contact error: %s", gn_error_print(gnokii_error));
					break;
				}
			}

			if (contact == NULL)
				continue;

			OSyncChange *change = osync_change_new();
			osync_change_set_member(change, env->member);

			// prepare UID with gnokii-contact-<memory type>-<memory location>
			uid = gnokii_contact_uid(contact);
			osync_change_set_uid(change, uid);
			g_free(uid);

			// get hash of contact 
			hash = gnokii_contact_hash(contact);
			osync_change_set_hash(change, hash);	
			g_free(hash);

			osync_change_set_objformat_string(change, "gnokii-contact"); 
			osync_change_set_objtype_string(change, "contact");

			osync_change_set_data(change, (void *)contact, sizeof(gn_phonebook_entry), TRUE);
		
			if (osync_hashtable_detect_change(env->hashtable, change)) {
				osync_trace(TRACE_INTERNAL, "Position: %i Needs to be reported (!= hash)", contact->location);
				osync_context_report_change(ctx, change);
				osync_hashtable_update_hash(env->hashtable, change);
			}	
		}
	}

	osync_trace(TRACE_INTERNAL, "number of contact notes: %i", location - 1);

	osync_hashtable_report_deleted(env->hashtable, ctx, "contact");

	osync_trace(TRACE_EXIT, "%s()", __func__);
	return TRUE;
}

/* The function commit changes of contact entries to the cellphone.
 * 
 * Return: bool
 * ReturnVal: TRUE on success
 * ReturnVal: FALSE on error
 */
osync_bool gnokii_contact_commit(OSyncContext *ctx, OSyncChange *change) {

	osync_trace(TRACE_ENTRY, "%s() (%p, %p)", __func__, ctx, change);

	OSyncError *error = NULL;
	gn_phonebook_entry *contact = NULL;
	char *uid = NULL;
	char *hash = NULL;
	
	gnokii_environment *env = (gnokii_environment *)osync_context_get_plugin_data(ctx);

	// Get changed contact note
	contact = (gn_phonebook_entry *) osync_change_get_data(change);

	// Check for type of changes
	switch (osync_change_get_changetype(change)) {
		case CHANGE_DELETED:
			// Delete the change

			// memory leak? XXX XXX
			if (!gnokii_contact_delete(osync_change_get_uid(change), env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to delete contact.");
				goto error;
			}
	
			
			break;
		case CHANGE_ADDED:
			// Add the change
			if (!gnokii_contact_write(contact, env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to write contact.");
				goto error;
			}

			// memory location is known after note was written
			uid = gnokii_contact_uid(contact);
			osync_change_set_uid(change, uid);
			g_free(uid);

			// generate and set hash of entry
			hash = gnokii_contact_hash(contact);
			osync_change_set_hash(change, hash);
			g_free(hash);

			break;
		case CHANGE_MODIFIED:
			// set the memory location of the contact entry
			gnokii_contact_memlocation(osync_change_get_uid(change), contact);

			// overwrite the changed contact with the write function 
			if (!gnokii_contact_write(contact, env->state)) {
				osync_error_set(&error, OSYNC_ERROR_GENERIC, "Unable to modify (write) contact.");
				goto error;
			}

			// set hash for the modified contact entry
			hash = gnokii_contact_hash(contact);
			osync_change_set_hash(change, hash);
			g_free(hash);

			break;
		default:
			osync_trace(TRACE_INTERNAL, "Unknown change type...");
			break;
	}
	
	// answer the call
	osync_context_report_success(ctx);

	// update hashtable
	osync_hashtable_update_hash(env->hashtable, change);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return TRUE;

error:
	osync_context_report_osyncerror(ctx, &error);
	osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(&error));
	osync_error_free(&error);
	return FALSE;
}

