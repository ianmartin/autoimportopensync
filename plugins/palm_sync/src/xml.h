#ifndef XML_H
#define XML_H

int open_xml_file(palm_connection *conn, xmlDocPtr *doc, xmlNodePtr *cur, char *file, char *topentry);
int load_palm_settings(palm_connection *conn, char *configfile);
void save_palm_settings(palm_connection *conn);

#endif /*  XML_H */
