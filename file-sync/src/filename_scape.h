#ifndef __FILENAME_SCAPER__
#define __FILENAME_SCAPER__

/**
 * @file   filename_scape.h
 * @author Adenilson Cavalcanti <savagobr@yahoo.com>
 * @date   Tue Oct 21 15:31:10 2008
 *
 * @brief  An auxiliary module to scape invalid characters from filename.
 *
 * This code is free software; you can redistribute it and/or
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

 */

static const char reserved_chars[] = { '/', '!', '?', ':', '*', '\\', '>', '<', '@' };
static const int reserved_count = 9;
static const char scaper = '_';

static void filename_scape_characters(char *input)
{

	int i;

	while (*input) {
		for (i = 0; i < reserved_count; ++i)
			if (*input ==  reserved_chars[i]) {
				*input++ = scaper;
				goto done;
			}
		++input;
	done:
		;
	}

}

#endif
