#include <opensync.h>

void get_info(OSyncFormatEnv *env)
{
	osync_conv_register_objtype(env, "data");
}
