/*
 * libopensync - A synchronization framework
 * Copyright (C) 2006  NetNix Finland Ltd <netnix@netnix.fi>
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
 * Author: Daniel Friedrich <daniel.friedrich@opensync.org>
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

/**
 * @brief Search for the points in the sorted OSyncXMLPoints array for a given fieldname.
 * @param points The sorted points array
 * @param cur_pos Pointer to the actual line in the points array
 * @param basic_points Points which should be returned if fieldname will not found in the points array 
 * @param fieldname The name of the field for which the points should be returned
 * @returns The points for the fieldname
 */
int _osync_xmlformat_get_points(OSyncXMLPoints points[], int* cur_pos, int basic_points, const char* fieldname)
{
	for(; points[(*cur_pos)].fieldname; (*cur_pos)++) {
		int res = strcmp(points[(*cur_pos)].fieldname, fieldname);
		if(res == 0) 
			return points[(*cur_pos)].points;
		if(res > 0)
			return basic_points;
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

/**
 * @brief Creates a new xmlformat object
 * @param objtype The name of the objtype (e.g.: contact)
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSyncXMLFormat *osync_xmlformat_new(const char *objtype, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p)", __func__, objtype, error);
	osync_assert(objtype);
	
	OSyncXMLFormat *xmlformat = osync_try_malloc0(sizeof(OSyncXMLFormat), error);
	if(!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	xmlformat->doc = xmlNewDoc(BAD_CAST "1.0");
	xmlformat->doc->children = xmlNewDocNode(xmlformat->doc, NULL, BAD_CAST objtype, NULL);
	xmlformat->ref_count = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->sorted = FALSE;
	xmlformat->doc->_private = xmlformat;
	
	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

/**
 * @brief Creates a new xmlformat object from a xml document. 
 * @param buffer The pointer to the xml document
 * @param size The size of the xml document
 * @param error The error which will hold the info in case of an error
 * @return The pointer to the newly allocated xmlformat object or NULL in case of error
 */
OSyncXMLFormat *osync_xmlformat_parse(const char *buffer, unsigned int size, OSyncError **error)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %i, %p)", __func__, buffer, size, error);
	osync_assert(buffer);

	OSyncXMLFormat *xmlformat = osync_try_malloc0(sizeof(OSyncXMLFormat), error);
	if(!xmlformat) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}
	
	xmlformat->doc = xmlReadMemory(buffer, size, NULL, NULL, XML_PARSE_NOBLANKS);
	if(!xmlformat->doc) {
		g_free(xmlformat);
		osync_error_set(error, OSYNC_ERROR_GENERIC, "Could not parse XML.");
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;	
	}

	xmlformat->ref_count = 1;
	xmlformat->first_child = NULL;
	xmlformat->last_child = NULL;
	xmlformat->child_count = 0;
	xmlformat->doc->_private = xmlformat;
		
	xmlNodePtr cur = xmlDocGetRootElement(xmlformat->doc);
	cur = cur->children;
	while (cur != NULL) {
		OSyncXMLField *xmlfield = _osync_xmlfield_new(xmlformat, cur, error);
		if(!xmlfield) {
			osync_xmlformat_unref(xmlformat);
			osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
			return NULL;
		}
		cur = cur->next;
	}

	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlformat);
	return xmlformat;
}

/**
 * @brief Increments the reference counter
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_xmlformat_ref(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	g_atomic_int_inc(&(xmlformat->ref_count));
}

/**
 * @brief Decrement the reference counter. The xmlformat object will 
 *  be freed if there is no more reference to it.
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_xmlformat_unref(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	if (g_atomic_int_dec_and_test(&(xmlformat->ref_count))) {
		OSyncXMLField *cur, *tmp;
		cur = xmlformat->first_child;
		while(cur != NULL)
		{
			tmp = osync_xmlfield_get_next(cur);
			osync_xmlfield_delete(cur);
			cur = tmp;
		}
		xmlFreeDoc(xmlformat->doc);
		g_free(xmlformat);
	}
}

/**
 * @brief Get the objtype of a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The objtype of the xmlformat
 */
const char *osync_xmlformat_get_objtype(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	return (const char *)xmlDocGetRootElement(xmlformat->doc)->name;
}

/**
 * @brief Get the first field of a xmlformat
 * @param xmlformat The pointer to a xmlformat object
 * @return The first field of the xmlformat
 */
