/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  Daniel Friedrich <daniel.friedrich@opensync.org>
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
 
#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

/**
 * @defgroup OSyncXMLFormatPrivateAPI OpenSync XMLFormat Internals
 * @ingroup OSyncPrivate
 * @brief The private part of the OSyncXMLFormat
 * 
 */
/*@{*/

int _osync_xmlformat_get_points(OSyncXMLPoints points[], int* cur_pos, int basic_points, const char* fieldname)
{
	while(points[(*cur_pos)].fieldname) {
		int res = strcmp(points[(*cur_pos)].fieldname, fieldname);
		if(res == 0) 
			return points[(*cur_pos)].points;
		if(res > 0)
			return basic_points;
		(*cur_pos)++;
	};
	return basic_points;
}

/*@}*/

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

OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	return xmlformat->first_child;
}

OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, ...)
{
	int index;
	OSyncXMLField *cur, *key, *res;
	
	g_assert(xmlformat);
	
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
	res = *(OSyncXMLField **)bsearch(&key, liste, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compare_stdlib);
	
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
	xmlDocDumpFormatMemoryEnc(xmlformat->doc, (xmlChar **) buffer, size, NULL, 1);
	return TRUE;	
}

osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat)
{
	g_assert(xmlformat);
	
	int res;
 	xmlSchemaParserCtxtPtr xmlSchemaParserCtxt;
 	xmlSchemaPtr xmlSchema;
 	xmlSchemaValidCtxtPtr xmlSchemaValidCtxt;
	
 	xmlSchemaParserCtxt = xmlSchemaNewParserCtxt("xmlformat.xsd");
 	xmlSchema = xmlSchemaParse(xmlSchemaParserCtxt);
 	xmlSchemaFreeParserCtxt(xmlSchemaParserCtxt);

 	xmlSchemaValidCtxt = xmlSchemaNewValidCtxt(xmlSchema);
 	if (xmlSchemaValidCtxt == NULL) {
 		xmlSchemaFree(xmlSchema);
   		res = 1;
 	}else{
 		/* Validate the document */
 		res = xmlSchemaValidateDoc(xmlSchemaValidCtxt, xmlformat->doc);
	 	xmlSchemaFree(xmlSchema);
		xmlSchemaFreeValidCtxt(xmlSchemaValidCtxt);
 	}

	if(res == 0)
 		return TRUE;
 	else
 		return FALSE;
}

void osync_xmlformat_sort(OSyncXMLFormat *xmlformat)
{
	int index;
	OSyncXMLField *cur;
	
	g_assert(xmlformat);

	if(xmlformat->child_count <= 1)
		return;
	
	void **list = malloc(sizeof(OSyncXMLField *) * xmlformat->child_count);
	
	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	while(cur != NULL)
	{
		list[index] = cur;
		index++;
		xmlUnlinkNode(cur->node);
		cur = osync_xmlfield_get_next(cur);
	}
	
	qsort(list, xmlformat->child_count, sizeof(OSyncXMLField *), osync_xmlfield_compare_stdlib);
	
	/** bring the xmlformat and the xmldoc in a consistent state */
	xmlformat->first_child = ((OSyncXMLField *)list[0])->node->_private;
	xmlformat->last_child = ((OSyncXMLField *)list[xmlformat->child_count-1])->node->_private;

	index = 0;
	while(index < xmlformat->child_count)
	{
		cur = (OSyncXMLField *)list[index];
		xmlAddChild(xmlDocGetRootElement(xmlformat->doc), cur->node);
			
		if(index < xmlformat->child_count-1)
			cur->next = (OSyncXMLField *)list[index+1];
		else
			cur->next = NULL;
		
		if(index)
			cur->prev = (OSyncXMLField *)list[index-1];
		else
			cur->prev = NULL;
		
		index++;
	}
	
	free(list);
}

