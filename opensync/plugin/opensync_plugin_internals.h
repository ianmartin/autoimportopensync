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

#ifndef _OPENSYNC_PLUGIN_INTERNALS_H_
#define _OPENSYNC_PLUGIN_INTERNALS_H_

struct OSyncPlugin {
	/** The version of Opensync API this plugin uses*/
	int version;
	/** The name of this plugin */
	char *name;
	/** The longer, more descriptive name of the plugin */
	char *longname;
	/** A short description what the plugin does */
	char *description;
	/** The function to initialize the plugin. */
	initialize_fn initialize;
	/** The function to finalize the plugin. The input will be the output of the initialize function */
	finalize_fn finalize;
	
	usable_fn useable;
	/** Does the plugin have configuration options? */
	OSyncConfigurationTypes config_type;
	/** The pointer to the plugin (for internal use) */
	OSyncModule *module;
	/** Plugin-specific data
	 *
	 * Can be used when a single module registers many plugins,
	 * such as the python-module plugin
	 */
	void *plugin_data;
	int ref_count;
};

#endif //_OPENSYNC_PLUGIN_INTERNALS_H_
