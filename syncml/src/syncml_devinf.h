
#ifndef _SYNCML_DEVINF_H
#define _SYNCML_DEVINF_H

#include "syncml_common.h"

const char *get_database_pref_content_type(
				SmlDatabase *database,
				OSyncError **error);

SmlDevInfDataStore *add_devinf_datastore(
				SmlDevInf *devinf, 
				SmlDatabase *database, 
				OSyncError **error);

SmlBool store_devinf(
		SmlDevInf *devinf,
		const char *filename,
		OSyncError **error);

char *get_devinf_identifier();

SmlDevInf *get_new_devinf(
		SmlPluginEnv *env,
		SmlDevInfDevTyp type,
		SmlError **serror);

#endif //_SYNCML_DEVINF_H
