#include "xml.h"

#include <vobject.h>
#include <vcc.h>

static osync_bool conv_xml_to_vcard21(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("EVO21", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	
	VObjectO *nameprop = NULL, *prop = NULL;
	char *vcardptr;
	
	xmlNode *root = osxml_format_parse(input, inpsize, "contact");
	
	VObjectO *vcard = newVObjectO(VCCardPropO);
	addPropValueO(vcard, VCVersionPropO, "2.1");
	
	while (root) {
		char *str = xmlNodeGetContent(root);
		if (str) {
			if (!xmlStrcmp(root->name, (const xmlChar *)"Name")) {
				//Decode Name
				nameprop = addPropO(vcard, "N");
				addPropValueO(nameprop, VCFamilyNamePropO, osxml_find_node(root, "LastName"));
				addPropValueO(nameprop, VCGivenNamePropO, osxml_find_node(root, "FirstName"));
			}
			
			if (!xmlStrcmp(root->name, (const xmlChar *)"Organization")) {
				nameprop = addPropO(vcard, "ORG");
				addPropValueO(nameprop, "ORGNAME", osxml_find_node(root, "Name"));
			}
			
			if (!xmlStrcmp(root->name, (const xmlChar *)"FullName"))
				addPropValueO(vcard, "FN", str);
			
			if (!xmlStrcmp(root->name, (const xmlChar *)"EMail")) {
				prop = addPropValueO(vcard, "EMAIL", str);
				if (osxml_has_property(root, "Internet"))
					addPropO(prop, "INTERNET");
			}
			
			//Unknown tags
			if (!xmlStrcmp(root->name, (const xmlChar *)"Unknown")) {
				addPropValueO(vcard, osxml_find_property(root, "Name"), str);
			}
			xmlFree(str);
		}
		root = root->next;
	}
	
	vcardptr = writeMemVObjectO(0,0,vcard);
	*output = g_strdup(vcardptr);
	free(vcardptr);
	deleteVObjectO(vcard);

	*outpsize = strlen(*output) + 1;
	return TRUE;
}

static void myMimeErrorHandler(char *s)
{
  printf("%s\n", s);
}

static osync_bool conv_vcard21_to_xml(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	VObjectO *v, *prop, *vcal;
	VObjectIteratorO iter;
	const char *attributes;
	
	registerMimeErrorHandlerO(myMimeErrorHandler);
	vcal = Parse_MIMEO(input, inpsize);
	if (!vcal)
		return FALSE;
	initPropIteratorO(&iter,vcal);
	
	OSyncXML *xml = osxml_format_new();
	OSyncXMLNode *root = osxml_node_add_root(xml, "contact");
	//OSyncXMLNode *unknown = osxml_node_add(root, "Unknown", NULL);
	
	while(moreIterationO(&iter)) {
        v = nextVObjectO(&iter);
		attributes = vObjectNameO(v);

		//Name
		if (!strcmp(attributes, "N")) {
			OSyncXMLNode *name = osxml_node_add(root, "Name", NULL);
			if ((prop = isAPropertyOfO(v, "F")))
				osxml_node_add(name, "FirstName", fakeCStringO(vObjectUStringZValueO(prop)));
			if ((prop = isAPropertyOfO(v, "G")))
				osxml_node_add(name, "LastName", fakeCStringO(vObjectUStringZValueO(prop)));
			continue;
		}
		
		//FullName
		if (!strcmp(attributes, "FN")) {
			osxml_node_add(root, "FullName", fakeCStringO(vObjectUStringZValueO(v)));
			continue;
		}
		
		//EMail
		if (!strcmp(attributes, "EMAIL")) {
			OSyncXMLNode *email = osxml_node_add(root, "EMail", fakeCStringO(vObjectUStringZValueO(v)));
			if (isAPropertyOfO(v, "INTERNET"))
				osxml_node_add_property(email, "Type", "Internet");
			continue;
		}
		
		//Company
		if (!strcmp(attributes, "ORG")) {
			OSyncXMLNode *org = osxml_node_add(root, "Organization", NULL);
			if ((prop = isAPropertyOfO(v, "ORGNAME")))
				osxml_node_add(org, "Name", fakeCStringO(vObjectUStringZValueO(prop)));
			continue;
		}
		
		//Unknown tag.
		if (!strcmp(attributes, "VERSION"))
			continue;
		OSyncXMLNode *unknown = osxml_node_add(root, "Unknown", fakeCStringO(vObjectUStringZValueO(v)));
		osxml_node_add_property(unknown, "Name", attributes);
		//if ((prop = isAPropertyOfO(v, "ORGNAME")))
		//	osxml_node_add(org, "Name", fakeCStringO(vObjectUStringZValueO(prop)));
		//continue;
	}
	deleteVObjectO(vcal);

	osxml_format_dump(xml, output, outpsize);
	printf("xml string is: %s\n", *output);
	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "x-opensync-xml");

	osync_conv_register_converter(env, CONVERTER_CONV, "x-opensync-xml", "vcard21", conv_xml_to_vcard21, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "vcard21", "x-opensync-xml", conv_vcard21_to_xml, 0);
}
