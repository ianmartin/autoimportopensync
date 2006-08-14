#ifndef OPENSYNC_ARCHIVE_H_
#define OPENSYNC_ARCHIVE_H_

OSyncArchive *osync_archive_new(const char *filename, const char *objtype, OSyncError **error);
void osync_archive_ref(OSyncArchive *archive);
void osync_archive_unref(OSyncArchive *archive);

osync_bool osync_archive_save_data(OSyncArchive *archive, const char *uid, const char *data, unsigned int size, OSyncError **error);
osync_bool osync_archive_load_data(OSyncArchive *archive, const char *uid, const char **data, unsigned int *size, OSyncError **error);

long long int osync_archive_save_change(OSyncArchive *archive, long long int id, const char *uid, long long int mappingid, long long int memberid, OSyncError **error);
osync_bool osync_archive_load_changes(OSyncArchive *archive, OSyncList **ids, OSyncList **uids, OSyncList **mappingids, OSyncList **memberids, OSyncError **error);
osync_bool osync_archive_delete_change(OSyncArchive *archive, long long int id, OSyncError **error);

#endif /*OPENSYNC_ARCHIVE_H_*/
