#include "evolution_sync.h"

/*open xml file check it an return pointer to first entry*/
int open_xml_file(evo_environment *env, xmlDocPtr *doc, xmlNodePtr *cur, char *file, char *topentry)
{
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		evo_debug(env, 1, "File %s does not exist", file);
		return 1;
	}
	
	*doc = xmlParseFile(file);

	if (!*doc) {
		evo_debug(env, 1, "Could not open: %s", file);
		return 1;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		evo_debug(env, 0, "%s seems to be empty", file);
		xmlFreeDoc(*doc);
		return 1;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		evo_debug(env, 0, "%s seems not to be a valid configfile.\n", file);
		xmlFreeDoc(*doc);
		return 1;
	}

	*cur = (*cur)->xmlChildrenNode;
	return 0;
}


/*Load the state from a xml file and return it in the conn struct*/
int load_evo_settings(evo_environment *env, char *configfile)
{	
	xmlDocPtr doc;
	xmlNodePtr cur;
	evo_debug(env, 2, "Loading state from file %s", configfile);

	//set defaults
	env->adressbook_path = NULL;
	env->calendar_path = NULL;
	env->tasks_path = NULL;

	if (open_xml_file(env, &doc, &cur, configfile, "config")) {
		return 1;
	}

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
	evo_debug(env, 3, "end: load_palm_state");
	return 0;
}
