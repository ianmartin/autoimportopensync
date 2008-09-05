#ifndef EVO2_SYNC_H
#define EVO2_SYNC_H

//#include "evo2_sync.h"

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-plugin.h>
#include <opensync/opensync-context.h>
#include <opensync/opensync-data.h>
#include <opensync/opensync-helper.h>
#include <opensync/opensync-merger.h>
#include <opensync/opensync-version.h>

#include <libecal/e-cal.h>
#include <libebook/e-book.h>
#include <libebook/e-vcard.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

typedef struct evo2_location {
	char *name;
	char *uri;
} evo2_location;

typedef struct OSyncEvoEnv {
	char *change_id;
	
	const char *addressbook_path;
	EBook *addressbook;
	OSyncObjTypeSink *contact_sink;
	OSyncObjFormat *contact_format;
	
	const char *calendar_path;
	ECal *calendar;
	OSyncObjTypeSink *calendar_sink;
	OSyncObjFormat *calendar_format;
	
	const char *memos_path;
	ECal *memos;
	OSyncObjTypeSink *memos_sink;
	OSyncObjFormat *memos_format;
	
	const char *tasks_path;
	ECal *tasks;
	OSyncObjTypeSink *tasks_sink;
	OSyncObjFormat *tasks_format;

	OSyncPluginInfo *pluginInfo;	
} OSyncEvoEnv;

ESource *evo2_find_source(ESourceList *list, char *uri);
void evo2_report_change(OSyncContext *ctx, OSyncObjFormat *format, char *data, unsigned int size, const char *uid, OSyncChangeType changetype);

#include "evolution2_ebook.h"
#include "evolution2_ecal.h"
#include "evolution2_memo.h"
#include "evolution2_etodo.h"

#endif
