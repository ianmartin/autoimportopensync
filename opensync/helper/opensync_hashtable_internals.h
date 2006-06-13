#ifndef _OPENSYNC_HASHTABLE_INTERNALS_H_
#define _OPENSYNC_HASHTABLE_INTERNALS_H_

#include <sqlite3.h>

/*! @brief Represent a hashtable which can be used to check if changes have been modifed or deleted */
struct OSyncHashTable {
	sqlite3 *dbhandle;
	GHashTable *used_entries;
	char *tablename;
};

#endif //_OPENSYNC_HASHTABLE_INTERNALS_H_
