/*
 * xml - A plugin for xml objects for the opensync framework
 * Copyright (C) 2004-2005  Armin Bauer <armin.bauer@opensync.org>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
 * 
 */
 
#include "xml.h"

xmlNode *osxml_node_add_root(xmlDoc *doc, const char *name)
{
	doc->children = xmlNewDocNode(doc, NULL, name, NULL);
	return doc->children;
}

xmlNode *osxml_node_get_root(xmlDoc *doc, const char *name, OSyncError **error)
{
	xmlNode *cur = xmlDocGetRootElement(doc);
	if (!cur) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Unable to get xml root element");
		return NULL;
	}
	
	if (xmlStrcmp((cur)->name, (const xmlChar *) name)) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Wrong xml root element");
		return NULL;
	}
	
	cur = (cur)->xmlChildrenNode;
	return cur;
}

void osxml_node_set(xmlNode *node, const char *name, const char *data, OSyncXMLEncoding encoding)
{
	if (name)
		xmlNodeSetName(node, name); //FIXME Free previous name?
		
	if (data)
		xmlNewTextChild(node, NULL, "Content", data);
}

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data)
{
	xmlNode *node = xmlNewTextChild(parent, NULL, name, data);
	return node;
}

void osxml_node_add_property(xmlNode *parent, const char *name, const char *data)
{
	xmlNewProp(parent, name, data);
}

void osxml_node_mark_unknown(xmlNode *parent)
{
	if (!xmlHasProp(parent, "Type"))
		xmlNewProp(parent, "Type", "Unknown");
}

char *osxml_find_node(xmlNode *parent, const char *name)
{
	xmlNode *cur = (parent)->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)name))
			return xmlNodeGetContent(cur);
		cur = cur->next;
	}
	return NULL;
}

xmlXPathObject *osxml_get_nodeset(xmlDoc *doc, const char *expression)
{
	xmlXPathContext *xpathCtx = NULL;
	xmlXPathObject *xpathObj = NULL;
    
    /* Create xpath evaluation context */
    xpathCtx = xmlXPathNewContext(doc);
    if(xpathCtx == NULL) {
        fprintf(stderr,"Error: unable to create new XPath context\n");
        return NULL;
    }
    
    /* Evaluate xpath expression */
    xpathObj = xmlXPathEvalExpression(expression, xpathCtx);
    if(xpathObj == NULL) {
        fprintf(stderr,"Error: unable to evaluate xpath expression \"%s\"\n", expression);
        xmlXPathFreeContext(xpathCtx); 
        return NULL;
    }

	xmlXPathFreeContext(xpathCtx);
	/* Cleanup of XPath data */
   // xmlXPathFreeObject(xpathObj);
   return xpathObj;
}

xmlXPathObject *osxml_get_unknown_nodes(xmlDoc *doc)
{
	return osxml_get_nodeset(doc, "/*/*[@Type='Unknown']");
}

osync_bool osxml_has_property_full(xmlNode *parent, const char *name, const char *data)
{
	if (osxml_has_property(parent, name))
		return (strcmp(xmlGetProp(parent, name), data) == 0);
	return FALSE;
}

char *osxml_find_property(xmlNode *parent, const char *name)
{
	return xmlGetProp(parent, name);
}

osync_bool osxml_has_property(xmlNode *parent, const char *name)
{
	return (xmlHasProp(parent, name) != NULL);
}

static osync_bool osxml_compare_node(xmlNode *leftnode, xmlNode *rightnode)
{
	char *leftcontent = xmlNodeGetContent(leftnode);
	char *rightcontent = xmlNodeGetContent(rightnode);
	if (leftcontent == rightcontent)
		return TRUE;
	if (!leftcontent || !rightcontent)
		return FALSE;
	osync_bool ret = (strcmp(leftcontent, rightcontent) == 0);
	g_free(leftcontent);
	g_free(rightcontent);
	return ret;
}

OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores)
{
	int z = 0, i = 0, n;
	xmlDoc *leftdoc = xmlCopyDoc(leftinpdoc, TRUE);
	xmlDoc *rightdoc = xmlCopyDoc(rightinpdoc, TRUE);
	
	int res_score = 0;
	while (scores && scores[z].path) {
		OSyncXMLScore *score = &scores[z];
		z++;
		xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, score->path);
		xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, score->path);
		
		xmlNodeSet *lnodes = leftxobj->nodesetval;
		xmlNodeSet *rnodes = rightxobj->nodesetval;
		
		int lsize = (lnodes) ? lnodes->nodeNr : 0;
		int rsize = (rnodes) ? rnodes->nodeNr : 0;
		
		for(i = 0; i < lsize; i++) {
			for (n = 0; n < rsize; n++) {
				if (!rnodes->nodeTab[n])
					continue;
				if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
					res_score += score->value;
					xmlUnlinkNode(lnodes->nodeTab[i]);
					xmlFreeNode(lnodes->nodeTab[i]);
					lnodes->nodeTab[i] = NULL;
					xmlUnlinkNode(rnodes->nodeTab[n]);
					xmlFreeNode(rnodes->nodeTab[n]);
					rnodes->nodeTab[n] = NULL;
					goto next;
				}
			}
			res_score -= score->value;
			next:;
		}
		for(i = 0; i < rsize; i++) {
			if (!rnodes->nodeTab[i])
				continue;
			res_score -= score->value;
		}
		xmlXPathFreeObject(leftxobj);
		xmlXPathFreeObject(rightxobj);
	}
	
	xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, "/contact/*");
	xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, "/contact/*");
	
	xmlNodeSet *lnodes = leftxobj->nodesetval;
	xmlNodeSet *rnodes = rightxobj->nodesetval;
	
	int lsize = (lnodes) ? lnodes->nodeNr : 0;
	int rsize = (rnodes) ? rnodes->nodeNr : 0;
	
	osync_bool same = TRUE;
	for(i = 0; i < lsize; i++) {		
		for (n = 0; n < rsize; n++) {
			if (!rnodes->nodeTab[n])
				continue;
			if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
				xmlUnlinkNode(lnodes->nodeTab[i]);
				xmlFreeNode(lnodes->nodeTab[i]);
				lnodes->nodeTab[i] = NULL;
				xmlUnlinkNode(rnodes->nodeTab[n]);
				xmlFreeNode(rnodes->nodeTab[n]);
				rnodes->nodeTab[n] = NULL;
				goto next2;
			}
		}
		same = FALSE;
		goto out;
		next2:;
	}
	for(i = 0; i < lsize; i++) {
		if (!lnodes->nodeTab[i])
			continue;
		same = FALSE;
		goto out;
	}
	for(i = 0; i < rsize; i++) {
		if (!rnodes->nodeTab[i])
			continue;
		same = FALSE;
		goto out;
	}
	out:
	xmlXPathFreeObject(leftxobj);
	xmlXPathFreeObject(rightxobj);
	/*
	
	for (lcur = lroot->children;lcur; lcur = lcur->next) {
		rcur = rroot->children;
		printf("%p\n", rcur);
		for (;rcur; rcur = rcur->next) {
			printf("comparing %s with %s\n", xmlNodeGetContent(lcur), xmlNodeGetContent(rcur));
			if (osxml_compare_node(lcur, rcur)) {
				xmlUnlinkNode(lcur);
				xmlFreeNode(lcur);
				xmlUnlinkNode(rcur);
				xmlFreeNode(rcur);
				goto next2;
			}
		}
		same = FALSE;
		break;
		next2:;
		lcur = lroot->children;
		printf("lcur %p\n", rcur);
	}
	if (lroot->children || rroot->children)
		same = FALSE;
	*/
	
	printf("end score %i\n", res_score);
	if (same)
		return CONV_DATA_SAME;
	if (res_score >= 5)
		return CONV_DATA_SIMILAR;
	return CONV_DATA_MISMATCH;
}
