#include "xml.h"

static osync_bool conv_x_evo_to_xml(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	/*xmlNode *root = osxml_node_get_root((xmlDoc *)input, "contact", error);
	if (!root)
		return FALSE;
	
	printf("We have to following unknown nodes:\n");
	xmlXPathObject *object = osxml_get_unknown_nodes(root->doc);
	xmlNodeSet *nodes = object->nodesetval;
	
	int size = (nodes) ? nodes->nodeNr : 0;
	
	printf("found %i nodes\n", size);
	
	int i;
	for(i = 0; i < size; i++) {
		g_assert(nodes->nodeTab[i]);
		printf("found node %s\n", nodes->nodeTab[i]->name);
	}
	
	printf("done searching\n");*/
	return TRUE;
}

static osync_bool conv_xml_to_x_evo(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);

	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "x-opensync-xml");
	
	osync_conv_register_extension(env, "x-opensync-xml", conv_x_evo_to_xml, conv_xml_to_x_evo);
}
