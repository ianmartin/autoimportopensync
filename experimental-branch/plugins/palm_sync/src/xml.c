#include "palm_sync.h"
#include "xml.h"

/*open xml file check it an return pointer to first entry*/
int open_xml_file(palm_connection *conn, xmlDocPtr *doc, xmlNodePtr *cur, char *file, char *topentry)
{
	if (!g_file_test(file, G_FILE_TEST_EXISTS)) {
		palm_debug(conn, 1, "File %s does not exist", file);
		return 1;
	}
	
	*doc = xmlParseFile(file);

	if (!*doc) {
		palm_debug(conn, 1, "Could not open: %s", file);
		return 1;
	}

	*cur = xmlDocGetRootElement(*doc);

	if (!*cur) {
		palm_debug(conn, 0, "%s seems to be empty", file);
		xmlFreeDoc(*doc);
		return 1;
	}

	if (xmlStrcmp((*cur)->name, (const xmlChar *) topentry)) {
		palm_debug(conn, 0, "%s seems not to be a valid configfile.\n", file);
		xmlFreeDoc(*doc);
		return 1;
	}

	*cur = (*cur)->xmlChildrenNode;
	return 0;
}


/*Load the state from a xml file and return it in the conn struct*/
int load_palm_settings(palm_connection *conn, char *configfile)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	palm_debug(conn, 2, "Loading state from file %s", configfile);

	//set defaults
	conn->sockaddr = "/dev/pilot";
	strcpy(conn->username, "");
	strcpy(conn->codepage, "cp1252");
	conn->id = 0;
	conn->debuglevel = 0;
	conn->conntype = 0;
	conn->speed = 57600;
	conn->timeout = 2;
	conn->mismatch = 1;
	conn->popup = 0;

	if (open_xml_file(conn, &doc, &cur, configfile, "config")) {
		return 1;
	}

	while (cur != NULL) {
	        char *str = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
		if (str && 
		    (!xmlStrcmp(cur->name, (const xmlChar *)"sockaddr"))) {
		        conn->sockaddr = g_strdup(str);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"username"))) {
		        strncpy(conn->username, str, 1024);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"debuglevel"))) {
			conn->debuglevel = atoi(str);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"timeout"))) {
			conn->timeout = atoi(str);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"type"))) {
			conn->conntype = atoi(str);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"speed"))) {
			conn->speed = atoi(str);
		}

		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"id"))) {
    		        conn->id = atoi(str);
		}
		
		if (str &&
		    (!xmlStrcmp(cur->name, (const xmlChar *)"codepage"))) {
    		        strncpy(conn->codepage, str, 1024);
		}

		if ((!xmlStrcmp(cur->name, (const xmlChar *)"popup"))) {
			conn->popup = atoi(str);
		}

		if ((!xmlStrcmp(cur->name, (const xmlChar *)"mismatch"))) {
			conn->mismatch = atoi(str);
		}
		if (str)
 		        xmlFree(str);
		cur = cur->next;
	}

	xmlFreeDoc(doc);
	palm_debug(conn, 3, "end: load_palm_state");
	return 0;
}

/*Save the state from the conn struct to a xmlfile*/
void save_palm_settings(palm_connection *conn)
{
	xmlDocPtr doc;
	char debuglevel[256], type[256], speed[256], timeout[256], id[256], mismatch[256], popup[256];
	palm_debug(conn, 2, "Saving state to file %s", conn->statefile);

	doc = xmlNewDoc("1.0");
	doc->children = xmlNewDocNode(doc, NULL, "config", NULL);

	snprintf(debuglevel, 256, "%i", conn->debuglevel);
	snprintf(id, 256, "%i", conn->id);
	snprintf(type, 256, "%i", conn->conntype);
	snprintf(speed, 256, "%i", conn->speed);
	snprintf(timeout, 256, "%i", conn->timeout);
	snprintf(popup, 256, "%i", conn->popup);
	snprintf(mismatch, 256, "%i", conn->mismatch);


	xmlNewChild(doc->children, NULL, "username", conn->username);
	xmlNewChild(doc->children, NULL, "id", id);
	xmlNewChild(doc->children, NULL, "debuglevel", debuglevel);
	xmlNewChild(doc->children, NULL, "type", type);
	xmlNewChild(doc->children, NULL, "speed", speed);
	xmlNewChild(doc->children, NULL, "timeout", timeout);
	xmlNewChild(doc->children, NULL, "sockaddr", conn->sockaddr);
	xmlNewChild(doc->children, NULL, "popup", popup);
	xmlNewChild(doc->children, NULL, "mismatch", mismatch);
	xmlNewChild(doc->children, NULL, "codepage", conn->codepage);

	xmlSaveFile(conn->statefile, doc);
	xmlFreeDoc(doc);
	palm_debug(conn, 3, "end: save_palm_state");
}
