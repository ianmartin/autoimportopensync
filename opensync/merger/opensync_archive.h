#ifndef OPENSYNC_ARCHIVE_H_
#define OPENSYNC_ARCHIVE_H_

OSyncArchive *osync_archive_new(const char* filename, OSyncError **error);
void osync_archive_close(OSyncArchive *archive);
int osync_archive_store(OSyncArchive *archive, const char* data, unsigned int size);
void osync_archive_restore(OSyncArchive *archive, int id, const char** data, unsigned int *size);

#endif /*OPENSYNC_ARCHIVE_H_*/
