/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
 * 
 */
 
#ifndef OPENSYNC_VERSION_INTERNALS_H_
#define OPENSYNC_VERSION_INTERNALS_H_



OSyncCapabilities *osync_version_find_capabilities(OSyncVersion *version, OSyncError **error);

OSyncList *osync_version_load_from_default_descriptions(OSyncError **error);
OSyncList *osync_version_load_from_descriptions(OSyncError **error, const char *descriptiondir, const char *schemadir);

char *osync_version_get_plugin(OSyncVersion *version);
char *osync_version_get_priority(OSyncVersion *version);
char *osync_version_get_vendor(OSyncVersion *version);
char *osync_version_get_modelversion(OSyncVersion *version);
char *osync_version_get_firmwareversion(OSyncVersion *version);
char *osync_version_get_softwareversion(OSyncVersion *version);
char *osync_version_get_hardwareversion(OSyncVersion *version);
char *osync_version_get_identifier(OSyncVersion *version);


int osync_version_matches(OSyncVersion *pattern, OSyncVersion *version, OSyncError **error);

#endif /* OPENSYNC_VERSION_INTERNALS_H_ */

