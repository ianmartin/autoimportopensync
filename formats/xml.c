#include <opensync.h>
#include <glib.h>
#include <string.h>
#include <stdio.h>

static osync_bool conv_vcard30_to_xml(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	*output = g_strdup("blubb");
	*outpsize = strlen(*output) + 1;
	return TRUE;
}

static osync_bool conv_xml_to_vcard21(const char *input, int inpsize, char **output, int *outpsize)
{
	osync_debug("FILE", 4, "start: %s", __func__);
	printf("input is %i\n%s\n", inpsize, input);
	*output = g_strdup("blubb2");
	*outpsize = strlen(*output) + 1;
	return TRUE;
}

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	osync_conv_register_objformat(env, "contact", "x-multisync-xml");
	//osync_conv_format_set_compare_func(mxml, compare_vcard);
	//osync_conv_format_set_create_func(mxml, create_vcard);
	
	//osync_conv_register_data_detector(env, "plain", "vcard", detect_plain_as_vcard);

	//osync_conv_format_set_like(vcard, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
	
	//osync_conv_register_filter_function(env, "vcard_categories_filter", "contact", "vcard", vcard_categories_filter);
	osync_conv_register_converter(env, CONVERTER_CONV, "vcard30", "x-multisync-xml", conv_vcard30_to_xml, 0);
	osync_conv_register_converter(env, CONVERTER_CONV, "x-multisync-xml", "vcard21", conv_xml_to_vcard21, 0);
}
