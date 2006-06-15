#include "opensync.h"
#include "opensync_internals.h"

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
	OSyncXMLField *cur;
	
	g_assert(xmlformat);
	
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
