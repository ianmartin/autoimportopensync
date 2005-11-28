/*
 * file-sync - A plugin for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */

#ifndef _FILE_PLUGIN_H
#define _FILE_PLUGIN_H

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

osync_bool fs_parse_settings(filesyncinfo *env, xmlDocPtr, OSyncError **error);

xmlDocPtr fs_get_config (const char *path);
osync_bool fs_set_config (const char *path, xmlDocPtr doc);

#endif //_FILE_PLUGIN_H
