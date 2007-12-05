
#ifndef _SYNCML_DEVINF_H
#define _SYNCML_DEVINF_H

#include "syncml_common.h"

const char *get_database_pref_content_type(
				SmlDatabase *database,
				OSyncError **error);

SmlDevInfDataStore *add_dev_inf_datastore(
				SmlDevInf *devinf, 
				SmlDatabase *database, 
				OSyncError **error);

#endif //_SYNCML_DEVINF_H
