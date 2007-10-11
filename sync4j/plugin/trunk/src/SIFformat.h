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

#ifndef SIFFORMAT_H
#define SIFFORMAT_H

#include <opensync/opensync.h>
#include <opensync/opensync-format.h>

osync_bool get_format_info(OSyncFormatEnv* env, OSyncError** error);
osync_bool get_conversion_info(OSyncFormatEnv* env, OSyncError** error);
char* print_sif(const char* data, unsigned int size);
void destroy_sif(char* input, size_t inpsize);
int get_version(void);
static osync_bool get_specific_format_info(OSyncFormatEnv* env, OSyncError** error,
										  const char* formatname, const char* objtype);

#endif





