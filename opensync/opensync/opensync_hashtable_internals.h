#ifndef _OPENSYNC_HASHTABLE_INTERNALS_H_
#define _OPENSYNC_HASHTABLE_INTERNALS_H_

/*! @brief Represent a hashtable which can be used to check if changes have been modifed or deleted */
struct OSyncHashTable {
#ifndef DOXYGEN_SHOULD_SKIP_THIS
	OSyncDB *dbhandle;
	GHashTable *used_entries;
#endif
};

#endif //_OPENSYNC_HASHTABLE_INTERNALS_H_
