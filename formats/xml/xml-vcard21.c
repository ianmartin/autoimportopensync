#include "xml.h"

#include <vobject.h>
#include <vcc.h>

static VObjectO *add_value(VObjectO *prop, const char *name, char *data)
{
	VObjectO *tmp = addPropValueO(prop, name, data);
	g_free(data);
	return tmp;
}

static osync_bool conv_xml_to_vcard21(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("XML21", 4, "start: %s", __func__);
	
	VObjectO *nameprop = NULL, *prop = NULL;
	char *vcardptr = NULL;
	
	xmlNode *root = osxml_format_parse(input, inpsize, "contact", error);
	if (!root)
		return FALSE;
	
	VObjectO *vcard = newVObjectO(VCCardPropO);
	addPropValueO(vcard, VCVersionPropO, "2.1");
	
	while (root) {
		char *str = xmlNodeGetContent(root);
		if (str) {
			if (!xmlStrcmp(root->name, (const xmlChar *)"Name")) {
				//Decode Name
				nameprop = addPropO(vcard, "N");
				add_value(nameprop, VCFamilyNamePropO, osxml_find_node(root, "LastName"));
				add_value(nameprop, VCGivenNamePropO, osxml_find_node(root, "FirstName"));
			}
			
			if (!xmlStrcmp(root->name, (const xmlChar *)"Organization")) {
				nameprop = addPropO(vcard, "ORG");
				add_value(nameprop, "ORGNAME", osxml_find_node(root, "Name"));
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
				char *name = osxml_find_property(root, "Name");
				addPropValueO(vcard, osxml_find_property(root, "Name"), str);
				g_free(name);
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

static OSyncXMLEncoding property_to_xml_encoding(VObjectO *v)
{
	OSyncXMLEncoding encoding;
	memset(&encoding, 0, sizeof(encoding));
	VObjectO *prop;
	if ((prop = isAPropertyOfO(v, "ENCODING"))) {
		if (!strcmp((char *)vObjectUStringZValueO(prop), "QUOTED-PRINTABLE"))
			encoding.encoding = OSXML_QUOTED_PRINTABLE;
		else if (!g_ascii_strcasecmp((char *)vObjectUStringZValueO(prop), "B"))
			encoding.encoding = OSXML_BASE64;
		else
			printf("Unknown encoding: %s\n", (char *)vObjectUStringZValueO(prop));
	}
	if ((prop = isAPropertyOfO(v, "CHARSET"))) {
		if (!strcmp((char *)vObjectUStringZValueO(prop), "UTF-8"))
			encoding.charset = OSXML_UTF8;
		else
			printf("Unknown charset: %s\n", (char *)vObjectUStringZValueO(prop));
	}
	return encoding;
}

static osync_bool conv_vcard21_to_xml(const char *input, int inpsize, char **output, int *outpsize, OSyncError **error)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	printf("\n%s\n", input);
	VObjectO *v, *prop, *vcal;
	VObjectIteratorO iter;
	VObjectIteratorO iter2;
	const char *attributes;
	OSyncXMLEncoding encoding;
	
	registerMimeErrorHandlerO(myMimeErrorHandler);
	vcal = Parse_MIMEO(input, inpsize);
	if (!vcal)
		return FALSE;
	initPropIteratorO(&iter,vcal);
	
	OSyncXML *xml = osxml_format_new();
	xmlNode *root = osxml_node_add_root(xml, "contact");
	
	while(moreIterationO(&iter)) {
        v = nextVObjectO(&iter);
        encoding = property_to_xml_encoding(v);
        attributes = vObjectNameO(v);
		
		if (!strcmp(attributes, "VERSION"))
			continue;
		
		printf("Processing node %s\n", attributes);
		
		xmlNode *current = xmlNewChild(root, NULL, "test", NULL);
		
		//First we decode the properties
		initPropIteratorO(&iter2, v);
		while(moreIterationO(&iter2)) {
			prop = nextVObjectO(&iter2);
			const char *propname = vObjectNameO(prop);
			printf("Processing property %s\n", propname);
			if (propname) {
				if (!strcmp(propname, "QUOTED-PRINTABLE"))
					continue;
				
				char *content = fakeCStringO(vObjectUStringZValueO(prop));
				if (!strcmp(propname, "WORK"))
					osxml_node_add(current, "Location", "Work", encoding);
				else if (!strcmp(propname, "HOME"))
					osxml_node_add(current, "Location", "Home", encoding);
				else if (!strcmp(propname, "POSTAL"))
					osxml_node_add(current, "Type", "Postal", encoding);
				else if (!strcmp(propname, "PARCEL"))
					osxml_node_add(current, "Type", "Parcel", encoding);
				else if (!strcmp(propname, "DOM"))
					osxml_node_add(current, "Range", "Domestic", encoding);
				else if (!strcmp(propname, "INTL"))
					osxml_node_add(current, "Range", "International", encoding);
				else if (!strcmp(propname, "F"))
					osxml_node_add(current, "FirstName", content, encoding);
				else if (!strcmp(propname, "G"))
					osxml_node_add(current, "LastName", content, encoding);
				else if (!strcmp(propname, VCAdditionalNamesPropO))
					osxml_node_add(current, "Additional", content, encoding);
				else if (!strcmp(propname, VCNamePrefixesPropO))
					osxml_node_add(current, "Prefix", content, encoding);
				else if (!strcmp(propname, VCNameSuffixesPropO))
					osxml_node_add(current, "Suffix", content, encoding);
				else if (!strcmp(propname, "PREF"))
					osxml_node_add_property(current, "Order", "1");
				else if (!strcmp(propname, VCPostalBoxPropO))
					osxml_node_add(current, "PostalBox", content, encoding);
				else if (!strcmp(propname, VCExtAddressPropO))
					osxml_node_add(current, "ExtendedAddress", content, encoding);
				else if (!strcmp(propname, VCStreetAddressPropO))
					osxml_node_add(current, "Street", content, encoding);
				else if (!strcmp(propname, VCCityPropO))
					osxml_node_add(current, "City", content, encoding);
				else if (!strcmp(propname, VCRegionPropO))
					osxml_node_add(current, "Region", content, encoding);
				else if (!strcmp(propname, VCPostalCodePropO))
					osxml_node_add(current, "PostalCode", content, encoding);
				else if (!strcmp(propname, VCCountryNamePropO))
					osxml_node_add(current, "Country", content, encoding);
				else if (!strcmp(propname, "VOICE"))
					osxml_node_add(current, "Type", "Voice", encoding);
				else if (!strcmp(propname, "FAX"))
					osxml_node_add(current, "Type", "Fax", encoding);
				else if (!strcmp(propname, "MSG"))
					osxml_node_add(current, "Type", "Message", encoding);
				else if (!strcmp(propname, "CELL"))
					osxml_node_add(current, "Type", "Cellular", encoding);
				else if (!strcmp(propname, "PAGER"))
					osxml_node_add(current, "Type", "Pager", encoding);
				else if (!strcmp(propname, "BBS"))
					osxml_node_add(current, "Type", "BulletinBoard", encoding);
				else if (!strcmp(propname, "MODEM"))
					osxml_node_add(current, "Type", "Modem", encoding);
				else if (!strcmp(propname, "CAR"))
					osxml_node_add(current, "Location", "Car", encoding);
				else if (!strcmp(propname, "ISDN"))
					osxml_node_add(current, "Type", "ISDN", encoding);
				else if (!strcmp(propname, "VIDEO"))
					osxml_node_add(current, "Type", "Video", encoding);
				else if (!strcmp(propname, "INTERNET"))
					osxml_node_add(current, "Type", "Internet", encoding);
				else if (!strcmp(propname, "ORGNAME"))
					osxml_node_add(current, "Name", content, encoding);
				else if (!strcmp(propname, VCOrgUnitPropO))
					osxml_node_add(current, "Unit", content, encoding);
				else if (!strcmp(propname, "X509"))
					osxml_node_add(current, "Type", "X509", encoding);
				else if (!strcmp(propname, "PGP"))
					osxml_node_add(current, "Type", "PGP", encoding);
				else {
					printf("unknown property\n");
					xmlNode *property = osxml_node_add(current, propname, content, encoding);
					osxml_node_add_property(property, "Type", "Property");
					osxml_node_mark_unknown(current);
				}
				if (content)
					g_free(content);
			}
		}
		
		//FullName
		if (!strcmp(attributes, "FN")) {
			osxml_node_set(current, "FullName", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Name
		if (!strcmp(attributes, "N")) {
			osxml_node_set(current, "Name", NULL, encoding);
			continue;
		}
				
		//Photo
		if (!strcmp(attributes, "PHOTO")) {
			printf("Photo not supported yet\n");
			continue;
		}
		
		//Birthday
		if (!strcmp(attributes, "BDAY")) {
			osxml_node_set(current, "Birthday", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Address
		if (!strcmp(attributes, "ADR")) {
			osxml_node_set(current, "Address", NULL, encoding);
			continue;
		}
		
		//Address Labeling
		if (!strcmp(attributes, "LABEL")) {
			osxml_node_set(current, "AddressLabel", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Telephone
		if (!strcmp(attributes, "TEL")) {
			osxml_node_set(current, "Telephone", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//EMail
		if (!strcmp(attributes, "EMAIL")) {
			osxml_node_set(current, "EMail", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Mailer
		if (!strcmp(attributes, "MAILER")) {
			osxml_node_set(current, "Mailer", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Timezone
		if (!strcmp(attributes, "TZ")) {
			osxml_node_set(current, "Timezone", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Location
		if (!strcmp(attributes, "GEO")) {
			osxml_node_set(current, "Location", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Title
		if (!strcmp(attributes, "TITLE")) {
			osxml_node_set(current, "Title", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Role
		if (!strcmp(attributes, "ROLE")) {
			osxml_node_set(current, "Role", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Logo
		if (!strcmp(attributes, "LOGO")) {
			printf("Logo is not supported yet\n");
			continue;
		}
		
		//Company
		if (!strcmp(attributes, "ORG")) {
			osxml_node_set(current, "Organization", NULL, encoding);
			continue;
		}
		
		//Note
		if (!strcmp(attributes, "NOTE")) {
			osxml_node_set(current, "Note", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Revision
		if (!strcmp(attributes, "REV")) {
			osxml_node_set(current, "Revision", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Sound
		if (!strcmp(attributes, "SOUND")) {
			printf("Sound is not supported yet\n");
			continue;
		}
		
		//Url
		if (!strcmp(attributes, "URL")) {
			osxml_node_set(current, "Url", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Uid
		if (!strcmp(attributes, "UID")) {
			osxml_node_set(current, "Uid", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Public Key
		if (!strcmp(attributes, "KEY")) {
			osxml_node_set(current, "Key", fakeCStringO(vObjectUStringZValueO(v)), encoding);
			continue;
		}
		
		//Unknown tag.
		osxml_node_mark_unknown(current);
		printf("unknown attribute\n");
		osxml_node_set(current, attributes, fakeCStringO(vObjectUStringZValueO(v)), encoding);
		//if ((prop = isAPropertyOfO(v, "ORGNAME")))
		//	osxml_node_add(org, "Name", fakeCStringO(vObjectUStringZValueO(prop)));
		//continue;
	}
	deleteVObjectO(vcal);

	osxml_format_dump(xml, output, outpsize);
	printf("\noutput:\n%s\n", *output);
	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "x-opensync-xml");

	osync_conv_register_converter(env, CONVERTER_CONV, "x-opensync-xml", "vcard21", conv_xml_to_vcard21, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "vcard21", "x-opensync-xml", conv_vcard21_to_xml, 0);
}
