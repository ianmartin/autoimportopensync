/*
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2008 Instituto Nokia de Tecnologia
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

/**
 * @file   gdata_format.c
 * @author Adenilson Cavalcanti da Silva <adenilson.silva@indt.org.br>
 * @date   Wed Sep  3 12:33:08 2008
 *
 * @brief  A google data (contacts/calendar events) format plugin.
 *
 * It seems to be requirement to make the whole thing work, yada, yada, yada. I'm taking
 * as a base the mock_format.c/vformat-xmlformat.c files from opensync trunk and
 * hoping for the best.
 * \todo:
 * - make the convertion (should be easy thanks to xslt_aux functions)
 *
 */

#include "xslt_aux.h"

#include <glib.h>
#include <opensync/opensync.h>
#include <opensync/opensync-xmlformat.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-time.h>

osync_bool xmlcontact_to_gcontact(char *input, unsigned int inpsize,
				  char **output, unsigned int *outpsize,
				  osync_bool *free_input, const char *config,
				  void *userdata, OSyncError **error)
{
	/* TODO: how to get path to XSLT file using 'config'? */
	return FALSE;
}

osync_bool xmlevent_to_gevent(char *input, unsigned int inpsize,
			      char **output, unsigned int *outpsize,
			      osync_bool *free_input, const char *config,
			      void *userdata, OSyncError **error)
{
	/* TODO: how to get path to XSLT file using 'config'? */
	return FALSE;
}

osync_bool gcontact_to_xmlcontact(char *input, unsigned int inpsize,
				  char **output, unsigned int *outpsize,
				  osync_bool *free_input, const char *config,
				  void *userdata, OSyncError **error)
{
	/* TODO: how to get path to XSLT file using 'config'? */
	return FALSE;
}

osync_bool gevent_to_xmlevent(char *input, unsigned int inpsize,
			      char **output, unsigned int *outpsize,
			      osync_bool *free_input, const char *config,
			      void *userdata, OSyncError **error)
{
	/* TODO: how to get path to XSLT file using 'config'? */
	return FALSE;
}


osync_bool get_format_info(OSyncFormatEnv *env, OSyncError **error)
{
	OSyncObjFormat *gcont = osync_objformat_new("google-contact", "contact", error);
	if (!gcont)
		return FALSE;

	OSyncObjFormat *gevent = osync_objformat_new("google-event", "event", error);
	if (!gevent)
		return FALSE;

	/* TODO: register (and write) auxiliary functions: compare/create/destroy/blah...*/

	osync_format_env_register_objformat(env, gcont);
	osync_objformat_unref(gcont);

	osync_format_env_register_objformat(env, gevent);
	osync_objformat_unref(gevent);
}

void *initialize(OSyncError **error)
{
	struct xslt_resources *converter = NULL;
	converter = xslt_new();

	return converter;
}

void finalize(void *userdata)
{
	struct xslt_resources *converter = NULL;
	if (!userdata)
		return;
	else
		fprintf(stderr, "\n\nuserdata is not NULL: %x\n", userdata);

	converter = (struct xslt_resources *)userdata;
	xslt_delete(converter);
}

osync_bool get_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv = NULL;
	OSyncError *error = NULL;

	/* osync xml formats */
	OSyncObjFormat *xml_contact = osync_format_env_find_objformat(env, "xmlformat-contact");
	OSyncObjFormat *xml_event = osync_format_env_find_objformat(env, "xmlformat-event");

	/* gdata formats */
	OSyncObjFormat *gevent = osync_format_env_find_objformat(env, "google-event");
	osync_assert(gevent);
	OSyncObjFormat *gcontact = osync_format_env_find_objformat(env, "google-contact");
	osync_assert(gcontact);

	/* from xmlformat to gdata */
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xml_contact, gcontact,
				   xmlcontact_to_gcontact, &error);
	osync_assert(conv);
	osync_converter_set_initialize_func(conv, initialize);
	osync_converter_set_finalize_func(conv, finalize);
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xml_event, gevent,
				   xmlevent_to_gevent, &error);
	osync_assert(conv);
	osync_converter_set_initialize_func(conv, initialize);
	osync_converter_set_finalize_func(conv, finalize);
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	/* from gdata to xmlformat */
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, gcontact, xml_contact,
				   gcontact_to_xmlcontact, &error);
	osync_assert(conv);
	osync_converter_set_initialize_func(conv, initialize);
	osync_converter_set_finalize_func(conv, finalize);
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, gevent, xml_event,
				   gevent_to_xmlevent, &error);
	osync_assert(conv);
	osync_converter_set_initialize_func(conv, initialize);
	osync_converter_set_finalize_func(conv, finalize);

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	return TRUE;
}

int get_version(void)
{
	return 1;
}
