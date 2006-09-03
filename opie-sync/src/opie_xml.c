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


xmlDoc *opie_xml_fd_open(int fd) {
	xmlDoc *doc = xmlReadFd(fd, "/", NULL, 0);
	if (!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to parse XML data");
		goto error;
	}

	return doc;
	
error:
	return NULL;
}


xmlDoc *opie_xml_file_open(const gchar *xml_file) {
	xmlDoc *doc = xmlParseFile(xml_file);
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
	
	if(strcasecmp(cur->name, listelement)) {
		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if(!strcasecmp(cur->name, listelement))
				break;
			cur = cur->next;
		}
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

xmlNode *opie_xml_find_by_uid(xmlDoc *doc, const char *listelement, const char *itemelement, const char *find_uid) {
	char *uid;
	
	xmlNode *node = opie_xml_get_first(doc, listelement, itemelement);
	while(node) {
		uid = opie_xml_get_uid(node);
		if(!strcmp(uid, find_uid)) {
			xmlFree(uid);
			break;
		}
		xmlFree(uid);
		node = opie_xml_get_next(node);
	}
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

xmlNode *opie_xml_update_node(xmlDoc *doc, const char *listelement, xmlNode *new_node) {
	char *new_uid = opie_xml_get_uid(new_node);
	xmlNode *node = opie_xml_find_by_uid(doc, listelement, new_node->name, new_uid); 
	xmlFree(new_uid);
	if(!node) {
		osync_trace(TRACE_INTERNAL, "Unable to find existing node to update");
		goto error;
	}
	
	xmlNode *new_copy = xmlCopyNode(new_node, 1);
	if(!new_copy) {
		osync_trace(TRACE_INTERNAL, "Unable to duplicate node");
		goto error;
	}
	
	xmlReplaceNode(node, new_copy);
	
	return new_copy;
	
error:
	return NULL;
}

void opie_xml_remove_by_tagged_uid(xmlDoc *doc, const char *listelement, const char *itemelement, const char *tagged_uid) {
	char *uid = opie_xml_untag_uid(tagged_uid, itemelement);
	xmlNode *node = opie_xml_find_by_uid(doc, listelement, itemelement, uid); 
	g_free(uid);
	if(!node) {
		osync_trace(TRACE_INTERNAL, "Unable to find existing node to remove");
		return;
	}
	
	xmlUnlinkNode(node);
	xmlFreeNode(node);
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
	int length = xmlBufferLength(bufptr);
	char *nodetext = g_malloc0(length+1);
	memcpy(nodetext, xmlBufferContent(bufptr), length);
	xmlBufferFree(bufptr);
	return nodetext;
}

char *opie_xml_untag_uid(const char *tagged_uid, const char *node_name) {
	char *uidtag = "uid-unknown-%32s";
	if(!strcasecmp(node_name, "Contact")) {
		uidtag = "uid-contact-%32s";
	}
	else if(!strcasecmp(node_name, "Task")) {
		uidtag = "uid-todo-%32s";
	}
	else if(!strcasecmp(node_name, "event")) {
		uidtag = "uid-event-%32s";
	}
	
	char *uid = g_malloc0(32);
	sscanf(tagged_uid, uidtag, uid);
	return uid;
}

void opie_xml_set_tagged_uid(xmlNode *node, const char *tagged_uid) {
	char *uid = opie_xml_untag_uid(tagged_uid, node->name);	
	opie_xml_set_uid(node, uid);
	g_free(uid);
}

char *opie_xml_get_tagged_uid(xmlNode *node) {
	char *uidtag = "uid-unknown-%32s";
	if(!strcasecmp(node->name, "Contact")) {
		uidtag = "uid-contact-%s";
	}
	else if(!strcasecmp(node->name, "Task")) {
		uidtag = "uid-todo-%s";
	}
	else if(!strcasecmp(node->name, "event")) {
		uidtag = "uid-event-%s";
	}
	
	char *uid = opie_xml_get_uid(node);
	if(uid) {
		char *tagged_uid = g_strdup_printf(uidtag, uid);
		xmlFree(uid);
		return tagged_uid;
	}
	else {
		return NULL;
	}
}

char *opie_xml_get_uid(xmlNode *node) {
	char *uidattr;
	if(!strcasecmp(node->name, "event")) {
		uidattr = "uid";
	}
	else {
		uidattr = "Uid";
	}
	
	return xmlGetProp(node, uidattr);
}

void opie_xml_set_uid(xmlNode *node, const char *uid) {
	char *uidattr;
	if(!strcasecmp(node->name, "event")) {
		uidattr = "uid";
	}
	else {
		uidattr = "Uid";
	}
	xmlSetProp(node, uidattr, uid);
}

int opie_xml_save_to_fd(xmlDoc *doc, int fd) {
	ftruncate(fd, 0);
	xmlOutputBuffer *buf = xmlOutputBufferCreateFd(fd, NULL);
	/* Prevent the fd from being closed after writing */
	buf->closecallback = NULL;
	/* Write the XML */
	int bytes = xmlSaveFormatFileTo(buf, doc, NULL, 1);
	return bytes;
}
