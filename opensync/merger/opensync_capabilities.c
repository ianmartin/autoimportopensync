#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"


OSyncCapabilities *osync_capabilities_new(void)
{
	osync_trace(TRACE_ENTRY, "%s()", __func__);
	
	OSyncCapabilities *capabilities = g_malloc0(sizeof(OSyncCapabilities));
	capabilities->refcount = 1;
	capabilities->doc = xmlNewDoc((xmlChar*)"1.0");
	capabilities->doc->children = xmlNewDocNode(capabilities->doc, NULL, (xmlChar*)"capabilities", NULL);
	capabilities->first_child = NULL;
	capabilities->last_child = NULL;
		
	osync_trace(TRACE_EXIT, "%s: %p", __func__, capabilities);
	return capabilities;
}

void osync_capabilities_free(OSyncCapabilities *capabilities)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, capabilities);
	g_assert(capabilities);
	
	osync_capability_free(capabilities->first_child);
	xmlFreeDoc(capabilities->doc);
	g_free(capabilities);
	
	osync_trace(TRACE_EXIT, "%s");
}

void osync_capabilities_ref(OSyncCapabilities *capabilities)
{
	g_assert(capabilities);
	capabilities->refcount++;
}

void osync_capabilities_unref(OSyncCapabilities *capabilities)
{
	g_assert(capabilities);
	capabilities->refcount--;
	if(capabilities->refcount <= 0)
		osync_capabilities_free(capabilities);
}

osync_bool osync_capabilities_read_xml(OSyncCapabilities *capabilities, const char *path, OSyncError **error)
{
	xmlFreeDoc(capabilities->doc);
	capabilities->doc = NULL;
	xmlNodePtr cur = NULL;
	
	capabilities->doc = xmlReadFile(path, NULL, XML_PARSE_NOBLANKS);

	cur = xmlDocGetRootElement(capabilities->doc);
	cur = cur->children;
	while (cur != NULL && cur->type == XML_ELEMENT_NODE) {
		_osync_capability_new_content_type(capabilities, cur);
		cur = cur->next;
	}
	return TRUE;
}

void osync_capabilities_write_xml(OSyncCapabilities *capabilities, const char *path, OSyncError **error)
{
	//xmlChar *str = osxml_write_to_string(capabilities->doc->doc);
	//osync_file_write(path, (const char *)str, sizeof(str), 0660, NULL);
}

OSyncCapability *osync_capabilities_get_first(OSyncCapabilities *capabilities)
{
	g_assert(capabilities);
	return capabilities->first_child;
}	

void osync_algorithm_quicksort(void * array[], int left, int right, const char *(*getString)(void *))
{
  register int i, j;
  void *x, *temp;

  i = left; j = right;
  x = array[(left+right)/2];

  do {
    while((strcmp(getString(array[i]), getString(x)) < 0) && (i < right)) i++;
    while((strcmp(getString(array[j]), getString(x)) > 0) && (j > left)) j--;
    if(i <= j) {
      temp = array[i];
      array[i] = array[j];
      array[j] = temp;
      i++; j--;
   }
  } while(i <= j);

  if(left < j) osync_algorithm_quicksort(array, left, j, getString);
  if(i < right) osync_algorithm_quicksort(array, i, right, getString);
}

void osync_capabilities_sort(OSyncCapabilities *capabilities)
{
	int count, index;
	OSyncCapability *contenttype, *cur;
	
	contenttype = osync_capabilities_get_first(capabilities);
	while(contenttype != NULL)
	{
		count = 0;
		cur = osync_capability_get_first_child(contenttype);
		while(cur != NULL)
		{
			count++;
			cur = osync_capability_get_next(cur);
		}
		
		if(count <= 1)
			return;
	
		void **liste = malloc(sizeof(xmlNodePtr) * count);
	
		index = 0;
		cur = osync_capability_get_first_child(contenttype);
		while(cur != NULL)
		{
			liste[index] = cur;
			index++;
			xmlUnlinkNode(cur->node);
			cur = osync_capability_get_next(cur);
		}
	
		osync_algorithm_quicksort(liste, 0, count-1, _osync_capability_get_sortname);
	
		/** bring the capabilities and xmldoc in a consistent state */
		contenttype->first_child = ((OSyncCapability *)liste[0])->node->_private;
		contenttype->last_child = ((OSyncCapability *)liste[count-1])->node->_private;

		index = 0;
		while(index < count)
		{
			cur = (OSyncCapability *)liste[index];
			xmlAddChild(contenttype->node, cur->node);
			
			if(index < count-1)
				cur->next = (OSyncCapability *)liste[index+1];
			else
				cur->next = NULL;
			
			index++;
		}
		contenttype = osync_capability_get_next(contenttype);
	}
}
