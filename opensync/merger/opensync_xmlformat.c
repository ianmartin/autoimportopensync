#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

#include <libxml/xmlschemas.h>

/**
 * @defgroup OSyncXMLFormatAPI OpenSync XMLFormat
 * @ingroup OSyncPublic
 * @brief The public part of the OSyncXMLFormat
 * 
 */
/*@{*/

OSyncXMLFormat *osync_xmlformat_new(const char *objtype)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, objtype);
	
	g_assert(objtype);
	
	OSyncXMLFormat *xmlformat = g_malloc0(sizeof(OSyncXMLFormat));
	xmlformat->doc = xmlNewDoc(BAD_CAST "1.0");
	xmlformat->doc->children = xmlNewDocNode(xmlformat->doc, NULL, BAD_CAST objtype, NULL);
	xmlformat->refcount = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->doc->_private = xmlformat;
		
	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, buffer);
	
	g_assert(buffer);

	OSyncXMLFormat *xmlformat = g_malloc0(sizeof(OSyncXMLFormat));
	xmlformat->refcount = 1;
	xmlformat->doc = xmlReadMemory(buffer, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if(xmlformat->doc == NULL)
	{
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse XML.");
		g_free(xmlformat);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
	}
	xmlformat->refcount = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->doc->_private = xmlformat;
	
	xmlNodePtr cur = xmlDocGetRootElement(xmlformat->doc);
	cur = cur->children;
	while (cur != NULL) {
		_osync_xmlfield_new(xmlformat, cur);
		cur = cur->next;
	}
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

void osync_xmlformat_ref(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	xmlformat->refcount++;
}

void osync_xmlformat_unref(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	
	xmlformat->refcount--;
	if(xmlformat->refcount <= 0)
	{
		OSyncXMLField *cur, *tmp;
		cur = xmlformat->first_child;
		while(cur != NULL)
		{
			tmp = osync_xmlfield_get_next(cur);
			osync_xmlfield_free(cur);
			cur = tmp;
		}
		xmlFreeDoc(xmlformat->doc);
		g_free(xmlformat);
	}
}

const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	return (const char *)xmlDocGetRootElement(xmlformat->doc)->name;
}

void osync_xmlformat_sort(OSyncXMLFormat *xmlformat)
{
	int index;
//	xmlNodePtr root;
	OSyncXMLField *cur;
	
	g_assert(xmlformat);
	
//	root = xmlDocGetRootElement(xmlformat->doc);
//	count = 0;
//	cur = osync_xmlformat_get_first_field(xmlformat);
//	while(cur != NULL)
//	{
//		count++;
//		cur = osync_xmlfield_get_next(cur);
//	}

	if(xmlformat->child_count <= 1)
		return;
	
	void **liste = malloc(sizeof(OSyncXMLField *) * xmlformat->child_count);
	
	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	while(cur != NULL)
	{
		liste[index] = cur;
		index++;
		xmlUnlinkNode(cur->node);
		cur = osync_xmlfield_get_next(cur);
	}
	
	//osync_algorithm_quicksort(liste, 0, count-1, _osync_xmlfield_get_sortname);
	qsort(liste, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compaire);
	/** bring the xmlformat and the xmldoc in a consistent state */
	xmlformat->first_child = ((OSyncXMLField *)liste[0])->node->_private;
	xmlformat->last_child = ((OSyncXMLField *)liste[xmlformat->child_count-1])->node->_private;

	index = 0;
	while(index < xmlformat->child_count)
	{
		cur = (OSyncXMLField *)liste[index];
		xmlAddChild(xmlDocGetRootElement(xmlformat->doc), cur->node);
			
		if(index < xmlformat->child_count-1)
			cur->next = (OSyncXMLField *)liste[index+1];
		else
			cur->next = NULL;
		
		if(index)
			cur->prev = (OSyncXMLField *)liste[index-1];
		else
			cur->prev = NULL;
		
		index++;
	}
	
	free(liste);
}

OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	return xmlformat->first_child;
}

OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, ...)
{
	int index;
//	xmlNodePtr root;
	OSyncXMLField *cur, *key, *res;
	
	g_assert(xmlformat);
	
//	root = xmlDocGetRootElement(xmlformat->doc);
//	count = 0;
//	cur = osync_xmlformat_get_first_field(xmlformat);
//	while(cur != NULL)
//	{
//		count++;
//		cur = osync_xmlfield_get_next(cur);
//	}
	
	void **liste = malloc(sizeof(OSyncXMLField *) * xmlformat->child_count);

	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	while(cur != NULL)
	{
		liste[index] = cur;
		index++;
		cur = osync_xmlfield_get_next(cur);
	}
	
	key = g_malloc0(sizeof(OSyncXMLField));
	key->node = xmlNewNode(NULL, BAD_CAST name);
	res = *(OSyncXMLField **)bsearch(&key, liste, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compaire);
	
	free(liste);

	cur = res;
	while(cur->prev != NULL && !strcmp(osync_xmlfield_get_name(cur->prev), name))
	{
		cur = cur->prev;
	}
	
	OSyncXMLFieldList *xmlfieldlist = osync_xmlfieldlist_new();
	osync_xmlfieldlist_add(xmlfieldlist, cur);

	while(cur->next != NULL && !strcmp(osync_xmlfield_get_name(cur->next), name))
	{
		va_list args;
		const char *attr, *value;
		va_start(args, name);
		do {
			attr = va_arg(args, char *);
			value = va_arg(args, char *);
			if(!strcmp(value, osync_xmlfield_get_attr(cur, attr)))
				osync_xmlfieldlist_add(xmlfieldlist, cur->next);
			
		}while(key != 0 || value != 0);
		va_end(args);

		cur = cur->next;
	}
	
	

	return xmlfieldlist;
}

osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, int *size)
{
	g_assert(xmlformat);
	g_assert(buffer);
	g_assert(size);
	//g_assert(error);
	
	xmlDocDumpFormatMemoryEnc(xmlformat->doc, (xmlChar **) buffer, size, NULL, 1);
	return TRUE;	
}

void osync_xmlformat_merging(OSyncXMLFormat *xmlfield, OSyncCapabilities *capabilities, OSyncXMLFormat *original)
{
	OSyncXMLField *old_cur, *new_cur, *tmp;
	OSyncCapability *cap_contenttype;
	OSyncCapability *cap_cur;
	int ret;
	
	g_assert(xmlfield);
	g_assert(original);
	g_assert(capabilities);
	
	 /* get the right capabilites for the content type */
	 cap_contenttype = osync_capabilities_get_first(capabilities);
	 while(cap_contenttype != NULL)
	 {
	 	if(strcmp(osync_xmlformat_get_objtype(xmlfield), osync_capability_get_name(cap_contenttype)) == 0)
	 	{
	 		break;	
	 	}
	 }
	 if(cap_contenttype == NULL)
	 	return;	
		 
	 /* compairision */
	 cap_cur = osync_capability_get_first_child(cap_contenttype);
	 new_cur = osync_xmlformat_get_first_field(xmlfield);
	 old_cur = osync_xmlformat_get_first_field(original);
	 while(old_cur != NULL)
	 {
	 	ret = strcmp(osync_xmlfield_get_name(new_cur), osync_xmlfield_get_name(old_cur));
	 	if(ret < 0)
	 	{
	 		if(new_cur->next != NULL)
			{
				new_cur = osync_xmlfield_get_next(new_cur);
				continue;
	 		}
		 }
		 
	 	if(cap_cur == NULL)
	 	{
			tmp = old_cur;
			old_cur = osync_xmlfield_get_next(old_cur);
			osync_xmlfield_unlink(tmp);
			if(ret >= 0)
			{
				osync_xmlfield_link_before_field(new_cur, tmp);
			} else {
				osync_xmlfield_link_after_field(new_cur, tmp);
			}
			continue;		 		
	 	}

 		ret = strcmp(osync_capability_get_name(cap_cur), osync_xmlfield_get_name(old_cur));
	 	if(ret == 0)
	 	{
	 		old_cur = osync_xmlfield_get_next(old_cur);
			continue;		 		
	 	}
	 	else if(ret < 0)
	 	{
	 		cap_cur = osync_capability_get_next(cap_cur);
	 		continue;
	 	}
	 	else if(ret > 0)
	 	{
			tmp = old_cur;
			old_cur = osync_xmlfield_get_next(old_cur);
			osync_xmlfield_unlink(tmp);
			osync_xmlfield_link_before_field(new_cur, tmp);
	 		continue;
	 	}
		g_assert_not_reached();
	 }
}

osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat)
{
	int res;
 	xmlSchemaValidCtxtPtr xsdCtxt;
 	xmlSchemaPtr wxschemas;
 	xmlSchemaParserCtxtPtr ctxt;
	
 	ctxt = xmlSchemaNewParserCtxt("../opensync/merger/xmlformat.xsd");
 	wxschemas = xmlSchemaParse(ctxt);

 	xsdCtxt = xmlSchemaNewValidCtxt(wxschemas);
 	if (xsdCtxt == NULL) {
 		xmlSchemaFreeValidCtxt(xsdCtxt);
 		xmlSchemaFreeParserCtxt(ctxt);
   		return (FALSE);
 	}

 	/* Validate the document */
 	res = xmlSchemaValidateDoc(xsdCtxt, xmlformat->doc);
 	xmlSchemaFreeValidCtxt(xsdCtxt);
 	xmlSchemaFreeParserCtxt(ctxt);
 	if(res == 0)
 		return TRUE;
 	else
 		return FALSE;
}

//OSyncConvCmpResult osync_xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints *xmlscores, int default_value, int treshold)
//{
//	OSyncXMLField *cur_left, *cur_right, *tmp;
//	int score_ptr;
//	int res_score;
//	int value;
//	
//	cur_left = osync_xmlformat_get_first_field(xmlformat1);
//	cur_right = osync_xmlformat_get_first_field(xmlformat2);
//	score_ptr = 0;
//	res_score = 0;
//	
//	
//	while(cur_left)
//	{
//		char *fieldname = osync_xmlfield_get_name(cur_left);
//		
//		SyncXMLFieldList *list_left = osync_xmlfieldlist_new();		
//		while(!strcmp(osync_xmlfield_(fieldname, osync_xmlfield_get_name(cur_lefright))))
//			osync_xmlfieldlist_add(list_right, cur_right);
//			cur_right = osync_xmlfield_get_next(cur_right);
//			continue;
//		}
//		
//		SyncXMLFieldList *list_right = osync_xmlfieldlist_new();		
//		while(!strcmp(osync_xmlfield_(fieldname, osync_xmlfield_get_name(cur_right))))
//			osync_xmlfieldlist_add(list_right, cur_right);
//			cur_right = osync_xmlfield_get_next(cur_right);
//			continue;
//		}
//		
//		
//		int res = strcmp(osync_xmlfield_(fieldname, xmlscores[score_ptr].path));
//		do{
//			if(res = 0)
//			{
//				value = xmlscores[score_ptr].value;
//				break;
//			}else(res < 0) {
//				value = default_value;
//				break;
//			}else(res > 0) {
//				score_ptr += 1;
//			}
//		}while(1)
//		
//		int i, j, found;
//		for(i=0; i< osync_xmlfieldlist_getLength(list_left); i++)
//		{
//			for (j = 0; j < osync_xmlfieldlist_getLength(list_right); j++) {
//				b = FALSE;
//				if (osync_xmlfield_compaire(osync_xmlfieldlist_item(list_left, i),
//											osync_xmlfieldlist_item(list_right, j)))
//				{
//					found = TRUE;
//					osync_xmlfield_remove(list_left, j);
//					res_score += value;
//				}
//			}
//			if(!found) {
//				res_score += value;
//			}
//		
//		}
//	}
//	
//	while(cur_right)
//	{
//		
//		cur_right = osync_xmlfield_get_next(cur_right);
//	}
//	
//	osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", res_score, treshold);
//	if (same) {
//		osync_trace(TRACE_EXIT, "%s: SAME", __func__);
//		return CONV_DATA_SAME;
//	}
//	if (res_score >= treshold) {
//		osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
//		return CONV_DATA_SIMILAR;
//	}
//	osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
//	return CONV_DATA_MISMATCH;
//}