void osync_xmlformat_merging(OSyncXMLFormat *xmlformat, OSyncCapabilities *capabilities, OSyncXMLFormat *original)
{
	OSyncXMLField *old_cur, *new_cur, *tmp;
	OSyncCapability *cap_cur;
	int ret;
	
	g_assert(xmlformat);
	g_assert(original);
	g_assert(capabilities);
	
	 /* compairision */
	 cap_cur = osync_capabilities_get_first(capabilities, osync_xmlformat_get_objtype(xmlformat));
	 if(!cap_cur)
	 	return;
	 	
	 new_cur = osync_xmlformat_get_first_field(xmlformat);
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
			if(ret >= 0) {
				osync_xmlfield_link_before_field(new_cur, tmp);
			} else {
				osync_xmlfield_link_after_field(new_cur, tmp);
			}
			continue;		 		
	 	}

 		ret = strcmp(osync_capability_get_name(cap_cur), osync_xmlfield_get_name(old_cur));
	 	if(ret == 0)
	 	{
	 		/* merge key/value pairs (second level)*/
	 		printf("%s %s\n",osync_capability_get_name(cap_cur), osync_xmlfield_get_name(new_cur));
	 		if( osync_capability_has_key(cap_cur) &&
	 			strcmp(osync_capability_get_name(cap_cur), osync_xmlfield_get_name(new_cur)) == 0) 
	 		{
		 		int r, s;
	 			xmlNodePtr tmp;
				xmlNodePtr cap_node = cap_cur->node->children;
				xmlNodePtr new_node = new_cur->node->children;
				xmlNodePtr old_node = old_cur->node->children;
				xmlNodePtr new_par_node = new_cur->node;
				
				while(old_node)
	 			{
	 				r = 0;
	 				while(cap_node != NULL) {
		 				r = strcmp((const char *) cap_node->name, (const char *)  old_node->name);
	 					if(r < 0)
 							cap_node = cap_node->next;
 						if(r >= 0)
 							break;
	 				}
	 				
	 				/* merge all avalible nodes */
	 				if(cap_node == NULL) {
	 					while(old_node != NULL) {
							tmp = old_node;
		 					old_node = old_node->next;
		 					xmlUnlinkNode(tmp);
		 					xmlAddChild(new_par_node, tmp);	 						
	 					}
	 					break;
	 				}

	 				/* merge node */
	 				if(r > 0) {
		 				tmp = old_node;
		 				old_node = old_node->next;
		 				xmlUnlinkNode(tmp);
		 				if(!new_node) {
		 					new_node = xmlAddChild(new_par_node, tmp);
	 					}else{
		 					do {
		 						s = strcmp((const char *) new_node->name, (const char *) cap_node->name);
		 						if(s <= 0 && new_node->next) {
		 							new_node = new_node->next;
		 							continue;
		 						}else if(s <= 0 && !new_node->next) {
		 							new_node = xmlAddChild(new_par_node, tmp);
		 							break;
		 						}else if(s > 0) {
		 							xmlAddPrevSibling(new_node, tmp);
		 							break;
		 						}
		 						g_assert_not_reached(); 						
		 					}while(1);
		 				}
		 				if(!old_node)
			 				break;
	 				}
	 				
	 				if(r == 0) {
	 					old_node = old_node->next;
	 					continue;
	 				}
	 				
	 				g_assert_not_reached();
	 			}
	 		}
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

OSyncConvCmpResult osync_xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints points[], int basic_points, int treshold)
{
	int res, collected_points, cur_pos;
	OSyncXMLField *xmlfield1 = osync_xmlformat_get_first_field(xmlformat1);
	OSyncXMLField *xmlfield2 = osync_xmlformat_get_first_field(xmlformat2);
	cur_pos = 0;
	collected_points = 0;
	osync_bool same = TRUE;
	
	while(xmlfield1 != NULL && xmlfield2 != NULL)
	{
		/* subtract points for xmlfield2*/
		if(xmlfield1 == NULL) {
			same = FALSE;
			collected_points -= _osync_xmlformat_get_points(points, &cur_pos, basic_points, (const char *) xmlfield2->node->name);
			xmlfield2 = xmlfield2->next;
			continue;	
		}
		
		/* subtract points for xmlfield1*/
		if(xmlfield2 == NULL) {
			same = FALSE;
			collected_points -= _osync_xmlformat_get_points(points, &cur_pos, basic_points, (const char *) xmlfield1->node->name);
			xmlfield1 = xmlfield1->next;
			continue;
		}
		
		res = strcmp((const char *)xmlfield1->node->name, (const char *)xmlfield2->node->name);
		
		/* subtract points for xmlfield1*/
		if(res < 0) {
			same = FALSE;
			collected_points -= _osync_xmlformat_get_points(points, &cur_pos, basic_points, (const char *) xmlfield1->node->name);
			xmlfield1 = xmlfield1->next;
			continue;		
		}
		
		/* subtract points for xmlfield2*/
		if(res > 0) {
			same = FALSE;
			collected_points -= _osync_xmlformat_get_points(points, &cur_pos, basic_points, (const char *) xmlfield2->node->name);			
			xmlfield2 = xmlfield2->next;
			continue;			
		}
		
		/* make lists and compare */
		if(res == 0)
		{
			GSList *fieldlist1;
			GSList *fieldlist2;
			fieldlist1 = NULL;
			fieldlist2 = NULL;
			
			const char *curfieldname = (const char *)xmlfield1->node->name;
			do {
				fieldlist1 = g_slist_prepend(fieldlist1, xmlfield1);
				xmlfield1 = xmlfield1->next;
				if(xmlfield1 == NULL)
					break;
				res = strcmp((const char *)xmlfield1->node->name, curfieldname);
			} while(res == 0);
			
			do {
				fieldlist2 = g_slist_prepend(fieldlist2, xmlfield2);
				xmlfield2 = xmlfield2->next;
				if(xmlfield2 == NULL)
					break;
				res = strcmp((const char *)xmlfield2->node->name, curfieldname);
			} while(res == 0);			
			
			do {
				/* get the points*/
				int p = _osync_xmlformat_get_points(points, &cur_pos, basic_points, curfieldname);
				
				/* if same then compare and give points*/
				do {
					if(!same)
						break;
						
					/* both lists must have the same length */	
					if(g_slist_length(fieldlist1) != g_slist_length(fieldlist2)) {
						same = FALSE;
						break;
					}
					
					GSList *cur_list1;
					GSList *cur_list2;
					do {
						cur_list1 = fieldlist1;
						cur_list2 = fieldlist2;

						do {
							if(osync_xmlfield_compare((OSyncXMLField *)cur_list1->data, (OSyncXMLField *)cur_list2->data) == TRUE)
								break;
							cur_list2 = g_slist_next(cur_list2);
							if(cur_list2 == NULL) {
								same = FALSE;	
								break;
							}
						}while(1);
						
						if(same) {
							/* add the points */
							collected_points += p;
							fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
							fieldlist2 = g_slist_delete_link(fieldlist2, cur_list2);
						}else
							break;							

					}while(fieldlist1 != NULL);
				} while(0);
				
				/* if similar then compare and give points*/
				do {
					if(same)
						break;
					/* if no points to add or to subtract we need no compair of similarity */
					if(!p) {
						g_slist_free(fieldlist1);
						g_slist_free(fieldlist2);
						fieldlist1 = NULL;
						fieldlist2 = NULL;
						break;
					}
					
					GSList *cur_list1;
					GSList *cur_list2;
					osync_bool found;
					int subtracted_count;
					subtracted_count = 0;
					do {
						found = FALSE;
						cur_list1 = fieldlist1;
						cur_list2 = fieldlist2;
						
						do {
							if(osync_xmlfield_compare_similar(	(OSyncXMLField *)cur_list1->data,
																(OSyncXMLField *)cur_list2->data,
																points[cur_pos].keys) == TRUE) {
								found = TRUE;	
								break;
							}
							cur_list2 = g_slist_next(cur_list2);
							if(cur_list2 == NULL)
								break;
						}while(1);
						
						/* add or subtract the points */
						if(found) {
							/* add the points */
							collected_points += p;
							fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
							fieldlist2 = g_slist_delete_link(fieldlist2, cur_list2);
						}else{
							/* subtract the points */
							collected_points -= p;
							subtracted_count++;
							fieldlist1 = g_slist_delete_link(fieldlist1, cur_list1);
						}
					}while(fieldlist1 != NULL);
					
					/* subtract the points for the remaining elements in the list2 */
					while(fieldlist2) {
						/* subtract the points */
						if(subtracted_count > 0)
							subtracted_count--;
						else
							collected_points -= p;
						fieldlist2 = g_slist_delete_link(fieldlist2, fieldlist2);
					}
	
				}while(0);
				
			}while(0);

			/* the lists should not exist */
			g_assert(!fieldlist1);
			g_assert(!fieldlist2);
		}
	};
	
printf("collected points: %i\n", collected_points);
	osync_trace(TRACE_INTERNAL, "Result is: %i, Treshold is: %i", collected_points, treshold);
	if (same) {
		osync_trace(TRACE_EXIT, "%s: SAME", __func__);
		return OSYNC_CONV_DATA_SAME;
	}
	if (collected_points >= treshold) {
		osync_trace(TRACE_EXIT, "%s: SIMILAR", __func__);
		return OSYNC_CONV_DATA_SIMILAR;
	}
	osync_trace(TRACE_EXIT, "%s: MISMATCH", __func__);
	return OSYNC_CONV_DATA_MISMATCH;
}

/*@}*/
