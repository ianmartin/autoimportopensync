#ifndef _OPENSYNC_HASHTABLE_INTERNALS_H_
#define _OPENSYNC_HASHTABLE_INTERNALS_H_

/*! @brief Represent a hashtable which can be used to check if changes have been modifed or deleted */
struct OSyncHashTable {
	OSyncDB *dbhandle;
	GHashTable *used_entries;
};

#endif //_OPENSYNC_HASHTABLE_INTERNALS_H_
