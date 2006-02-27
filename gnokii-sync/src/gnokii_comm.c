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
#include <opensync/opensync.h>

/* Connects the cellphone with libgnokii functions
 * 
 * Returns: bool
 * ReturnVal: true	on success
 * ReturnVal: false	on error
 */
osync_bool gnokii_comm_connect(struct gn_statemachine *state)
{
	gn_error gsm_error;

	osync_trace(TRACE_ENTRY, "%s()", __func__);

	if ((gsm_error = gn_gsm_initialise(state)) != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s (libgnokii): %s", __func__, gn_error_print(gsm_error)); 
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s()", __func__);
	return TRUE;
} 

/* Disconnects the cellphone with libgnokii functions
 * 
 * Returns: bool
 * ReturnVal: true	on success
 * ReturnVal: false	on error
 */
osync_bool gnokii_comm_disconnect(struct gn_statemachine *state) 
{
	gn_error gsm_error;

	osync_trace(TRACE_ENTRY, "%s()", __func__);
	
	if ((gsm_error = gn_sm_functions(GN_OP_Terminate, NULL, state)) != GN_ERR_NONE) {
		osync_trace(TRACE_EXIT_ERROR, "%s (libgnokii): %s", __func__, gn_error_print(gsm_error));
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s()", __func__);
	
	return TRUE;
}
