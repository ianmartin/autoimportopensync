#include "conversion.h"

void conv(const char *objtype, const char *filename, const char *extension)
{
	char *command = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s .", filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncData *data = NULL;
	OSyncError *error = NULL;
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	fail_unless(osync_format_env_load_plugins(format_env, NULL, &error), NULL);

	char *buffer;
	unsigned size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new(&error);
	osync_change_set_uid(change, file);		
	g_free(file);


	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	data = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(data != NULL, NULL);

	osync_change_set_data(change, data);
	
	sourceformat = osync_format_env_detect_objformat_full(format_env, data, &error);
	fail_unless(sourceformat != NULL, NULL);

	
	OSyncObjFormat *targetformat = NULL;
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo10"))
		targetformat = osync_format_env_find_objformat(format_env, "vtodo20");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vtodo20"))
		targetformat = osync_format_env_find_objformat(format_env, "vtodo10");

	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent10"))
		targetformat = osync_format_env_find_objformat(format_env, "vevent20");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vevent20"))
		targetformat = osync_format_env_find_objformat(format_env, "vevent10");

	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard21"))
		targetformat = osync_format_env_find_objformat(format_env, "vcard30");
	
	if (!strcmp(osync_objformat_get_name(sourceformat), "vcard30"))
		targetformat = osync_format_env_find_objformat(format_env, "vcard21");

	fail_unless(targetformat != NULL, NULL);
	
	// Create new change .. duplicate and give new uid
	OSyncChange *newchange = osync_change_new(&error);
	OSyncData *newdata = osync_data_clone(data, &error);
	osync_data_set_objformat(newdata, osync_format_env_detect_objformat_full(format_env, newdata, &error));

	osync_change_set_data(newchange, newdata);
	char *newuid = g_strdup_printf("%s_original", osync_change_get_uid(change)); 
	osync_change_set_uid(newchange, newuid);
	g_free(newuid);

	fail_unless(newchange != NULL, NULL);

	OSyncFormatConverterPath *path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);
	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	//Convert to
	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);

	//Detect the output
	fail_unless(osync_data_get_objformat(data) == targetformat, NULL);
	
	//Compare old to new
