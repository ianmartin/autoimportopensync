/*
 * libopensync - A synchronization framework
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

#ifndef OPENSYNC_PLUGIN_INFO_PRIVATE_H_
#define OPENSYNC_PLUGIN_INFO_PRIVATE_H_

struct OSyncPluginInfo {
	void *loop;
	OSyncPluginConfig *config;
	GList *objtypes;
	char *configdir;

	/* The main sink */
	OSyncObjTypeSink *main_sink;

	OSyncObjTypeSink *current_sink;
	OSyncFormatEnv *formatenv;
	//devinfo
	int ref_count;
	char *groupname;
	OSyncVersion *version;
	OSyncCapabilities *capabilities;

#ifdef OPENSYNC_UNITTESTS
	long long int memberid; // introduced only for testing purpose (mock-sync)
#endif	
};

#endif /* OPENSYNC_PLUGIN_INFO_PRIVATE_H_ */
