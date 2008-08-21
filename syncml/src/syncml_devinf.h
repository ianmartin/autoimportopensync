
#ifndef _SYNCML_DEVINF_H
#define _SYNCML_DEVINF_H

#include "syncml_common.h"

const char *get_database_pref_content_type(
				SmlDatabase *database,
				OSyncError **error);

/* ************************************************ */
/* *****     Device Information Callbacks     ***** */
/* ************************************************ */

SmlBool _write_devinf(
		SmlDataSyncObject *dsObject,
		SmlDevInf *devinf,
		void *userdata,
		SmlError **error);

SmlDevInf *_read_devinf(
		SmlDataSyncObject *dsObject,
		const char *devid,
		void *userdata,
		SmlError **error);

#endif //_SYNCML_DEVINF_H
