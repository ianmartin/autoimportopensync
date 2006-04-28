/* 
   OpenSync Plugin for Mozilla Sunbird
   Copyright (C) 2005-2006 Markus Meyer <meyer@mesw.de>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation;

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS.
   IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) AND AUTHOR(S) BE LIABLE FOR ANY
   CLAIM, OR ANY SPECIAL INDIRECT OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES 
   WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN 
   ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF 
   OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

   ALL LIABILITY, INCLUDING LIABILITY FOR INFRINGEMENT OF ANY PATENTS, 
   COPYRIGHTS, TRADEMARKS OR OTHER RIGHTS, RELATING TO USE OF THIS 
   SOFTWARE IS DISCLAIMED.
*/

#include "sunbird_sync.h"

#include <opensync/opensync.h>

#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "webdav.h"


/* Forward declarations */

const char* get_datapath(plugin_environment* env);
GList* get_calendar_files_list(plugin_environment* env);
int do_webdav(plugin_environment* env, int upload);
plugin_environment* get_plugin_environment(OSyncContext* ctx);
GString* get_default_calendar(plugin_environment* env);
void write_changes_to_calendars(GList* entries, plugin_environment* env);
int get_calendar_changes(GList** changes_ptr, int *slow_sync, plugin_environment* env);
void read_config_from_xml_doc(xmlDocPtr doc, plugin_environment* env);


/* Implementations of user functions */

const char* get_datapath(plugin_environment* env)
{
    return osync_member_get_configdir(env->member);
}

GString* get_local_path_from_url(plugin_environment* env, const char* url)
{
    GString* gstr;
    const char* p = url + strlen(url);
    while (p > url && *p != '/')
        p--;
    gstr = g_string_new(get_datapath(env));
    g_string_append(gstr, p);
    return gstr;
}

GList* get_calendar_files_list(plugin_environment* env)
{
    GList* files = NULL;
    GList* cur;
    char* s;
    
    for (cur = g_list_first(env->config_calendars); cur; cur = cur->next)
    {
        plugin_calendar_config* cfg = (plugin_calendar_config*)cur->data;

        if (cfg->typ == TYP_WEBDAV)
        {
            /* Get base name and preceed with data directory */
            GString* gstr = get_local_path_from_url(env, cfg->filename->str);
            s = (char*)strdup(gstr->str);
            g_string_free(gstr, TRUE);
        } else
        {
            /* Return string as-is */
            s = (char*)strdup(cfg->filename->str);
        }
        
        files = g_list_append(files, s);
    }
    
    return files;
}

int do_webdav(plugin_environment *env, int upload)
{
    int result = TRUE;
    GList* cur;
    
    osync_trace(TRACE_ENTRY, "do_webdav(upload=%i)", upload);
    
    for (cur = g_list_first(env->config_calendars); cur; cur = cur->next)
    {
        plugin_calendar_config* cfg = (plugin_calendar_config*)cur->data;
        
        if (cfg->typ == TYP_WEBDAV)
        {
            int tmpresult;
            GString *local_path = get_local_path_from_url(env, cfg->filename->str);
            
            if (upload)
            {
                osync_trace(TRACE_INTERNAL, "Uploading %s -> %s",
                            local_path->str, cfg->filename->str);
                tmpresult = webdav_upload(local_path->str, cfg->filename->str,
                                          cfg->username->str, cfg->password->str);
            } else
            {
                osync_trace(TRACE_INTERNAL, "Downloading %s -> %s",
                            cfg->filename->str, local_path->str);
                tmpresult = webdav_download(local_path->str, cfg->filename->str,
                                            cfg->username->str, cfg->password->str);
            }
            
            if (tmpresult != WEBDAV_SUCCESS)
            {
                osync_trace(TRACE_INTERNAL, "ERROR: webdav function returned status %i", tmpresult);
                result = FALSE;
            }
            
            g_string_free(local_path, TRUE);
        }
    }

    osync_trace(TRACE_EXIT, "do_webdav");
    
    return result;
}

plugin_environment* get_plugin_environment(OSyncContext* ctx)
{
    return (plugin_environment*)osync_context_get_plugin_data(ctx);
}

GString* get_basename(char* filename)
{
    char *basename = filename + strlen(filename) - 1;
    while (basename > filename && *(basename-1) != '/')
        basename--;
    return g_string_new(basename);
}

