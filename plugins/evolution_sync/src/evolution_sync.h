#ifndef EVOLUTION_SYNC_H
#define EVOLUTION_SYNC_H

#include "evo_sync.h"

#include <opensync/opensync.h>
#include <cal-client/cal-client.h>
#include <cal-client/cal-client-types.h>
#include <cal-util/cal-component.h>
#include <ebook/e-book.h>
#include <ebook/e-card-simple.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <sys/stat.h>
#include <string.h>

typedef struct {
	OSyncMember *member;
	char *configfile;
	char *adressbook_path;
	EBook *addressbook;
	EBookView *ebookview;
	char *calendar_path;
	CalClient *calendar;
	char *tasks_path;
	CalClient *tasks;
	int debuglevel;
	int dbs_to_load;
	int dbs_loaded;
} evo_environment;

#include "evolution_ebook.h"
#include "evolution_ecal.h"
#include "evolution_etodo.h"
#include "evolution_xml.h"

#endif
