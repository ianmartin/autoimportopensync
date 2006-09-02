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

/*
 *  $Id: opie_xml.c,v 1.11 2004/02/20 15:55:14 irix Exp $
 */

#include "opie_xml.h"
#include "opie_sync.h"
#include "opie_comms.h"
#include <openssl/md5.h>

#include <string.h>


xmlDoc *opie_xml_file_open(const gchar *xml_file) {
	xmlDoc *doc;
	
	doc = xmlParseFile(xml_file);
	if (!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to parse XML file %s", xml_file);
		goto error;
	}

	return doc;
	
error:
	return NULL;
}

xmlNode *opie_xml_get_collection(xmlDoc *doc, const char *listelement) {
	xmlNode *cur = xmlDocGetRootElement(doc);
	if (!cur) {
		osync_trace(TRACE_INTERNAL, "Unable to get root element");
		goto error;
	}
	
	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if(!strcasecmp(cur->name, listelement))
			break;
		cur = cur->next;
	}

	if (!cur) {
		osync_trace(TRACE_INTERNAL, "Unable to get list element %s", listelement);
		goto error;
	}

	return cur;
	
error:
		return NULL;
}

xmlNode *opie_xml_get_first(xmlDoc *doc, const char *listelement, const char *itemelement) {
	xmlNode *cur = opie_xml_get_collection(doc, listelement);
	if(!cur)
		goto error;
	
	cur = cur->xmlChildrenNode;
	while(cur && strcmp(itemelement, cur->name))
		cur = cur->next;

	return cur;
	
error:
	return NULL;
}

xmlNode *opie_xml_get_next(xmlNode *prev_node) {
	xmlNode *node = prev_node->next;
	while(node && strcmp(prev_node->name, node->name))
		node = node->next;
	return node;
}

xmlNode *opie_xml_add_node(xmlDoc *doc, const char *listelement, xmlNode *new_node) {
	xmlNode *collection_node = opie_xml_get_collection(doc, listelement);
	if(!collection_node)
		goto error;
	
	xmlNode *new_copy = xmlCopyNode(new_node, 1);
	if(!new_copy) {
		osync_trace(TRACE_INTERNAL, "Unable to duplicate node");
		goto error;
	}
	
	if(!xmlAddChild(collection_node, new_copy)) {
		osync_trace(TRACE_INTERNAL, "Unable to add node to document");
		xmlFreeNode(new_copy);
		goto error;
	}	
	
	return new_copy;
	
error:
	return NULL;
}

xmlDoc *opie_xml_change_parse(const char *change_data, xmlNode **node) {
	xmlDoc *change_doc = xmlRecoverMemory(change_data, strlen(change_data));
	if(change_doc) {
		*node = xmlDocGetRootElement(change_doc);
	}
	return change_doc;
}

char *hash_str(const char *str) {
	unsigned char* t_hash;
	MD5_CTX c;
	
	MD5_Init(&c);
	t_hash = g_malloc0(MD5_DIGEST_LENGTH + 1);
	
	MD5_Update(&c, str, strlen(str));

	/* compute the hash */
	MD5_Final(t_hash, &c);

	return t_hash;
}

char *hash_xml_node(xmlDoc *doc, xmlNode *node) {
	unsigned char* t_hash;
	xmlBufferPtr bufptr;
	
	bufptr = xmlBufferCreate();
	xmlNodeDump(bufptr, doc, node, 0, 0);
	const char *bufstr = xmlBufferContent(bufptr);
	t_hash = hash_str(bufstr);
	xmlBufferFree(bufptr);

	return t_hash;
}

char *xml_node_to_text(xmlDoc *doc, xmlNode *node) {
	xmlBufferPtr bufptr = xmlBufferCreate();
	xmlNodeDump(bufptr, doc, node, 0, 0);
	char *nodetext = strdup(xmlBufferContent(bufptr));
	xmlBufferFree(bufptr);
	return nodetext;
}

