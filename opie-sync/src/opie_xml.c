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

gint gslist_sort_attr(gconstpointer a, gconstpointer b) {
	if(a && b)
		return strcmp(((const struct _xmlAttr *)a)->name, ((const struct _xmlAttr *)b)->name);
	else if(a)
		return 1;
	else if(b)
		return -1;
	else
		return 0;
}


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
	
	if(!strcasecmp("note", new_copy->name)) {
		xmlSetProp(new_copy, "changed", "1");
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
	
	if(!strcasecmp("note", new_copy->name)) {
		xmlSetProp(new_copy, "changed", "1");
	}
	
	xmlReplaceNode(node, new_copy);
	
	return new_copy;
	
error:
	return NULL;
}

void opie_xml_remove_by_uid(xmlDoc *doc, const char *listelement, const char *itemelement, const char *uid) {
	xmlNode *node = opie_xml_find_by_uid(doc, listelement, itemelement, uid); 
	if(!node) {
		osync_trace(TRACE_INTERNAL, "Unable to find existing node to remove");
		return;
	}
	
	if(!strcasecmp("note", itemelement)) {
		/* We just mark notes as deleted and then delete them later */
		xmlSetProp(node, "changed", "1");
		xmlSetProp(node, "deleted", "1");
	}
	else {	
		xmlUnlinkNode(node);
		xmlFreeNode(node);
	}
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
	char *uidattrname;
	struct _xmlAttr *iprop;
	int iscontact;
	
	if(!strcasecmp(node->name, "note")) {
		/* For notes we just hash the content only */
		char *content = (char*)xmlNodeGetContent(node);
		if(content) {
			t_hash = hash_str(content);
			xmlFree(content);
		}
		else
			t_hash = hash_str("");
	}
	else {
		/* Copy the attributes into a list, inserting them in sort order.
		   we also discard some attributes we don't want */
		
		GSList *attrlist = NULL;
		GSList *attrlistptr = NULL;
		
		uidattrname = opie_xml_get_uidattr(node);
		iscontact = !strcasecmp(node->name, "Contact");
		for (iprop = node->properties; iprop; iprop=iprop->next) {
			if (iprop->children && iprop->children->content) {
				if((!iscontact || strcasecmp(iprop->name, "opie-contactfield-order"))
								 && strcasecmp(iprop->name, uidattrname)) {
					attrlist = g_slist_insert_sorted(attrlist, iprop, gslist_sort_attr);
				}
			}
		}
		
		if(attrlist) {
			/* List is sorted, now create a node and put them into it.
			  Clearly this is a lot of effort to go to, but I wanted to produce
			  the same results as if the node was produced by libxml2, and what
			  better way than just doing exactly that? */
			xmlDoc *dummydoc = xmlNewDoc((xmlChar*)"1.0");
			xmlNode *dummyroot = xmlNewNode(NULL, "Temp");
			xmlDocSetRootElement(dummydoc, dummyroot);
			xmlNode *dupnode = xmlNewTextChild( dummyroot, NULL, node->name, NULL);
			
			attrlistptr = attrlist;
			while(attrlistptr) {
				iprop = (struct _xmlAttr *)(attrlistptr->data);
				xmlSetProp(dupnode, iprop->name, iprop->children->content);
				attrlistptr = g_slist_next(attrlistptr);
			}
			g_slist_free(attrlist);
			
			/* Put the node into a buffer in order to get it as a string */
			bufptr = xmlBufferCreate();
			if(!bufptr) {
				osync_trace(TRACE_INTERNAL, "hash_xml_node: unable to create buffer");
				return NULL;
			}
			xmlNodeDump(bufptr, doc, dupnode, 0, 0);
			const char *bufstr = xmlBufferContent(bufptr);
			/* Calculate the hash */
			t_hash = hash_str(bufstr);
			xmlBufferFree(bufptr);
			
			xmlFreeDoc(dummydoc);
		}
		else
			t_hash = hash_str("");
	}

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

char *opie_xml_strip_uid(const char *ext_uid) {
	if(g_str_has_prefix(ext_uid, "uid-")) {
		const char *uidptr = ext_uid;
		GString *uid = g_string_new("-"); 
		int innum = 0;
		while(*uidptr != 0) {
			if(g_ascii_isdigit(*uidptr)) {
				g_string_append_c(uid, *uidptr);
				innum = 1;
			}
			else if(innum)
				break;
			uidptr++;
		}
		
		char *uidstr = g_strdup(uid->str);
		g_string_free(uid, TRUE);
		
		if(strlen(uidstr) < 6 || atoi(uidstr+1) > 1999999999) {
			g_free(uidstr);
			return NULL;
		}
		else
			return uidstr;
	}
	else
		return NULL;
}

char *opie_xml_set_ext_uid(xmlNode *node, xmlDoc *doc, const char *listelement,
																				const char *itemelement, const char *tagged_uid) {
	char *uid = opie_xml_strip_uid(tagged_uid);	
	if(!uid)
		uid = opie_xml_generate_uid(doc, listelement, itemelement);
	opie_xml_set_uid(node, uid);
	return uid;
}

char *opie_xml_generate_uid(xmlDoc *doc, const char *listelement, const char *itemelement) {
	/* Generate a random uid that hasn't already been used */
	char *uid = g_malloc(16);
	do {
		sprintf(uid, "-%d", g_random_int_range(100, 1999999999));
	} while(opie_xml_find_by_uid(doc, listelement, itemelement, uid));
	return uid;
}

char *opie_xml_tag_uid(const char *uid, const char *nodename) {
	char *uidtag = "uid-unknown-%32s";
	if(!strcasecmp(nodename, "Contact")) {
		uidtag = "uid-contact-%s";
	}
	else if(!strcasecmp(nodename, "Task")) {
		uidtag = "uid-todo-%s";
	}
	else if(!strcasecmp(nodename, "event")) {
		uidtag = "uid-event-%s";
	}
	else if(!strcasecmp(nodename, "note")) {
		uidtag = "%s";
	}
	
	return g_strdup_printf(uidtag, uid);
}

char *opie_xml_get_tagged_uid(xmlNode *node) {
	char *uid = opie_xml_get_uid(node);
	if(uid) {
		char *tagged_uid = opie_xml_tag_uid(uid, node->name);
		xmlFree(uid);
		return tagged_uid;
	}
	else {
		return NULL;
	}
}

char *opie_xml_get_uidattr(xmlNode *node) {
	char *uidattr;
	
	if(!strcasecmp(node->name, "event")) {
		uidattr = "uid";
	}
	else if(!strcasecmp(node->name, "note")) {
		/* Notes don't have a UID on the Opie side, but the name should be unique */
		uidattr = "name";
	}
	else if(!strcasecmp(node->name, "Category")) {
		uidattr = "id";
	}
	else {
		uidattr = "Uid";
	}
	return uidattr;
}

char *opie_xml_get_uid(xmlNode *node) {
	char *uidattr = opie_xml_get_uidattr(node);
	return xmlGetProp(node, uidattr);
}

void opie_xml_set_uid(xmlNode *node, const char *uid) {
	char *uidattr = opie_xml_get_uidattr(node);
	xmlSetProp(node, uidattr, uid);
}

int opie_xml_save_to_fd(xmlDoc *doc, int fd) {
	if (ftruncate(fd, 0) == -1) {
		perror("ftruncate");
	}

	xmlOutputBuffer *buf = xmlOutputBufferCreateFd(fd, NULL);
	/* Prevent the fd from being closed after writing */
	buf->closecallback = NULL;
	/* Write the XML */
	int bytes = xmlSaveFormatFileTo(buf, doc, NULL, 1);
	return bytes;
}

char *opie_xml_category_name_to_id(xmlDoc *categories_doc, xmlNode *categories_node, const char *name) {
	xmlNode *category_node = categories_node->xmlChildrenNode;
	int count = 0;
	
	while(category_node && strcmp("Category", category_node->name))
		category_node = category_node->next;
	
	char *category_id = NULL;
	while(category_node) {
		char *cname = xmlGetProp(category_node, "name");
		if(cname) {
			if(!strcasecmp(name, cname)) {
				char *cid =  xmlGetProp(category_node, "id");
				if(cid) {
					category_id = g_strdup(cid);
					xmlFree(cid);
				}
				break;
			} 
			xmlFree(cname);
		}
		category_node = opie_xml_get_next(category_node);
		count++;
	}
	
	if(!category_id) {
		/* Need to add a new category */
		xmlNode *new_node = xmlNewNode(NULL, "Category");
		category_id = opie_xml_generate_uid(categories_doc, "Categories", "Category");
		
		if(!new_node) {
			osync_trace(TRACE_INTERNAL, "Unable to create new category node");
			return NULL;
		}
		xmlSetProp(new_node, "id", category_id);
		xmlSetProp(new_node, "name", name);
		if(!xmlAddChild(categories_node, new_node)) {
			osync_trace(TRACE_INTERNAL, "Unable to add category node node to document");
			xmlFreeNode(new_node);
			return NULL;
		}
		/* Flag categories document as modified */
		categories_doc->_private = 0;
	}
	
	return category_id;
}

void opie_xml_category_ids_to_names(xmlDoc *categories_doc, xmlNode *change_node) {
	int i;
	
	char *attr_value = opie_xml_get_categories(change_node);
	if(attr_value) {
		GString *cat_names = g_string_new(""); 
		gchar **categories = g_strsplit(attr_value, ";", 0);
		xmlNode *category_node = opie_xml_get_first(categories_doc, "Categories", "Category");
		while(category_node) {
			char *cid = xmlGetProp(category_node, "id");
			if(cid) {
				for(i=0; categories[i] != NULL; i++) {
					if(!strcmp(cid, categories[i])) {
						char *cname = xmlGetProp(category_node, "name");
						if(cname) {
							g_string_append_printf(cat_names, "%s|", cname);
							xmlFree(cname);
							break;
						}
					}
				}
				xmlFree(cid);
			}
			category_node = opie_xml_get_next(category_node);
		}
		
		if(cat_names->len > 0)
			g_string_truncate(cat_names, cat_names->len - 1);
		opie_xml_set_categories(change_node, cat_names->str);
		
		g_strfreev(categories);
		g_string_free(cat_names, TRUE); 
		xmlFree(attr_value);
	}
}

void opie_xml_category_names_to_ids(xmlDoc *categories_doc, xmlNode *change_node) {
	int i;
	
	char *attr_value = opie_xml_get_categories(change_node);
	if(attr_value) {
		xmlNode *categories_node = opie_xml_get_collection(categories_doc, "Categories");
		
		GString *cat_ids = g_string_new("");
		gchar **categories = g_strsplit(attr_value, "|", 0);
		for(i=0; categories[i] != NULL; i++) {
			char *cid = opie_xml_category_name_to_id(categories_doc, categories_node, categories[i]);
			if(cid) {
				g_string_append_printf(cat_ids, "%s;", cid);
				g_free(cid);
			}
		}
		
		if(cat_ids->len > 0)
			g_string_truncate(cat_ids, cat_ids->len - 1);
		opie_xml_set_categories(change_node, cat_ids->str);
		
		g_strfreev(categories);
		g_string_free(cat_ids, TRUE); 
		xmlFree(attr_value);
	}
}

char *opie_xml_get_categories(xmlNode *item_node) {
	char *attr_name;
	if(!strcasecmp(item_node->name, "event")) {
		attr_name = "categories";
	}
	else {
		attr_name = "Categories";
	}
	
	char *value = xmlGetProp(item_node, attr_name);
	if(value) {
		char *rvalue = g_strdup(value);
		xmlFree(value);
		return rvalue;
	}
	else {
		return NULL;
	}
}

void opie_xml_set_categories(xmlNode *item_node, const char *value) {
	char *attr_name;
	if(!strcasecmp(item_node->name, "event")) {
		attr_name = "categories";
	}
	else {
		attr_name = "Categories";
	}
	
	xmlSetProp(item_node, attr_name, value);
}

xmlDoc *opie_xml_create_contacts_doc(void) {

	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	
	xmlNode *root = xmlNewNode(NULL, "Addressbook");
	xmlDocSetRootElement(doc, root);
	xmlNode *cur = xmlNewNode(NULL, "Contacts");
	xmlAddChild(root, cur);
	
	return doc;
}

xmlDoc *opie_xml_create_todos_doc(void) {
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	
	xmlNode *root = xmlNewNode(NULL, "Tasks");
	xmlDocSetRootElement(doc, root);
	
	return doc;
}

xmlDoc *opie_xml_create_calendar_doc(void) {
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	
	xmlNode *root = xmlNewNode(NULL, "DATEBOOK");
	xmlDocSetRootElement(doc, root);
	xmlNode *cur = xmlNewNode(NULL, "events");
	xmlAddChild(root, cur);
	
	return doc;
}

xmlDoc *opie_xml_create_categories_doc(void) {
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	
	xmlNode *root = xmlNewNode(NULL, "Categories");
	xmlDocSetRootElement(doc, root);
	
	return doc;
}

xmlDoc *opie_xml_create_notes_doc(void) {
	xmlDoc *doc = xmlNewDoc((xmlChar*)"1.0");
	if(!doc) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	
	xmlNode *root = xmlNewNode(NULL, "notes");
	xmlDocSetRootElement(doc, root);
	
	return doc;
}

xmlDoc *opie_xml_create_doc(OPIE_OBJECT_TYPE objtype) {
	switch(objtype) {
		case OPIE_OBJECT_TYPE_CONTACT:
			return opie_xml_create_contacts_doc();
		case OPIE_OBJECT_TYPE_TODO:
			return opie_xml_create_todos_doc();
		case OPIE_OBJECT_TYPE_EVENT:
			return opie_xml_create_calendar_doc();
		case OPIE_OBJECT_TYPE_NOTE:
			return opie_xml_create_notes_doc();
		case OPIE_OBJECT_TYPE_CATEGORY:
			return opie_xml_create_categories_doc();
		default:
			osync_trace(TRACE_INTERNAL, "opie_xml_create_doc: unknown object format");
			return NULL;
	}
	/* Does not handle categories, since they aren't an objtype */
}

xmlNode *opie_xml_add_note_node(xmlDoc *doc, char *name, char *direntry, char *content) {
	xmlNode *notes_node = opie_xml_get_collection(doc, "notes");
	if(!notes_node) {
		osync_trace(TRACE_INTERNAL, "Unable to create new XML document");
		return NULL;
	}
	xmlNode *note_node = xmlNewTextChild(notes_node, NULL, (xmlChar*)"note", NULL);
	
/* FIXME this needs implementing
		
	char *month = NULL;
	char *time = NULL;
	int year = 0;
	
	// Compress out extra whitespace
	GString *buffer = g_string_new("");
	char *ptr;
	int spacecount = 0;
	int fieldcount = 0;
	for(ptr=direntry; *ptr != 0; ptr++) {
		if(g_ascii_isspace(ptr)) {
			if(spacecount == 0) {
				if(fieldcount == 5)
					
				g_string_truncate(buffer, 0);
				fieldcount++;
			}
			spacecount++;
		}
		else {
			spacecount = 0;
			g_string_append_c(buffer, *ptr);
		}
	}
*/	
	xmlSetProp(note_node, (xmlChar*)"name", name);
	xmlNewTextChild(note_node, NULL, (xmlChar*)"content", content);
	
	return note_node;
}
