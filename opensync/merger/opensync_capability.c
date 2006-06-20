#include "opensync.h"
#include "opensync_internals.h"

#include "opensync-merger.h"
#include "opensync-merger_internals.h"

OSyncCapability *osync_capability_new(OSyncCapability *capability, const char *name)
{
	xmlNodePtr node = xmlNewChild(capability->node, NULL, (xmlChar *)name, NULL);
	return _osync_capability_new(capability, node);
}

OSyncCapability *osync_capability_new_content_type(OSyncCapabilities *capabilities, const char *name)
{
	xmlNodePtr node = xmlNewChild(xmlDocGetRootElement(capabilities->doc), NULL, (xmlChar *)name, NULL);
	return _osync_capability_new_content_type(capabilities, node);
}

/** internal */
OSyncCapability *_osync_capability_new(OSyncCapability *capability, xmlNodePtr node)
{
	OSyncCapability *new_capability = g_malloc0(sizeof(OSyncCapability));
	
	if(!capability->first_child)
		capability->first_child = new_capability;
	if(capability->last_child)
		capability->last_child->next = new_capability;
	capability->last_child = new_capability;
	
	new_capability->first_child = NULL;
	new_capability->last_child = NULL;
	new_capability->next = NULL;
	new_capability->node = node;
	node->_private = new_capability;
	
	xmlNodePtr cur = new_capability->node->children;
	while(cur != NULL && cur->type == XML_ELEMENT_NODE)
	{
		_osync_capability_new(new_capability, cur);
		cur = cur->next;	
	}
			
	return new_capability;
}

/** internal */
OSyncCapability *_osync_capability_new_content_type(OSyncCapabilities *capabilities, xmlNodePtr node)
{
	OSyncCapability *new_capability = g_malloc0(sizeof(OSyncCapability));
	
	if(!capabilities->first_child)
		capabilities->first_child = new_capability;
	if(capabilities->last_child)
		capabilities->last_child->next = new_capability;
	capabilities->last_child = new_capability;
	
	new_capability->first_child = NULL;
	new_capability->last_child = NULL;
	new_capability->next = NULL;
	new_capability->node = node;
	node->_private = new_capability;
	
	xmlNodePtr cur = new_capability->node->children;
	while(cur != NULL  && cur->type == XML_ELEMENT_NODE)
	{
		_osync_capability_new(new_capability, cur);
		cur = cur->next;	
	}
	
	return new_capability;	
}

const char *_osync_capability_get_sortname(void *node)
{
	return (const char *)((OSyncCapability *)node)->node->name;
}

void osync_capability_free(OSyncCapability *capability)
{
	osync_trace(TRACE_ENTRY, "%s(%p)", __func__, capability);
	g_assert(capability);
	
	if(osync_capability_get_next(capability))
		osync_capability_free(osync_capability_get_next(capability));
	g_free(capability);
	
	osync_trace(TRACE_EXIT, "%s");
}

const char *osync_capability_get_name(OSyncCapability *capability)
{
	g_assert(capability);
	return (const char *) capability->node->name;
}

OSyncCapability *osync_capability_get_next(OSyncCapability *capability)
{
	g_assert(capability);
	return capability->next;
}

OSyncCapability *osync_capability_get_first_child(OSyncCapability *capability)
{
	g_assert(capability);
	return capability->first_child;
}

int osync_capability_get_field_count(OSyncCapability *capability)
{
	int count = 0;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	while(child != NULL) {
		count++;
		child = child->next;
	}
	
	return count;
}

const char *osync_capability_get_nth_field(OSyncCapability *capability, int nth)
{
	int count = 0;
	xmlNodePtr child = capability->node->xmlChildrenNode;
	
	while(child != NULL) {
		count++;
		if(count == nth)
		{
			return (const char *)child->name;
		}
		child = child->next;
	}
	return NULL;
}

void osync_capability_add_Field(OSyncCapability *capabilitiy, const char *name)
{
	xmlNewChild(capabilitiy->node, NULL, (xmlChar*)name, NULL);
}
