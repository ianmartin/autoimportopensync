/* 
   MultiSync Plugin for Mozilla Sunbird
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

#include "tools.h"

#include <opensync/opensync.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static unsigned int uidcounter = 1;

void free_calendar_entry(calendar_entry* e)
{
    g_string_free(e->id, TRUE);
    g_string_free(e->sourcefile, TRUE);
    g_string_free(e->last_modified, TRUE);
    if (e->data)
        g_string_free(e->data, TRUE);
    if (e->remove_priority)
        g_string_free(e->remove_priority, TRUE);
    g_free(e);
}

void free_events_list(GList *lst)
{
    GList* cur;
    
    if (!lst)
        return;

    for (cur = g_list_first(lst); cur; cur = cur->next)
        free_calendar_entry((calendar_entry*)cur->data);
        
    g_list_free(lst);
}

int write_key_file(char* filename, GList *entries)
{
    GList* cur;
    FILE* f;
    
    f = fopen(filename, "w");
    if (!f)
        return 0;
        
    fprintf(f, "BEGIN:VCALENDAR\nVERSION:2.0\n");
    
    for (cur = g_list_first(entries); cur; cur = cur->next)
    {
        calendar_entry* e = (calendar_entry*)cur->data;
        
        fprintf(f, "BEGIN:VEVENT\n");
        fprintf(f, "UID\n");
        fprintf(f, " :%s\n", e->id->str);
        fprintf(f, "LAST-MODIFIED\n");
        fprintf(f, " :%s\n", e->last_modified->str);
        fprintf(f, "X-SOURCEFILE\n");
        fprintf(f, " :%s\n", e->sourcefile->str);
        fprintf(f, "X-DELETED\n");
        if (e->deleted)
            fprintf(f, " :1\n");
        else
            fprintf(f, " :0\n");
        fprintf(f, "END:VEVENT\n");
    }
    
    fprintf(f, "END:VCALENDAR\n");
    fclose(f);
    
    return 1;
}

void dump_calendar_entries(GList* entries)
{
    GList* cur;
    
    osync_trace(TRACE_INTERNAL, "\n*** DEBUG DUMP OF CALENDAR ENTRIES ***\n");
    
    if (entries)
    {
        for (cur = g_list_first(entries); cur; cur = cur->next)
        {
            calendar_entry* e = (calendar_entry*)cur->data;
            
            if (e)
            {
                if (e->id)
                    osync_trace(TRACE_INTERNAL, "entry id = %s\n", e->id->str);
                else
                    osync_trace(TRACE_INTERNAL, "ERROR: id is null pointer\n");
                if (e->last_modified)
                    osync_trace(TRACE_INTERNAL, "last modified: %s\n", e->last_modified->str);
                else
                    osync_trace(TRACE_INTERNAL, "ERROR: last modified is null pointer\n");
                if (e->sourcefile)
                    osync_trace(TRACE_INTERNAL, "sourcefile: %s\n", e->sourcefile->str);
                else
                    osync_trace(TRACE_INTERNAL, "ERROR: sourcefile is null pointer\n");
                osync_trace(TRACE_INTERNAL, "deleted: %i\n", e->deleted);
            } else
            {
                osync_trace(TRACE_INTERNAL, "ERROR: element is null pointer\n");
            }
        }
    }
    
    osync_trace(TRACE_INTERNAL, "*** END DEBUG DUMP ***\n\n");
}

void free_string_list(GList *lst)
{
    GList* cur;
    
    if (!lst)
        return;
    
    for (cur = g_list_first(lst); cur; cur = cur->next)
    {
        g_free(cur->data);
    }
    
    g_list_free(lst);
}

calendar_entry* clone_calendar_entry(calendar_entry* e)
{
    calendar_entry* new_entry = (calendar_entry*)g_malloc0(sizeof(calendar_entry));

    new_entry->remote_change_type = e->remote_change_type;
    new_entry->id = g_string_new(e->id->str);
    new_entry->sourcefile = g_string_new(e->sourcefile->str);
    new_entry->last_modified = g_string_new(e->last_modified->str);
    new_entry->deleted = e->deleted;

    if (e->data)
        new_entry->data = g_string_new(e->data->str);
    else
        new_entry->data = NULL;

    if (e->remove_priority)
        new_entry->remove_priority = g_string_new(e->remove_priority->str);
    else
        new_entry->remove_priority = NULL;
    
    return new_entry;
}

int read_icalendar_file(char* filename, GList **entries_ptr)
{
    int num_entries = 0;
    int buf_size = 4096, len;
    char buf[buf_size];
    FILE* f;
    calendar_entry *cur_entry = NULL;
    char *basename_ptr, *basename;

    f = fopen(filename, "r");
    if (!f)
        return 0;

    basename_ptr = (char*)strdup(filename);
    basename = basename_ptr + strlen(basename_ptr) - 1;
    while (basename > basename_ptr && *(basename-1) != '/')
        basename--;
        
    while (!feof(f))
    {
        if (!fgets(buf, buf_size, f))
            break;

        len = strlen(buf);
        while (len > 0 && (buf[len-1] == '\n' || buf[len-1] == '\r'))
        {
            len--;
            buf[len] = 0;
        }
            
        if (strcmp(buf, "BEGIN:VEVENT") == 0)
        {
            cur_entry = (calendar_entry*)g_malloc0(sizeof(calendar_entry));
            memset(cur_entry, 0, sizeof(calendar_entry));
            cur_entry->sourcefile = g_string_new(basename);
        }
        
        if (strcmp(buf, "END:VEVENT") == 0)
        {
            char *uid, *last_modified, *dtend, *x_sourcefile, *x_deleted, *vcal;
            
            num_entries++;
            
            g_string_append(cur_entry->data, "\nEND:VEVENT\nEND:VCALENDAR\n");
            vcal = cur_entry->data->str;
            
            uid = get_key_data(vcal, "UID");
            last_modified = get_key_data(vcal, "LAST-MODIFIED");
            dtend = get_key_data(vcal, "DTEND");
            x_sourcefile = get_key_data(vcal, "X-SOURCEFILE");
            x_deleted = get_key_data(vcal, "X-DELETED");
            
            if (uid)
            {
                cur_entry->id = g_string_new(uid);
                g_free(uid);
            } else
                osync_trace(TRACE_INTERNAL, "WARNING: %i. entry in %s has no UID\n", num_entries, filename);
            
            if (last_modified)
            {
                cur_entry->last_modified = g_string_new(last_modified);
                g_free(last_modified);
            } else
            {
                /* default last-modified tag */
                cur_entry->last_modified = g_string_new("(new)");
            }
            
            if (dtend)
            {
                cur_entry->remove_priority = g_string_new(dtend);
                g_free(dtend);
            }
            
            if (x_sourcefile)
            {
                cur_entry->sourcefile = g_string_new(x_sourcefile);
                g_free(x_sourcefile);
            }
            
            if (x_deleted)
            {
                if (x_deleted[0] == '0')
                    cur_entry->deleted = 0;
                else
                    cur_entry->deleted = 1;
                g_free(x_deleted);
            }

            *entries_ptr = g_list_append(*entries_ptr, cur_entry);
            cur_entry = NULL;
        }
        
        if (cur_entry)
        {
            if (strlen(buf) > 2 && buf[0] == ' ' && buf[1] == ':')
            {
                /* Merge this line with the previous one.
                   This e.g. merges
                       CLASS
                        :PUBLIC
                   into
                       CLASS:PUBLIC
                */
                g_string_append(cur_entry->data, buf+1);
            } else
            {
                if (cur_entry->data)
                    g_string_append(cur_entry->data, "\n");
                else
                    cur_entry->data = g_string_new("BEGIN:VCALENDAR\nVERSION:2.0\n");
            
                g_string_append(cur_entry->data, buf);
            }
        }
    }
    
    free(basename_ptr);
    fclose(f);
    return 1;
}

