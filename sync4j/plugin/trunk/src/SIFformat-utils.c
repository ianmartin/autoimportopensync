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

#include "SIFformat-utils.h"
#include "config.h"

#include <string.h>
#include <libxslt/xsltutils.h>

TranslationEnv* init_sif_2_xml(char* input, unsigned int inpsize,
							   char* defFile, OSyncError** error) {
	osync_trace(TRACE_INTERNAL, "Input sift is:\n%s", input);

	TranslationEnv* env = osync_try_malloc0(sizeof(TranslationEnv), error);

	if (!env) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to create " \
				"translation environment");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	env->input = input;
	env->isize = inpsize;
	env->error = error;
	env->sheet_definition = defFile;
	env->sheet = NULL;
	env->in = NULL;
	env->out = NULL;
	env->out_xml = NULL;
	env->out_size = 0;

	return env;
}

TranslationEnv* init_xml_2_sif(char* input, char* defFile, OSyncError** error) {
	TranslationEnv* env = osync_try_malloc0(sizeof(TranslationEnv), error);

	if (!env) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to create " \
				"translation environment");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		return NULL;
	}

	env->input = NULL;
	env->isize = 0;
	env->error = error;
	env->sheet_definition = defFile;
	env->sheet = NULL;
	env->in = NULL;
	env->out = NULL;
	env->out_xml = NULL;
	env->out_size = 0;
	
	if (!osync_xmlformat_assemble((OSyncXMLFormat*) input, &(env->input), &(env->isize))) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Failed to create xml document");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(error));
		freeEnv(env);
		return NULL;
	}

	osync_trace(TRACE_INTERNAL, "Input XMLFormat is:\n%s", env->input);

	return env;
}

osync_bool translate(TranslationEnv* env) {
	xmlSubstituteEntitiesDefault(1);
	xmlLoadExtDtdDefaultValue = 1;
		
	int len = strlen(XSLTDIR) + strlen(env->sheet_definition) + 2;
	char xslt_file[len];
	sprintf(xslt_file, "%s/%s", XSLTDIR, env->sheet_definition);
	env->sheet = xsltParseStylesheetFile((const xmlChar *)xslt_file);

	if(!env->sheet) {
		osync_error_set(env->error, OSYNC_ERROR_GENERIC, "Could not open " \
				"xslt definition located at '%s'", xslt_file);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(env->error));
		freeEnv(env);
		return FALSE;
	}
	
	env->in = xmlParseMemory(env->input, env->isize);
	if(!env->in) {
		osync_error_set(env->error, OSYNC_ERROR_GENERIC, "Could not parse sifn object:\n%s", env->input);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(env->error));
		freeEnv(env);
		return FALSE;
	}

	const char* params[1];
	params[0] = NULL;
	env->out = xsltApplyStylesheet(env->sheet, env->in, params);
	if(!env->out) {
		osync_error_set(env->error, OSYNC_ERROR_GENERIC, "Error in applying xslt stylesheet");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(env->error));
		freeEnv(env);
		return FALSE;
	}

	xmlIndentTreeOutput = 1;
	xmlDocDumpFormatMemory(env->out, &(env->out_xml), &(env->out_size), 1);
	if(!env->out_xml) {
		osync_error_set(env->error, OSYNC_ERROR_GENERIC, "Error in generating xmlnote object");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s", __func__, osync_error_print(env->error));
		freeEnv(env);
		return FALSE;
	}

	return TRUE;

}

void freeEnv(TranslationEnv* env) {
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, env);
	if(env->sheet)
		xsltFreeStylesheet(env->sheet);
	if(env->in)
		xmlFreeDoc(env->in);
	if(env->out)
		xmlFreeDoc(env->out);
	if(env->out_xml)
		g_free(env->out_xml);

	g_free(env);

	xsltCleanupGlobals();
	xmlCleanupParser();

	osync_trace(TRACE_EXIT, "%s", __func__);
}
