#ifndef EVO2_SYNC_H
#define EVO2_SYNC_H

#include "evo2_sync.h"

#include <opensync/opensync.h>
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

typedef struct {
	OSyncMember *member;
	char *change_id;
	char *configfile;
	char *adressbook_path;
	EBook *adressbook;
	char *calendar_path;
	ECal *calendar;
	char *tasks_path;
	ECal *tasks;
	int debuglevel;
} evo_environment;

ESource *evo2_find_source(ESourceList *list, char *uri);
void evo2_report_change(OSyncContext *ctx, char *objtypestr, char *objformatstr, char *data, int datasize, const char *uid, OSyncChangeType type);

#include "evolution_ebook.h"
#include "evolution_ecal.h"
#include "evolution_etodo.h"
#include "evolution_xml.h"

#endif
