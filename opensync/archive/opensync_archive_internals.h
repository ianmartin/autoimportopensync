#ifndef OPENSYNC_ARCHIVE_INTERNALS_H_
#define OPENSYNC_ARCHIVE_INTERNALS_H_

#include <sqlite3.h>

/**
 * @brief Represent a Archive object
 * @ingroup OSyncArchivePrivateAPI
 */
struct OSyncArchive {
	/** The reference counter for this object */
	int ref_count;
	/**  */
	sqlite3 *db;
	/**  */
};

void _osync_archive_trace(void *data, const char *query);
char *_osync_archive_sql_escape(const char *s);

#endif /*OPENSYNC_ARCHIVE_INTERNALS_H_*/
