#include "evolution_sync.h"

/*Load the state from a xml file and return it in the conn struct*/
osync_bool evo2_parse_settings(evo_environment *env, char *data, int size)
{	
	xmlDocPtr doc;
	xmlNodePtr cur;
	osync_debug("EVO2-SYNC", 4, "start: %s", __func__);

	//set defaults
	env->adressbook_path = NULL;
	env->calendar_path = NULL;
	env->tasks_path = NULL;

	*doc = xmlParseMemory(data, size);

	if (!*doc) {
		osync_debug(env, 1, "Could not parse data!\n");
		return FALSE;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		evo_debug(env, 0, "%s seems to be empty", file);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		evo_debug(env, 0, "%s seems not to be a valid configfile.\n", file);
		xmlFreeDoc(*doc);
		return FALSE;
	}

	*cur = (*cur)->xmlChildrenNode;

	while (cur != NULL) {
		char *str = xmlNodeGetContent(cur);
		if (str) {
			if (!xmlStrcmp(cur->name, (const xmlChar *)"adress_path")) {
				env->adressbook_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"calendar_path")) {
				env->calendar_path = g_strdup(str);
			}
			if (!xmlStrcmp(cur->name, (const xmlChar *)"tasks_path")) {
				env->tasks_path = g_strdup(str);	
			}
			xmlFree(str);
		}
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	return TRUE;
}
