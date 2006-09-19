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
	doc->children = xmlNewDocNode(doc, NULL, (xmlChar*)name, NULL);
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
		xmlNodeSetName(node, (xmlChar*)name); //FIXME Free previous name?
		
	if (data)
		xmlNewTextChild(node, NULL, (xmlChar*)"Content", (xmlChar*)data);
}

xmlNode *osxml_node_add(xmlNode *parent, const char *name, const char *data)
{
	if (!data)
		return NULL;
	if (strlen(data) == 0)
		return NULL;
	xmlNode *node = xmlNewTextChild(parent, NULL, (xmlChar*)name, (xmlChar*)data);
	return node;
}

void osxml_node_add_property(xmlNode *parent, const char *name, const char *data)
{
	xmlNewProp(parent, (xmlChar*)name, (xmlChar*)data);
}

void osxml_node_mark_unknown(xmlNode *parent)
{
	if (!xmlHasProp(parent, (xmlChar*)"Type"))
		xmlNewProp(parent, (xmlChar*)"Type", (xmlChar*)"Unknown");
}

void osxml_node_remove_unknown_mark(xmlNode *node)
{
	xmlAttr *attr = xmlHasProp(node, (xmlChar*)"Type");
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
	return (char*)xmlNodeGetContent(osxml_get_node(parent, name));
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
    xpathObj = xmlXPathEvalExpression((xmlChar*)expression, xpathCtx);
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
		return (strcmp((char*)xmlGetProp(parent, (xmlChar*)name), data) == 0);
	return FALSE;
}

char *osxml_find_property(xmlNode *parent, const char *name)
{
	return (char*)xmlGetProp(parent, (xmlChar*)name);
}

osync_bool osxml_has_property(xmlNode *parent, const char *name)
{
	return (xmlHasProp(parent, (xmlChar*)name) != NULL);
}