//OSyncConvCmpResult osxml_compare(xmlDoc *leftinpdoc, xmlDoc *rightinpdoc, OSyncXMLScore *scores, int default_score, int treshold)
//{
//	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p)", __func__, leftinpdoc, rightinpdoc, scores);
//	int z = 0, i = 0, n = 0;
//	int res_score = 0;
//	
//	xmlDoc *leftdoc = xmlCopyDoc(leftinpdoc, TRUE);
//	xmlDoc *rightdoc = xmlCopyDoc(rightinpdoc, TRUE);
//	
//	osync_trace(TRACE_INTERNAL, "Comparing given score list");
//	while (scores && scores[z].path) {
//		OSyncXMLScore *score = &scores[z];
//		z++;
//		xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, score->path);
//		xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, score->path);
//		
//		xmlNodeSet *lnodes = leftxobj->nodesetval;
//		xmlNodeSet *rnodes = rightxobj->nodesetval;
//		
//		int lsize = (lnodes) ? lnodes->nodeNr : 0;
//		int rsize = (rnod es) ? rnodes->nodeNr : 0;
//		osync_trace(TRACE_INTERNAL, "parsing next path %s", score->path);
//		
//		if (!score->value) {
//			for (i = 0; i < lsize; i++) {
//				xmlUnlinkNode(lnodes->nodeTab[i]);
//				xmlFreeNode(lnodes->nodeTab[i]);
//				lnodes->nodeTab[i] = NULL;
//			}
//			
//			for (n = 0; n < rsize; n++) {
//				xmlUnlinkNode(rnodes->nodeTab[n]);
//				xmlFreeNode(rnodes->nodeTab[n]);
//				rnodes->nodeTab[n] = NULL;
//			}
//		} else {
//			for (i = 0; i < lsize; i++) {
//				if (!lnodes->nodeTab[i])
//					continue;
//				for (n = 0; n < rsize; n++) {
//					if (!rnodes->nodeTab[n])
//						continue;
//					osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i], "Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));
//					if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
//						osync_trace(TRACE_INTERNAL, "Adding %i for %s", score->value, score->path);
//						res_score += score->value;
//						xmlUnlinkNode(lnodes->nodeTab[i]);
//						xmlFreeNode(lnodes->nodeTab[i]);
//						lnodes->nodeTab[i] = NULL;
//						xmlUnlinkNode(rnodes->nodeTab[n]);
//						xmlFreeNode(rnodes->nodeTab[n]);
//						rnodes->nodeTab[n] = NULL;
//						goto next;
//					}
//				}
//				osync_trace(TRACE_INTERNAL, "Subtracting %i for %s", score->value, score->path);
//				res_score -= score->value;
//				next:;
//			}
//			for(i = 0; i < rsize; i++) {
//				if (!rnodes->nodeTab[i])
//					continue;
//				res_score -= score->value;
//			}
//		}
//		
//		xmlXPathFreeObject(leftxobj);
//		xmlXPathFreeObject(rightxobj);
//	}
//	
//	xmlXPathObject *leftxobj = osxml_get_nodeset(leftdoc, "/*/*");
//	xmlXPathObject *rightxobj = osxml_get_nodeset(rightdoc, "/*/*");
//	
//	xmlNodeSet *lnodes = leftxobj->nodesetval;
//	xmlNodeSet *rnodes = rightxobj->nodesetval;
//	
//	int lsize = (lnodes) ? lnodes->nodeNr : 0;
//	int rsize = (rnodes) ? rnodes->nodeNr : 0;
//	
//	osync_trace(TRACE_INTERNAL, "Comparing remaining list");
//	osync_bool same = TRUE;
//	for(i = 0; i < lsize; i++) {		
//		for (n = 0; n < rsize; n++) {
//			if (!rnodes->nodeTab[n])
//				continue;
//			osync_trace(TRACE_INTERNAL, "cmp %i:%s (%s), %i:%s (%s)", i, lnodes->nodeTab[i]->name, osxml_find_node(lnodes->nodeTab[i], "Content"), n, rnodes->nodeTab[n]->name, osxml_find_node(rnodes->nodeTab[n], "Content"));
//			if (osxml_compare_node(lnodes->nodeTab[i], rnodes->nodeTab[n])) {
//				xmlUnlinkNode(lnodes->nodeTab[i]);
//				xmlFreeNode(lnodes->nodeTab[i]);
//				lnodes->nodeTab[i] = NULL;
//				xmlUnlinkNode(rnodes->nodeTab[n]);
//				xmlFreeNode(rnodes->nodeTab[n]);
//				rnodes->nodeTab[n] = NULL;
//				osync_trace(TRACE_INTERNAL, "Adding %i", default_score);
//				res_score += default_score;
//				goto next2;
//			}
//		}
//		osync_trace(TRACE_INTERNAL, "Subtracting %i", default_score);
//		res_score -= default_score;
//		same = FALSE;
//		//goto out;
//		next2:;
//	}
//	
//	for(i = 0; i < lsize; i++) {
//		if (!lnodes->nodeTab[i])
//			continue;
//		osync_trace(TRACE_INTERNAL, "left remaining: %s", lnodes->nodeTab[i]->name);
//		same = FALSE;
//		goto out;
//	}
//	
//	for(i = 0; i < rsize; i++) {
//		if (!rnodes->nodeTab[i])
//			continue;
//		osync_trace(TRACE_INTERNAL, "right remaining: %s", rnodes->nodeTab[i]->name);
//		same = FALSE;
//		goto out;
//	}
//	out:
//	xmlXPathFreeObject(leftxobj);
//	xmlXPathFreeObject(rightxobj);
//
//	xmlFreeDoc(leftdoc);
//	xmlFreeDoc(rightdoc);
//
//	osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", res_score, treshold);
//	if (same) {
//		osync_trace(TRACE_EXIT, "%s: SAME", __func__);
//		return CONV_DATA_SAME;
//	}
//	if (res_score >= treshold) {
//		osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
//		return CONV_DATA_SIMILAR;
//	}
//	osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
//	return CONV_DATA_MISMATCH;
//}


//osync_boo l osync_xmlformat_read_xml(OSyncXMLFormat *xmlformat, const char *path, OSyncError **error)
//{
//	xmlFreeDoc(xmlformat->doc);
//	xmlformat->doc = NULL;
//	xmlNodePtr cur = NULL;
//
//	//if (!_osync_open_xml_file(&(xmlformat->doc), &cur, path, "xmlformat", error))
//	//{
//	//		return FALSE;
//	//}
//	xmlformat->doc = xmlReadFile(path, NULL, XML_PARSE_NOBLANKS);
//	xmlformat->doc->_private = xmlformat;
//	cur = xmlDocGetRootElement(xmlformat->doc);
//	cur = cur->children;
//	while (cur != NULL) {
//		_osync_xmlfield_new(xmlformat, cur);
//		cur = cur->next;
//	}
//	return TRUE;
//}

/*@}*/
