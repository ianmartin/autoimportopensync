#include "xml.h"

#include "e-vcard.h"
static const char *property_get_nth_value(EVCardAttributeParam *param, int nth)
{
	const char *ret = NULL;
	GList *values = e_vcard_attribute_param_get_values(param);
	if (!values)
		return NULL;
	ret = g_list_nth_data(values, nth);
	g_list_free(values);
	return ret;
}

static const char *attribute_get_nth_value(EVCardAttribute *attr, int nth)
{
	const char *ret = NULL;
	GList *values = e_vcard_attribute_get_values(attr);
	if (!values)
		return NULL;
	ret = g_list_nth_data(values, nth);
	//g_list_free(values);
	return ret;
}

static osync_bool conv_vcard30_to_xml(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("VCARD30", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	GList *p = NULL;
	GList *a = NULL;
	
	g_type_init();
	EVCard *vcard = e_vcard_new_from_string(input);
	GList *attributes = e_vcard_get_attributes(vcard);
	OSyncXML *xml = osxml_format_new();
	OSyncXMLNode *root = osxml_node_add_root(xml, "contact");
	
	for (a = attributes; a; a = a->next) {
		EVCardAttribute *attr = a->data;
		const char *name = e_vcard_attribute_get_name(attr);

		//Name
		if (!strcmp(name, "N")) {
			OSyncXMLNode *name = osxml_node_add(root, "Name", NULL);
			osxml_node_add(name, "LastName", attribute_get_nth_value(attr, 0));
			osxml_node_add(name, "FirstName", attribute_get_nth_value(attr, 1));
			continue;
		}
		
		//FullName
		if (!strcmp(name, "FN")) {
			osxml_node_add(root, "FullName", attribute_get_nth_value(attr, 0));
			continue;
		}
		
		//EMail
		if (!strcmp(name, "EMAIL")) {
			OSyncXMLNode *email = osxml_node_add(root, "EMail", attribute_get_nth_value(attr, 0));
			GList *params = e_vcard_attribute_get_params(attr);
			for (p = params; p; p = p->next) {
				EVCardAttributeParam *param = p->data;
				if (!strcmp(property_get_nth_value(param, 0), "INTERNET"))
					osxml_node_add_property(email, "Internet", "Type");
			}
			continue;
		}
		
		//Company
		if (!strcmp(name, "ORG")) {
			OSyncXMLNode *name = osxml_node_add(root, "Organization", NULL);
			osxml_node_add(name, "Name", attribute_get_nth_value(attr, 0));
			continue;
		}
		
		//Unknown tag.
		if (!strcmp(name, "VERSION"))
			continue;
		
		GList *values = e_vcard_attribute_get_values(attr);
		GString *string = g_string_new(attribute_get_nth_value(attr, 0));
		for (p = values->next; p; p = p->next) {
			g_string_sprintfa(string, ";%s", (char *)p->data);
		}
		OSyncXMLNode *unknown = osxml_node_add(root, "Unknown", string->str);
		g_string_free(string, 1);
		
		values = e_vcard_attribute_get_params(attr);
		for (p = values; p; p = p->next) {
			EVCardAttributeParam *param = p->data;
			if (!strcmp(e_vcard_attribute_param_get_name(param), "TYPE"))
				osxml_node_add_property(unknown, property_get_nth_value(param, 0), "Type");
		}
		
		osxml_node_add_property(unknown, "Name", name);
		//if ((prop = isAPropertyOfO(v, "ORGNAME")))
		//	osxml_node_add(org, "Name", fakeCStringO(vObjectUStringZValueO(prop)));
		//continue;
	}

	osxml_format_dump(xml, output, outpsize);
	printf("xml string is: %s\n", *output);
	return TRUE;
}

static void add_parameter(EVCardAttribute *attr, const char *name, const char *data)
{
	EVCardAttributeParam *param = e_vcard_attribute_param_new(name);
	if (data)
		e_vcard_attribute_add_param_with_value(attr, param, data);
	else
		e_vcard_attribute_add_param(attr, param);
}

static osync_bool conv_xml_to_vcard30(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	
	xmlNode *root = osxml_format_parse(input, inpsize, "contact");
	
	g_type_init();
	EVCard *vcard = e_vcard_new();
	
	while (root) {
		char *str = xmlNodeGetContent(root);
		if (str) {
			if (!xmlStrcmp(root->name, (const xmlChar *)"Name")) {
				//Decode Name
				EVCardAttribute *attr = e_vcard_attribute_new(NULL, EVC_N);
				e_vcard_attribute_add_value(attr, osxml_find_node(root, "FirstName"));
				e_vcard_attribute_add_value(attr, osxml_find_node(root, "LastName"));
				e_vcard_add_attribute(vcard, attr);
			}
			if (!xmlStrcmp(root->name, (const xmlChar *)"Org")) {
				printf("%s\n", str);
			}
			if (!xmlStrcmp(root->name, (const xmlChar *)"FullName")) {
				EVCardAttribute *attr = e_vcard_attribute_new(NULL, EVC_FN);
				e_vcard_add_attribute_with_value(vcard, attr, str);
			}
			if (!xmlStrcmp(root->name, (const xmlChar *)"EMail")) {
				//Decode Name
				EVCardAttribute *attr = e_vcard_attribute_new(NULL, EVC_EMAIL);
				if (osxml_has_property(root, "Internet"))
					add_parameter(attr, "TYPE", "INTERNET");
				e_vcard_add_attribute_with_value(vcard, attr, str);
			}
			
			//Unknown tags
			if (!xmlStrcmp(root->name, (const xmlChar *)"Unknown")) {
				EVCardAttribute *attr = e_vcard_attribute_new(NULL, osxml_find_property(root, "Name"));
				e_vcard_add_attribute_with_value(vcard, attr, str);
			}
			xmlFree(str);
		}
		root = root->next;
	}
	
	*output = e_vcard_to_string(vcard, EVC_FORMAT_VCARD_30);
	*outpsize = strlen(*output) + 1;
	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "x-opensync-xml");
	
	osync_conv_register_converter(env, CONVERTER_CONV, "vcard30", "x-opensync-xml", conv_vcard30_to_xml, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "x-opensync-xml", "vcard30", conv_xml_to_vcard30, 0);
}