static osync_bool osxml_compare_node(xmlNode *leftnode, xmlNode *rightnode)
{
	osync_trace(TRACE_ENTRY, "%s(%p:%s, %p:%s)", __func__, leftnode, leftnode->name, rightnode, rightnode->name);

	char *left, *right;

	if (strcmp((char*)leftnode->name, (char*)rightnode->name)) {
		osync_trace(TRACE_EXIT, "%s: FALSE: Different Name", __func__);
		return FALSE;
	}
	
	leftnode = leftnode->children;
	rightnode = rightnode->children;
	xmlNode *rightstartnode = rightnode;
	
	if (!leftnode && !rightnode) {
		osync_trace(TRACE_EXIT, "%s: TRUE. Both 0", __func__);
		return TRUE;
	}
	
	if (!leftnode || !rightnode) {
		osync_trace(TRACE_EXIT, "%s: FALSE. One 0", __func__);
		return FALSE;
	}
	
	do {
		if (!strcmp("UnknownParam", (char*)leftnode->name))
			continue;
		if (!strcmp("Order", (char*)leftnode->name))
			continue;
		rightnode = rightstartnode;
		char *leftcontent = (char*)xmlNodeGetContent(leftnode);
		
		do {
			if (!strcmp("UnknownParam", (char*)rightnode->name))
				continue;
			char *rightcontent = (char*)xmlNodeGetContent(rightnode);

			osync_trace(TRACE_INTERNAL, "leftnode %s, rightnode %s", leftnode->name, rightnode->name);
			osync_trace(TRACE_SENSITIVE, "leftcontent %s, rightcontent %s\n", leftcontent, rightcontent);
			
			if (leftcontent == rightcontent) {
				g_free(rightcontent);
				goto next;
			}

			/* We compare the striped content to work around bugs in
			 * applications like evo2 which always strip the content
			 * and would therefore cause conflicts. This change should not break
			 * anything since it does not touch the actual content */			
			char *strip_right = g_strstrip(g_strdup(rightcontent));
			char *strip_left = g_strstrip(g_strdup(leftcontent));
			if (!strcmp(strip_left, strip_right)) {
				g_free(strip_right);
				g_free(strip_left);
				g_free(rightcontent);
				goto next;
			}
			g_free(strip_right);
			g_free(strip_left);

			if (!leftcontent || !rightcontent) {
				osync_trace(TRACE_EXIT, "%s: One is empty", __func__);
				return FALSE;
			}

			/* Workaround for palm-sync. palm-sync is not able to set a correct Completed date-timestamp so ignore value */ 
			if (!strcmp("Completed", (char*)rightnode->name) && !strcmp("Completed",(char*)leftnode->name)) {
				if ((leftcontent && rightcontent) || (!leftcontent && !rightcontent)) {
					osync_trace(TRACE_INTERNAL, "PALM-SYNC workaround active!");
					g_free(rightcontent);
					goto next;
				}
			}

			/* Workaround for kdepim-sync. kdepim-sync always creates a AlarmTrigger with DISPLAY also when DESCRIPTION is empty */ 
			if (!strcmp("Alarm", (char*)rightnode->name) && !strcmp("Alarm",(char*)leftnode->name)) {

				left = leftcontent;
				right = rightcontent;

				if (strstr(leftcontent, "DISPLAY"))
					left += 7;

				if (strstr(rightcontent, "DISPLAY"))
					right += 7; 

				osync_trace(TRACE_SENSITIVE, "Alarm Display Workaround - left: %s right: %s", left, right);
				if (!strcmp(left, right)) {
					osync_trace(TRACE_INTERNAL, "KDEPIM-SYNC (ALARM trigger) workaround active!");
					g_free(rightcontent);
					goto next;
				}
			}

			/* Workaround for comparing UTC and localtime stamps. This will be replaced ASAP (beep!) when task #331 is done */
			if ((!strcmp("DateStarted", (char*)rightnode->name) && !strcmp("DateStarted", (char*)leftnode->name))
					|| (!strcmp("DateEnd", (char*)rightnode->name) && !strcmp("DateEnd", (char*)leftnode->name))) {

				int ret = 0;

				if (osync_time_isutc(leftcontent)) {
					left = osync_time_vtime2localtime(leftcontent);
				} else {
					left = g_strdup(leftcontent);
				}

				if (osync_time_isutc(rightcontent)) {
					right = osync_time_vtime2localtime(rightcontent);
				} else {
					right = g_strdup(rightcontent);
				}

				osync_trace(TRACE_SENSITIVE, "utc2localtime conversion - left: %s right: %s", left, right);

				ret = strcmp(left, right);

				g_free(left);
				g_free(right);

				if (!ret) {
					osync_trace(TRACE_INTERNAL, "utc2localtime workaround active!");
					g_free(rightcontent);
					goto next;
				}
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
					osync_trace(TRACE_INTERNAL, "cmp %i:%s (leftcontent), %i:%s (rightcontent)", i, lnodes->nodeTab[i]->name,  
							n, rnodes->nodeTab[n]->name);
					osync_trace(TRACE_SENSITIVE, "cmp %i:%s (%s), %i:%s (%s)\n", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i], 
							"Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));

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

	// if nodeTab[0] is an Event or Todo we need a new node structure (/*/*/*)
	if ((!strcmp((char*)lnodes->nodeTab[0]->name, "Event") && \
		!strcmp((char*)rnodes->nodeTab[0]->name, "Event")) || \
	    (!strcmp((char*)lnodes->nodeTab[0]->name, "Todo") && \
		!strcmp((char*)rnodes->nodeTab[0]->name, "Todo"))) {

		xmlXPathFreeObject(leftxobj);
		xmlXPathFreeObject(rightxobj);

		leftxobj = osxml_get_nodeset(leftdoc, "/*/*/*");
		rightxobj = osxml_get_nodeset(rightdoc, "/*/*/*");

		lnodes = leftxobj->nodesetval;
		rnodes = rightxobj->nodesetval;

	}

	int lsize = (lnodes) ? lnodes->nodeNr : 0;
	int rsize = (rnodes) ? rnodes->nodeNr : 0;
	
	osync_trace(TRACE_INTERNAL, "Comparing remaining list");
	osync_bool same = TRUE;
	for(i = 0; i < lsize; i++) {		
		for (n = 0; n < rsize; n++) {
			if (!rnodes->nodeTab[n])
				continue;
			osync_trace(TRACE_INTERNAL, "cmp %i:%s (leftcontent), %i:%s (rightcontent)", i, lnodes->nodeTab[i]->name,  
							n, rnodes->nodeTab[n]->name);
			osync_trace(TRACE_SENSITIVE, "cmp %i:%s (%s), %i:%s (%s)\n", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i],
					"Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));

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

	xmlFreeDoc(leftdoc);
	xmlFreeDoc(rightdoc);

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

xmlChar *osxml_write_to_string(xmlDoc *doc)
{
	xmlKeepBlanksDefault(0);
	xmlChar *temp = NULL;
	int size = 0;
	xmlDocDumpFormatMemoryEnc(doc, &temp, &size, NULL, 1);
	return temp;
}

osync_bool osxml_copy(const char *input, int inpsize, char **output, int *outpsize)
{
	xmlDoc *doc = (xmlDoc *)(input);
	xmlDoc *newdoc = xmlCopyDoc(doc, TRUE);
	*output = (char *)newdoc;
	*outpsize = sizeof(newdoc);
	return TRUE;
}

osync_bool osxml_marshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	xmlDoc *doc = (xmlDoc*)input;
	xmlChar *result;
	int size;
	xmlDocDumpMemory(doc, &result, &size);
	*output = (char*)result;
	*outpsize = size;
	return TRUE;
}

osync_bool osxml_demarshall(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	xmlDoc *doc = xmlParseMemory(input, inpsize);
	if (!doc) {
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Invalid XML data received");
		goto error;
	}

	*output = (char*)doc;
	*outpsize = sizeof(*doc);
	return TRUE;

error:
	return FALSE;
}


