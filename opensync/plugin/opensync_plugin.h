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

#ifndef _OPENSYNC_PLUGIN_H_
#define _OPENSYNC_PLUGIN_H_

typedef void * (* initialize_fn) (OSyncMember *, OSyncError **);
typedef void (* finalize_fn) (void *);
typedef osync_bool (* usable_fn) (OSyncError **);

/*! @brief Gives information about wether the plugin
 * has to be configured or not
 * 
 * @ingroup OSyncPluginAPI 
 **/
typedef enum {
	/** Plugin has no configuration options */
	NO_CONFIGURATION = 0,
	/** Plugin can be configured, but will accept the default config in the initialize function */
	OPTIONAL_CONFIGURATION = 1,
	/** Plugin must be configured to run correctly */
	NEEDS_CONFIGURATION = 2
} OSyncConfigurationTypes;

OSyncPlugin *osync_plugin_new(OSyncError **error);
void osync_plugin_unref(OSyncPlugin *plugin);
void osync_plugin_ref(OSyncPlugin *plugin);

const char *osync_plugin_get_name(OSyncPlugin *plugin);
void osync_plugin_set_name(OSyncPlugin *plugin, const char *name);

const char *osync_plugin_get_longname(OSyncPlugin *plugin);
void osync_plugin_set_longname(OSyncPlugin *plugin, const char *longname);

const char *osync_plugin_get_description(OSyncPlugin *plugin);
void osync_plugin_set_description(OSyncPlugin *plugin, const char *description);

void *osync_plugin_get_plugin_data(OSyncPlugin *plugin);
void osync_plugin_set_plugin_data(OSyncPlugin *plugin, void *data);

initialize_fn osync_plugin_get_initialize(OSyncPlugin *plugin);
void osync_plugin_set_initialize(OSyncPlugin *plugin, initialize_fn init);

finalize_fn osync_plugin_get_finalize(OSyncPlugin *plugin);
void osync_plugin_set_finalize(OSyncPlugin *plugin, finalize_fn fin);

void osync_plugin_set_is_usable(OSyncPlugin *plugin, usable_fn useable);
osync_bool osync_plugin_is_usable(OSyncPlugin *plugin, OSyncError **error);

#endif //_OPENSYNC_PLUGIN_H_