OSyncXMLField *osync_xmlformat_get_first_field(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	return xmlformat->first_child;
}

/**
 * @brief Serarch for xmlfields in the given xmlformat. It's up to the caller to
 *  free the returned list with OSyncXMLFieldList::osync_xmlfieldlist_free
 * @param xmlformat The pointer to a xmlformat object 
 * @param name The name of the xmlfields to search for
 * @param error The error which will hold the info in case of an error
 * @param ... If the xmlfield should have a attribute with spezial value,
 *  then it is possible to specify the attribute name and the attribute 
 *  value. But always there have to set both parametes! There can be more 
 *  than one attribute pair. The last parameter has always to be NULL.
 * @return The Pointer to the xmlfieldlist which hold all founded xmlfields
 *  or NULL in case of error
 */
OSyncXMLFieldList *osync_xmlformat_search_field(OSyncXMLFormat *xmlformat, const char *name, OSyncError **error, ...)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %s, %p, ...)", __func__, xmlformat, name, error);
	osync_assert(xmlformat);
	osync_assert(name);
	
	int index;
	void *ret;
	OSyncXMLField *cur, *key, *res;

	/* Searching breaks if the xmlformat is not sorted (bsearch!)
	   FIXME: get rid of to many search */
	if (!xmlformat->sorted)
		osync_xmlformat_sort(xmlformat);

	OSyncXMLFieldList *xmlfieldlist = _osync_xmlfieldlist_new(error);
	if(!xmlfieldlist) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	void **liste = osync_try_malloc0(sizeof(OSyncXMLField *) * xmlformat->child_count, error);
	if(!liste) {
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	for(; cur != NULL; cur = osync_xmlfield_get_next(cur)) {
		liste[index] = cur;
		index++;
	}

	key = osync_try_malloc0(sizeof(OSyncXMLField), error);
	if(!key) {
		g_free(liste);
		osync_trace(TRACE_EXIT_ERROR, "%s: %s" , __func__, osync_error_print(error));
		return NULL;
	}

	key->node = xmlNewNode(NULL, BAD_CAST name);
	
	ret = bsearch(&key, liste, xmlformat->child_count, sizeof(OSyncXMLField *), _osync_xmlfield_compare_stdlib);

	/* no result - return empty xmlfieldlist */
	if (!ret)
		goto end;

	/* if ret is valid pointer (not NULL!) - reference it here. avoid segfaults */
	res = *(OSyncXMLField **) ret;

	/* we set the cur ptr to the first field from the fields with name name because -> bsearch -> more than one field with the same name*/
	for(cur = res; cur->prev != NULL && !strcmp(osync_xmlfield_get_name(cur->prev), name); cur = cur->prev) ;

	osync_bool all_attr_equal;
	for(; cur != NULL && !strcmp(osync_xmlfield_get_name(cur), name); cur = cur->next) {
		const char *attr, *value;
		all_attr_equal = TRUE;
		va_list args;
		va_start(args, error);
		do {
			attr = va_arg(args, char *);
			value = va_arg(args, char *);
			if(	attr == NULL || value == NULL)
				break;
			if(strcmp(value, osync_xmlfield_get_attr(cur, attr)) != 0)
				all_attr_equal = FALSE;
			
		}while(1);
		va_end(args);
		if(all_attr_equal)
			_osync_xmlfieldlist_add(xmlfieldlist, cur);
	}



end:	
	/* free lists here (later) - bsearch result is still pointing in liste array */
	xmlFreeNode(key->node);
	g_free(key);
	g_free(liste);

	osync_trace(TRACE_EXIT, "%s: %p", __func__, xmlfieldlist);
	return xmlfieldlist;
}

/**
 * @brief Dump the xmlformat into the memory.
 * @param xmlformat The pointer to a xmlformat object 
 * @param buffer The pointer to the buffer which will hold the xml document
 * @param size The pointer to the buffer which will hold the size of the xml document
 * @return The xml document and the size of it. It's up to the caller to free
 *  the buffer. Always it return TRUE.
 */
osync_bool osync_xmlformat_assemble(OSyncXMLFormat *xmlformat, char **buffer, unsigned int *size)
{
	osync_assert(xmlformat);
	osync_assert(buffer);
	osync_assert(size);
	
	xmlDocDumpFormatMemoryEnc(xmlformat->doc, (xmlChar **)buffer, (int *)size, NULL, 1);
	return TRUE;	
}

