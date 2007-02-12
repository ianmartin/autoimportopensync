#include <stdlib.h>
#include <glib.h>

/* Holds information about calendars */
typedef struct {
    GString* id; /* iCalendar id */
    GString* sourcefile; /* Source calendar file (.ics) */
    GString* last_modified; /* as iCalendar format date */
    GString* data; /* The actual entry data in iCalendar format */
    GString* remove_priority;
    int deleted; /* Notification that entry has been deleted */
    int remote_change_type; /* Remote change type, one of SYNC_OBJ_MODIFIED/ADDED/HARDDELETED,
                             or 0, when this is a notification of a local change */
} calendar_entry;

/*
   Duplicate string in a GString. Returns NULL, if GString is also NULL,
   otherwise it returns a pointer to char* which must be freed after use.
*/
char* copy_from_g_string(GString* str);

/* Only for debugging */
void dump_calendar_entries(GList* entries);

/* Free memory for calendar entry */
void free_calendar_entry(calendar_entry* e);

/* Make exact copy of calendar entry and its contents */
calendar_entry* clone_calendar_entry(calendar_entry* e);

/* Free a list of events, including their contents */
void free_events_list(GList *lst);

/* Free a list of strings (list of char* pointers, not GStrings) */
void free_string_list(GList *lst);

/* Patch a calendar file (held in memory as a string),
   adding/deleting/modifying the given entry */
void patch_calendar(GString* calendar, int change_type, char* id, char* data);

/* Return a new UID as a GString */
GString* create_new_uid();

/* Extract the first VEVENT from this VCARD (and remove BEGIN:VCARD/END:VCARD) */
GString* extract_first_vevent(char* data);

/*
   Note that this function, despite its name, will at the moment not read
   every icalendar file, because it doesn't really parse the file, but
   assumes the structure Mozilla Calendar uses when it writes these files.
*/
int read_icalendar_file(char* filename, GList **entries_ptr);

/*
   Given a list of calendar entries, removes all entries from the list that
   are more than 'days' days old.
 */
void delete_old_entries(GList **entries_ptr, int days);
 
/*
   Write a file which remembers all the keys which have already been
   synced and their last modification date. This is essentially a
   "pseudo-vcalendar-file" and only used internally by the plugin.
*/
int write_key_file(char* filename, GList *entries);

/*
   Get key data from a VEVENT/VCARD. This will allocate the memory needed
   for the string. This reimplements the old MultiSync's sync_get_key_data()
   function.
 */
char* get_key_data(const char* vevent, const char* key_name);
