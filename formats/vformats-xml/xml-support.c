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
 
#include "xml-support.h"
#include <glib.h>

static osync_bool osxml_compare_time(xmlNode *leftnode, xmlNode *rightnode) {

	osync_trace(TRACE_ENTRY, "%s(%s(%p), %s(%p))", __func__, leftnode->name, leftnode, rightnode->name, rightnode);
	int ret = 0;
	char *left = NULL, *right = NULL;
	char *leftcontent = osxml_find_node(leftnode, "Content");
	char *rightcontent = osxml_find_node(rightnode, "Content");
	
	osync_trace(TRACE_SENSITIVE, "time compare - left: %s right: %s", leftcontent, rightcontent);


	if (!osync_time_isutc(leftcontent))
		left = osync_time_tzlocal2utc(leftnode, (char *) leftnode->name); 

	if (!left)
		left = g_strdup(leftcontent);

	if (!osync_time_isutc(rightcontent))
		right = osync_time_tzlocal2utc(rightnode, (char *) rightnode->name); 

	if (!right)
		right = g_strdup(rightcontent);

	g_free(leftcontent);
	g_free(rightcontent);

	osync_trace(TRACE_SENSITIVE, "AFTER convert - left: %s right: %s", left, right);

	ret = strcmp(left, right);

	g_free(left);
	g_free(right);

	if (ret) {
		osync_trace(TRACE_EXIT, "%s: FALSE", __func__);
		return FALSE;
	}

	osync_trace(TRACE_EXIT, "%s: TRUE", __func__);
	return TRUE; 

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

			/* Workaround for palm-sync. palm-sync is not able to set a correct Completed date-timestamp so ignore value (objtype: todo) */ 
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

			g_free(rightcontent);

			if ((!strcmp("DateStarted", (char*)rightnode->name) && !strcmp("DateStarted", (char*)leftnode->name))
				|| (!strcmp("DateEnd", (char*)rightnode->name) && !strcmp("DateEnd", (char*)leftnode->name))) {
				
				if (osxml_compare_time(leftnode, rightnode))
					goto next;
			}

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