/**
 * @brief Validate the xmlformat against its schema
 * @param xmlformat The pointer to a xmlformat object 
 * @return TRUE if xmlformat valid else FALSE
 */
osync_bool osync_xmlformat_validate(OSyncXMLFormat *xmlformat)
{
	osync_assert(xmlformat);
	
	char *schemafilepath = g_strdup_printf("%s%c%s%s%s",
		OPENSYNC_SCHEMASDIR,
		G_DIR_SEPARATOR,
		"xmlformat-",
		osync_xmlformat_get_objtype(xmlformat),
		".xsd");
 	osync_bool res = osxml_validate_document(xmlformat->doc, schemafilepath);
 	g_free(schemafilepath);
	
	return res;
}

/**
 * @brief Sort all xmlfields of the xmlformat. This function has to
 *  be called after a xmlfield was added to the xmlformat.
 * @param xmlformat The pointer to a xmlformat object
 */
void osync_xmlformat_sort(OSyncXMLFormat *xmlformat)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, xmlformat);
	osync_assert(xmlformat);
	
	int index;
	OSyncXMLField *cur;
	
	if(xmlformat->child_count <= 1) {
		osync_trace(TRACE_INTERNAL, "child_count <= 1 - no need to sort");
		goto end;
	}
	
	void **list = g_malloc0(sizeof(OSyncXMLField *) * xmlformat->child_count);
	
	index = 0;
	cur = osync_xmlformat_get_first_field(xmlformat);
	for(; cur != NULL; cur = osync_xmlfield_get_next(cur)) {
		list[index] = cur;
		index++;
		xmlUnlinkNode(cur->node);
	}
	
	qsort(list, xmlformat->child_count, sizeof(OSyncXMLField *), _osync_xmlfield_compare_stdlib);
	
	/** bring the xmlformat and the xmldoc in a consistent state */
	xmlformat->first_child = ((OSyncXMLField *)list[0])->node->_private;
	xmlformat->last_child = ((OSyncXMLField *)list[xmlformat->child_count-1])->node->_private;

	for(index = 0; index < xmlformat->child_count; index++) {
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
	}
	g_free(list);

end:	
	xmlformat->sorted = TRUE;
	osync_trace(TRACE_EXIT, "%s", __func__);
}

/**
 * @brief Compares two xmlformat objects with each other
 * @param xmlformat1 The pointer to a xmlformat object
 * @param xmlformat2 The pointer to a xmlformat object
 * @param points The sorted points array
 * @param basic_points Points which should be used if a xmlfield name is not found in the points array
 * @param treshold If the two xmlformats are not the same, then this value will decide if the two xmlformats are similar 
 * @return One of the values of the OSyncConvCmpResult enumeration
 */
OSyncConvCmpResult osync_xmlformat_compare(OSyncXMLFormat *xmlformat1, OSyncXMLFormat *xmlformat2, OSyncXMLPoints points[], int basic_points, int treshold)
{
	osync_trace(TRACE_ENTRY, "%s(%p, %p, %p, %i, %i)", __func__, xmlformat1, xmlformat2, points, basic_points, treshold);

	int res, collected_points, cur_pos;
	OSyncXMLField *xmlfield1 = osync_xmlformat_get_first_field(xmlformat1);
	OSyncXMLField *xmlfield2 = osync_xmlformat_get_first_field(xmlformat2);
	cur_pos = 0;
	collected_points = 0;
	osync_bool same = TRUE;
	
	while(xmlfield1 != NULL || xmlfield2 != NULL)
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
		osync_trace(TRACE_INTERNAL, "result of strcmp(): %i (%s || %s)", res, xmlfield1->node->name, xmlfield2->node->name);
		
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
						osync_trace(TRACE_INTERNAL, "both list don't have the same length");
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
								osync_trace(TRACE_INTERNAL, "one field is alone: %s", osync_xmlfield_get_name(cur_list1->data));
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
						
						while(cur_list2) {

							if(osync_xmlfield_compare_similar(	(OSyncXMLField *)cur_list1->data,
																(OSyncXMLField *)cur_list2->data,
																points[cur_pos].keys) == TRUE) {
								found = TRUE;	
								break;
							}

							cur_list2 = g_slist_next(cur_list2);
						}
						
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