GString* get_default_calendar(plugin_environment* env)
{
    GList* cur;
    
    for (cur = g_list_first(env->config_calendars); cur; cur = cur->next)
    {
        plugin_calendar_config* cfg = (plugin_calendar_config*)cur->data;
        
        if (cfg->isdefault)
            return get_basename(cfg->filename->str);
    }
    
    return NULL; /* no default calendar */
}

void write_changes_to_calendars(GList* entries, plugin_environment* env)
{
    char keyfile[256];
    GList* files = get_calendar_files_list(env), *calendars = NULL, *cur, *cur2, *curfile, *curcal, *cached_entries = NULL;
    GString *default_calendar = get_default_calendar(env);
    
    osync_trace(TRACE_ENTRY, "write_changes_to_calendars");
    
    if (!files)
    {
        osync_trace(TRACE_EXIT, "write_changes_to_calendars");
        return;
    }

    if (!(default_calendar && strlen(default_calendar->str) > 0))
    {
        default_calendar = get_basename((char*)g_list_first(files)->data);
        
        osync_trace(TRACE_INTERNAL, "WARNING: No default calendar set, using first calendar: %s", default_calendar->str);
    }
    
    strcpy(keyfile, get_datapath(env));
    strcat(keyfile, "/mozilla_keyfile.ics");
    
    osync_trace(TRACE_INTERNAL, "Reading keyfile '%s'...", keyfile);
    
    if (!read_icalendar_file(keyfile, &cached_entries))
    {
        osync_trace(TRACE_INTERNAL, "WARNING: Keyfile not found!");
    }

    osync_trace(TRACE_INTERNAL, "Reading all calendars into memory...");
    for (cur = g_list_first(files); cur; cur = cur->next)
    {
        int filesize;
        char *filename = (char*)cur->data, *buffer;
        FILE* f = fopen(filename, "r");
        if (!f)
        {
            osync_trace(TRACE_INTERNAL, "ERROR: Could not read file: %s", filename);
            osync_trace(TRACE_EXIT_ERROR, "write_changes_to_calendars");
            return;
        }
        
        fseek(f, 0, SEEK_END);
        filesize = ftell(f);
        rewind(f);
        buffer = (char*)g_malloc0(filesize+1);
        if (!buffer)
        {
            osync_trace(TRACE_INTERNAL, "ERROR: Out of memory");
            osync_trace(TRACE_EXIT_ERROR, "write_changes_to_calendars");
            return;
        }
        fread(buffer,1,filesize,f);
        fclose(f);
        buffer[filesize] = 0; /* end of string delimiter */
        
        calendars = g_list_append(calendars, g_string_new(buffer));
    }
    
    for (cur = g_list_first(entries); cur; cur = cur->next)
    {
        calendar_entry* e = (calendar_entry*)cur->data;

        /* Find name of source file for this object */
        if (e->remote_change_type == CHANGE_ADDED)
        {
            /* This is a new object, source file is always the default calendar */
            osync_trace(TRACE_INTERNAL, "Scheduling new object %s for adding to default calendar %s", e->id->str, default_calendar->str);
            e->sourcefile = g_string_new(default_calendar->str);
        } else if (e->remote_change_type == CHANGE_DELETED || e->remote_change_type == CHANGE_MODIFIED)
        {
            /* This is an old object, look it up in the key file */
            for (cur2 = g_list_first(cached_entries); cur2; cur2 = cur2->next)
            {
                calendar_entry* e2 = (calendar_entry*)cur2->data;
                
                if (strcmp(e2->id->str, e->id->str) == 0)
                {
                    e->sourcefile = g_string_new(e2->sourcefile->str);
                    osync_trace(TRACE_INTERNAL, "Modified/Deleted object %s (%s) has been looked up in keyfile",
                        e->id->str, e->sourcefile->str);
                    break;
                }
            }
            
            if (!e->sourcefile)
            {
                osync_trace(TRACE_INTERNAL, "Warning: Object %s could not be found in keyfile, although change mode is CHANGE_MODIFIED.",
                       e->id->str);
                osync_trace(TRACE_INTERNAL, "         Adding the object to the default calendar");
                e->sourcefile = g_string_new(default_calendar->str);
            }
        }

        /* Find correct calendar for this object */
        curfile = g_list_first(files);
        curcal = g_list_first(calendars);
        while (curfile)
        {
            char* filepath = (char*)curfile->data;
            char* calendarname;

            if (e->sourcefile)
            {
                calendarname = e->sourcefile->str;
            
                if (strlen(filepath) >= strlen(calendarname) &&
                    strcmp(calendarname, filepath + strlen(filepath) - strlen(calendarname)) == 0)
                {
                    /* This is the correct calendar */
                    break;
                }
            }
            
            curfile = curfile->next;
            curcal = curcal->next;
        }
        
        if (!curcal)
        {
            osync_trace(TRACE_INTERNAL, "ERROR: Calendar not found for object %s, changes to this object are not synced!", e->id->str);
        } else if (e->remote_change_type == CHANGE_ADDED || e->remote_change_type == CHANGE_MODIFIED ||
                   e->remote_change_type == CHANGE_DELETED)
        {
            osync_trace(TRACE_INTERNAL, "Adding/Modifying/Deleting object %s (%s)", e->id->str, e->sourcefile->str);

            GString* calcontent = (GString*)curcal->data;
            patch_calendar(calcontent, e->remote_change_type, e->id->str, e->data ? e->data->str : NULL);
            
            osync_trace(TRACE_INTERNAL, "Done.");
        }
    }
    
    osync_trace(TRACE_INTERNAL, "Writing all calendars to disk...");
    curcal = g_list_first(calendars);
    for (cur = g_list_first(files); cur; cur = cur->next)
    {
        char *textdata = (char*)(((GString*)curcal->data)->str);
        char *filename = (char*)cur->data;
        FILE* f = fopen(filename, "w");

        if (!f)
        {
            osync_trace(TRACE_INTERNAL, "ERROR: Could not open file for writing: %s", filename);
            return;
        }

        fwrite(textdata, 1, strlen(textdata), f);
        fclose(f);
        curcal = curcal->next;
    }
    
    if (calendars)
    {
        for (cur = g_list_first(calendars); cur; cur = cur->next)
        {
            GString* s = (GString*)cur->data;
            g_string_free(s, TRUE);
        }
        g_list_free(calendars);
    }
    
    free_string_list(files);
    free_events_list(cached_entries);

    osync_trace(TRACE_EXIT, "write_changes_to_calendars");
}

