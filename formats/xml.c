#include <opensync.h>
#include <glib.h>
#include <string.h>

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "contact");
	OSyncObjFormat *mxml = osync_conv_register_objformat(env, "contact", "x-multisync-xml");
	osync_conv_format_set_compare_func(mxml, compare_vcard);
	//osync_conv_format_set_create_func(mxml, create_vcard);
	
	//osync_conv_register_data_detector(env, "plain", "vcard", detect_plain_as_vcard);

	//osync_conv_format_set_like(vcard, "plain", CONV_NOTLOSSY, CONV_DETECTFIRST);
	
	//osync_conv_register_filter_function(env, "vcard_categories_filter", "contact", "vcard", vcard_categories_filter);
	osync_conv_register_converter(env, CONVERTER_CONV, "vcard", "x-multisync-xml", conv_vcard_to_xml, 0);
}