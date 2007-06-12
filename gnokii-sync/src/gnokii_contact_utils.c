/***************************************************************************
 *   Copyright (C) 2006 by Daniel Gollub                                   *
 *                            <dgollub@suse.de>                            *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#include "gnokii_sync.h"

/* This function cleans the telephone number. Nokia cellphones only 
 * supports +*0123456789 as charaters in the number.  
 *
 * 
 * Returns *char of a clean number
 */
char *gnokii_contact_util_cleannumber(char *number) {
	osync_trace(TRACE_ENTRY, "%s(%s)", __func__, number);

	int i;
	int len = 0;
	char *tmp = g_strdup("");

	len = (int) strlen(number);

	osync_trace(TRACE_INTERNAL, "strlen %i\n", len);
	
	for (i=0; i < len; i++) {
		switch (number[i]) {
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':	
			case '+':	
			case '*':
			case '#':
			case 'p':
			case 'w':	
				tmp = g_strdup_printf("%s%c", tmp, number[i]);
			default:
				break;	
		}

	}	

	osync_trace(TRACE_EXIT, "%s: %s", __func__, tmp);
	return tmp;
}

