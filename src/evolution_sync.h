#ifndef EVO2_SYNC_H
#define EVO2_SYNC_H

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

void evo_debug(evo_environment *env, int level, char *message, ...);
ESource *find_source(ESourceList *list, char *uri);

#include "xml.h"

#endif
