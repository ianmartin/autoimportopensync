#ifndef OPENSYNC_ARCHIVE_INTERNALS_H_
#define OPENSYNC_ARCHIVE_INTERNALS_H_

#include <sqlite3.h>

struct OSyncArchive {
	int ref_count;
	sqlite3 *db;
	char *tablename;
};

void _osync_archive_db_trace(void *data, const char *query);

#endif /*OPENSYNC_ARCHIVE_INTERNALS_H_*/
