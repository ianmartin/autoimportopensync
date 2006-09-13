#ifndef OPENSYNC_ARCHIVE_H_
#define OPENSYNC_ARCHIVE_H_

OSYNC_EXPORT OSyncArchive *osync_archive_new(const char *filename, OSyncError **error);
OSYNC_EXPORT void osync_archive_ref(OSyncArchive *archive);
OSYNC_EXPORT void osync_archive_unref(OSyncArchive *archive);

OSYNC_EXPORT osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_load_data(OSyncArchive *archive, const char *uid, char **data, unsigned int *size, OSyncError **error);

OSYNC_EXPORT long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, const char *objtype, long long int mappingid, long long int memberid, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error);
OSYNC_EXPORT osync_bool osync_archive_load_changes(OSyncArchive *archive, const char *objtype, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error);
char *osync_archive_get_objtype(OSyncArchive *archive, const char *uid, OSyncError **error);

#endif /*OPENSYNC_ARCHIVE_H_*/
