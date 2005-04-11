/** @file
 *
 * Auxiliary functions for opensync.pyx pyrex module
 *
 * @author Eduardo Pereira Habkost
 *
 */

#include <string.h>

#include "pywrap.h"

void pywrap_accept_objtype(PythonPluginInfo *info, const char *objtype)
{
	osync_plugin_accept_objtype(info->osync_info, objtype);
}

void pywrap_accept_objformat(PythonPluginInfo *info, const char *objtype, const char *objformat, const char *extension)
{
	osync_plugin_accept_objformat(info->osync_info, objtype, objformat, extension);
	osync_plugin_set_commit_objformat(info->osync_info, objtype, objformat, info->commit_fn);
	osync_plugin_set_access_objformat(info->osync_info, objtype, objformat, info->access_fn);
}

void pywrap_set_name_and_version(PythonPluginInfo *info, const char *name, int version)
{
	info->osync_info->name = strdup(name);
	info->osync_info->version = version;
}
