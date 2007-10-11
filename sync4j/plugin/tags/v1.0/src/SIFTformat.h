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

#ifndef SIFTFORMAT_H
#define SIFTFORMAT_H

#include <opensync/opensync.h>

#define SIFT_2_XML_FORMAT_TODO_FILE 	"sift2xmlformat-todo.xsl"
#define XML_FORMAT_TODO_2_SIFT_FILE		"xmlformat-todo2sift.xsl"
#define SIFT			"sift"
#define OBJ_TYPE_TODO	"todo"

osync_bool get_sift_conversion_info(OSyncFormatEnv* env, OSyncError** error);

osync_bool sift_2_xmltodo(char* input, unsigned int inpsize,
	char** output, unsigned int* outpsize, osync_bool* free_input,
	const char* config, OSyncError** error);

osync_bool xmltodo_2_sift(char* input, unsigned int inpsize,
	char** output, unsigned int * outpsize, osync_bool* free_input,
	const char* config, OSyncError** error);

#endif