int get_calendar_changes(GList** changes_ptr, /* OUT: List of changes */
                         int *slow_sync,      /* IN/OUT: Slow sync flag */
                         plugin_environment* env)
{
    char keyfile[256];
    GList *entries = NULL, *cached_entries = NULL, *files = NULL, *cur, *cur2;
    
    osync_trace(TRACE_ENTRY, "get_calendar_changes");
    
    /* Free previous pending changes, if any */
    if (env->pending_changes)
    {
        osync_trace(TRACE_INTERNAL, "Warning: Resetting pending changes");
        free_events_list(env->pending_changes);
        env->pending_changes = NULL;
    }
    
    files = get_calendar_files_list(env);
    if (!files)
        return FALSE;
        
    strcpy(keyfile, get_datapath(env));
    strcat(keyfile, "/mozilla_keyfile.ics");
    
    if (*slow_sync)
    {
        osync_trace(TRACE_INTERNAL, "Remote requested slow sync, removing old keyfile, if any");
        if (unlink(keyfile) == -1 && errno != ENOENT)
        {
            /*
             * This will only be called if the file really cannot be
             * deleted. If it is just not there, errno contains ENOENT.
             */
            osync_trace(TRACE_INTERNAL, "Could not remove old keyfile");
            goto err;
        }
    } else
    {
        osync_trace(TRACE_INTERNAL, "Reading keyfile '%s'...", keyfile);
    
        if (!read_icalendar_file(keyfile, &cached_entries))
        {
            osync_trace(TRACE_INTERNAL, "Keyfile not found, doing complete resync!");
            *slow_sync = TRUE;
        }
    }
    
    osync_trace(TRACE_INTERNAL, "Reading calendar files...");
    
    cur2 = g_list_first(env->config_calendars);
    for (cur = g_list_first(files); cur; cur = cur->next)
    {
        char* filename = (char*)cur->data;
        plugin_calendar_config* cfg = (plugin_calendar_config*)cur2->data;
        
        osync_trace(TRACE_INTERNAL, "Reading calendar file '%s'...", filename);
        if (!read_icalendar_file(filename, &entries))
        {
            osync_trace(TRACE_INTERNAL, "Error reading calendar file!");
            goto err;
        }
        
        if (cfg->deletedaysold != 0)
        {
            osync_trace(TRACE_INTERNAL,
                "Removing in-memory items that are older than %i days...",
                cfg->deletedaysold);
            delete_old_entries(&entries, cfg->deletedaysold);
        }
        
        cur2 = cur2->next;
    }
    
    osync_trace(TRACE_INTERNAL, "Syncing entries...");
    
    /* For all entries check, if they are already cached, new or modified */
    for (cur = g_list_first(entries); cur; cur = cur->next)
    {
        OSyncChange* change = NULL;
        calendar_entry* cached = NULL;
        calendar_entry* e = (calendar_entry*)cur->data;
        
        /* Is this entry already cached? */
        for (cur2 = g_list_first(cached_entries); cur2; cur2 = cur2->next)
        {
            calendar_entry* e2 = (calendar_entry*)cur2->data;
            
            if (strcmp(e2->id->str, e->id->str) == 0)
            {
                cached = e2;
                break;
            }
        }

        if (cached)
        {
            /* This element is already cached, check if it has been modified */
            if (strcmp(cached->last_modified->str, e->last_modified->str) != 0)
            {
                /* This element has been modified, notify the sync engine */
                osync_trace(TRACE_INTERNAL, "Entry %s has been modified", e->id->str);

                change = osync_change_new();
                osync_change_set_changetype(change, CHANGE_MODIFIED);
            }
        } else
        {
            /* This element is not already cached, notify the sync engine */
            osync_trace(TRACE_INTERNAL, "Entry %s is new", e->id->str);

            change = osync_change_new();
            osync_change_set_changetype(change, CHANGE_ADDED);
        }
        
        if (change)
        {
            /* Complete entry */
            osync_change_set_member(change, env->member);
            osync_change_set_objformat_string(change, "vevent20");
            osync_change_set_uid(change, g_strdup(e->id->str));
            osync_change_set_data(change, g_strdup(e->data->str), strlen(e->data->str), TRUE);
            
            osync_trace(TRACE_INTERNAL, "Entry data length: ", strlen(e->data->str));

            /* Add it to list of changes */
            *changes_ptr = g_list_append(*changes_ptr, change);

            /* Add it to "pending-changes" entries */
            env->pending_changes = g_list_append(env->pending_changes, clone_calendar_entry(e));
        }
    }
    
    /* For all cached entries check if they have been deleted in the real calendar */
    for (cur = g_list_first(cached_entries); cur; cur = cur->next)
    {
        int found = 0;
        calendar_entry* e = (calendar_entry*)cur->data;
        
        for (cur2 = g_list_first(entries); cur2; cur2 = cur2->next)
        {
            calendar_entry* e2 = (calendar_entry*)cur2->data;
            
            if (strcmp(e2->id->str, e->id->str) == 0)
            {
                found = 1;
                break;
            }
        }
        
        if (!found)
        {
            /* This entry has been deleted in the real calendar, since it is in the */
            /* cached entries but not in the real calendar entries, notify sync engine */
            OSyncChange* change;
            
            osync_trace(TRACE_INTERNAL, "Entry %s was deleted", e->id->str);
            
            calendar_entry* deleted_entry = clone_calendar_entry(e);
            deleted_entry->deleted = 1;
            env->pending_changes = g_list_append(env->pending_changes, deleted_entry);
            
            change = osync_change_new();
            osync_change_set_member(change, env->member);
            osync_change_set_objformat_string(change, "vevent20");
            osync_change_set_uid(change, g_strdup(e->id->str));
            osync_change_set_changetype(change, CHANGE_DELETED);
            /* osync_change_set_data(change, NULL, 0, TRUE); */

            *changes_ptr = g_list_append(*changes_ptr, change);
        }        
    }
    
    osync_trace(TRACE_INTERNAL, "Done!");

    osync_trace(TRACE_INTERNAL, "Freeing lists...");    
    free_string_list(files);
    free_events_list(cached_entries);
    free_events_list(entries);
    osync_trace(TRACE_INTERNAL, "Done!");

    osync_trace(TRACE_EXIT, "get_calendar_changes");

    return TRUE;

err:
    osync_trace(TRACE_INTERNAL, "Freeing lists...");    
    free_string_list(files);
    free_events_list(cached_entries);
    free_events_list(entries);
    osync_trace(TRACE_INTERNAL, "Done!");

    osync_trace(TRACE_EXIT_ERROR, "get_calendar_changes");
    
    return FALSE;
}

