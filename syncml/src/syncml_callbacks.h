/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2008  Michael Bell <michael.bell@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
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
 *
 */
 
#ifndef _SYNCML_CALLBACKS_H
#define _SYNCML_CALLBACKS_H

#include "syncml_common.h"

/* **************************************** */
/* *****     Management Callbacks     ***** */
/* **************************************** */

void _recv_event(
		SmlDataSyncObject *dsObject,
		SmlDataSyncEventType type,
		void *userdata,
		SmlError *error);

/* *********************************** */
/* *****     Alert Callbacks     ***** */
/* *********************************** */

SmlAlertType _get_alert_type(
			SmlDataSyncObject *dsObject,
			const char *source,
			SmlAlertType type,
			void *userdata,
			SmlError **error);

char *_get_anchor(
		SmlDataSyncObject *dsObject,
		const char *name,
		void *userdata,
		SmlError **error);

SmlBool _set_anchor(
		SmlDataSyncObject *dsObject,
		const char *name,
		const char *value,
		void *userdata,
		SmlError **error);

/* ************************************ */
/* *****     Change Callbacks     ***** */
/* ************************************ */

SmlBool _recv_change(
		SmlDataSyncObject *dsObject,
		const char *source,
		SmlChangeType type,
		const char *uid,
		char *data,
		unsigned int size,
		void *userdata,
		SmlError **error);

SmlBool _recv_unwanted_change(
		SmlDataSyncObject *dsObject,
		const char *source,
		SmlChangeType type,
		const char *uid,
		char *data,
		unsigned int size,
		void *userdata,
		SmlError **error);

SmlBool _recv_change_status(
		SmlDataSyncObject *dsObject,
		unsigned int code,
		const char *newuid,
		void *userdata,
		SmlError **error);

#endif //_SYNCML_CALLBACKS_H
