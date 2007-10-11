/*
 * Copyright (C) 2007 Michael Unterkalmsteiner, <michael.unterkalmsteiner@stud-inf.unibz.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "SIFTformat.h"
#include "SIFformat-utils.h"

#include <opensync/opensync-format.h>
#include <opensync/opensync-merger.h>

osync_bool get_sift_conversion_info(OSyncFormatEnv* env, OSyncError** error) {
	OSyncObjFormat* format_sift = osync_format_env_find_objformat(env, SIFT);
	if (!format_sift) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find '%s' format", SIFT);
		return FALSE;
	}
	
	OSyncObjFormat* format_xmltodo = osync_format_env_find_objformat(env, "xmlformat-todo");
	if (!format_xmltodo) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find xmlformat-todo format");
		return FALSE;
	}

	OSyncFormatConverter* conv = osync_converter_new(OSYNC_CONVERTER_CONV,
			format_sift, format_xmltodo, sift_2_xmltodo, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, format_xmltodo, format_sift,
							   xmltodo_2_sift, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	return TRUE;
}

osync_bool sift_2_xmltodo(char* input, unsigned int inpsize,
                                 char** output, unsigned int* outpsize, osync_bool* free_input,
                                 const char* config, OSyncError** error) {

	osync_trace(TRACE_ENTRY, "%s", __func__);

	osync_bool success = FALSE;
	TranslationEnv* env = init_sif_2_xml(input, inpsize, SIFT_2_XML_FORMAT_TODO_FILE, error);
	if(!env)
		return FALSE;

	success = translate(env);

	if (success) {
		OSyncXMLFormat* xmlformat = osync_xmlformat_parse((char *)env->out_xml,
		                            env->out_size, env->error);

		if (!xmlformat) {
			osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(env->error));
			success = FALSE;
		} else {
			*free_input = TRUE;
			*output = (char *) xmlformat;
			*outpsize = sizeof(xmlformat);

			osync_xmlformat_sort(xmlformat);
			
			unsigned int size;
			char *str;
			osync_xmlformat_assemble(xmlformat, &str, &size);
			osync_trace(TRACE_INTERNAL, "... Output XMLFormat is: \n%s", str);
			g_free(str);

			if (osync_xmlformat_validate(xmlformat) == FALSE)
				osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Not valid!");
			else
				osync_trace(TRACE_INTERNAL, "XMLFORMAT TODO: Valid!");
		}

		freeEnv(env);
	}

	return success;
}

osync_bool xmltodo_2_sift(char* input, unsigned int inpsize,
                                 char** output, unsigned int * outpsize, osync_bool* free_input,
                                 const char* config, OSyncError** error) {

	osync_trace(TRACE_ENTRY, "%s", __func__);

	osync_bool success = FALSE;
	TranslationEnv* env = init_xml_2_sif(input, XML_FORMAT_TODO_2_SIFT_FILE, error);
	if(!env)
		return FALSE;

	success = translate(env);

	if (success) {
		*free_input = TRUE;
		*output = g_strdup((char *) env->out_xml);
		*outpsize = env->out_size;

		osync_trace(TRACE_INTERNAL, "... Output SIFT is: \n%s", *output);
	}

	freeEnv(env);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return success;
}