void read_config_from_xml_doc(xmlDocPtr doc, plugin_environment* env)
{
    xmlNode *root, *cur;
    
    osync_trace(TRACE_ENTRY, "read_config_from_xml_doc");
    
    root = xmlDocGetRootElement(doc);
    
    if (root->type != XML_ELEMENT_NODE ||
        strcmp((char*)root->name, "config") != 0)
    {
        osync_trace(TRACE_INTERNAL, "root node name must be 'config'");
        osync_trace(TRACE_EXIT_ERROR, "read_config_from_xml_doc");
        return;
    }
    
    for (cur = root->children; cur; cur = cur->next)
    {
        if (cur->type == XML_ELEMENT_NODE && (
              strcmp((char*)cur->name, "file")== 0 || strcmp((char*)cur->name, "webdav") == 0))
        {
            plugin_calendar_config *cfg;
            xmlChar *attr_default, *attr_username, *attr_password;
            xmlChar *attr_filename, *attr_deletedaysold;

            osync_trace(TRACE_INTERNAL, "reading node of type '%s'", cur->name);

            cfg  = (plugin_calendar_config*)g_malloc0(sizeof(plugin_calendar_config));
            cfg->isdefault = 0;
            cfg->filename = NULL;
            cfg->username = NULL;
            cfg->password = NULL;
            cfg->deletedaysold = 0;
            if (strcmp((char*)cur->name, "file") == 0)
                cfg->typ = TYP_FILE;
            else
                cfg->typ = TYP_WEBDAV;
                
            /* Parse attributes of this node */
            attr_default = xmlGetProp(cur, (const xmlChar*)"default");
            attr_username = xmlGetProp(cur, (const xmlChar*)"username");
            attr_password = xmlGetProp(cur, (const xmlChar*)"password");
            attr_deletedaysold = xmlGetProp(cur, (const xmlChar*)"deletedaysold");

            if (cfg->typ == TYP_FILE)
                attr_filename = xmlGetProp(cur, (const xmlChar*)"path");
            else
                attr_filename = xmlGetProp(cur, (const xmlChar*)"url");
                
            if (attr_default)
            {
                cfg->isdefault = atoi((char*)attr_default);
                xmlFree(attr_default);
                osync_trace(TRACE_INTERNAL, "set isdefault to %i", cfg->isdefault);
            }
            
            if (attr_username)
            {
                cfg->username = g_string_new((char*)attr_username);
                xmlFree(attr_username);
                osync_trace(TRACE_INTERNAL, "set username to *****");
            }
            
            if (attr_password)
            {
                cfg->password = g_string_new((char*)attr_password);
                xmlFree(attr_password);
                osync_trace(TRACE_INTERNAL, "set password to *****");
            }
            
            if (attr_filename)
            {
                cfg->filename = g_string_new((char*)attr_filename);
                xmlFree(attr_filename);
                osync_trace(TRACE_INTERNAL, "set filename to %s", cfg->filename->str);
            }
            
            if (attr_deletedaysold)
            {
                cfg->deletedaysold = atoi((char*)attr_deletedaysold);
                xmlFree(attr_deletedaysold);
                osync_trace(TRACE_INTERNAL, "set deletedaysold to %i", cfg->deletedaysold);
            }
            
            /* Add node to list, if complete */
            if ( (cfg->typ == TYP_FILE && cfg->filename) ||
                 (cfg->typ == TYP_WEBDAV && cfg->filename
                                         && cfg->username
                                         && cfg->password) )
            {
                osync_trace(TRACE_INTERNAL, "Adding node to calendar list");
                env->config_calendars = g_list_append(env->config_calendars, cfg);
            } else
            {
                g_free(cfg);
                osync_trace(TRACE_INTERNAL, "Warning: Ignoring incomplete node!");
            }
        }
    }
    
    osync_trace(TRACE_EXIT, "read_config_from_xml_doc");
}

