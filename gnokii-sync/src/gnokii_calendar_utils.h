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

#include <gnokii.h>

char *gnokii_util_unix2wday(const time_t *date);
char *gnokii_util_caltype2string(gn_calnote_type type);
gn_timestamp gnokii_util_unix2timestamp(time_t time);
gn_timestamp gnokii_util_tm2timestamp(const struct tm *timetm);
time_t gnokii_util_timestamp2unix(gn_timestamp *timestamp);
int gnokii_util_alarmevent2secs(const char *alarm);
char *gnokii_util_secs2alarmevent(int secs_before_event); 
osync_bool gnokii_util_valid_number(const char *number); 
gn_calnote_type gnokii_util_calendar_type(gn_calnote *calnote, osync_bool alldayevent);

