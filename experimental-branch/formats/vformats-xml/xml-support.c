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
 
#include "opensync-xml.h"
#include <glib.h>

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
	if (!data)
		return NULL;
	if (strlen(data) == 0)
		return NULL;
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

void osxml_node_remove_unknown_mark(xmlNode *node)
{
	xmlAttr *attr = xmlHasProp(node, "Type");
	if (!attr)
		return;
	xmlRemoveProp(attr);
}

xmlNode *osxml_get_node(xmlNode *parent, const char *name)
{
	xmlNode *cur = (parent)->xmlChildrenNode;
	while (cur) {
		if (!xmlStrcmp(cur->name, (const xmlChar *)name))
			return cur;
		cur = cur->next;
	}
	return NULL;
}

char *osxml_find_node(xmlNode *parent, const char *name)
{
	return xmlNodeGetContent(osxml_get_node(parent, name));
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

void osxml_map_unknown_param(xmlNode *node, const char *paramname, const char *newname)
{
	xmlNode *cur = node->xmlChildrenNode;
	while (cur) {
		if (xmlStrcmp(cur->name, (const xmlChar *)"UnknownParam"))
			goto next;
		
		char *name = osxml_find_node(cur, "ParamName");
		char *content = osxml_find_node(cur, "Content");
		if (!strcmp(name, paramname)) {
			osxml_node_add(node, newname, content);
			osxml_node_remove_unknown_mark(node);
			
			xmlUnlinkNode(cur);
			xmlFreeNode(cur);
			g_free(name);
			g_free(content);
			return;
		}
		
		g_free(name);
		g_free(content);
			
		next:;
		cur = cur->next;
	}
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
	osync_trace(TRACE_ENTRY, "%s(%p:%s, %p:%s)", __func__, leftnode, leftnode->name, rightnode, rightnode->name);
	if (strcmp(leftnode->name, rightnode->name)) {
		osync_trace(TRACE_EXIT, "%s: FALSE: Different Name", __func__);
		return FALSE;
	}
	
	leftnode = leftnode->children;
	rightnode = rightnode->children;
	xmlNode *rightstartnode = rightnode;
	do {
		if (!strcmp("UnknownParam", leftnode->name))
			continue;
		if (!strcmp("Order", leftnode->name))
			continue;
		rightnode = rightstartnode;
		char *leftcontent = xmlNodeGetContent(leftnode);
		
		do {
			if (!strcmp("UnknownParam", rightnode->name))
				continue;
			char *rightcontent = xmlNodeGetContent(rightnode);
			
			osync_trace(TRACE_INTERNAL, "leftcontent %s (%s), rightcontent %s (%s)", leftcontent, leftnode->name, rightcontent, rightnode->name);
			if (leftcontent == rightcontent) {
				g_free(rightcontent);
				goto next;
			}
			if (!strcmp(leftcontent, rightcontent)) {
				g_free(rightcontent);
				goto next;
			}
			if (!leftcontent || !rightcontent) {
				osync_trace(TRACE_EXIT, "%s: One is empty", __func__);
				return FALSE;
			}
			g_free(rightcontent);
		} while ((rightnode = rightnode->next));
		osync_trace(TRACE_EXIT, "%s: Could not match one", __func__);
		g_free(leftcontent);
		return FALSE;
		next:;
		g_free(leftcontent);
	} while ((leftnode = leftnode->next));
	
	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE;
}

OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, leftinpdoc, rightinpdoc, scores);
	int z = 0, i = 0, n = 0;
	int res_score = 0;
	
	xmlDoc *leftdoc = xmlCopyDoc(leftinpdoc, TRUE);
	xmlDoc *rightdoc = xmlCopyDoc(rightinpdoc, TRUE);
	
	osync_trace(TRACE_INTERNAL, "Comparing given score list");
	while (scores && scores[z].path) {
		OSyncXMLScore *score = &scores[z];
		z++;
		xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, score->path);
		xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, score->path);
		
		xmlNodeSet *lnodes = leftxobj->nodesetval;
		xmlNodeSet *rnodes = rightxobj->nodesetval;
		
		int lsize = (lnodes) ? lnodes->nodeNr : 0;
		int rsize = (rnodes) ? rnodes->nodeNr : 0;
		osync_trace(TRACE_INTERNAL, "parsing next path %s", score->path);
		
		if (!score->value) {
			for (i = 0; i < lsize; i++) {
				xmlUnlinkNode(lnodes->nodeTab[i]);
				xmlFreeNode(lnodes->nodeTab[i]);
				lnodes->nodeTab[i] = NULL;
			}
			
			for (n = 0; n < rsize; n++) {
				xmlUnlinkNode(rnodes->nodeTab[n]);
				xmlFreeNode(rnodes->nodeTab[n]);
				rnodes->nodeTab[n] = NULL;
			}
		} else {
			for (i = 0; i < lsize; i++) {
				if (!lnodes->nodeTab[i])
					continue;
				for (n = 0; n < rsize; n++) {
					if (!rnodes->nodeTab[n])
						continue;
					osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i], "Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));
					if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
						osync_trace(TRACE_INTERNAL, "Adding %i for %s", score->value, score->path);
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
				osync_trace(TRACE_INTERNAL, "Subtracting %i for %s", score->value, score->path);
				res_score -= score->value;
				next:;
			}
			for(i = 0; i < rsize; i++) {
				if (!rnodes->nodeTab[i])
					continue;
				res_score -= score->value;
			}
		}
		
		xmlXPathFreeObject(leftxobj);
		xmlXPathFreeObject(rightxobj);
	}
	
	xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, "/*/*");
	xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, "/*/*");
	
	xmlNodeSet *lnodes = leftxobj->nodesetval;
	xmlNodeSet *rnodes = rightxobj->nodesetval;
	
	int lsize = (lnodes) ? lnodes->nodeNr : 0;
	int rsize = (rnodes) ? rnodes->nodeNr : 0;
	
	osync_trace(TRACE_INTERNAL, "Comparing remaining list");
	osync_bool same = TRUE;
	for(i = 0; i < lsize; i++) {		
		for (n = 0; n < rsize; n++) {
			if (!rnodes->nodeTab[n])
				continue;
			osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i], "Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));
			if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
				xmlUnlinkNode(lnodes->nodeTab[i]);
				xmlFreeNode(lnodes->nodeTab[i]);
				lnodes->nodeTab[i] = NULL;
				xmlUnlinkNode(rnodes->nodeTab[n]);
				xmlFreeNode(rnodes->nodeTab[n]);
				rnodes->nodeTab[n] = NULL;
				osync_trace(TRACE_INTERNAL, "Adding %i", default_score);
				res_score += default_score;
				goto next2;
			}
		}
		osync_trace(TRACE_INTERNAL, "Subtracting %i", default_score);
		res_score -= default_score;
		same = FALSE;
		//goto out;
		next2:;
	}
	
	for(i = 0; i < lsize; i++) {
		if (!lnodes->nodeTab[i])
			continue;
		osync_trace(TRACE_INTERNAL, "left remaining: %s", lnodes->nodeTab[i]->name);
		same = FALSE;
		goto out;
	}
	
	for(i = 0; i < rsize; i++) {
		if (!rnodes->nodeTab[i])
			continue;
		osync_trace(TRACE_INTERNAL, "right remaining: %s", rnodes->nodeTab[i]->name);
		same = FALSE;
		goto out;
	}
	out:
	xmlXPathFreeObject(leftxobj);
	xmlXPathFreeObject(rightxobj);

	osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", res_score, treshold);
	if (same) {
		osync_trace(TRACE_EXIT, "%s: SAME", __func__);
		return CONV_DATA_SAME;
	}
	if (res_score >= treshold) {
		osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
		return CONV_DATA_SIMILAR;
	}
	osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
	return CONV_DATA_MISMATCH;
}

char *osxml_write_to_string(xmlDoc *doc)
{
	xmlKeepBlanksDefault(0);
	xmlChar *temp = NULL;
	int size = 0;
	xmlDocDumpFormatMemoryEnc(doc, &temp, &size, NULL, 1);
	return (char *)temp;
}

osync_bool osxml_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	xmlDoc *doc = (xmlDoc *)(input);
	xmlDoc *newdoc = xmlCopyDoc(doc, TRUE);
	*output = (char *)newdoc;
	*outpsize = sizeof(newdoc);
	return TRUE;
}