/*************************************************************
 *                  PLUGIN CALLBACKS BELOW                   *
 *************************************************************/

static void connect(OSyncContext *ctx)
{
    plugin_environment *env = get_plugin_environment(ctx);

    osync_trace(TRACE_ENTRY, "connect");

    /* Initialize data valid within one connect */
    env->pending_changes = NULL;

    if (do_webdav(env, 0))
    {
    	osync_context_report_success(ctx);
        osync_trace(TRACE_EXIT, "connect");
    } else
    {
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
                                   "Error getting calendars through WebDav");
        osync_trace(TRACE_EXIT_ERROR, "connect");
    }
}

static void disconnect(OSyncContext *ctx)
{
    plugin_environment *env = get_plugin_environment(ctx);

    osync_trace(TRACE_ENTRY, "disconnect");

    if (env->pending_changes){
        osync_trace(TRACE_INTERNAL, "Warning: Discarding pending changes");
        free_events_list(env->pending_changes);
        env->pending_changes = NULL;
    }

	osync_context_report_success(ctx);

    osync_trace(TRACE_EXIT, "disconnect");
}

static void get_changeinfo(OSyncContext *ctx)
{
    GList *changes = NULL;
    plugin_environment *env = get_plugin_environment(ctx);
    int slow_sync = osync_member_get_slow_sync(env->member, "event");
	
    if (get_calendar_changes(&changes, &slow_sync, env))
    {
        GList* cur;
        
        /* Inform engine about our own slow sync decision */
        osync_member_set_slow_sync(env->member, "event", slow_sync);

        for (cur = g_list_first(changes); cur; cur = cur->next)
        {
            OSyncChange *change = (OSyncChange*)cur->data;
            osync_context_report_change(ctx, change);
        }
        
        g_list_free(changes);
        osync_context_report_success(ctx);
    } else
    {
        /* An error occured during getting calendar changes */
        osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
                                   "Error getting calendar changes");
    }
}

