#ifndef _XMLVCARD_H_
#define _XMLVCARD_H_

typedef struct OSyncHooksTable OSyncHooksTable;

struct OSyncHooksTable {
	GHashTable *table;
	GHashTable *tztable;
	GHashTable *comptable;
	GHashTable *compparamtable;
	GHashTable *alarmtable;
};

#define HANDLE_IGNORE (void *)1

#endif /*_XMLVCARD_H_*/
