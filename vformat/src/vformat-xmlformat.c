/*
 * vformat-plugin - OpenSync plugin for vObject formats
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * Copyright (C) 2007  Daniel Gollub <dgollub@suse.de>
 * Copyright (C) 2007  Christopher Stender <cstender@suse.de>
 * Copyright (C) 2007  Jerry Yu <jijun.yu@sun.com>
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

#include "vformat.h"
#include "xmlformat-vevent.h"
#include "xmlformat-vcard.h"
#include "xmlformat-vnote.h"

#ifdef BUILD_XMLFORMAT_VCALENDAR
static osync_bool xmlformat_vcalendar_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;
	OSyncObjFormat *xmlformat = NULL;

	//event stuff	
	xmlformat = osync_format_env_find_objformat(env, "xmlformat-event");
	OSyncObjFormat *vevent10 = osync_format_env_find_objformat(env, "vevent10");
	OSyncObjFormat *vevent20 = osync_format_env_find_objformat(env, "vevent20");
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vevent10, conv_xmlformat_to_vevent10, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vevent10, xmlformat, conv_vevent10_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vevent20, conv_xmlformat_to_vevent20, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vevent20, xmlformat, conv_vevent20_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	//todo stuff
	xmlformat = osync_format_env_find_objformat(env, "xmlformat-todo");
	OSyncObjFormat *vtodo10 = osync_format_env_find_objformat(env, "vtodo10");
	OSyncObjFormat *vtodo20 = osync_format_env_find_objformat(env, "vtodo20");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vtodo20, conv_xmlformat_to_vtodo20, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vtodo20, xmlformat, conv_vtodo20_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vtodo10, conv_xmlformat_to_vtodo10, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vtodo10, xmlformat, conv_vtodo10_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);


	return TRUE;
}
#endif // BUILD_XMLFORMAT_VCALENDAR

#ifdef BUILD_XMLFORMAT_VCARD
static osync_bool xmlformat_vcard_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv;
	OSyncError *error = NULL;
	
	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-contact");
	OSyncObjFormat *vcard21 = osync_format_env_find_objformat(env, "vcard21");
	OSyncObjFormat *vcard30 = osync_format_env_find_objformat(env, "vcard30");
	
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vcard21, conv_xmlformat_to_vcard21, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcard21, xmlformat, conv_vcard_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vcard30, conv_xmlformat_to_vcard30, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);
	
	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vcard30, xmlformat, conv_vcard_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	return TRUE;
}
#endif // BUILD_XMLFORMAT_VCARD


#ifdef BUILD_XMLFORMAT_VNOTE
static osync_bool xmlformat_vnote_conversion_info(OSyncFormatEnv *env)
{
	OSyncFormatConverter *conv = NULL;
	OSyncError *error = NULL;

	OSyncObjFormat *xmlformat = osync_format_env_find_objformat(env, "xmlformat-note");
	OSyncObjFormat *vnote = osync_format_env_find_objformat(env, "vnote11");
	OSyncObjFormat *vjournal = osync_format_env_find_objformat(env, "vjournal");

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vnote, conv_xmlformat_to_vnote, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vnote, xmlformat, conv_vnote_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, xmlformat, vjournal, conv_xmlformat_to_vjournal, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, vjournal, xmlformat, conv_vjournal_to_xmlformat, &error);
	if (!conv) {
		osync_trace(TRACE_ERROR, "Unable to register format converter: %s", osync_error_print(&error));
		osync_error_unref(&error);
		return FALSE;
	}
	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);


	return TRUE;
}
#endif // BUILD_XMLFORMAT_VNOTE

osync_bool get_conversion_info(OSyncFormatEnv *env)
{
#ifdef BUILD_XMLFORMAT_VCARD
	if (!xmlformat_vcard_conversion_info(env))
		return FALSE;
#endif

#ifdef BUILD_XMLFORMAT_VCALENDAR
	if (!xmlformat_vcalendar_conversion_info(env))
		return FALSE;
#endif

#ifdef BUILD_XMLFORMAT_VNOTE
	if (!xmlformat_vnote_conversion_info(env))
		return FALSE;
#endif

	return TRUE;
}

int get_version(void)
{
	return 1;
}