static osync_bool commit_calendar_change(OSyncContext *ctx, OSyncChange *change)
{
    int datasize = osync_change_get_datasize(change);
    const char* uid = osync_change_get_uid(change);
	plugin_environment *env = get_plugin_environment(ctx);
	
    osync_trace(TRACE_ENTRY, "commit_calendar_change");
    
    calendar_entry* entry = (calendar_entry*)g_malloc0(sizeof(calendar_entry));
    entry->remote_change_type = osync_change_get_changetype(change);

    if (datasize > 0)
    {
        entry->data = g_string_new_len(osync_change_get_data(change), datasize);
    } else {
        /* 
         * No data provided, so we assume that the entry has been deleted
         * This is needed because e.g. IRMC plugin always uses change_type == SYNC_OBJ_MODIFIED
         *
         * Note: The above was true for MultiSync, not sure if this is still true for OpenSync
         */
        entry->remote_change_type = CHANGE_DELETED;
        entry->data = NULL;
    }

    if (uid && strlen(uid) > 0)
    {
        entry->id = g_string_new(uid);
    } else
    {
        /* We don't have an ID yet, so always create a new entry */
        entry->remote_change_type = CHANGE_ADDED;
                
        /* Create ID */
        entry->id = create_new_uid();
        
        /* Inform OpenSync about the new ID */
        osync_change_set_uid(change, g_strdup(entry->id->str));

        osync_trace(TRACE_INTERNAL, "Created new id '%s' for entry", entry->id->str);
    }
            
    /* Always add ID as "UID" to vcard, if not there already */
    if (entry->data)
    {
        char* uid = get_key_data(entry->data->str, "UID");
        if (uid)
        {
            if (strcmp(uid, entry->id->str) != 0)
            {
                /* If this happens, something has gone totally wrong */
                osync_trace(TRACE_INTERNAL, "ERROR: uid='%s' not the same as id='%s'", uid, entry->id->str);
            }
            
            g_free(uid);
        } else
        {
            /* Add UID to VCARD */
            char *s, buf[256];
                
            sprintf(buf, "BEGIN:VEVENT\r\nUID:%s", entry->id->str);
            s = strstr(entry->data->str, "BEGIN:VEVENT");
            g_string_erase(entry->data, 0, s - entry->data->str + 12);
            g_string_insert(entry->data, 0, buf);
        }
    }

    entry->last_modified = NULL; // will eventually be set later
    entry->sourcefile = NULL; // unknown at this stage
            
    if (entry->remote_change_type == CHANGE_DELETED)
        entry->deleted = 1;
    else
        entry->deleted = 0;

    if (entry->data)
    {
        char* last_modified = get_key_data(entry->data->str, "LAST-MODIFIED");
        if (last_modified)
        {
            entry->last_modified = g_string_new(last_modified);
            g_free(last_modified);
        }
    }
            
    if (!entry->last_modified)
    {
        /* we need a last-modified entry in the key file */
        entry->last_modified = g_string_new("(new)");
    }

    /* Add change to list of pending changes */
    env->pending_changes = g_list_append(env->pending_changes, entry);
                
    osync_trace(TRACE_INTERNAL, "*** recorded new change ***");
    osync_trace(TRACE_INTERNAL, "entry->remote_change_type = %i", entry->remote_change_type);
    if (entry->id)
        osync_trace(TRACE_INTERNAL, "entry->id = %s", entry->id->str);
    else
        osync_trace(TRACE_INTERNAL, "entry has no id");
    if (entry->data)
        osync_trace(TRACE_INTERNAL, "entry->data:\n%s", entry->data->str);
    else
        osync_trace(TRACE_INTERNAL, "entry has no data");

    osync_trace(TRACE_EXIT, "commit_calendar_change");

	osync_context_report_success(ctx);
	return TRUE;
}

