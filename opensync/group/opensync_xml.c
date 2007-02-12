
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

#include "opensync.h"
#include "opensync_internals.h"

#include "opensync_xml.h"

/*! @brief Opens a xml document
 * 
 * Opens a xml document
 * 
 * @param doc Pointer to a xmldoc
 * @param cur The pointer to the first node
 * @param path The path of the document
 * @param topentry the name of the top node
 * @param error Pointer to a error struct
 * @returns TRUE if successful, FALSE otherwise
 * 
 */
osync_bool osync_open_xml_file(xmlDocPtr *doc, xmlNodePtr *cur, const char *path, const char *topentry, OSyncError **error)
{
	if (!g_file_test(path, G_FILE_TEST_EXISTS)) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "File %s does not exist", path);
		return FALSE;
	}
	
	*doc = xmlParseFile(path);
	if (!*doc) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "Could not open: %s", path);
		goto error;
	}

	*cur = xmlDocGetRootElement(*doc);
	if (!*cur) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems to be empty", path);
		goto error_free_doc;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		osync_error_set(error, OSYNC_ERROR_IO_ERROR, "%s seems not to be a valid configfile.\n", path);
		goto error_free_doc;
	}

	*cur = (*cur)->xmlChildrenNode;
	return TRUE;

error_free_doc:
	xmlFreeDoc(*doc);
error:
	osync_trace(TRACE_ERROR, "%s: %s", __func__, osync_error_print(error));
	return FALSE;
}
