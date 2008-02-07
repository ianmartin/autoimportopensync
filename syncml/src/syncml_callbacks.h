/*
 * syncml plugin - A syncml plugin for OpenSync
 * Copyright (C) 2005  Armin Bauer <armin.bauer@opensync.org>
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

void _manager_event(
		SmlManager *manager,
		SmlManagerEventType type,
		SmlSession *session,
		SmlError *error,
		void *userdata);

/* *************************************** */
/* *****     DsSession Callbacks     ***** */
/* *************************************** */

void _ds_event(SmlDsSession *dsession, SmlDsEvent event, void *userdata);

void _ds_alert(SmlDsSession *dsession, void *userdata);

/* *********************************** */
/* *****     Alert Callbacks     ***** */
/* *********************************** */

/* The real alert callbacks are specific for DS clients and servers.
 * So please check syncml_ds_client.h and syncml_ds_server.h for more details.
 */

void _recv_alert_reply(SmlSession *session, SmlStatus *status, void *userdata);

/* ********************************** */
/* *****     Sync Callbacks     ***** */
/* ********************************** */

void _recv_sync(SmlDsSession *dsession, unsigned int numchanges, void *userdata);

void _recv_sync_reply(SmlSession *session, SmlStatus *status, void *userdata);

/* ************************************ */
/* *****     Change Callbacks     ***** */
/* ************************************ */

SmlBool _recv_change(
		SmlDsSession *dsession,
		SmlChangeType type,
		const char *uid,
		char *data, unsigned int size,
		const char *contenttype,
		void *userdata,
		SmlError **smlerror);

void _recv_change_reply(
		SmlDsSession *dsession,
		SmlStatus *status,
		const char *newuid,
		void *userdata);

/* ********************************* */
/* *****     Map Callbacks     ***** */
/* ********************************* */

void _recv_map_reply(SmlSession *session, SmlStatus *status, void *userdata);

/* ******************************************* */
/* *****     Authentication Callback     ***** */
/* ******************************************* */

void _verify_user(
		SmlAuthenticator *auth,
		const char *username,
		const char *password,
		void *userdata,
		SmlErrorType *reply);

#endif //_SYNCML_CALLBACKS_H
