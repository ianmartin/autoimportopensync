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

#ifndef SIFFORMATUTILS_H
#define SIFFORMATUTILS_H

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>
#include <opensync/opensync-merger.h>
#include <libxslt/xslt.h>
#include <libxslt/transform.h>
#include <glib.h>

typedef struct TranslationEnv {
	char* input;
	unsigned int isize;
	OSyncError** error;
	char* sheet_definition;
	xsltStylesheetPtr sheet;
	xmlDocPtr in;
	xmlDocPtr out;
	xmlChar* out_xml;
	int out_size;
} TranslationEnv;

TranslationEnv* init_sif_2_xml(char* input, unsigned int inpsize,
							   char* defFile, OSyncError** error);
TranslationEnv* init_xml_2_sif(char* input, char* defFile, OSyncError** error);
osync_bool translate(TranslationEnv* env);
void freeEnv(TranslationEnv* env);

#endif

