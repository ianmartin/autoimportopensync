/** @file
 *
 * Auxiliary functions for opensync.pyx pyrex module
 *
 * @author Eduardo Pereira Habkost
 *
 */
#ifndef _OSYNC_PYWRAP_H_
#define _OSYNC_PYWRAP_H_

#include <opensync/opensync.h>

/** Plugin information necessary to implement accept_objformat
 * correctly
 */
typedef struct PythonPluginInfo {
	osync_bool (*commit_fn)(OSyncContext *, OSyncChange *);
	osync_bool (*access_fn)(OSyncContext *, OSyncChange *);
	OSyncPluginInfo *osync_info;
} PythonPluginInfo;

#define osync_member_from_void(v) ((OSyncMember*)(v))
#define osync_context_from_void(v) ((OSyncContext*)(v))
#define osync_plginfo_from_void(v) ((PythonPluginInfo *)(v))
#define osync_change_from_void(v) ((OSyncChange*)(v))

void pywrap_accept_objtype(PythonPluginInfo *info, const char *objtype);
void pywrap_accept_objformat(PythonPluginInfo *info, const char *objtype, const char *objformat, const char *extension);
void pywrap_set_name_and_version(PythonPluginInfo *info, const char *name, int version);

#endif /* _OSYNC_PYWRAP_H_ */