static void sync_done(OSyncContext *ctx)
{
	plugin_environment *env = get_plugin_environment(ctx);

    osync_trace(TRACE_ENTRY, "sync_done");
    
    if (env->pending_changes)
    {
        char keyfile[256];
        GList *cached_entries = NULL, *cur, *remote_changes = NULL;
        
        for (cur = g_list_first(env->pending_changes); cur; cur = cur->next)
        {
            calendar_entry *e = (calendar_entry*)cur->data;
            if (e->remote_change_type != 0)
                remote_changes = g_list_append(remote_changes, e);
        }
        
        if (remote_changes)
        {
            osync_trace(TRACE_INTERNAL, "Writing remote changes to calendars...");
            write_changes_to_calendars(remote_changes, env);
            g_list_free(remote_changes); // don't delete contents
            osync_trace(TRACE_INTERNAL, "Done writing remote changes to calendars.");
        }

        osync_trace(TRACE_INTERNAL, "Sync done, remembering changes");
        
        strcpy(keyfile, get_datapath(env));
        strcat(keyfile, "/mozilla_keyfile.ics");
        
        osync_trace(TRACE_INTERNAL, "Reading keyfile '%s'...", keyfile);
        if (!read_icalendar_file(keyfile, &cached_entries))
            osync_trace(TRACE_INTERNAL, "Keyfile not found, creating new one");
        
        osync_trace(TRACE_INTERNAL, "Merging changes with keyfile");
        
        for (cur = g_list_first(env->pending_changes); cur; cur = cur->next)
        {
            GList* cur2;
            calendar_entry *e = (calendar_entry*)cur->data;

            if (e->deleted || e->remote_change_type == CHANGE_DELETED)
            {
                /* Delete entry from cached entries list */
                cur2 = g_list_first(cached_entries);
                while(cur2)
                {
                    calendar_entry *e2 = (calendar_entry*)cur2->data;
                    cur2 = cur2->next;
                    
                    if (strcmp(e2->id->str, e->id->str) == 0)
                    {
                        /* This is the entry, delete it */
                        osync_trace(TRACE_INTERNAL, "Removing entry %s", e2->id->str);
                        cached_entries = g_list_remove(cached_entries, e2);
                        free_calendar_entry(e2);
                        break;
                    }
                }
            } else
            {
                /* Modify/Add entry to cached list */
                
                /* Check if entry is already in cached list. */
                cur2 = g_list_first(cached_entries);
                while (cur2)
                {
                    calendar_entry *e2 = (calendar_entry*)cur2->data;
                    cur2 = cur2->next;
                    
                    if (strcmp(e2->id->str, e->id->str) == 0)
                    {
                        /* Entry already in cached entries list, delete it */
                        osync_trace(TRACE_INTERNAL, "Temporarily removing modifed entry %s", e2->id->str);
                        cached_entries = g_list_remove(cached_entries, e2);
                        free_calendar_entry(e2);
                        break;
                    }
                }
                
                /* Append entry */
                osync_trace(TRACE_INTERNAL, "Appending entry %s", e->id->str);
                
                cached_entries = g_list_append(cached_entries, e);
            }

        }
        
        /* Notice that we do not delete the list entries because they are now in the cached_entries list */
        g_list_free(env->pending_changes);
        env->pending_changes = NULL;
        
        osync_trace(TRACE_INTERNAL, "Writing keyfile '%s'...", keyfile);
        if (write_key_file(keyfile, cached_entries))
        {
            osync_trace(TRACE_INTERNAL, "Keyfile written succesfully.");
        } else
        {
            osync_trace(TRACE_INTERNAL, "ERROR: Error writing key file, but what should I do?");
        }

        free_events_list(cached_entries);

        if (!do_webdav(env, 1))
        {
            osync_context_report_error(ctx, OSYNC_ERROR_GENERIC,
                "Could not upload all calendars to server. They are still stored in the configuration "
                "directory inside .opensync. You must upload them yourself or your calendars will not "
                "be up-to-date!");
            osync_trace(TRACE_EXIT, "sync_done");
            return;
        }
    } else
    {
        osync_trace(TRACE_INTERNAL, "Sync done, no changes");
    }
    
	osync_context_report_success(ctx);

    osync_trace(TRACE_EXIT, "sync_done");
}

