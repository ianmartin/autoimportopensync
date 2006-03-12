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
#include <string.h>
#include <glib.h>
#include <opensync/opensync.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "config.h"

#include "gnokii_config.h"
#include "gnokii_comm.h"
#include "gnokii_calendar.h"

typedef struct gnokii_environment {
	OSyncMember		*member;
	OSyncHashTable		*hashtable;
	gn_config		*config;
	struct gn_statemachine 	*state;

} gnokii_environment;

