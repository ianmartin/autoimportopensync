#ifndef FILE_PLUGIN_H
#define FILE_PLUGIN_H

#include <opensync/opensync.h>
#include <sys/stat.h>
//#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <config.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#ifdef HAVE_FAM
#include <fam.h>
#endif

typedef struct filesyncinfo {
        char *path;
        OSyncMember *member;
        GDir *dir;
        OSyncHashTable *hashtable;
        osync_bool recursive;
#ifdef HAVE_FAM
        FAMConnection *famConn;
        FAMRequest *famRequest;
#endif
} filesyncinfo;

typedef struct fs_fileinfo {
	//char *basepath;
	struct stat filestats;
	char *data;
	int size;
} fs_fileinfo;

osync_bool fs_parse_settings(filesyncinfo *env, char *data, int size, OSyncError **error);

#endif
