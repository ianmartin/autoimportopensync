#ifndef OPENSYNC_ARCHIVE_INTERNALS_H_
#define OPENSYNC_ARCHIVE_INTERNALS_H_

struct OSyncArchive {
	sqlite3 *db;
};

void _osync_archive_db_trace(void *data, const char *query);

#endif /*OPENSYNC_ARCHIVE_INTERNALS_H_*/
