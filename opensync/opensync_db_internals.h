#ifndef OPENSYNCDB_INTERNALS_H_
#define OPENSYNCDB_INTERNALS_H_

#include <sqlite3.h>

struct OSyncDB {
	sqlite3 *db;
};

#endif /* OPENSYNCDB_INTERNALS_H_ */
