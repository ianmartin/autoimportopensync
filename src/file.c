#include <opensync/opensync.h>
#include <glib.h>
#include <file_sync.h>

static OSyncConvCmpResult compare_file(OSyncChange *leftchange, OSyncChange *rightchange)
{
	fs_fileinfo *leftfile = (fs_fileinfo *)osync_change_get_data(leftchange);
	fs_fileinfo *rightfile = (fs_fileinfo *)osync_change_get_data(rightchange);
	
	gboolean data_same = FALSE;
	gboolean path_same = FALSE;
	
	if (!strcmp(osync_change_get_uid(leftchange), osync_change_get_uid(rightchange)))
		path_same = TRUE;
	
	if (leftfile->size == rightfile->size) {
		if (leftfile->data == rightfile->data) {
			data_same = TRUE;
		} else {
			if (!memcmp(leftfile->data, rightfile->data, leftfile->size))
				data_same = TRUE;
		}
	}
	
	if (data_same && path_same)
		return CONV_DATA_SAME;
	if (path_same)
		return CONV_DATA_SIMILAR;
	
	return CONV_DATA_MISMATCH;
}

#ifdef STRESS_TEST
static void create_file(OSyncChange *change)
{
	fs_fileinfo *file_info = g_malloc0(sizeof(fs_fileinfo));
	int file_size = g_random_int_range(0, 1000);
	osync_change_set_data(change, (char *)file_info, sizeof(fs_fileinfo), TRUE);
	
	file_info->data = g_malloc0(file_size * 105 * sizeof(char));
	file_info->size = file_size * 100 * sizeof(char);
	
	char *datap  = file_info->data;
	FILE *fd = fopen("/dev/urandom", "r");
	if (fd) {
		for (; file_size > 5; file_size--) {
			fread(datap, 100, 1, fd);
			datap += 100 * sizeof(char);
		}
	}
	fclose(fd);
	osync_change_set_uid(change, osync_rand_str(4));
}
#endif

static osync_bool conv_file_to_vcard(const char *input, int inpsize, char **output, int *outpsize)
{
	return TRUE;
}

void duplicate_file(OSyncChange *change)
{
	char *newuid = g_strdup_printf ("%s-dupe", osync_change_get_uid(change));
	osync_change_set_uid(change, newuid);
	g_free(newuid);
}

void detect_file(OSyncConvEnv *env, OSyncChange *change)
{
	//OSyncConvEnv *env = osync_member_get_conv_env(member);
	//fs_fileinfo *file_info = (fs_fileinfo *)data;
	
	//Call the data detectors here
	/*osync_conv_detect_data(file_info->data, file_info->size);
	 * 
	
	//Call the smart detectors here
	char buffer[256];
	memset(buffer, 0, sizeof(buffer));
	FILE *file = fopen(filename, "r");
	fgets(buffer, 255, file);
	fclose(file);
	g_strstrip(buffer);
	buffer[255] = 0;
	
	if (!) {
		osync_change_set_type_and_format(change, env, "vcard", "file");
	} else if (!strcmp(buffer, "BEGIN:VEVENT")) {
		osync_change_set_type_and_format(change, env, "calendar", "file");
	} else if (!strcmp(buffer, "BEGIN:VTODO")) {
		osync_change_set_type_and_format(change, env, "todo", "file");
	} else {
		osync_change_set_type_and_format(change, env, "data", "file");
	}*/
}

void get_info(OSyncConvEnv *env)
{
	OSyncObjType *type = osync_conv_register_objtype(env, "data");
	g_assert(type);
	
	OSyncObjFormat *format = osync_conv_register_objformat(type, "file");
	osync_conv_format_set_compare_func(format, compare_file);
	osync_conv_format_set_detect_func(format, detect_file);
	osync_conv_format_set_duplicate_func(format, duplicate_file);
#ifdef STRESS_TEST
	osync_conv_format_set_create_func(format, create_file);
#endif
	osync_conv_register_converter(type, CONVERTER_CONV, "file", "vcard", conv_file_to_vcard);
}
