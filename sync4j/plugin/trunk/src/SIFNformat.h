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

#ifndef SIFNFORMAT_H
#define SIFNFORMAT_H

#include <opensync/opensync.h>

#define SIFN_2_XML_FORMAT_NOTE_FILE 	"sifn2xmlformat-note.xsl"
#define XML_FORMAT_NOTE_2_SIFN_FILE		"xmlformat-note2sifn.xsl"
#define SIFN 			"sifn"
#define OBJ_TYPE_NOTE	"note"

osync_bool get_sifn_conversion_info(OSyncFormatEnv* env, OSyncError** error);

osync_bool sifn_2_xmlnote(char* input, unsigned int inpsize,
	char** output, unsigned int* outpsize, osync_bool* free_input,
	const char* config, OSyncError** error);

osync_bool xmlnote_2_sifn(char* input, unsigned int inpsize,
	char** output, unsigned int * outpsize, osync_bool* free_input,
	const char* config, OSyncError** error);

#endif
