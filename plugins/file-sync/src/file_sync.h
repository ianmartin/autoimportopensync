#ifndef FILE_PLUGIN_H
#define FILE_PLUGIN_H

#include <opensync/opensync.h>
#include <sys/stat.h>
//#include <unistd.h>
#include <stdio.h>
#include <glib.h>
#include <string.h>
#include <config.h>

#ifdef HAVE_FAM
#include <fam.h>
#endif

typedef struct filesyncinfo {
        char *path;
        OSyncMember *member;
        GDir *dir;
        OSyncHashTable *hashtable;
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

#endif
