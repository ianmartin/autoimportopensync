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

#include "SIFNformat.h"
#include "SIFformat-utils.h"

osync_bool get_sifn_conversion_info(OSyncFormatEnv* env, OSyncError** error) {
	OSyncObjFormat* format_sifn = osync_format_env_find_objformat(env, SIFN);
	if (!format_sifn) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find '%s' format", SIFN);
		return FALSE;
	}

	OSyncObjFormat* format_xmlnote = osync_format_env_find_objformat(env, "xmlformat-note");
	if (!format_xmlnote) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to find xmlformat-note format");
		return FALSE;
	}

	OSyncFormatConverter* conv = osync_converter_new(OSYNC_CONVERTER_CONV, format_sifn,
			format_xmlnote, sifn_2_xmlnote, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	conv = osync_converter_new(OSYNC_CONVERTER_CONV, format_xmlnote, format_sifn,
							   xmlnote_2_sifn, error);
	if (!conv)
		return FALSE;

	osync_format_env_register_converter(env, conv);
	osync_converter_unref(conv);

	return TRUE;
}

osync_bool sifn_2_xmlnote(char *input, unsigned int inpsize,
                                 char **output, unsigned int *outpsize, osync_bool *free_input,
                                 const char *config, OSyncError **error) {

	osync_trace(TRACE_ENTRY, "%s", __func__);

	osync_bool success = FALSE;
	TranslationEnv* env = init_sif_2_xml(input, inpsize, SIFN_2_XML_FORMAT_NOTE_FILE, error);
	if(!env)
		return FALSE;
	
	success = translate(env);

	if (success) {
		OSyncXMLFormat* xmlformat = osync_xmlformat_parse((char *) env->out_xml,
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
				osync_trace(TRACE_INTERNAL, "XMLFORMAT NOTE: Not valid!");
			else
				osync_trace(TRACE_INTERNAL, "XMLFORMAT NOTE: Valid!");
		}

		freeEnv(env);
	}

	return success;
}

osync_bool xmlnote_2_sifn(char* input, unsigned int inpsize,
                                 char** output, unsigned int * outpsize, osync_bool* free_input,
                                 const char* config, OSyncError** error) {

	osync_trace(TRACE_ENTRY, "%s", __func__);

	osync_bool success = FALSE;
	TranslationEnv* env = init_xml_2_sif(input, XML_FORMAT_NOTE_2_SIFN_FILE, error);
	if(!env)
		return FALSE;

	success = translate(env);

	if (success) {
		*free_input = TRUE;
		*output = g_strdup((char *) env->out_xml);
		*outpsize = env->out_size;

		osync_trace(TRACE_INTERNAL, "... Output SIFN is: \n%s", *output);
	}

	freeEnv(env);

	osync_trace(TRACE_EXIT, "%s", __func__);

	return success;
}
