/*
 * gpe-sync - A plugin for the opensync framework
 * Copyright (C) 2005  Martin Felis <martin@silef.de>
 * Copyright (C) 2007  Graham R. Cobb <g+opensync@cobb.uk.net>
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

#include "gpe_sync.h"

/*! \brief Reads the uid from "gpe-contact-0123"
 *
 * \param string	The string with the type and uid
 *
 * \return the part behind the last '-'
 */
int get_type_uid (const char *string)
{
	if (!string)
		return 0;
	
	gchar *p;
	p = strrchr (string, '-');
	p++;
	return atoi(p);
}

/*! \brief Seperates a string of the format "<a>:<b>" into "<a>" and "<b>"
 *
 * \param string	The string that should be separated
 * \param value		The string where <a> should be stored
 * \param modified	The string where <b> should be stored
 *
 * Note: The pointers of value and modified are pointing into string, so
 * if string gets freed, value and modified are no more valid!
 *
 */
osync_bool parse_value_modified (char *string, char **value, char **modified)
{
	char *c = NULL;
	*value = string;
	*modified = (strchr (string, ':'));
	if (*modified == NULL)
		return FALSE;
	*modified += 1;
	
	c = strchr (*modified, '\n');
	if (c)
		c[0] = '\0';
	
	c = strchr (*value, ':');
	c[0] = '\0';

	return TRUE;
}

/*! \brief Reports a single change (vcard, vtodo, vevent)
 *
 * \param ctx	The context of the plugin
 * \param type	Must be "contact", "event", or "todo"
 * \param uid	The uid of the item
 * \param hash	The hash of the item
 * \param data	The whole vcard, vevent or vtodo
 *
 */
osync_bool report_change (sink_environment *sinkenv, OSyncContext *ctx, gchar *type, gchar *uid, gchar *hash, char *string)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p, %p, %p, %p, %p, %p)", __func__, sinkenv, ctx, type, uid, hash, string);
	osync_trace(TRACE_SENSITIVE, "GPE-SYNC %s: reporting item type = %s, uid = %s, hash = %s, string = %s", __func__, type, uid, hash, string);

	OSyncError *error = NULL;

	OSyncData *data = osync_data_new(string, strlen(string), sinkenv->objformat, &error);
	if (!data) {
	  osync_context_report_osyncwarning(ctx, error);
	  osync_error_unref(&error);
	  return FALSE;
	}

	OSyncChange *change = osync_change_new (&error);
	if (!change) {
	  osync_context_report_osyncwarning(ctx, error);
	  osync_error_unref(&error);
	  return FALSE;
	}

	/* We report the uids as
	 * gpe-contacts-0123, gpe-todo-0123, etc. so that file-sync
	 * seperates the different types.
	 */
	gchar buf[25];
	sprintf (buf, "gpe-%s-%s", type, uid);
	osync_change_set_uid (change, buf);

	osync_change_set_objtype (change, type);
	osync_change_set_hash (change, hash);
	osync_change_set_data (change, data);
	
	OSyncChangeType changetype = osync_hashtable_get_changetype(sinkenv->hashtable, change);
	if (changetype != OSYNC_CHANGE_TYPE_UNMODIFIED) {
	  osync_change_set_changetype(change, changetype);
	  osync_context_report_change (ctx, change);
	  osync_hashtable_update_hash (sinkenv->hashtable, changetype, osync_change_get_uid(change), hash);
	}
	osync_hashtable_report(sinkenv->hashtable, osync_change_get_uid(change));

	osync_change_unref(change);

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);

	return TRUE;
}

/*! \brief Reports deleted entries using hashtable
 *
 * \param sinkenv The environment of the sink
 * \param ctx	The context of the operation
 *
 */
void report_deleted (sink_environment *sinkenv, OSyncContext *ctx)
{
	osync_trace(TRACE_ENTRY, "GPE-SYNC %s(%p, %p)", __func__, sinkenv, ctx);

	OSyncError *error = NULL;

	//check for deleted entries ... via hashtable
	int i;
	OSyncList *u, *uids = osync_hashtable_get_deleted(sinkenv->hashtable);
	for (u = uids; uids; u = u->next) {
	  OSyncChange *change = osync_change_new(&error);
	  if (!change) {
	    osync_context_report_osyncwarning(ctx, error);
	    osync_error_unref(&error);
	    continue;
	  }
	  
	  const char *uid = u->data;
	  osync_trace(TRACE_INTERNAL, "%s: deleting uid %s", __func__, uid);
	  osync_change_set_uid(change, uid);
	  osync_change_set_changetype(change, OSYNC_CHANGE_TYPE_DELETED);
 	
	  OSyncData *odata = osync_data_new(NULL, 0, sinkenv->objformat, &error);
	  if (!odata) {
	    osync_change_unref(change);
	    osync_context_report_osyncwarning(ctx, error);
	    osync_error_unref(&error);
	    continue;
	  }
	  
	  osync_data_set_objtype(odata, osync_objtype_sink_get_name(sinkenv->sink));
	  osync_change_set_data(change, odata);
	  osync_data_unref(odata);
	  
	  osync_context_report_change(ctx, change);
	  
	  osync_hashtable_update_change(sinkenv->hashtable, change);
	  
	  osync_change_unref(change);
	}
	osync_list_free(uids);

	osync_trace(TRACE_EXIT, "GPE-SYNC %s", __func__);
}
