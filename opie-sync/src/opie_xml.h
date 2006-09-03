#ifndef _OPIE_XML_H_
#define _OPIE_XML_H_

/* 

   Copyright 2005 Paul Eggleton

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

*/

#include <glib.h>

#include "opie_comms.h"
#include "opie_sync.h"

xmlDoc *opie_xml_fd_open(int fd);
xmlDoc *opie_xml_file_open(const gchar *xml_file);
xmlNode *opie_xml_get_collection(xmlDoc *doc, const char *listelement);
xmlNode *opie_xml_get_first(xmlDoc *doc, const char *listelement, const char *itemelement);
xmlNode *opie_xml_get_next(xmlNode *prev_node);
xmlNode *opie_xml_find_by_uid(xmlDoc *doc, const char *listelement, const char *itemelement, const char *find_uid);
xmlNode *opie_xml_add_node(xmlDoc *doc, const char *listelement, xmlNode *new_node);
xmlNode *opie_xml_update_node(xmlDoc *doc, const char *listelement, xmlNode *new_node);
void opie_xml_remove_by_tagged_uid(xmlDoc *doc, const char *listelement, const char *itemelement, const char *tagged_uid);
xmlDoc *opie_xml_change_parse(const char *change_data, xmlNode **node);

char *hash_xml_node(xmlDoc *doc, xmlNode *node);
char *hash_str(const char *str);
char *xml_node_to_text(xmlDoc *doc, xmlNode *node); 
void xml_node_to_attr(xmlNode *node_from, const char *nodename, xmlNode *node_to, const char *attrname);
char *opie_xml_untag_uid(const char *tagged_uid, const char *node_name);
void opie_xml_set_tagged_uid(xmlNode *node, const char *tagged_uid);
char *opie_xml_get_tagged_uid(xmlNode *node);
char *opie_xml_get_uid(xmlNode *node);
void opie_xml_set_uid(xmlNode *node, const char *uid);
int opie_xml_save_to_fd(xmlDoc *doc, int fd);
		
#endif