//	fail_unless(osync_change_compare(newchange, change) == OSYNC_CONV_DATA_SAME, NULL);
	
	//Convert back
	path = osync_format_env_find_path(format_env, targetformat, sourceformat, &error);

	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);

	
	//Detect the output again
	fail_unless(osync_data_get_objformat(data) == sourceformat, NULL);

	// converter old and new to XMLFormat-event or XMLFormat-todo
	if (!strcmp(objtype, "event"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-event");
	else if (!strcmp(objtype, "todo"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-todo");
	else if (!strcmp(objtype, "contact"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-contact");


	path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);
	fail_unless(path != NULL, NULL);
	osync_converter_path_set_config(path, extension);

	fail_unless(osync_format_env_convert(format_env, path, data, &error), NULL);
	fail_unless(osync_format_env_convert(format_env, path, newdata, &error), NULL);

	char *xml1 = osync_data_get_printable(data);
	char *xml2 = osync_data_get_printable(newdata);
	osync_trace(TRACE_INTERNAL, "ConvertedXML:\n%s\nOriginal:\n%s\n", xml1, xml2);
	g_free(xml1);
	g_free(xml2);

	//Compare again
	fail_unless(osync_change_compare(newchange, change) == OSYNC_CONV_DATA_SAME, NULL);

	osync_format_env_free(format_env);
	
	destroy_testbed(testbed);
}

void compare(const char *objtype, const char *lfilename, const char *rfilename, OSyncConvCmpResult result)
{
	char *command1 = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s lfile", lfilename);
	char *command2 = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s rfile", rfilename);
	char *testbed = setup_testbed(NULL);
	system(command1);
	g_free(command1);
	system(command2);
	g_free(command2);
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	fail_unless(osync_format_env_load_plugins(format_env, NULL, &error), NULL);

	char *buffer;
	unsigned int size;
	
	// left data
	fail_unless(osync_file_read("lfile", &buffer, &size, &error), NULL);
	
	OSyncChange *lchange = osync_change_new(&error);
	osync_change_set_uid(lchange, "lfile");

	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	OSyncData *ldata = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(ldata != NULL, NULL);

	osync_change_set_data(lchange, ldata);


	sourceformat = osync_format_env_detect_objformat_full(format_env, ldata, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_data_set_objformat(ldata, sourceformat);

	// right data
	fail_unless(osync_file_read("rfile", &buffer, &size, &error), NULL);
	
	OSyncChange *rchange = osync_change_new(&error);
	osync_change_set_uid(rchange, "rfile");

	sourceformat = osync_objformat_new("plain", "data", &error);
	OSyncData *rdata = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(rdata != NULL, NULL);

	osync_change_set_data(rchange, rdata);


	sourceformat = osync_format_env_detect_objformat_full(format_env, rdata, &error);
	fail_unless(sourceformat != NULL, NULL);
	osync_data_set_objformat(rdata, sourceformat);

	OSyncObjFormat *targetformat = NULL;

	// right and left data to XMLFormat-event
	if (!strcmp(osync_objformat_get_objtype(sourceformat), "event"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-event");
	else if (!strcmp(objtype, "todo"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-todo");
	else if (!strcmp(objtype, "contact"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-contact");


	OSyncFormatConverterPath *path = osync_format_env_find_path(format_env, sourceformat, targetformat, &error);
	fail_unless(path != NULL, NULL);

	fail_unless(osync_format_env_convert(format_env, path, rdata, &error), NULL);
	fail_unless(osync_format_env_convert(format_env, path, ldata, &error), NULL);

	// compare
	fail_unless(osync_change_compare(lchange, rchange) == result, NULL);
	
	osync_format_env_free(format_env);
	destroy_testbed(testbed);
}

time_t get_revision(const char *objtype, const char *filename, const char *extension)
{
	char *command = g_strdup_printf("cp "OPENSYNC_TESTDATA"%s .", filename);
	char *testbed = setup_testbed(NULL);
	system(command);
	g_free(command);
	
	
	OSyncError *error = NULL;
	
	OSyncFormatEnv *format_env = osync_format_env_new(&error);
	fail_unless(format_env != NULL, NULL);

	fail_unless(osync_format_env_load_plugins(format_env, NULL, &error), NULL);

	char *buffer;
	unsigned int size;
	
	char *file = g_path_get_basename(filename);
	fail_unless(osync_file_read(file, &buffer, &size, &error), NULL);
	
	OSyncChange *change = osync_change_new(&error);
	osync_change_set_uid(change, file);
	g_free(file);


	// sourceformat
	OSyncObjFormat *sourceformat = osync_objformat_new("plain", "data", &error);

	OSyncData *data = osync_data_new(buffer, size, sourceformat, &error);
	fail_unless(data != NULL, NULL);

	osync_change_set_data(change, data);

	sourceformat = osync_format_env_detect_objformat_full(format_env, data, &error);
	fail_unless(sourceformat != NULL, NULL);

	// targetformat
	OSyncObjFormat *targetformat = NULL;

	// right and left data to XMLFormat-xxxx
	if (!strcmp(objtype, "event"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-event");
	else if (!strcmp(objtype, "todo"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-todo");
	else if (!strcmp(objtype, "contact"))
		targetformat = osync_format_env_find_objformat(format_env, "xmlformat-contact");

	fail_unless(targetformat != NULL, NULL);

	// Find converter
	OSyncFormatConverter *conv_env = osync_format_env_find_converter(format_env, sourceformat, targetformat);
	fail_unless(conv_env != NULL, NULL);

	// convert
	fail_unless (osync_converter_invoke(conv_env, data, extension, &error), NULL);

	
	time_t time = osync_data_get_revision(data, &error);
	
	osync_format_env_free(format_env);
	
	destroy_testbed(testbed);
	return time;
}