char* copy_from_g_string(GString* str)
{
    char* p;
    
    if (!str)
        return NULL;
        
    p = (char*)g_malloc0(str->len + 1);
    
    if (p)
        memcpy(p, str->str, str->len + 1);
    
    return p;
}

GString* extract_first_vevent(char* data)
{
    GString* g = g_string_new(data);
    char *s;
    s = strstr(g->str, "BEGIN:VEVENT");
    if (s)
        g_string_erase(g, 0, s-g->str);
    s = strstr(g->str, "END:VEVENT");
    if (s)
        g_string_truncate(g, s-g->str+10);
    return g;
}

void patch_calendar(GString* calendar, int change_type, char* id, char* data)
{
    char *line, *begin_vevent = NULL;
    char *curpos = calendar->str;

    osync_trace(TRACE_INTERNAL, "patching calendar (change_type: %i)\n", change_type);
    
    while (*curpos)
    {
        char *pos2 = curpos;
        while (*pos2 != 0 && *pos2 != '\r' && *pos2 != '\n')
            pos2++;
            
        line = (char*)g_malloc0((pos2-curpos+1)*sizeof(char));
        line[pos2-curpos] = 0;
        memcpy(line, curpos, pos2-curpos);

        while (*pos2 == '\r' || *pos2 == '\n')
            pos2++;
            
        if (strcmp(line, "END:VCALENDAR") == 0)
        {
            /* We must insert new stuff right before this last line */
            /* osync_trace(TRACE_INTERNAL, "end of calendar at %i\n", curpos-calendar->str); */

            if (change_type == CHANGE_MODIFIED || change_type == CHANGE_ADDED)
            {
                int insertpos = curpos - calendar->str;
                GString* stripped_data = extract_first_vevent(data);
                /* osync_trace(TRACE_INTERNAL, "Adding changed/new event with ID %s to calendar at position %i (len=%i)\n",
                       id, insertpos, strlen(calendar->str)); */
                g_string_insert(calendar, insertpos, "\r\n");
                g_string_insert(calendar, insertpos, stripped_data->str);
                g_string_free(stripped_data, TRUE);
            }
            
            osync_trace(TRACE_INTERNAL, "done patching calendar\n");
            return;
        }
            
        if (strcmp(line, "BEGIN:VEVENT") == 0)
        {
            /* osync_trace(TRACE_INTERNAL, "begin of vevent at %i\n", curpos-calendar->str); */
            begin_vevent = curpos;
        }
            
        if (strcmp(line, "END:VEVENT") == 0)
        {
            char *event_id, *nlpos;
            int len = curpos-begin_vevent+strlen("END:VEVENT");
            char* vcard = (char*)g_malloc0((len+1)*sizeof(char));
            vcard[len] = 0;
            memcpy(vcard, begin_vevent, len);
            
            /* osync_trace(TRACE_INTERNAL, "end of vevent at %i\n", curpos-calendar->str); */

            /* get_key_data() does not work when value for data is on the next line, */
            /* so just remove the newlines and the space to get the UID. */
            nlpos = strstr(vcard, "UID\r\n :");
            if (nlpos)
            {
                /* Remove the '\r\n ' */
                int ofs = nlpos+6-vcard;
                memmove(nlpos+3, nlpos+6, len-ofs+1); // also copy the null-byte at the end
            }
            
            event_id = get_key_data(vcard, "UID");

            if (event_id)
            {
                if (strcmp(event_id,id) == 0)
                {
                    int begin_pos = begin_vevent - calendar->str;
                    int end_pos = (curpos - calendar->str) + strlen(line) + 2; /* Skip additional \r\n */
                    /* osync_trace(TRACE_INTERNAL, "Found event with ID %s, removing old instance\n", id); */
                    g_string_erase(calendar, begin_pos, end_pos-begin_pos);
                    pos2 = calendar->str + begin_pos;
                }
                
                g_free(event_id);
            } else
            {
                osync_trace(TRACE_INTERNAL, "ERROR: VEVENT has no ID!\n*** Dumping data: ***\n%s\n*** End dump ***\n", vcard);
            }
            
            g_free(vcard);
            begin_vevent = NULL;
        }
        
        g_free(line);
        curpos = pos2;
    }
    
    osync_trace(TRACE_INTERNAL, "ERROR: EOF while trying to patch calendar (no END:VCALENDAR found)!\n");
}

GString* create_new_uid()
{
    char s[256];
    sprintf(s, "t%ic%i", (int)time(NULL), uidcounter++);
    return g_string_new(s);
}

char* get_key_data(const char* vevent, const char* key_name)
{
    GString* key_pattern = g_string_new("");
    g_string_printf(key_pattern, "\n%s:", key_name);
    char* start = strstr(vevent, key_pattern->str);
    char* ret = NULL;
    
    /* osync_trace(TRACE_ENTRY, "get_key_data('%s', '%s')", vevent, key_name); */
    
    if (start)
        start += strlen(key_pattern->str);

    g_string_free(key_pattern, TRUE);        
    
    if (start)
    {
        char* end = start;
        while (!(*end==0 || *end=='\r' || *end=='\n'))
            end++;
        
        int len = end - start;
        ret = (char*)g_malloc0((len+1)*sizeof(char));
        memcpy(ret, start, len);
        ret[len] = 0;
    }

/*    if (ret)
        osync_trace(TRACE_INTERNAL, "returning '%s'", ret);
    else
        osync_trace(TRACE_INTERNAL, "returning NULL");
        
    osync_trace(TRACE_EXIT, "get_key_data"); */

    return ret;
}