static void *initialize(OSyncMember *member, OSyncError **error)
{
	char *configdata;
	int configsize;
	plugin_environment *env;
	xmlDocPtr doc;
	
    osync_trace(TRACE_ENTRY, "initialize");
    
    /* Initialize XML library */
    LIBXML_TEST_VERSION

    /* Allocate and initialize plugin environment struct */
    env = g_malloc0(sizeof(plugin_environment));
	env->member = member;
    env->config_calendars = NULL;
    env->pending_changes = NULL;

    /* Read and parse config */    	
	if (!osync_member_get_config(member, &configdata, &configsize, error))
	{
		osync_error_update(error, "Unable to get config data: %s", osync_error_print(error));
		g_free(env);
		return NULL;
	}

	doc = xmlReadMemory(configdata, configsize, "noname.xml", NULL, 0);
	g_free(configdata);
	
	if (doc)
	{
	   read_config_from_xml_doc(doc, env);
	   xmlFreeDoc(doc);
	   
	   if (!env->config_calendars)
	   {
	       osync_error_update(error, "Config data contains no calendars");
	       g_free(env);
	       return NULL;
	   }
	} else
	{
	   osync_error_update(error, "Unable to parse config data");
	   g_free(env);
	   return NULL;
	}

    osync_trace(TRACE_EXIT, "initialize");

	return env;
}

static void finalize(void *data)
{
/*	plugin_environment *env = (plugin_environment *)data; */
	
    osync_trace(TRACE_ENTRY, "finalize");

	/* Free memory from XML parser */
	xmlCleanupParser();

    osync_trace(TRACE_EXIT, "finalize");
}

void get_info(OSyncEnv* env)
{
	OSyncPluginInfo *info;

    osync_trace(TRACE_ENTRY, "get_info");

    info = osync_plugin_new_info(env);

	info->name = "sunbird-sync";
	info->longname = "Sunbird / Mozilla Calendar Sync";
	info->description = "Synchronisation with one or more Sunbird / Mozilla Calendar calendars";

	info->version = 1; /* There is no API 2 yet */
	info->is_threadsafe = FALSE; /* Actually not sure if it is or not */

	info->functions.initialize = initialize;
	info->functions.connect = connect;
	info->functions.sync_done = sync_done;
	info->functions.disconnect = disconnect;
	info->functions.finalize = finalize;
	info->functions.get_changeinfo = get_changeinfo;

    /* Loading calendars from WebDAV servers can take a minute or two... */
	info->timeouts.connect_timeout = 120; /* seconds */

    /* We only accept calendar events in VEvent 2.0 format at the moment */
	osync_plugin_accept_objtype(info, "event");
	osync_plugin_accept_objformat(info, "event", "vevent20", NULL);
	osync_plugin_set_commit_objformat(info, "event", "vevent20", commit_calendar_change);
	
	osync_trace(TRACE_EXIT, "get_info");
}
